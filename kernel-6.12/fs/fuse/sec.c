// SPDX-License-Identifier: GPL-2.0
#include <linux/proc_fs.h>
#include <linux/freezer.h>

#include "fuse_i.h"

#ifndef ST_LOG
#define ST_LOG(fmt, ...) pr_err(fmt, ##__VA_ARGS__)
#endif

static struct proc_dir_entry *fuse_proc_dir;
#ifdef CONFIG_FUSE_PERF
static struct proc_dir_entry *fuse_perf_proc_dir;
#endif
#ifdef CONFIG_FUSE_INFLIGHT
static struct proc_dir_entry *fuse_inflight_proc_dir;
#endif

int fuse_sec_init(void)
{
	fuse_proc_dir = proc_mkdir("fs/fuse", NULL);
	if (!fuse_proc_dir)
		goto out;

	if (!fuse_perf_proc_create(fuse_proc_dir))
		goto out;
	if (!fuse_inflight_proc_create(fuse_proc_dir))
		goto out;

	return 0;

out:
	fuse_inflight_proc_remove();
	fuse_perf_proc_remove();

	if (fuse_proc_dir)
		proc_remove(fuse_proc_dir);
	return 1;
}

void fuse_sec_cleanup(void)
{
	struct proc_dir_entry *temp;

	fuse_inflight_proc_remove();
	fuse_perf_proc_remove();

	temp = xchg(&fuse_proc_dir, NULL);
	if (temp)
		proc_remove(temp);
}

#ifdef CONFIG_FUSE_WATCHDOG

#define FUSE_WATCHDOG_MAX_WAIT 2
#define FUSE_WATCHDOG_INTERVAL_MSECS (10 * MSEC_PER_SEC)

struct fuse_watchdog_data {
	struct fuse_conn *fc;
	struct task_struct *daemon;
};

static int watch_daemon(void *data)
{
	struct fuse_watchdog_data *fwd = data;
	struct fuse_conn *fc = fwd->fc;
	struct task_struct *daemon = fwd->daemon;
	int cnt = 0;
	DECLARE_WAIT_QUEUE_HEAD_ONSTACK(wq);

	ST_LOG("<%s> dev = %u:%u  fuse watchdog start\n",
			__func__, MAJOR(fc->dev), MINOR(fc->dev));

	set_freezable();

	while (cnt < FUSE_WATCHDOG_MAX_WAIT) {
		fuse_perf_check_last_read(fc);

		/*
		 * Use conditional wait to prevent race between system freezing
		 * request and sleep.
		 */
		wait_event_interruptible_timeout(wq,
				kthread_should_stop() || freezing(current) ||
				daemon->exit_state == EXIT_DEAD,
				msecs_to_jiffies(FUSE_WATCHDOG_INTERVAL_MSECS));

		if (kthread_should_stop()) {
			ST_LOG("<%s> dev = %u:%u  kthread_should_stop\n",
					__func__, MAJOR(fc->dev), MINOR(fc->dev));
			break;
		}

		/*
		 * The fuse daemon process have exited normally.
		 */
		if (daemon->exit_state == EXIT_DEAD) {
			ST_LOG("<%s> dev = %u:%u  EXIT_DEAD\n",
					__func__, MAJOR(fc->dev), MINOR(fc->dev));
			break;
		}

		if (try_to_freeze())
			continue;

		/*
		 * P231024-04181
		 * : The group_leader is a zombie, and other threads have exited.
		 *
		 * The group leader has completed do_exit(). However, the binder
		 * thread must call do_notify_parent() so that the parent process
		 * can clean up the MediaProvider process.
		 *
		 * do_exit()
		 *   exit_notify()
		 *     do_notify_parent()
		 */
		if (daemon->exit_state == EXIT_ZOMBIE) {
			ST_LOG("<%s> dev = %u:%u  EXIT_ZOMBIE\n",
					__func__, MAJOR(fc->dev), MINOR(fc->dev));
			cnt++;
			continue;
		}

		/*
		 * P231128-07435
		 * : Waiting for the binder thread to finish core dump.
		 *
		 * all threads wait at
		 *   do_exit()
		 *     coredump_task_exit()
		 */
		if (daemon->flags & PF_POSTCOREDUMP) {
			ST_LOG("<%s> dev = %u:%u  PF_POSTCOREDUMP\n",
					__func__, MAJOR(fc->dev), MINOR(fc->dev));
			cnt++;
			continue;
		}

	}

	if (cnt >= FUSE_WATCHDOG_MAX_WAIT) {
		ST_LOG("<%s> dev = %u:%u  fuse watchdog abort_fuse_conn\n",
				__func__, MAJOR(fc->dev), MINOR(fc->dev));
		fuse_abort_conn(fc);
	}

	ST_LOG("<%s> dev = %u:%u  fuse watchdog end\n",
			__func__, MAJOR(fc->dev), MINOR(fc->dev));

	put_task_struct(daemon);
	kfree(fwd);
	return 0;
}

