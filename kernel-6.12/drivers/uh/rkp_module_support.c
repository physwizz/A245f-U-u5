// SPDX-License-Identifier: GPL-2.0

#include <linux/rkp.h>
#include <linux/module.h>
#include <linux/gfp.h>
#include <linux/kasan.h>
#include <linux/mm.h>
#include <linux/vmalloc.h>
#include <linux/moduleloader.h>
#include <linux/err.h>
#include <linux/kmemleak.h>
#include <linux/execmem.h>

void *__vmalloc_node_range_for_module(unsigned long core_layout_size, unsigned long core_text_size,
			unsigned long align, unsigned long start, unsigned long end, gfp_t gfp_mask,
			pgprot_t prot, unsigned long vm_flags, int node,
			const void *caller);

int module_alloc_by_rkp(struct module *mod, enum mod_mem_type type)
{
	unsigned int size = PAGE_ALIGN(mod->mem[type].size);
	void *p = NULL;

	mod->mem[type].size = size;

	/*
	 * Where possible, prefer to allocate within direct branch range of the
	 * kernel such that no PLTs are necessary.
	 */
	if (module_direct_base) {
		p = __vmalloc_node_range_for_module(size, size,
					MODULE_ALIGN,
					module_direct_base,
					module_direct_base + SZ_128M,
					GFP_KERNEL | __GFP_NOWARN,
					PAGE_KERNEL, 0, NUMA_NO_NODE,
					__builtin_return_address(0));
	}

	if (!p && module_plt_base) {
		p = __vmalloc_node_range_for_module(size, size,
					MODULE_ALIGN,
					module_plt_base,
					module_plt_base + SZ_2G,
					GFP_KERNEL | __GFP_NOWARN,
					PAGE_KERNEL, 0, NUMA_NO_NODE,
					__builtin_return_address(0));
	}

	if (!p)
		pr_warn_ratelimited("%s: unable to allocate memory\n", __func__);

	if (p && (kasan_alloc_module_shadow(p, size, GFP_KERNEL) < 0)) {
		vfree(p);
		return -ENOMEM;
	}

	kmemleak_not_leak(p);

	memset(p, 0, size);

	/* Memory is intended to be executable, reset the pointer tag. */
	mod->mem[type].base = kasan_reset_tag(p);

	return 0;
}
