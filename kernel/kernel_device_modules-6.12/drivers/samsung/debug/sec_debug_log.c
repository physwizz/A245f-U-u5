/*
 * Copyright (c) 2022 Samsung Electronics Co., Ltd.
 *      http://www.samsung.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/module.h>
#include <linux/console.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/stat.h>
#include <linux/uaccess.h>
#include <linux/proc_fs.h>
#include <linux/memblock.h>
#include <linux/sched/clock.h>
#include <linux/of.h>
#include <linux/of_reserved_mem.h>
#include <linux/vmalloc.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/sec_ext.h>
#include <linux/sec_debug.h>
#include <linux/notifier.h>
#include <linux/spinlock.h>

#include <trace/hooks/logbuf.h>
#include <trace/hooks/debug.h>
#include <trace/hooks/traps.h>

#include "printk_ringbuffer.h"

static u32 klog_rmem_base;
static u32 klog_rmem_size;

static char *secdbg_klog_buf = NULL;
static unsigned int secdbg_klog_size;
static unsigned int *secdbg_klog_buf_pos;
static unsigned int *secdbg_klog_head;

static DEFINE_SPINLOCK(secdbg_klog_lock);

void *secdbg_log_rmem_set_vmap(phys_addr_t base, phys_addr_t size)
{
	pgprot_t prot = __pgprot(PROT_NORMAL_NC);
	int i, page_size;
	struct page *page, **pages;
	void *vaddr;

	page_size = size / PAGE_SIZE;
	pages = kzalloc(sizeof(struct page *) * page_size, GFP_KERNEL);
	page = phys_to_page(base);

	for (i = 0; i < page_size; i++)
		pages[i] = page++;

	vaddr = vmap(pages, page_size, VM_NO_GUARD | VM_MAP, prot);
	kfree(pages);
	
	pr_notice("%s, base=%llx, size=%llx, vaddr=%llx\n", __func__, base, size, (u64)vaddr);

	return vaddr;
}
EXPORT_SYMBOL(secdbg_log_rmem_set_vmap);

struct reserved_mem *secdbg_log_get_rmem(const char *compatible)
{
	struct reserved_mem *rmem;
	struct device_node *rmem_np;

	pr_info("%s: start to get %s\n", __func__, compatible);

	rmem_np = of_find_compatible_node(NULL, NULL, compatible);
	if (!rmem_np) {
		pr_info("%s: no such reserved mem compatable with %s\n", __func__,
			compatible);

		return 0;
	}

	rmem = of_reserved_mem_lookup(rmem_np);
	if (!rmem) {
		pr_info("%s: no such reserved mem compatable with %s\n", __func__,
			compatible);

		return 0;
	} else if (!rmem->base || !rmem->size) {
		pr_info("%s: wrong base(0x%llx) or size(0x%llx)\n",
				__func__, rmem->base, rmem->size);

		return 0;
	}

	pr_info("%s: found (base=%llx, size=%llx)\n", __func__, 
		(unsigned long long)rmem->base, (unsigned long long)rmem->size);

	return rmem;
}
EXPORT_SYMBOL(secdbg_log_get_rmem);


static u32 secdbg_lastklog_base;
static u32 secdbg_lastklog_size;

static char *last_kmsg_buffer;
static unsigned int last_kmsg_size;

static void secdbg_save_lastklog(unsigned int lastkbase, unsigned int lastksize)
{
	/* provide previous log as last_kmsg */
	unsigned int pos = *secdbg_klog_buf_pos;
	last_kmsg_buffer = phys_to_virt(lastkbase);

	if (last_kmsg_buffer) {
		if (*secdbg_klog_head == 1) {
			last_kmsg_size = secdbg_klog_size;
			memcpy(last_kmsg_buffer, &secdbg_klog_buf[pos], secdbg_klog_size - pos);
			memcpy(&last_kmsg_buffer[secdbg_klog_size - pos], secdbg_klog_buf, pos);
		} else {
			last_kmsg_size = pos;
			memcpy(last_kmsg_buffer, secdbg_klog_buf, lastksize);
		}
		pr_info("%s: saved old log at %u@%p\n",
			__func__, last_kmsg_size, last_kmsg_buffer);
	} else
		pr_err("%s: failed saving old log %u@%p\n",
		       __func__, last_kmsg_size, last_kmsg_buffer);
}

static ssize_t sec_last_kmsg_read(struct file *file, char __user *buf,
				  size_t len, loff_t *offset)
{
	loff_t pos = *offset;
	ssize_t count;

	if (pos >= last_kmsg_size)
		return 0;

	count = min(len, (size_t) (last_kmsg_size - pos));
	if (copy_to_user(buf, last_kmsg_buffer + pos, count))
		return -EFAULT;

	*offset += count;
	return count;
}