// TODO: Temp code. Need to change to mount option in the future.
static bool am_i_mp(void)
{
	struct task_struct *gl = get_task_struct(current->group_leader);
	bool ret = false;

	if (!strncmp(gl->comm, "rs.media.module", strlen("rs.media.module")))
		ret = true;

	put_task_struct(gl);

	return ret;
}

void fuse_watchdog_start(struct fuse_conn *fc)
{
	struct fuse_watchdog_data *fwd;
	struct task_struct *watchdog;

	if (!am_i_mp())
		return;

	fwd = kmalloc(sizeof(*fwd), GFP_KERNEL);
	if (!fwd) {
		ST_LOG("<%s> dev = %u:%u  failed to alloc watchdog data\n",
				__func__, MAJOR(fc->dev), MINOR(fc->dev));
		return;
	}

	fwd->fc = fc;
	fwd->daemon = get_task_struct(current->group_leader);

	watchdog = kthread_run(watch_daemon, fwd,
			"fuse-wd%u:%u", MAJOR(fc->dev), MINOR(fc->dev));

	if (IS_ERR(watchdog)) {
		ST_LOG("<%s> dev = %u:%u  failed to start watchdog thread\n",
				__func__, MAJOR(fc->dev), MINOR(fc->dev));
		put_task_struct(fwd->daemon);
		kfree(fwd);
		return;
	}

	fc->watchdog = get_task_struct(watchdog);
}

void fuse_watchdog_stop(struct fuse_conn *fc)
{
	struct task_struct *temp = xchg(&fc->watchdog, NULL);

	if (temp) {
		kthread_stop(temp);
		put_task_struct(temp);
	}
}

#endif /* CONFIG_FUSE_WATCHDOG */

#ifdef CONFIG_FUSE_PERF

/* The opcode must be smaller than 64 to be stored in a single struct xa_node */
#define FUSE_PERF_FIELD_OPCODE GENMASK(5, 0)
#define FUSE_PERF_FIELD_UID GENMASK(63, 6)

/*
 * XArray can store intergers between 0 and LONG_MAX. So, fuse perf is turned on
 * only when BITS_PER_LONG is 64.
 */
#define FUSE_PERF_FILED_CNT GENMASK(15, 0)
#define FUSE_PERF_FILED_SUM GENMASK(47, 16)
#define FUSE_PERF_FILED_WORST GENMASK(62, 48)

#define FUSE_PERF_CNT_MAX ((unsigned long)U16_MAX)
#define FUSE_PERF_SUM_MAX ((unsigned long)U32_MAX)
#define FUSE_PERF_WORST_MAX ((unsigned long)S16_MAX)

#define FUSE_PERF_MAX_OPCODE 62
#define FUSE_PERF_OPCODE_CANONICAL_PATH 63

struct fuse_perf_xa {
	struct xarray xa;
	refcount_t refcount;
};

static void __fuse_perf_xa_init(struct fuse_perf_xa *perf_xa)
{
	xa_init(&perf_xa->xa);
	refcount_set(&perf_xa->refcount, 1);
}

static void __fuse_perf_xa_destroy(struct fuse_perf_xa *perf_xa)
{
	if (perf_xa) {
		xa_destroy(&perf_xa->xa);
		kfree(perf_xa);
	}
}

