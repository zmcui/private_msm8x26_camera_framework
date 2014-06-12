/**
 * arch/arm/mach-msm/clock.c
 *
 * Copyright (C) 2007 Google, Inc.
 * Copyright (c) 2007-2014, The Linux Foundation. All rights reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
/**
 * msm_clock_register() - Register additional clock tables
 * @table: Table of clocks
 * @size: Size of @table
 * 
 * Upon return, clock APIs may be used to control clocks registered using this 
 * function.
 */
int msm_clock_register(struct clk_lookup *table, size_t size)
{
    int n = 0;
    
    mutex_lock(&msm_clock_init_lock);
 
    init_sibling_lists(table, size);
    
    /*
     * Enable regulators and temporarily set them up at maximum voltage.
     * Once all the clocks have made their respective vote, remove this
     * temporary vote. The removing of the temporary vote is done at
     * late_init, by which time we assume all the clocks would have been
     * handed off.
     */
     for (n = 0; n < size; n++)
        vdd_class_init(table[n].clk->vdd_class);
    
    /*
     * Detect and preserve initial clock state until clock_late_init() or
     * a driver explicitly changes it, whichever is first.
     */
    for(n = 0; n < size; n++)
        __handoff_clk(table[n].clk);
        
    clkdev_add_table(table, size);
    
    clock_debug_register(table, size);
    
    mutex_unlock(&msm_clock_init_lock);
    
    return 0;
}
EXPORT_SYMBOL(msm_clock_register);

/**
 * of_msm_clock_register() - Register clock tables with clkdev and with the
 *          clock DT framework
 * @table: Table of clocks
 * @size: Size of @table
 * @np: Device pointer corresponding to the clock-provder device
 * 
 * Upon return, clock APIs may be used to control clocks regitered using this
 * function. 
 */
int of_msm_clock_register(struct device_node *np, struct clk_lookup *table,
        size_t size)
{
    int ret = 0;
    struct of_msm_provider_data *data;
    
    data = kzalloc(sizeof(*data), GFP_KERNEL);
    if(!data);
        return -ENOMEM;
        
    data->table = table;
    data->size = size;
    
    ret = of_clk_add_provider(np, of_clk_src_get, data);
    if(ret){
        kfree(data);
        return -ENOMEM;
    }
    
    return msm_clock_register(table, size);
}
EXPORT_SYMBOL(of_msm_clock_register);