static const struct proc_ops last_kmsg_file_ops = {
	.proc_lseek = default_llseek,
	.proc_read = sec_last_kmsg_read,
};

static int init_sec_last_kmsg_proc(void)
{
	struct proc_dir_entry *entry;

	if (last_kmsg_buffer == NULL)
		return 0;

	entry = proc_create("last_kmsg", S_IFREG | 0444,
			NULL, &last_kmsg_file_ops);
	if (!entry) {
		pr_err("%s: failed to create proc entry\n", __func__);
		return 0;
	}

	proc_set_size(entry, last_kmsg_size);
	return 0;
}

static void secdbg_lastkernel_log_init(void)
{
	struct reserved_mem *rmem;

	rmem = secdbg_log_get_rmem("samsung,secdbg-last-klog");

	secdbg_lastklog_base = rmem->base;
	secdbg_lastklog_size = rmem->size;

	pr_notice("%s: 0x%llx - 0x%llx (0x%llx)\n",
		"secdbg-last-klog", (unsigned long long)rmem->base,
		(unsigned long long)rmem->base + (unsigned long long)rmem->size,
		(unsigned long long)rmem->size);
}

static ssize_t sec_log_read(struct file *file, char __user *buf,
				  size_t len, loff_t *offset)
{
	loff_t pos = *offset;
	ssize_t count;

	if (pos >= klog_rmem_size)
		return 0;

	count = min(len, (size_t) (klog_rmem_size - pos));
	if (copy_to_user(buf, secdbg_klog_buf + pos, count))
		return -EFAULT;

	*offset += count;
	return count;
}

static const struct proc_ops sec_log_file_ops = {
	.proc_lseek = default_llseek,
	.proc_read = sec_log_read,
};

static int init_sec_log_proc(void)
{
	struct proc_dir_entry *entry;

	if (secdbg_klog_buf == NULL)
		return 0;

	entry = proc_create("sec_log", S_IFREG | 0444,
			NULL, &sec_log_file_ops);
	if (!entry) {
		pr_err("%s: failed to create proc entry\n", __func__);
		return 0;
	}

	proc_set_size(entry, klog_rmem_size);
	return 0;
}

static inline void secdbg_hook_kernel_log(const char *text, size_t size)
{
	unsigned long flags;
	unsigned int pos;
	unsigned int actual_size = (unsigned int)size;
	
	spin_lock_irqsave(&secdbg_klog_lock, flags);
	
	pos = *secdbg_klog_buf_pos;
	
	if (likely(actual_size + pos <= secdbg_klog_size)) {
		memcpy(&secdbg_klog_buf[pos], text, actual_size);
		*secdbg_klog_buf_pos += actual_size;
	} else {
		unsigned int first = secdbg_klog_size - pos;
		unsigned int second = actual_size - first;
		memcpy(&secdbg_klog_buf[pos], text, first);
		memcpy(&secdbg_klog_buf[0], text + first, second);
		*secdbg_klog_head = 1;
		*secdbg_klog_buf_pos = second;
	}
	
	/* Check overflow */
	if (unlikely(*secdbg_klog_buf_pos >= secdbg_klog_size))
		*secdbg_klog_buf_pos -= secdbg_klog_size;
	
	spin_unlock_irqrestore(&secdbg_klog_lock, flags);
}

static void (*hook_init_log)(const char *str, size_t size);
void register_hook_init_log(void (*func)(const char *str, size_t size))
{
	if (!func)
		return;

	hook_init_log = func;
	pr_info("%s done!\n", __func__);
}
EXPORT_SYMBOL_GPL(register_hook_init_log);

static void secdbg_klog_write(const char *buf, size_t size)
{
	secdbg_hook_kernel_log(buf, size);

 	if ((hook_init_log) && task_pid_nr(current) == 1)
		hook_init_log(buf, size);
}

#define TEMP_KERNEL_LOG_BUF_NUM	(8) /* NR_CPUS */
static u64 kmsg_seq;
static u64 kmsg_dropped;
static bool __read_mostly is_kmsg_init;

static size_t print_time(u64 ts, char *buf, size_t buf_size)
{
	unsigned long rem_nsec = do_div(ts, 1000000000);

	return snprintf(buf, buf_size, "[%5lu.%06lu]", (unsigned long)ts, rem_nsec / 1000);
}

static size_t print_caller(u32 id, char *buf, size_t buf_size)
{
	char caller[12];

	snprintf(caller, sizeof(caller), "%c%u",
		 id & 0x80000000 ? 'C' : 'T', id & ~0x80000000);
	return snprintf(buf, buf_size, "[%6s] ", caller);
}

static size_t print_fixed_caller(char *buf, size_t buf_size)
{
	size_t len = 0;

	len = snprintf(buf, buf_size, "%c[%1d:%15s:%5d] ", in_interrupt() ? 'I' : ' ',
			smp_processor_id(), current->comm, task_pid_nr(current));
	return len;
}