struct fuse_perf_struct {
	/*
	 * index: 58 bits uid, 6 bits opcode
	 * entry: 15 bits worst (ms), 32 bits sum (ms), 16 bits cnt
	 */
	struct fuse_perf_xa *perf_xa;

	/* To maintain reference through multiple read syscalls*/
	struct fuse_perf_xa *perf_xa_to_read;

	/* Used in proc */
	bool is_eof;

	/* /proc/fuse-perf/<dev id> */
	struct proc_dir_entry *proc_entry;

	/* Protect proc_entry */
	struct mutex lock;

	/* Used when accessing perf_xa */
	spinlock_t spinlock;

	atomic_long_t last_read;
};

void fuse_perf_check_last_read(struct fuse_conn *fc)
{
	long last_read;
	struct fuse_perf_struct *perf_struct = fc->perf_struct;
	struct fuse_perf_xa *perf_xa = NULL;
	struct fuse_perf_xa *perf_xa_to_read = NULL;

	if (!perf_struct)
		return;

	last_read = atomic_long_read(&perf_struct->last_read);
	if (time_is_after_jiffies(last_read + msecs_to_jiffies(60000)))
		return;

	spin_lock(&perf_struct->spinlock);
	perf_xa = xchg(&perf_struct->perf_xa, NULL);
	spin_unlock(&perf_struct->spinlock);

	if (mutex_trylock(&perf_struct->lock)) {
		perf_xa_to_read = xchg(&perf_struct->perf_xa_to_read, NULL);
		mutex_unlock(&perf_struct->lock);
	}

	if (perf_xa && refcount_dec_and_test(&perf_xa->refcount))
		__fuse_perf_xa_destroy(perf_xa);
	if (perf_xa_to_read && refcount_dec_and_test(&perf_xa_to_read->refcount))
		__fuse_perf_xa_destroy(perf_xa_to_read);
}

bool fuse_perf_proc_create(struct proc_dir_entry *parent)
{
	fuse_perf_proc_dir = proc_mkdir("perf", parent);
	return !!(fuse_perf_proc_dir);
}

void fuse_perf_proc_remove(void)
{
	struct proc_dir_entry *temp;

	temp = xchg(&fuse_perf_proc_dir, NULL);
	if (temp)
		proc_remove(temp);
}

static void *fuse_perf_seq_start(struct seq_file *s, loff_t *pos)
{
	struct xa_state *xas;
	struct inode *inode = file_inode(s->file);
	struct fuse_conn *fc = pde_data(inode);
	struct fuse_perf_struct *perf_struct = fc->perf_struct;

	if (!perf_struct)
		return NULL;

	mutex_lock(&perf_struct->lock);

	if (*pos == ULONG_MAX)
		return NULL;

	xas = kzalloc(sizeof(struct xa_state), GFP_KERNEL);
	if (!xas)
		return NULL;
	s->private = xas;

	if (*pos == 0 && !perf_struct->perf_xa_to_read) {
		struct fuse_perf_xa *new;

		new = kzalloc(sizeof(struct fuse_perf_xa), GFP_KERNEL);
		if (new)
			__fuse_perf_xa_init(new);

		/* just store NULL even if memory allocation fails */
		spin_lock(&perf_struct->spinlock);
		xchg(&perf_struct->perf_xa_to_read, perf_struct->perf_xa);
		xchg(&perf_struct->perf_xa, new);
		spin_unlock(&perf_struct->spinlock);

		perf_struct->is_eof = false;
		atomic_long_set(&perf_struct->last_read, jiffies);
	}
	if (!perf_struct->perf_xa_to_read)
		return NULL;

	xas->xa = &perf_struct->perf_xa_to_read->xa;
	xas_set(xas, *pos);

	/*
	 * Because of start() -> show() sequence, next() must be performed to
	 * go to the index of first entry.
	 */
	if (!xas_find(xas, ULONG_MAX)) {
		*pos = ULONG_MAX;
		perf_struct->is_eof = true;
		return NULL;
	}
	*pos = xas->xa_index;

	return xas;
}

