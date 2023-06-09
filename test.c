#include <linux/ftrace.h>
#include <linux/kallsyms.h>
#include <linux/kernel.h>
#include <linux/linkage.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <linux/kprobes.h>
#include <linux/delay.h>
#include <linux/seq_file.h>
#include <linux/memcontrol.h>

struct ftrace_handler {
	const char *symbol;
	unsigned long new_addr;
	unsigned long old_addr;
	struct ftrace_ops ops;
};

struct parent_symbol {
	const char *symbol;
	unsigned long start_addr;
	unsigned long end_addr;
};

static struct ftrace_handler g_handler = {
	.symbol = "__memory_events_show",
};

static struct parent_symbol g_parent_symbol[] = {
	{
		.symbol = "memory_events_show",
	},
};

static char g_buffer[100];

static void __memory_events_show_new(struct seq_file *m, atomic_long_t *events)
{
	seq_printf(m, "This is from __memory_events_show_new\n");
	seq_printf(m, "low %lu\n", atomic_long_read(&events[MEMCG_LOW]));
	seq_printf(m, "high %lu\n", atomic_long_read(&events[MEMCG_HIGH]));
	seq_printf(m, "max %lu\n", atomic_long_read(&events[MEMCG_MAX]));
	seq_printf(m, "oom %lu\n", atomic_long_read(&events[MEMCG_OOM]));
	seq_printf(m, "oom_kill %lu\n",
		   atomic_long_read(&events[MEMCG_OOM_KILL]));
}

static void notrace test_ftrace_handler(unsigned long ip,
					   unsigned long parent_ip,
					   struct ftrace_ops *fops,
					   struct pt_regs *regs)
{
	int i = 0;
	struct ftrace_handler *handler;

	handler = container_of(fops, struct ftrace_handler, ops);

	for (i = 0; i < ARRAY_SIZE(g_parent_symbol); ++i) {
		if (g_parent_symbol[i].start_addr <= parent_ip &&
			parent_ip < g_parent_symbol[i].end_addr) {
			regs->ip = handler->new_addr;
		}
	}
}

static void register_handler(struct ftrace_handler *handler)
{
	int ret;
	int i = 0;
	unsigned long size;
	char *slash;

	handler->old_addr = kallsyms_lookup_name(handler->symbol);
	handler->new_addr = (unsigned long)&__memory_events_show_new;
	handler->ops.func = test_ftrace_handler;
	handler->ops.flags = FTRACE_OPS_FL_SAVE_REGS |
		  FTRACE_OPS_FL_DYNAMIC |
		  FTRACE_OPS_FL_IPMODIFY;

	for (i = 0; i < ARRAY_SIZE(g_parent_symbol); ++i) {
		memset(g_buffer, 0, sizeof(g_buffer));
		g_parent_symbol[i].start_addr = kallsyms_lookup_name(g_parent_symbol[i].symbol);
		ret = sprint_symbol(g_buffer, g_parent_symbol[i].start_addr);
		slash = strchr(g_buffer, '/');
		ret = sscanf(slash + 1, "%lx", &size);
		g_parent_symbol[i].end_addr = g_parent_symbol[i].start_addr + size;

		pr_info("parent_symbol = %s, start_addr = 0x%lx, end_addr = 0x%lx, buffer = %s\n",
			g_parent_symbol[i].symbol, g_parent_symbol[i].start_addr, g_parent_symbol[i].end_addr,
			g_buffer);
	}

	ret = ftrace_set_filter_ip(&handler->ops, handler->old_addr, 0, 0);
	ret = register_ftrace_function(&handler->ops);
}

static void unregister_handler(struct ftrace_handler *handler)
{
	unregister_ftrace_function(&handler->ops);
	ftrace_set_filter_ip(&handler->ops, handler->old_addr, 1, 0);
}

static int test_hook_init(void)
{
	register_handler(&g_handler);
	pr_info("test hook init\n");
	return 0;
}

static void test_hook_exit(void)
{
	unregister_handler(&g_handler);

	synchronize_rcu();
	msleep(100);

	pr_info("test hook exit\n");
}

module_init(test_hook_init);
module_exit(test_hook_exit);

MODULE_LICENSE("GPL");
