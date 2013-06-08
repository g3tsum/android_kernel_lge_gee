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
#include <linux/mutex.h>
#include <linux/msm_tsens.h>
#include <linux/workqueue.h>
#include <linux/cpu.h>
#include <linux/cpufreq.h>
#include <linux/msm_tsens.h>
#include <linux/msm_thermal.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <mach/cpufreq.h>

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
static bool core_control_enabled;
static uint32_t cpus_offlined;
static DEFINE_MUTEX(core_control_mutex);

static int limit_idx;
static int limit_idx_low;
static int limit_idx_high;
static struct cpufreq_frequency_table *table;

static int msm_thermal_get_freq_table(void)
{
	int ret = 0;
	int i = 0;

	table = cpufreq_frequency_get_table(0);
	if (table == NULL) {
		pr_debug("%s: error reading cpufreq table\n", KBUILD_MODNAME);
		ret = -EINVAL;
		goto fail;
	}

	while (table[i].frequency != CPUFREQ_TABLE_END)
		i++;

	limit_idx_low = 0;
	limit_idx_high = limit_idx = i - 1;
	BUG_ON(limit_idx_high <= 0 || limit_idx_high <= limit_idx_low);
fail:
	return ret;
}

static int update_cpu_max_freq(int cpu, uint32_t max_freq)
{
	int ret = 0;

	ret = msm_cpufreq_set_freq_limits(cpu, MSM_CPUFREQ_NO_LIMIT, max_freq);
	if (ret)
		return ret;

	limited_max_freq = max_freq;
	if (max_freq != MSM_CPUFREQ_NO_LIMIT)
		pr_info("%s: Limiting cpu%d max frequency to %d\n",
				KBUILD_MODNAME, cpu, max_freq);
	else
		pr_info("%s: Max frequency reset for cpu%d\n",
				KBUILD_MODNAME, cpu);

unsigned int max_freq;
unsigned int freq_buffer;

	return ret;
}

static void __cpuinit do_core_control(long temp)
{
	int i = 0;
	int ret = 0;

	if (!core_control_enabled)
		return;

	/**
	 *  Offline cores starting from the max MPIDR to 1, when above limit,
	 *  The core control mask is non zero and allows the core to be turned
	 *  off.
	 *  The core was not previously offlined by this module
	 *  The core is the next in sequence.
	 *  If the core was online for some reason, even after it was offlined
	 *  by this module, offline it again.
	 *  Online the back on if the temp is below the hysteresis and was
	 *  offlined by this module and not already online.
	 */
	mutex_lock(&core_control_mutex);
	if (msm_thermal_info.core_control_mask &&
		temp >= msm_thermal_info.core_limit_temp_degC) {
		for (i = num_possible_cpus(); i > 0; i--) {
			if (!(msm_thermal_info.core_control_mask & BIT(i)))
				continue;
			if (cpus_offlined & BIT(i) && !cpu_online(i))
				continue;
			pr_info("%s: Set Offline: CPU%d Temp: %ld\n",
					KBUILD_MODNAME, i, temp);
			ret = cpu_down(i);
			if (ret)
				pr_err("%s: Error %d offline core %d\n",
					KBUILD_MODNAME, ret, i);
			cpus_offlined |= BIT(i);
			break;
		}
	} else if (msm_thermal_info.core_control_mask && cpus_offlined &&
		temp <= (msm_thermal_info.core_limit_temp_degC -
			msm_thermal_info.core_temp_hysteresis_degC)) {
		for (i = 0; i < num_possible_cpus(); i++) {
			if (!(cpus_offlined & BIT(i)))
				continue;
			cpus_offlined &= ~BIT(i);
			pr_info("%s: Allow Online CPU%d Temp: %ld\n",
					KBUILD_MODNAME, i, temp);
			/* If this core is already online, then bring up the
			 * next offlined core.
			 */
			if (cpu_online(i))
				continue;
			ret = cpu_up(i);
			if (ret)
				pr_err("%s: Error %d online core %d\n",
						KBUILD_MODNAME, ret, i);
			break;
		}
	}
	mutex_unlock(&core_control_mutex);
}

static void __cpuinit check_temp(struct work_struct *work)
{
	static int limit_init;
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

	INIT_DELAYED_WORK(&check_temp_work, check_temp);
	queue_delayed_work(wq, &check_temp_work, HZ*30);

	return ret;
}

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

int __init msm_thermal_late_init(void)
{
	return msm_thermal_add_cc_nodes();
}
module_init(msm_thermal_late_init);