static void *fuse_perf_seq_next(struct seq_file *s, void *v, loff_t *pos)
{
	struct inode *inode = file_inode(s->file);
	struct fuse_conn *fc = pde_data(inode);
	struct fuse_perf_struct *perf_struct = fc->perf_struct;
	struct xa_state *xas = s->private;

	if (!xas_find(xas, ULONG_MAX)) {
		*pos = ULONG_MAX;
		perf_struct->is_eof = true;
		return NULL;
	}
	*pos = xas->xa_index;

	return xas;
}

static void fuse_perf_seq_stop(struct seq_file *s, void *v)
{
	struct xa_state *xas;
	struct inode *inode = file_inode(s->file);
	struct fuse_conn *fc = pde_data(inode);
	struct fuse_perf_struct *perf_struct = fc->perf_struct;

	if (!perf_struct)
		return;

	if (perf_struct->is_eof) {
		struct fuse_perf_xa *temp;

		temp = xchg(&perf_struct->perf_xa_to_read, NULL);
		if (refcount_dec_and_test(&temp->refcount))
			__fuse_perf_xa_destroy(temp);
		perf_struct->is_eof = false;
	}

	xas = xchg(&s->private, NULL);
	kfree(xas);

	mutex_unlock(&perf_struct->lock);
}

static int fuse_perf_seq_show(struct seq_file *s, void *v)
{
	struct xa_state *xas = s->private;
	void *entryp;
	unsigned long val;
	uint32_t uid, opcode;
	unsigned long sum, worst, cnt;

	entryp = xas->xa_node->slots[xas->xa_offset];
	val = xa_to_value(entryp);

	uid = FIELD_GET(FUSE_PERF_FIELD_UID, xas->xa_index);
	opcode = FIELD_GET(FUSE_PERF_FIELD_OPCODE, xas->xa_index);

	sum = FIELD_GET(FUSE_PERF_FILED_SUM, val);
	cnt = FIELD_GET(FUSE_PERF_FILED_CNT, val);
	worst = FIELD_GET(FUSE_PERF_FILED_WORST, val);

	seq_printf(s, "uid: %u, opcode: %u, sum: %lu, cnt: %lu, worst: %lu\n",
			uid,
			opcode,
			sum,
			cnt,
			worst);
	return 0;
}

static const struct seq_operations fuse_perf_seq_ops = {
	.start = fuse_perf_seq_start,
	.next = fuse_perf_seq_next,
	.stop = fuse_perf_seq_stop,
	.show = fuse_perf_seq_show
};

static bool __fuse_perf_op(uint32_t opcode)
{
	if (opcode > FUSE_PERF_MAX_OPCODE && opcode != FUSE_CANONICAL_PATH)
		return false;
	return true;
}

/*
 * return: 0 if now allowed
 */
static unsigned long __fuse_perf_make_index(struct fuse_req *req)
{
	unsigned long index;
	uint32_t uid = req->in.h.uid;
	uint32_t opcode = req->in.h.opcode;

	if (!__fuse_perf_op(opcode))
		return 0;
	if (opcode == FUSE_CANONICAL_PATH)
		opcode = FUSE_PERF_OPCODE_CANONICAL_PATH;

	index = FIELD_PREP(FUSE_PERF_FIELD_UID, uid) |
		FIELD_PREP(FUSE_PERF_FIELD_OPCODE, opcode);
	return index;
}

void fuse_perf_start_hook(struct fuse_req *req)
{
	struct fuse_perf_struct *perf_struct = req->fm->fc->perf_struct;
	uint32_t opcode = req->in.h.opcode;

	if (!perf_struct)
		return;
	if (!__fuse_perf_op(opcode))
		return;

	req->dispatch_time = ktime_get();
}