static size_t info_print_prefix(const struct printk_info *info, char *buf,
				size_t buf_size, bool init)
{
	size_t len = 0;

	len += print_time(info->ts_nsec, buf + len, buf_size - len);

	if (init)
		len += print_fixed_caller(buf + len, buf_size - len);
	else
		len += print_caller(info->caller_id, buf + len, buf_size - len);

	return len;
}

static size_t record_print_text(struct printk_record *r, bool init)
{
	size_t text_len = r->info->text_len;
	size_t buf_size = r->text_buf_size;
	char *text = r->text_buf;
	char prefix[64];
	bool truncated = false;
	size_t prefix_len;
	size_t line_len;
	size_t len = 0;
	char *next;

	/*
	 * If the message was truncated because the buffer was not large
	 * enough, treat the available text as if it were the full text.
	 */
	if (text_len > buf_size)
		text_len = buf_size;

	prefix_len = info_print_prefix(r->info, prefix, sizeof(prefix), init);

	/*
	 * @text_len: bytes of unprocessed text
	 * @line_len: bytes of current line _without_ newline
	 * @text:     pointer to beginning of current line
	 * @len:      number of bytes prepared in r->text_buf
	 */
	for (;;) {
		next = memchr(text, '\n', text_len);
		if (next) {
			line_len = next - text;
		} else {
			/* Drop truncated line(s). */
			if (truncated)
				break;
			line_len = text_len;
		}

		/*
		 * Truncate the text if there is not enough space to add the
		 * prefix and a trailing newline and a terminator.
		 */
		if (len + prefix_len + text_len + 1 + 1 > buf_size) {
			/* Drop even the current line if no space. */
			if (len + prefix_len + line_len + 1 + 1 > buf_size)
				break;

			text_len = buf_size - len - prefix_len - 1 - 1;
			truncated = true;
		}

		memmove(text + prefix_len, text, text_len);
		memcpy(text, prefix, prefix_len);

		/*
		 * Increment the prepared length to include the text and
		 * prefix that were just moved+copied. If this is the last
		 * line, there is no newline.
		 */
		len += prefix_len + line_len + 1;
		if (text_len == line_len) {
			if (!init)
				len--;
			break;
		}

		/*
		 * Advance beyond the added prefix and the related line with
		 * its newline.
		 */
		text += prefix_len + line_len + 1;

		/*
		 * The remaining text has only decreased by the line with its
		 * newline.
		 *
		 * Note that @text_len can become zero. It happens when @text
		 * ended with a newline (either due to truncation or the
		 * original string ending with "\n\n"). The loop is correctly
		 * repeated and (if not truncated) an empty line with a prefix
		 * will be prepared.
		 */
		text_len -= line_len + 1;
	}

	/*
	 * If a buffer was provided, it will be terminated. Space for the
	 * string terminator is guaranteed to be available. The terminator is
	 * not counted in the return value.
	 */
	if (buf_size > 0)
		r->text_buf[len] = 0;

	return len;
}

#define INIT_LOGBUF_INVALID	(-1)
static int secdbg_init_prb(struct printk_ringbuffer *rb)
{
	static char text[4096];
	struct printk_info info;
	struct printk_record r;
	size_t len;
	int cpu, old_cpu;
	static atomic_t init_logbuf_cpu = ATOMIC_INIT(INIT_LOGBUF_INVALID);

	cpu = raw_smp_processor_id();
	old_cpu = atomic_cmpxchg(&init_logbuf_cpu, INIT_LOGBUF_INVALID, cpu);

	if (old_cpu != INIT_LOGBUF_INVALID)
		return -1;

	prb_rec_init_rd(&r, &info, text, sizeof(text));
	prb_read_valid(rb, kmsg_seq, &r);
	if (kmsg_seq != r.info->seq) {
		kmsg_dropped += r.info->seq - kmsg_seq;
		kmsg_seq = r.info->seq;
	}
	len = record_print_text(&r, false);
	kmsg_seq++;

	secdbg_klog_write(text, len);

	text[0] = '\n';
	prb_rec_init_rd(&r, &info, text + 1, sizeof(text) - 1);

	for(;;) {
		if (!prb_read_valid(rb, kmsg_seq, &r))
			break;

		if (kmsg_seq != r.info->seq) {
			kmsg_dropped += r.info->seq - kmsg_seq;
			kmsg_seq = r.info->seq;
		}

		len = record_print_text(&r, false);
		kmsg_seq++;

		secdbg_klog_write(text, len + 1);
	}

	return 0;
}

static void __maybe_unused secdbg_android_vh_logbuf(void *ignore,
					   struct printk_ringbuffer *rb,
					   struct printk_record *r)
{
	static char text_bufs[TEMP_KERNEL_LOG_BUF_NUM][4096];
	static atomic_t seq = ATOMIC_INIT(0);
	char *text;
	struct printk_info info;
	struct printk_record record;
	size_t len = 0;

