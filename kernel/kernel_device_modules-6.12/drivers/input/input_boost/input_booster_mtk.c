#include <linux/input/input_booster.h>
#if IS_ENABLED(CONFIG_MTK_DVFSRC)
#include <linux/interconnect.h>
#include <linux/platform_device.h>
#include "dvfsrc-exp.h"
#endif

#if IS_ENABLED(CONFIG_CPU_FREQ_LIMIT) || IS_ENABLED(CONFIG_CPU_FREQ_LTL_LIMIT)
#define DVFS_TOUCH_ID 1
#else
#define DVFS_TOUCH_ID 0
int set_freq_limit(unsigned long id, unsigned int freq)
{
	pr_err("%s is not yet implemented\n", __func__);
	return 0;
}
#endif

#if IS_ENABLED(CONFIG_MTK_DVFSRC)
#define DDR_OPP_NUM 4
#define DDR_4_IDX 0
#define DDR_5_IDX 1
#define DDR_FREQ_IDX 0
#define DDR_OPP_IDX 1
#define DDR_BW_IDX 2
#define DDR4_THRESHOLD 5000

static struct icc_path *bw_path;
unsigned int dvfsrc_opp_bw_table[DDR_OPP_NUM];
unsigned int dvfsrc_opp_table[2][DDR_OPP_NUM][3] = {
	{
		{4000, 0, 0},
		{3000, 2, 0},
		{2000, 3, 0},
		{1800, 6, 0}
	},
	{
		{9000, 0, 0},
		{8000, 2, 0},
		{7000, 5, 0},
		{6800, 8, 0}	
	}
};
int dvfsrc_opp_table_d5[DDR_OPP_NUM][3] = {

};

inline int trans_freq_to_bw(long request_ddr_freq)
{
	int i = 0;
	int ddr_idx;

	if (request_ddr_freq <= 0) {
		return 0;
	}
	
	if (request_ddr_freq > DDR4_THRESHOLD)
		ddr_idx = DDR_5_IDX;
	else
		ddr_idx = DDR_4_IDX;

	for (i = 0; i < DDR_OPP_NUM-1; i++) {
		if (request_ddr_freq > dvfsrc_opp_table[ddr_idx][i][DDR_FREQ_IDX]) {
			pr_booster("%s :: ddr freq value : freq=%ld opp=%u bw=%u", __func__, 
				request_ddr_freq, dvfsrc_opp_table[ddr_idx][i][DDR_OPP_IDX], dvfsrc_opp_table[ddr_idx][i][DDR_BW_IDX]);	
			return dvfsrc_opp_table[ddr_idx][i][DDR_BW_IDX];
		}
	}

	return dvfsrc_opp_table[ddr_idx][DDR_OPP_NUM-1][DDR_BW_IDX];
}
#endif

void ib_set_booster(long *qos_values)
{
	long value = -1;
	int res_type = 0;
	int ddr_level = 0;
	int cur_res_idx;
	int rc = 0;

	for (res_type = 0; res_type < allowed_res_count; res_type++) {

		cur_res_idx = allowed_resources[res_type];
		value = qos_values[cur_res_idx];

		if (value <= 0 && cur_res_idx != SCHEDBOOST)
			continue;

		switch (cur_res_idx) {
		case CPUFREQ:
			set_freq_limit(DVFS_TOUCH_ID, value);
			pr_booster("%s :: cpufreq value : %ld", __func__, value);
			break;
		case DDRFREQ:
#if IS_ENABLED(CONFIG_MTK_DVFSRC)
			if (bw_path != NULL)
				icc_set_bw(bw_path, 0, trans_freq_to_bw(value));
#endif
			break;
		default:
			pr_booster("%s :: cur_res_idx : %d is not used", __func__, cur_res_idx);
			break;
		}
	}
}

void ib_release_booster(long *rel_flags)
{
	int flag;
	int rc = 0;
	long value;

	int res_type = 0;
	int cur_res_idx;

	for (res_type = 0; res_type < allowed_res_count; res_type++) {

		cur_res_idx = allowed_resources[res_type];
		flag = rel_flags[cur_res_idx];
		if (flag <= 0)
			continue;

		value = release_val[cur_res_idx];

		switch (cur_res_idx) {
		case CPUFREQ:
			set_freq_limit(DVFS_TOUCH_ID, value);
			pr_booster("%s :: cpufreq value : %ld", __func__, value);
			break;
		case DDRFREQ:
#if IS_ENABLED(CONFIG_MTK_DVFSRC)
			if (bw_path != NULL)
				icc_set_bw(bw_path, 0, 0);
#endif
			break;
		default:
			pr_booster("%s :: cur_res_idx : %d is not used", __func__, cur_res_idx);
			break;
		}
	}
}

#if IS_ENABLED(CONFIG_MTK_DVFSRC)
static const struct of_device_id input_booster_dt_match[] = {
	{ .compatible = "samsung,input_booster", },
	{},
};

static int sec_input_booster_probe(struct platform_device *pdev)
{
	int idx = 0;

	bw_path = of_icc_get(&pdev->dev, "inputbooster-perf-bw");
	if (IS_ERR(bw_path)) {
		dev_info(&pdev->dev, "get inputbooster-perf-bw fail\n");
		bw_path = NULL;
	}
	
	for ( idx = 0; idx < DDR_OPP_NUM; idx++) {
		//dvfsrc_opp_bw_table[idx] = dvfsrc_get_required_opp_peak_bw(pdev->dev.of_node, idx);
		dvfsrc_opp_table[DDR_4_IDX][idx][DDR_BW_IDX] = 
			dvfsrc_get_required_opp_peak_bw(pdev->dev.of_node, dvfsrc_opp_table[DDR_4_IDX][idx][DDR_OPP_IDX]); 
		dvfsrc_opp_table[DDR_5_IDX][idx][DDR_BW_IDX] = 
			dvfsrc_get_required_opp_peak_bw(pdev->dev.of_node, dvfsrc_opp_table[DDR_5_IDX][idx][DDR_OPP_IDX]); 
	}

	return 0;
}

/*
static void sec_input_booster_remove(struct platform_device *pdev)
{
	return 0;
}
*/

struct platform_driver sec_input_booster_driver = {
	.probe = sec_input_booster_probe,
	//.remove = sec_input_booster_remove,
	.driver = {
		.name = "sec_input_booster",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(input_booster_dt_match),
	},
};
#endif

int input_booster_init_vendor(void)
{
#if IS_ENABLED(CONFIG_MTK_DVFSRC)
	platform_driver_register(&sec_input_booster_driver);
#endif

	return 1;
}

void input_booster_exit_vendor(void)
{
#if IS_ENABLED(CONFIG_MTK_DVFSRC)
	platform_driver_unregister(&sec_input_booster_driver);
#endif
}