static void __fuse_perf_update_data(struct fuse_perf_xa *perf_xa,
		unsigned long index,
		unsigned long duration)
{
	struct xarray *xa = &perf_xa->xa;
	XA_STATE(xas, xa, index);
	void *old, *new;
	unsigned long val, worst, sum, cnt;

	xas_lock(&xas);

	old = xas_load(&xas);
	if (!old)
		old = xa_mk_value(0);

	val = xa_to_value(old);

	worst = FIELD_GET(FUSE_PERF_FILED_WORST, val);
	worst = clamp(duration, worst, FUSE_PERF_WORST_MAX);

	sum = FIELD_GET(FUSE_PERF_FILED_SUM, val);
	sum = min(sum + duration, FUSE_PERF_SUM_MAX);

	cnt = FIELD_GET(FUSE_PERF_FILED_CNT, val);
	cnt = min(cnt + 1, FUSE_PERF_CNT_MAX);

	val = FIELD_PREP(FUSE_PERF_FILED_WORST, worst) |
		FIELD_PREP(FUSE_PERF_FILED_SUM, sum) |
		FIELD_PREP(FUSE_PERF_FILED_CNT, cnt);

	new = xa_mk_value(val);

	/* Don't retry because it's not important enough */
	xas_store(&xas, new);

	xas_unlock(&xas);
}

void fuse_perf_end_hook(struct fuse_req *req)
{
	struct fuse_perf_struct *perf_struct = req->fm->fc->perf_struct;
	struct fuse_perf_xa *perf_xa;
	unsigned long index;
	unsigned long duration;

	if (!perf_struct)
		return;

	index = __fuse_perf_make_index(req);
	if (index == 0)
		return;

	spin_lock(&perf_struct->spinlock);
	perf_xa = perf_struct->perf_xa;
	if (!perf_xa) {
		spin_unlock(&perf_struct->spinlock);
		return;
	}
	refcount_inc(&perf_xa->refcount);
	spin_unlock(&perf_struct->spinlock);

	duration = (unsigned long)(ktime_get() - req->dispatch_time) / 1000000;
	__fuse_perf_update_data(perf_xa, index, duration);

	if (refcount_dec_and_test(&perf_xa->refcount))
		__fuse_perf_xa_destroy(perf_xa);
}

void fuse_perf_init(struct fuse_conn *fc)
{
	struct fuse_perf_struct *perf_struct;

	if (!fc->perf_node_name || !fuse_perf_proc_dir)
		return;

	perf_struct = kzalloc(sizeof(struct fuse_perf_struct), GFP_KERNEL);
	if (!perf_struct)
		return;

	mutex_init(&perf_struct->lock);
	spin_lock_init(&perf_struct->spinlock);
	perf_struct->proc_entry = proc_create_seq_data(fc->perf_node_name,
			0,
			fuse_perf_proc_dir,
			&fuse_perf_seq_ops,
			fc);
	if (!perf_struct->proc_entry)
		goto free_perf_struct;

	/* When reading the seq_file, it attempts to allocate perf_xa */
	perf_struct->perf_xa = kzalloc(sizeof(struct fuse_perf_xa), GFP_KERNEL);
	if (perf_struct->perf_xa)
		__fuse_perf_xa_init(perf_struct->perf_xa);
	atomic_long_set(&perf_struct->last_read, jiffies);

	fc->perf_struct = perf_struct;
	return;

free_perf_struct:
	kfree(perf_struct);
}

void fuse_perf_destroy(struct fuse_conn *fc)
{
	struct fuse_perf_struct *perf_struct;

	perf_struct = xchg(&fc->perf_struct, NULL);
	if (!perf_struct)
		return;

	proc_remove(perf_struct->proc_entry);
	__fuse_perf_xa_destroy(perf_struct->perf_xa);
	__fuse_perf_xa_destroy(perf_struct->perf_xa_to_read);

	kfree(perf_struct);
}

#endif /* CONFIG_FUSE_PERF */

#ifdef CONFIG_FUSE_INFLIGHT

#define FUSE_INFLIGHT_REQUEST_TIME_MSECS 5000

/* protected by 'fuse_mutex' */
static unsigned int fuse_inflight_dev_id = 1;

struct fuse_inflight_info {
	struct proc_dir_entry *node;

	struct mutex lock;

	/* protected by lock */
	pid_t cur_reader;

	struct ratelimit_state last_read;

	struct ratelimit_state cur_read;
};

