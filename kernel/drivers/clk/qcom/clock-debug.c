/*
 * Copyright (C) 2007 Google, Inc.
 * Copyright (c) 2007-2013, The Linux Foundation. All rights reserved.
 * 
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
 
/* czm: for seq_file interface*/ 
#include <linux/seq_file.h>
....

#define clock_debug_output(m, c, fmt, ...)	\
do{											\
	if(m)									\
		seq_printf(m, fmt, ##__VA_ARGS__);	\
	else if (c)								\
		pr_cont(fmt, ##__VA_ARGS__);		\
	else									\
		pr_info(fmt, ##__VA_ARGS__);		\
}while (0)

static int clock_debug_print_clock(struct clk *c, struct seq_file m)
{
	char *start = "";
	
	if(!c || !c->prepare_count)
		return 0;
	
	clock_debug_output(m, 0, "\t");
	do{
		if(c->vdd_class)
			clock_debug_output(m, 1, "%s%s:%u:%u [%ld, %lu]", start,
			c->dbg_name, c->prepare_count, c->count,
			c->rate, c->vdd_class->cur_level);
		else
			clock_debug_output(m, 1, "%s%s:%u:%u [%ld]", start,
			c->dbg_name, c->prepare_count, c->count,
			c->rate);
		start = " -> ";
	}while((c = clk_get_parent(c)));
	
	clock_debug_output(m, 1, "\n");
	
	return 1;
}

/**
 * clock_debug_print_enabled_clocks() - Print names of enabled clocks
 * 
 */
static void clock_debug_print_enabled_clocks(struct seq_file *m)
{
	struct clk_table *table;
	int i, cnt = 0;
	
	if(!mutex_trylock(&clk_list_lock)){
		pr_err("clock_debug: Clocks are being registered. Cannot print clock state now. \n");
		return;
	}
	clock_debug_output(m, 0, "Enabled clocks:\n");
	list_for_each_entry(table, &clk_list, node){
		for(i = 0; i < table->num_clocks; i++)
			cnt += clock_debug_print_clock(table->clocks[i].clk, m);
	}
	mutex_unlock(&clk_list_lock);
	
	if(cnt)
		clock_debug_output(m, 0, "Enabled clock count: %\n", cnt);
	else
		clock_debug_output(m, 0, "No clocks enabled.\n");
}

static int enabled_clocks_show(struct seq_file *m, void *unused)
{
	clock_debug_print_enabled_clocks(m);
	return 0;
}

static int enabled_clocks_open(struct inode *inode, struct file *file)
{
	return single_open(file, enabled_clocks_show, inode->i_private);
}

static const struct file_operations enabled_clocks_fops = {
	.open	= enabled_clocks_open,
	.read	= seq_read,
	.llseek = seq_lseek,
	.release = seq_release, 
}

/**
 *	clock_debug_init() - Initialize clock debugfs
 * Lock clk_debug_lock before invoking this function.
 */
static int clock_debug_init(void) 
{
	if(clk_debug_init_once)
		return 0;
		
	clk_debug_init_once = 1;
	
	debugfs_base = debugfs_create_dir("clk", NULL);
	if(!debugfs_base)
		return -ENOMEM;
	
	if(!debug_create_u32("debug_suspend", S_IRUGO | S_IWUSR,
			debugfs_base, &debug_suspend)){
		debugfs_remove_recursive(debugfs_base);			
		return -ENOMEM;
	}
	
	if(!debugfs_create_file("enabled_clocks", S_IRUGO, debugfs_base, NULL,
		&enabled_clocks_fops))
	return -ENOMEM;
	
	return 0;
}