	if (unlikely(!is_kmsg_init)) {
		if (secdbg_init_prb(rb))
			return;

		is_kmsg_init = true;
		return;
	}

	text = text_bufs[atomic_inc_return(&seq) % ARRAY_SIZE(text_bufs)];
	text[len++] = '\n';
	prb_rec_init_rd(&record, &info, text + len, sizeof(text_bufs[0]) - len);

	if (record.text_buf_size < r->info->text_len + 1 + 1)
		return;

	kmsg_seq = r->info->seq;
	*record.info = *r->info;
	memcpy(record.text_buf, r->text_buf, r->info->text_len);

	len = record_print_text(&record, true);

	secdbg_klog_write(text, len);
}

static void __maybe_unused secdbg_android_vh_logbuf_pr_cont(void *ignore,
						   struct printk_record *r,
						   size_t text_len)
{
	char *text;

	if (unlikely(!is_kmsg_init))
		return;

	text = r->text_buf + r->info->text_len - text_len;
	secdbg_klog_write(text, text_len);
}

static void register_kmsg(void)
{

	register_trace_android_vh_logbuf(secdbg_android_vh_logbuf, NULL);
	register_trace_android_vh_logbuf_pr_cont(secdbg_android_vh_logbuf_pr_cont, NULL);
}

static int secdbg_klog_buf_init(void)
{
	unsigned int *klog_magic;

	secdbg_klog_buf = secdbg_log_rmem_set_vmap(klog_rmem_base, klog_rmem_size);
	secdbg_klog_size = klog_rmem_size - (sizeof(*secdbg_klog_head) + sizeof(*secdbg_klog_buf_pos) + sizeof(*klog_magic));
	secdbg_klog_head = (unsigned int *)(secdbg_klog_buf + secdbg_klog_size);
	secdbg_klog_buf_pos = (unsigned int *)(secdbg_klog_buf + secdbg_klog_size + sizeof(*secdbg_klog_head));
	klog_magic = (unsigned int *)(secdbg_klog_buf + secdbg_klog_size + sizeof(*secdbg_klog_head) + sizeof(*secdbg_klog_buf_pos));

	pr_info("%s: *secdbg_klog_head:%u, *klog_magic:%x, *secdbg_klog_buf_pos:%u, secdbg_klog_buf:0x%p, secdbg_klog_size:0x%x\n",
		__func__, *secdbg_klog_head, *klog_magic, *secdbg_klog_buf_pos, secdbg_klog_buf, secdbg_klog_size);

	if (*klog_magic != LOG_MAGIC) {
		pr_info("%s: no old log found\n", __func__);
		*secdbg_klog_head = 0;
		*secdbg_klog_buf_pos = 0;
		*klog_magic = LOG_MAGIC;
	} else if ((reset_reason == RR_K) ||
		(reset_reason == RR_D) ||
		(reset_reason == RR_P) ||
		(reset_reason == RR_C) ||
		(reset_reason == RR_M)) {
		last_kmsg_size = secdbg_lastklog_size;
		last_kmsg_buffer = phys_to_virt(secdbg_lastklog_base);
		pr_info("%s: previous klog on last_kmsg buffer (stored by lk_aee)\n", __func__);
	} else {
		secdbg_save_lastklog(secdbg_lastklog_base, secdbg_lastklog_size);
	}

	return 0;
}

static void secdbg_kernel_log_init(void)
{
	struct reserved_mem *rmem;

	rmem = secdbg_log_get_rmem("samsung,secdbg-kernel-log");

	klog_rmem_base = rmem->base;
	klog_rmem_size = rmem->size;

	pr_notice("%s: 0x%llx - 0x%llx (0x%llx)\n",
		"secdbg-kernel-log", (unsigned long long)rmem->base,
		(unsigned long long)rmem->base + (unsigned long long)rmem->size,
		(unsigned long long)rmem->size);

	secdbg_klog_buf_init();
}

/*
 * Log module loading order
 * 
 *  0. sec_debug_init() @ core_initcall
 *  1. secdbg_init_log_init() @ postcore_initcall
 *  2. secdbg_lastkernel_log_init() @ arch_initcall
 *  3. secdbg_kernel_log_init() @ arch_initcall
 *  4. secdbg_pmsg_log_init() @ device_initcall
 */
int sec_debug_log_init(void)
{
	pr_info("%s: start\n", __func__);	

	secdbg_lastkernel_log_init();
	secdbg_kernel_log_init();

	init_sec_log_proc();
	init_sec_last_kmsg_proc();

	register_kmsg();
	
	return 0;
}
EXPORT_SYMBOL(sec_debug_log_init);