static struct fuse_inflight_info *fuse_inflight_info_create(const char *proc_name,
		const struct seq_operations *proc_ops,
		void *proc_data)
{
	struct fuse_inflight_info *fi_info;

	fi_info = kzalloc(sizeof(*fi_info), GFP_KERNEL);
	if (!fi_info)
		return NULL;

	fi_info->node = proc_create_seq_data(proc_name,
			0444,
			fuse_inflight_proc_dir,
			proc_ops,
			proc_data);
	if (!fi_info->node)
		goto free_fi_info;
	mutex_init(&fi_info->lock);
	ratelimit_state_init(&fi_info->last_read, 5 * HZ, 1);
	ratelimit_state_init(&fi_info->cur_read, 5 * HZ, 1);

	return fi_info;
free_fi_info:
	kfree(fi_info);
	return NULL;
}

static void fuse_inflight_info_destroy(struct fuse_inflight_info *fi_info)
{
	if (fi_info) {
		proc_remove(fi_info->node);
		mutex_destroy(&fi_info->lock);
		kfree(fi_info);
	}
}

static bool fuse_inflight_info_idle(struct fuse_inflight_info *fi_info)
{
	return fi_info->cur_reader == 0;
}

static void fuse_inflight_info_set_reader(struct fuse_inflight_info *fi_info)
{
	fi_info->cur_reader = current->pid;
}

static void fuse_inflight_info_clear_reader(struct fuse_inflight_info *fi_info)
{
	fi_info->cur_reader = 0;
}

static bool is_request_over_threshold(struct fuse_req *req)
{
	return time_after(jiffies,
			req->create_time + msecs_to_jiffies(FUSE_INFLIGHT_REQUEST_TIME_MSECS));
}

static bool is_eof(struct seq_file *m)
{
	return m->count == 0;
}

/* Must not return an error */
static struct fuse_req *search_req(struct list_head *head)
{
	struct fuse_req *req;

	req = list_first_entry_or_null(head, struct fuse_req, list);
	if (!req)
		return NULL;
	if (!is_request_over_threshold(req))
		return NULL;
	return req;
}

/* Must not return an error */
static struct fuse_req *search_next_req(struct fuse_req *prev, struct list_head *head)
{
	struct fuse_req *next;

	if (list_is_last(&prev->list, head))
		return NULL;

	next = list_next_entry(prev, list);
	if (!is_request_over_threshold(next))
		return NULL;
	return next;
}

static bool is_read_allowed(struct fuse_inflight_info *fi_info)
{
	pid_t cur_reader = current->pid;

	if (fuse_inflight_info_idle(fi_info)) {
		if (__ratelimit(&fi_info->last_read)) {
			fuse_inflight_info_set_reader(fi_info);
			__ratelimit(&fi_info->cur_read);
			return true;
		}

		return false;
	}

	if (fi_info->cur_reader == cur_reader)
		return true;
	if (__ratelimit(&fi_info->cur_read)) {
		fuse_inflight_info_set_reader(fi_info);
		return true;
	}

	return false;
}

bool fuse_inflight_proc_create(struct proc_dir_entry *parent)
{
	fuse_inflight_proc_dir = proc_mkdir("inflight", parent);
	return !!(fuse_inflight_proc_dir);
}

void fuse_inflight_proc_remove(void)
{
	struct proc_dir_entry *temp;

	temp = xchg(&fuse_inflight_proc_dir, NULL);
	if (temp)
		proc_remove(temp);
}

static void *fuse_inflight_seq_pending_start(struct seq_file *m, loff_t *pos)
{
	struct inode *inode = file_inode(m->file);
	struct fuse_conn *fc = pde_data(inode);
	struct fuse_iqueue *fiq = &fc->iq;
	struct fuse_inflight_info *fi_info = fc->fi_pending;

	mutex_lock(&fi_info->lock);

	/* Must return error to distinguish spin-locked or not */
	if (!is_read_allowed(fi_info))
		return ERR_PTR(-EAGAIN);

	spin_lock(&fiq->lock);
	if (!fiq->connected)
		return NULL;
	if (*pos >= 1)
		return NULL;

	(*pos)++;

	return search_req(&fiq->pending);
}

static void fuse_inflight_seq_pending_stop(struct seq_file *m, void *v)
{
	struct inode *inode = file_inode(m->file);
	struct fuse_conn *fc = pde_data(inode);
	struct fuse_iqueue *fiq = &fc->iq;
	struct fuse_inflight_info *fi_info = fc->fi_pending;

	if (!IS_ERR(v)) {
		spin_unlock(&fiq->lock);
		if (is_eof(m))
			fuse_inflight_info_clear_reader(fi_info);
	}

	mutex_unlock(&fi_info->lock);
}

