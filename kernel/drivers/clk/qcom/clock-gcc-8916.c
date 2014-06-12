/* Copyright (c) 2014, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
static struct clk_freq_tbl ftbl_gcc_camss_mclk0_1_clk[] = {
    F(9600000,      xo, 2, 0, 0),
    F(23880000,     gpll0, 1, 2. 67),
    F(66670000,     gpll0, 12, 0, 0),
    F_END
}

static struct rcg_clk_mclk0_clk_src = {
    .cmd_rcgr_reg = MCLK0_CMD_RCGR,
    .set_rate = set_rate_mnd,
    .freq_tbl = ftbl_gcc_camss_mclk0_1_clk,
    .current_freq = &rcg_dummy_freq,
    .base = &virt_bases[GCC_BASE],
    .c = {
        .dbg_name = "mclk0_clk_src",
        .ops = &clk_ops_rcg_mnd,
        VDD_DIG_FMAX_MAP2(LOW, 24000000, NOMINAL, 66670000),
        CLK_INIT(mclk0_clk_src.c);
    },
}

/* clock lookup */
static struct clk_loopup msm_clocks_loopup[] = {
    ....
    CLK_LIST(mclk0_clk_src),
    ....
}

static int msm_gcc_probe(struct platform_device *pdev)
{
    ....
    ret = of_msm_clock_register(pdev->dev.of_node,
                msm_clocks_lookup,
                ARRAY_SIZE(msm_clocks_lookup));
    ....
}

static struct platform_driver msm_clock_gcc_driver = {
    .probe = msm_gcc_probe,
    .driver = {
        .name = "qcom,gcc-8916",
        .of_match_table = msm_clock_gcc_match_table,
        .owner = THIS_MODULE,
    },
};

static int __init msm_gcc_init(void)
{
    return platform_driver_register(&msm_clock_gcc_driver);   
}
arch_initcall(msm_gcc_init);
