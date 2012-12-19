/* Copyright (c) 2012-2013, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/cpufreq.h>
#include <linux/mutex.h>
#include <linux/msm_tsens.h>
#include <linux/workqueue.h>
#include <linux/cpu.h>

/*
 * Poll for temperature changes every 2 seconds.
 * It will scale based on the device temperature.
 */
unsigned int polling = HZ*2;

unsigned int temp_threshold = 60;
module_param(temp_threshold, int, 0755);

static struct msm_thermal_data msm_thermal_info;

static struct workqueue_struct *wq;
static struct delayed_work check_temp_work;

struct cpufreq_policy *policy = NULL;

unsigned int max_freq;
unsigned int freq_buffer;

unsigned short get_threshold(void)
{
	return temp_threshold;
#define DEF_TEMP_SENSOR      0
#define DEF_THERMAL_CHECK_MS 1250
#define DEF_ALLOWED_MAX_HIGH 70
#define DEF_ALLOWED_MAX_FREQ 810000

static int enabled;
static int allowed_max_high = DEF_ALLOWED_MAX_HIGH;
static int allowed_max_low = (DEF_ALLOWED_MAX_HIGH - 10);
static int allowed_max_freq = DEF_ALLOWED_MAX_FREQ;
static int check_interval_ms = DEF_THERMAL_CHECK_MS;
static int thermal_throttled = 0;
static int pre_throttled_max = 0;

module_param(allowed_max_high, int, 0);
module_param(allowed_max_freq, int, 0);
module_param(check_interval_ms, int, 0);

static struct delayed_work check_temp_work;

static int update_cpu_max_freq(struct cpufreq_policy *cpu_policy,
			       int cpu, int max_freq)
{
	int ret = 0;

	if (!cpu_policy)
		return -EINVAL;

	cpufreq_verify_within_limits(cpu_policy,
				cpu_policy->min, max_freq);
	cpu_policy->user_policy.max = max_freq;

	ret = cpufreq_update_policy(cpu);
	if (!ret)
		pr_info("msm_thermal: Limiting core%d max frequency to %d\n",
			cpu, max_freq);

	return ret;
}

static void check_temp(struct work_struct *work)
{
	struct tsens_device tsens_dev;
	long temp = 0;
    unsigned int cpu;

	policy = cpufreq_cpu_get(0);
	max_freq = policy->max;
	
	if (freq_buffer == 0)
		freq_buffer = max_freq;

	tsens_dev.sensor_num = msm_thermal_info.sensor_id;
	tsens_get_temp(&tsens_dev, &temp);

	/* device is really hot, it needs severe throttling even 
	   if it means a lag fest. Also poll faster */        
	if (temp >= (temp_threshold + 10)) 
	{
		max_freq = 702000;
		polling = HZ/8;
	}

	/* temperature is high, lets throttle even more and 
	   poll faster (every .25s) */
	else if (temp >= temp_threshold) 
	{
		max_freq = 1026000;
		polling = HZ/4;
	}

	/* the device is getting hot, lets throttle a little bit */
	else if (temp >= (temp_threshold - 5)) 
	{
		max_freq = 1350000;
	} 

	/* the device is in safe temperature, polling is normal (every second) */
	else if (temp < (temp_threshold - 10)) 
	{
		polling = HZ*2;
	}

	if (max_freq < freq_buffer || max_freq > freq_buffer) 
	{
		freq_buffer = max_freq;

		/* blocks hotplug operations - critical in this code path */
		get_online_cpus();
		for_each_possible_cpu(cpu) 
		{
			msm_cpufreq_set_freq_limits(cpu, MSM_CPUFREQ_NO_LIMIT, max_freq);
			pr_info("Thermal Throttle: max cpu%d frequency changes to %dMHz -" 
				"polling every %dms", 
				cpu, 
				max_freq/1000, 
				jiffies_to_msecs(polling));
		}
		put_online_cpus();
	struct cpufreq_policy *cpu_policy = NULL;
	struct tsens_device tsens_dev;
	unsigned long temp = 0;
	unsigned int max_freq = 0;
	int update_policy = 0;
	int cpu = 0;
	int ret = 0;

	tsens_dev.sensor_num = DEF_TEMP_SENSOR;
	ret = tsens_get_temp(&tsens_dev, &temp);
	if (ret) {
		pr_err("msm_thermal: Unable to read TSENS sensor %d\n",
				tsens_dev.sensor_num);
		goto reschedule;
	}

	for_each_possible_cpu(cpu) {
		update_policy = 0;
		cpu_policy = cpufreq_cpu_get(cpu);
		if (!cpu_policy) {
			pr_debug("msm_thermal: NULL policy on cpu %d\n", cpu);
			continue;
		}
		if (temp >= allowed_max_high) {
			if (cpu_policy->max > allowed_max_freq) {
				update_policy = 1;
				/* save pre-throttled max freq value */
				pre_throttled_max = cpu_policy->max;
				max_freq = allowed_max_freq;
				thermal_throttled = 1;
				pr_warn("Thermal Throttled! Set max freq to: \
					 %u\n", max_freq);
			} else {
				pr_debug("msm_thermal: policy max for cpu %d "
					 "already < allowed_max_freq\n", cpu);
			}
		} else if (temp < allowed_max_low && thermal_throttled) {
			if (cpu_policy->max < cpu_policy->cpuinfo.max_freq) {
				if (pre_throttled_max != 0)
					max_freq = pre_throttled_max;
				else
					max_freq = cpu_policy->
						cpuinfo.max_freq;
				update_policy = 1;
				/* wait until 2nd core is unthrottled */
				if (cpu == 1)
					thermal_throttled = 0;
				pr_warn("Thermal Throttling Ended! restore \
					max freq to: %u\n", max_freq);
			} else {
				pr_debug("msm_thermal: policy max for cpu %d "
					 "already at max allowed\n", cpu);
			}
		}

		if (update_policy)
			update_cpu_max_freq(cpu_policy, cpu, max_freq);

		cpufreq_cpu_put(cpu_policy);
	}

reschedule:
	if (enabled)
		schedule_delayed_work(&check_temp_work,
				msecs_to_jiffies(check_interval_ms));
}

static void disable_msm_thermal(void)
{
	int cpu = 0;
	struct cpufreq_policy *cpu_policy = NULL;

	/* make sure check_temp is no longer running */
	cancel_delayed_work(&check_temp_work);
	flush_scheduled_work();

	if (limited_max_freq == MSM_CPUFREQ_NO_LIMIT)
		return;

	for_each_possible_cpu(cpu) {
		cpu_policy = cpufreq_cpu_get(cpu);
		if (cpu_policy) {
			if (cpu_policy->max < cpu_policy->cpuinfo.max_freq)
				update_cpu_max_freq(cpu_policy, cpu,
						    cpu_policy->
						    cpuinfo.max_freq);
			cpufreq_cpu_put(cpu_policy);
		}
	}

	queue_delayed_work(wq, &check_temp_work, polling);
}

int __devinit msm_thermal_init(struct msm_thermal_data *pdata)
{
	int ret = 0;

	BUG_ON(!pdata);
	BUG_ON(pdata->sensor_id >= TSENS_MAX_SENSORS);
	memcpy(&msm_thermal_info, pdata, sizeof(struct msm_thermal_data));

	wq = alloc_workqueue("msm_thermal_workqueue", WQ_HIGHPRI, 0);
    
    if (!wq)
        return -ENOMEM;

static struct kernel_param_ops module_ops = {
	.set = set_enabled,
	.get = param_get_bool,
};

module_param_cb(enabled, &module_ops, &enabled, 0644);
MODULE_PARM_DESC(enabled, "enforce thermal limit on cpu");

static int __init msm_thermal_init(void)
{
	int ret = 0;

	enabled = 1;

	INIT_DELAYED_WORK(&check_temp_work, check_temp);
	queue_delayed_work(wq, &check_temp_work, HZ*30);

	return ret;
}
fs_initcall(msm_thermal_init);

static int __devinit msm_thermal_dev_probe(struct platform_device *pdev)
{
	int ret = 0;
	char *key = NULL;
	struct device_node *node = pdev->dev.of_node;
	struct msm_thermal_data data;

	memset(&data, 0, sizeof(struct msm_thermal_data));
	key = "qcom,sensor-id";
	ret = of_property_read_u32(node, key, &data.sensor_id);
	if (ret)
		goto fail;
	WARN_ON(data.sensor_id >= TSENS_MAX_SENSORS);

fail:
	if (ret)
		pr_err("%s: Failed reading node=%s, key=%s\n",
		       __func__, node->full_name, key);
	else
		ret = msm_thermal_init(&data);

	return ret;
}

static struct of_device_id msm_thermal_match_table[] = {
	{.compatible = "qcom,msm-thermal"},
	{},
};

static struct platform_driver msm_thermal_device_driver = {
	.probe = msm_thermal_dev_probe,
	.driver = {
		.name = "msm-thermal",
		.owner = THIS_MODULE,
		.of_match_table = msm_thermal_match_table,
	},
};

int __init msm_thermal_device_init(void)
{
	return platform_driver_register(&msm_thermal_device_driver);
}