/* Must not return an error */
static void *fuse_inflight_seq_pending_next(struct seq_file *m, void *v, loff_t *pos)
{
	struct fuse_req *req = v;
	struct inode *inode = file_inode(m->file);
	struct fuse_conn *fc = pde_data(inode);
	struct fuse_iqueue *fiq = &fc->iq;

	if (!fiq->connected)
		return NULL;
	return search_next_req(req, &fiq->pending);
}

static int fuse_inflight_seq_show_common(struct seq_file *m, void *v)
{
	struct fuse_req *req = v;

	seq_printf(m, "opcode: %u, unique: %llu, nodeid: %llu, uid: %u, pid: %u, elasped_ms: %u processed_by: %d\n",
			req->in.h.opcode,
			req->in.h.unique,
			req->in.h.nodeid,
			req->in.h.uid,
			req->in.h.pid,
			jiffies_to_msecs(jiffies - req->create_time),
			req->processed_by);
	return 0;
}

static const struct seq_operations fuse_inflight_seq_pending_ops = {
	.start = fuse_inflight_seq_pending_start,
	.stop = fuse_inflight_seq_pending_stop,
	.next = fuse_inflight_seq_pending_next,
	.show = fuse_inflight_seq_show_common,
};

static void *fuse_inflight_seq_bg_start(struct seq_file *m, loff_t *pos)
{
	struct inode *inode = file_inode(m->file);
	struct fuse_conn *fc = pde_data(inode);
	struct fuse_inflight_info *fi_info = fc->fi_bg;

	mutex_lock(&fi_info->lock);

	/* Must return error to distinguish spin-locked or not */
	if (!is_read_allowed(fi_info))
		return ERR_PTR(-EAGAIN);

	spin_lock(&fc->bg_lock);
	if (!fc->connected)
		return NULL;
	if (*pos >= 1)
		return NULL;

	(*pos)++;

	return search_req(&fc->bg_queue);
}

static void fuse_inflight_seq_bg_stop(struct seq_file *m, void *v)
{
	struct inode *inode = file_inode(m->file);
	struct fuse_conn *fc = pde_data(inode);
	struct fuse_inflight_info *fi_info = fc->fi_bg;

	if (!IS_ERR(v)) {
		spin_unlock(&fc->bg_lock);
		if (is_eof(m))
			fuse_inflight_info_clear_reader(fi_info);
	}

	mutex_unlock(&fi_info->lock);
}

/* Must not return an error */
static void *fuse_inflight_seq_bg_next(struct seq_file *m, void *v, loff_t *pos)
{
	struct fuse_req *req = v;
	struct inode *inode = file_inode(m->file);
	struct fuse_conn *fc = pde_data(inode);

	if (!fc->connected)
		return NULL;
	return search_next_req(req, &fc->bg_queue);
}

static const struct seq_operations fuse_inflight_seq_bg_ops = {
	.start = fuse_inflight_seq_bg_start,
	.stop = fuse_inflight_seq_bg_stop,
	.next = fuse_inflight_seq_bg_next,
	.show = fuse_inflight_seq_show_common,
};

/*
 * Read as musch information as possible from each request in the processing queue
 *
 * @pos: slot index of the processing queue
 */
static void *fuse_inflight_seq_processing_start(struct seq_file *m, loff_t *pos)
{
	struct inode *inode = file_inode(m->file);
	struct fuse_dev *fud = pde_data(inode);
	struct fuse_pqueue *pq = &fud->pq;
	struct fuse_inflight_info *fi_info = fud->fi_processing;
	struct fuse_req *req;

	mutex_lock(&fi_info->lock);

	/* Must return error to distinguish spin-locked or not */
	if (!is_read_allowed(fi_info))
		return ERR_PTR(-EAGAIN);

	spin_lock(&pq->lock);
	if (!pq->connected)
		return NULL;

	/* skip remaining of list */
	if (*pos != 0)
		(*pos)++;
	for (; *pos < FUSE_PQ_HASH_SIZE; (*pos)++) {
		req = search_req(&pq->processing[*pos]);
		if (req)
			return req;
	}

	/* *pos >= FUSE_PQ_HASH_SIZE */
	return NULL;
}

