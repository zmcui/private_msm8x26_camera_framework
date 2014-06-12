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

static int clock_debug_add(struct clk *clock)
{
	char temp[50], *ptr;
	struct dentry *clk_dir;
	
	if(!debugfs_base)
		return -ENOMEM;
		
	strlcpy(temp, clock->dbg_name, ARRAY_SIZE(temp));
	for(ptr = temp; *ptr; ptr++)
		*ptr = tolower(*ptr)
		
	clk_dir = debugfs_create_dir(temp, debugfs_base);
	if(!clk_dir)
		return -ENOMEM;
	
	/*czm: struct dentry *clk_dir in struct clk{} in inlcude/linux/clk/msm-clk-provider.h */	
	clock->clk_dir = clk_dir;
	
	if(!debugfs_create_file("rate", S_IRUGO | S_IWUSR, clk_dir,
			clock, &clock_rate_fops))
		goto error;
		
	if(!debugfs_create_file("enable", S_IRUGO | S_IWUSR, clk_dir,
			clock, &clock_enable_fops))
		goto error;
	
	if(!debugfs_create_file("is_local", S_IRUGO | S_IWUSR, clk_dir,
			clock, &clock_local_fops))
		goto error;
	
	if(!debugfs_create_file("has_hw_gating", S_IRUGO | S_IWUSR, clk_dir,
			clock, &clock_hwcg_fops))
		goto error;
		
	if(clock->ops->list_rate)
		if(!debugfs_create_file("list_rates"
			S_IRUGO, clk_dir, clock, &list_rates_fops))
			goto error;
	
	if(clock->vdd_class && !debugfs_create_file("fmax_rates",
			S_IRUGO, clk_dir, clock, &fmax_rates_fops))
		goto error;
		
	if(!debugfs_create_file("parent", S_IRUGO, clk_dir, clock,
			&clock_parent_fops))
		goto error;
		
	if(!debugfs_create_file("print", S_IRUGO, clk_dir, clock,
			&clock_print_hw_fops))
		goto error;
	
	clock_measure_add(clock);
	
	return 0;
error:
	debugfs_remove_recursive(clk_dir);
	return -ENOMEM;
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

/**
 * clock_debug_register() : Add additional clocks to clock debugfs hierarchy
 * @table: Table of clocks to create debugfs nodes for
 * @size : size of @table
 */
int clock_debug_register(struct clk_lookup *table, size_t size)
{
	struct clk_table *clk_table, *clk_table_tmp;
	int i, ret;
	
	mutex_lock(&clk_debug_lock);
	
	ret = clock_debug_init();
	if(ret)
		goto out;
		
	clk_table = kmalloc(sizeof(*clk_table), GFP_KERNEL);
	if(!clk_table){
		ret = -ENOMEM;
		goto out;
	}
	
	clk_table->clocks = table;
	clk_table->num_clocks = size;
	
	if(IS_ERR_OR_NULL(measure)){
		measure = clk_get_sys("debug", "measure");
		if(!IS_ERR(measure)){
			mutex_lock(&clk_list_lock);
			list_for_each_entry(clk_table_tmp, &clk_list, node){
			for(i = 0; i < clk_tbl_tmp->num_clocks; i++)
				clock_measure_add(clk_table_tmp->clocks[i].clk);
			}
			mutex_unlock(&clk_list_lock);
		}
	}
	
	mutex_lock(&clk_list_lock);
	list_add_tail(&clk_table->node, &clk_list);
	mutex_unlock(&clk_list_lock);
	
	for(i = 0; i < size; i++)
		clock_debug_add(table[i].clk);
out:
	mutex_unlock(&clk_debug_lock);
	return ret;
}	