static void fuse_inflight_seq_processing_stop(struct seq_file *m, void *v)
{
	struct inode *inode = file_inode(m->file);
	struct fuse_dev *fud = pde_data(inode);
	struct fuse_pqueue *pq = &fud->pq;
	struct fuse_inflight_info *fi_info = fud->fi_processing;

	if (!IS_ERR(v)) {
		spin_unlock(&pq->lock);
		if (is_eof(m))
			fuse_inflight_info_clear_reader(fi_info);
	}

	mutex_unlock(&fi_info->lock);
}

/* Must not return an error */
static void *fuse_inflight_seq_processing_next(struct seq_file *m, void *v, loff_t *pos)
{
	struct fuse_req *req = v;
	struct inode *inode = file_inode(m->file);
	struct fuse_dev *fud = pde_data(inode);
	struct fuse_pqueue *pq = &fud->pq;

	if (!pq->connected)
		return NULL;

	req = search_next_req(req, &pq->processing[*pos]);
	if (req)
		return req;

	while (++(*pos) < FUSE_PQ_HASH_SIZE) {
		req = search_req(&pq->processing[*pos]);
		if (req)
			return req;
	}

	return NULL;
}

static const struct seq_operations fuse_inflight_seq_processing_ops = {
	.start = fuse_inflight_seq_processing_start,
	.stop = fuse_inflight_seq_processing_stop,
	.next = fuse_inflight_seq_processing_next,
	.show = fuse_inflight_seq_show_common,
};

int fuse_inflight_create_node_on_mount(struct fuse_conn *fc, struct fuse_dev *fud)
{
	char name[NAME_MAX] = {0};

	if (!fuse_inflight_proc_dir)
		return 1;

	scnprintf(name, NAME_MAX, "%i:%i-pending", MAJOR(fc->dev), MINOR(fc->dev));
	fc->fi_pending = fuse_inflight_info_create(name,
			&fuse_inflight_seq_pending_ops,
			fc);

	scnprintf(name, NAME_MAX, "%i:%i-bg", MAJOR(fc->dev), MINOR(fc->dev));
	fc->fi_bg = fuse_inflight_info_create(name,
			&fuse_inflight_seq_bg_ops,
			fc);

	scnprintf(name, NAME_MAX, "%i:%i-processing0", MAJOR(fc->dev), MINOR(fc->dev));
	fud->fi_processing = fuse_inflight_info_create(name,
			&fuse_inflight_seq_processing_ops,
			fud);

	return 0;
}

void fuse_inflight_remove_node(struct fuse_conn *fc)
{
	struct fuse_inflight_info *fi_info;

	if (!fuse_inflight_proc_dir)
		return;

	fi_info = xchg(&fc->fi_pending, NULL);
	fuse_inflight_info_destroy(fi_info);

	fi_info = xchg(&fc->fi_bg, NULL);
	fuse_inflight_info_destroy(fi_info);
}

int fuse_inflight_create_processing_node_on_clone(struct fuse_dev *fud)
{
	struct fuse_conn *fc = fud->fc;
	char name[NAME_MAX] = {0};

	if (!fuse_inflight_proc_dir)
		return 1;

	scnprintf(name, NAME_MAX - 1, "%i:%i-processing%d",
			MAJOR(fc->dev),
			MINOR(fc->dev),
			fuse_inflight_dev_id++);
	fud->fi_processing = fuse_inflight_info_create(name,
			&fuse_inflight_seq_processing_ops,
			fud);

	return 0;
}

void fuse_inflight_remove_processing_node(struct fuse_dev *fud)
{
	struct fuse_inflight_info *fi_info;

	if (!fuse_inflight_proc_dir)
		return;

	fi_info = xchg(&fud->fi_processing, NULL);
	fuse_inflight_info_destroy(fi_info);
}

#endif /* CONFIG_FUSE_INFLIGHT */
