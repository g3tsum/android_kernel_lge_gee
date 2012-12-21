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
#include <linux/reboot.h>

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
#define DEF_TEMP_SENSOR0      0
#define DEF_TEMP_SENSOR1      1

//shutdown temp
#define DEF_SHUTDOWNTEMP 81

//max thermal limit
#define DEF_ALLOWED_MAX_HIGH 76
#define DEF_ALLOWED_MAX_FREQ 384000

//mid thermal limit
#define DEF_ALLOWED_MID_HIGH 72
#define DEF_ALLOWED_MID_FREQ 1026000

//low thermal limit
#define DEF_ALLOWED_LOW_HIGH 70
#define DEF_ALLOWED_LOW_FREQ 1350000

//Sampling interval
#define DEF_THERMAL_CHECK_MS 100

static DEFINE_MUTEX(emergency_shutdown_mutex);

static int enabled;

//Throttling indicator, 0=not throttled, 1=low, 2=mid, 3=max
static int thermal_throttled = 0;

//Safe the cpu max freq before throttling
static int pre_throttled_max = 0;

static struct delayed_work check_temp_work;
static struct workqueue_struct *check_temp_workq;

static struct msm_thermal_tuners {
	unsigned int shutdown_temp;

	unsigned int allowed_max_high;
	unsigned int allowed_max_low;
	unsigned int allowed_max_freq;

	unsigned int allowed_mid_high;
	unsigned int allowed_mid_low;
	unsigned int allowed_mid_freq;

	unsigned int allowed_low_high;
	unsigned int allowed_low_low;
	unsigned int allowed_low_freq;

	unsigned int check_interval_ms;
} msm_thermal_tuners_ins = {
	.shutdown_temp = DEF_SHUTDOWNTEMP,

	.allowed_max_high = DEF_ALLOWED_MAX_HIGH,
	.allowed_max_low = (DEF_ALLOWED_MAX_HIGH - 5),
	.allowed_max_freq = DEF_ALLOWED_MAX_FREQ,

	.allowed_mid_high = DEF_ALLOWED_MID_HIGH,
	.allowed_mid_low = (DEF_ALLOWED_MID_HIGH - 5),
	.allowed_mid_freq = DEF_ALLOWED_MID_FREQ,

	.allowed_low_high = DEF_ALLOWED_LOW_HIGH,
	.allowed_low_low = (DEF_ALLOWED_LOW_HIGH - 6),
	.allowed_low_freq = DEF_ALLOWED_LOW_FREQ,

	.check_interval_ms = DEF_THERMAL_CHECK_MS,
};

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
	struct tsens_device tsens_dev0;
	struct tsens_device tsens_dev1;
	unsigned long temp0 = 0, temp1 = 0;
	unsigned int max_freq = 0;
	int update_policy = 0;
	int cpu = 0;
	int ret0 = 0, ret1 = 0;

	tsens_dev0.sensor_num = DEF_TEMP_SENSOR0;
	ret0 = tsens_get_temp(&tsens_dev0, &temp0);
	tsens_dev1.sensor_num = DEF_TEMP_SENSOR1;
	ret1 = tsens_get_temp(&tsens_dev1, &temp1);
	if (ret0 && ret1) {
		pr_err("msm_thermal: FATAL: Unable to read TSENS sensor %d & %d\n",
				tsens_dev0.sensor_num, tsens_dev1.sensor_num);
		goto reschedule;
	}

        if ((max(temp0, temp1)) >= (msm_thermal_tuners_ins.shutdown_temp)) {
                mutex_lock(&emergency_shutdown_mutex);
                pr_warn("################################\n");
                pr_warn("################################\n");
                pr_warn("- %u OVERTEMP! SHUTTING DOWN! -\n", msm_thermal_tuners_ins.shutdown_temp);
                pr_warn("################################\n");
                pr_warn("################################\n");
                /* orderly poweroff tries to power down gracefully
                   if it fails it will force it. */
                orderly_poweroff(true);
                cancel_delayed_work_sync(&check_temp_work);
                for_each_possible_cpu(cpu) {
                        update_policy = true;
                        max_freq = msm_thermal_tuners_ins.allowed_max_freq;
                        thermal_throttled = 3;
                        pr_warn("msm_thermal: Emergency throttled CPU%i to %u! temp:%lu\n",
                                cpu, msm_thermal_tuners_ins.allowed_max_freq, (max(temp0, temp1)));
                }
                mutex_unlock(&emergency_shutdown_mutex);
        }

	for_each_possible_cpu(cpu) {
		update_policy = 0;
		cpu_policy = cpufreq_cpu_get(cpu);
		if (!cpu_policy) {
			pr_debug("msm_thermal: NULL policy on cpu %d\n", cpu);
			continue;
		}

		/* save pre-throttled max freq value */
                if (thermal_throttled == 0)
                        pre_throttled_max = cpu_policy->max;

		//low trip point
		if (((max(temp0, temp1)) >= msm_thermal_tuners_ins.allowed_low_high) &&
		    ((max(temp0, temp1)) < msm_thermal_tuners_ins.allowed_mid_high) &&
		    (cpu_policy->max > msm_thermal_tuners_ins.allowed_low_freq)) {
			update_policy = 1;
                    (thermal_throttled < 2)) {
			update_policy = true;
			max_freq = msm_thermal_tuners_ins.allowed_low_freq;
			thermal_throttled = 1;
			pr_warn("msm_thermal: Thermal Throttled (low)! temp:%lu by:%s\n",
                                (max(temp0, temp1)), (temp0>temp1) ? "0" : "1");
		//low clr point
		} else if (((max(temp0, temp1)) < msm_thermal_tuners_ins.allowed_low_low) &&
			   (thermal_throttled > 0)) {
			if (cpu_policy->max < cpu_policy->cpuinfo.max_freq) {
				if (pre_throttled_max != 0)
					max_freq = pre_throttled_max;
				else {
					max_freq = 1566000;
					pr_warn("msm_thermal: ERROR! pre_throttled_max=0, falling back to %u\n", max_freq);
				}
				update_policy = 1;
				/* wait until 4th core is unthrottled */
				if (cpu == 3)
					thermal_throttled = 0;
				pr_warn("msm_thermal: Low Thermal Throttling Ended! temp:%lu by:%s\n",
				update_policy = true;
                                thermal_throttled = 0;
				pr_warn("msm_thermal: CPU%i: Low thermal throttle ended! temp:%lu by:%s\n", cpu,
                                (max(temp0, temp1)), (temp0>temp1) ? "0" : "1");
			}
		//mid trip point
		} else if (((max(temp0, temp1)) >= msm_thermal_tuners_ins.allowed_low_high) &&
			   ((max(temp0, temp1)) < msm_thermal_tuners_ins.allowed_mid_low) &&
			   (cpu_policy->max > msm_thermal_tuners_ins.allowed_mid_freq)) {
			update_policy = 1;
			   ((max(temp0, temp1)) < msm_thermal_tuners_ins.allowed_max_high) &&
			   (thermal_throttled < 2)) {
			update_policy = true;
			max_freq = msm_thermal_tuners_ins.allowed_low_freq;
			thermal_throttled = 2;
			pr_warn("msm_thermal: Thermal Throttled (mid)! temp:%lu by:%s\n",
                                (max(temp0, temp1)), (temp0>temp1) ? "0" : "1");
		//mid clr point
		} else if (((max(temp0, temp1)) < msm_thermal_tuners_ins.allowed_mid_low) &&
			   (thermal_throttled > 1)) {
			if (cpu_policy->max < cpu_policy->cpuinfo.max_freq) {
				max_freq = msm_thermal_tuners_ins.allowed_low_freq;
				update_policy = 1;
				/* wait until 4th core is unthrottled */
				if (cpu == 3)
					thermal_throttled = 1;
				pr_warn("msm_thermal: Mid Thermal Throttling Ended! temp:%lu by:%s\n",
				update_policy = true;
                                thermal_throttled = 1;
				pr_warn("msm_thermal: CPU%i: Mid thermal throttle ended! temp:%lu by:%s\n", cpu,
                                (max(temp0, temp1)), (temp0>temp1) ? "0" : "1");
			}
		//max trip point
		} else if (((max(temp0, temp1)) >= msm_thermal_tuners_ins.allowed_max_high) &&
			   (cpu_policy->max > msm_thermal_tuners_ins.allowed_max_freq)) {
			update_policy = 1;
		} else if ((max(temp0, temp1)) >= msm_thermal_tuners_ins.allowed_max_high) {
			update_policy = true;
			max_freq = msm_thermal_tuners_ins.allowed_max_freq;
			thermal_throttled = 3;
			pr_warn("msm_thermal: Thermal Throttled (max)! temp:%lu by:%s\n",
                                (max(temp0, temp1)), (temp0>temp1) ? "0" : "1");
		//max clr point
		} else if (((max(temp0, temp1)) < msm_thermal_tuners_ins.allowed_max_low) &&
			   (thermal_throttled > 2)) {
			if (cpu_policy->max < cpu_policy->cpuinfo.max_freq) {
				max_freq = msm_thermal_tuners_ins.allowed_mid_freq;
				update_policy = 1;
				/* wait until 4th core is unthrottled */
				if (cpu == 3)
					thermal_throttled = 2;
				pr_warn("msm_thermal: Max Thermal Throttling Ended! temp:%lu by:%s\n",
				update_policy = true;
                                thermal_throttled = 2;
				pr_warn("msm_thermal: CPU%i: Max thermal throttle ended! temp:%lu by:%s\n", cpu,
                                (max(temp0, temp1)), (temp0>temp1) ? "0" : "1");
			}
		}

		if ((update_policy) && (cpu_policy->max > max_freq))
			update_cpu_max_freq(cpu_policy, cpu, max_freq);

		cpufreq_cpu_put(cpu_policy);
	}

reschedule:
	if (enabled)
		queue_delayed_work(check_temp_workq, &check_temp_work,
				msecs_to_jiffies(msm_thermal_tuners_ins.check_interval_ms));
}

static void disable_msm_thermal(void)
{
	int cpu = 0;
	struct cpufreq_policy *cpu_policy = NULL;

	/* make sure check_temp is no longer running */
	cancel_delayed_work_sync(&check_temp_work);
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

/**************************** SYSFS START ****************************/
struct kobject *msm_thermal_kobject;

#define show_one(file_name, object)					\
static ssize_t show_##file_name						\
(struct kobject *kobj, struct attribute *attr, char *buf)               \
{									\
	return sprintf(buf, "%u\n", msm_thermal_tuners_ins.object);				\
}

show_one(shutdown_temp, shutdown_temp);
show_one(allowed_max_high, allowed_max_high);
show_one(allowed_max_low, allowed_max_low);
show_one(allowed_max_freq, allowed_max_freq);
show_one(allowed_mid_high, allowed_mid_high);
show_one(allowed_mid_low, allowed_mid_low);
show_one(allowed_mid_freq, allowed_mid_freq);
show_one(allowed_low_high, allowed_low_high);
show_one(allowed_low_low, allowed_low_low);
show_one(allowed_low_freq, allowed_low_freq);
show_one(check_interval_ms, check_interval_ms);

static ssize_t store_shutdown_temp(struct kobject *a, struct attribute *b,
				   const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);
	if (ret != 1)
		return -EINVAL;

	msm_thermal_tuners_ins.shutdown_temp = input;

	return count;
}

static ssize_t store_allowed_max_high(struct kobject *a, struct attribute *b,
				   const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);
	if (ret != 1)
		return -EINVAL;

	msm_thermal_tuners_ins.allowed_max_high = input;

	return count;
}

static ssize_t store_allowed_max_low(struct kobject *a, struct attribute *b,
				   const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);
	if (ret != 1)
		return -EINVAL;

	msm_thermal_tuners_ins.allowed_max_low = input;

	return count;
}

static ssize_t store_allowed_max_freq(struct kobject *a, struct attribute *b,
				   const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);
	if (ret != 1)
		return -EINVAL;

	msm_thermal_tuners_ins.allowed_max_freq = input;

	return count;
}

static ssize_t store_allowed_mid_high(struct kobject *a, struct attribute *b,
				   const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);
	if (ret != 1)
		return -EINVAL;

	msm_thermal_tuners_ins.allowed_mid_high = input;

	return count;
}

static ssize_t store_allowed_mid_low(struct kobject *a, struct attribute *b,
				   const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);
	if (ret != 1)
		return -EINVAL;

	msm_thermal_tuners_ins.allowed_mid_low = input;

	return count;
}

static ssize_t store_allowed_mid_freq(struct kobject *a, struct attribute *b,
				   const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);
	if (ret != 1)
		return -EINVAL;

	msm_thermal_tuners_ins.allowed_mid_freq = input;

	return count;
}

static ssize_t store_allowed_low_high(struct kobject *a, struct attribute *b,
				   const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);
	if (ret != 1)
		return -EINVAL;

	msm_thermal_tuners_ins.allowed_low_high = input;

	return count;
}

static ssize_t store_allowed_low_low(struct kobject *a, struct attribute *b,
				   const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);
	if (ret != 1)
		return -EINVAL;

	msm_thermal_tuners_ins.allowed_low_low = input;

	return count;
}

static ssize_t store_allowed_low_freq(struct kobject *a, struct attribute *b,
				   const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);
	if (ret != 1)
		return -EINVAL;

	msm_thermal_tuners_ins.allowed_low_freq = input;

	return count;
}

static ssize_t store_check_interval_ms(struct kobject *a, struct attribute *b,
				   const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);
	if (ret != 1)
		return -EINVAL;

	msm_thermal_tuners_ins.check_interval_ms = input;

	return count;
}

define_one_global_rw(shutdown_temp);
define_one_global_rw(allowed_max_high);
define_one_global_rw(allowed_max_low);
define_one_global_rw(allowed_max_freq);
define_one_global_rw(allowed_mid_high);
define_one_global_rw(allowed_mid_low);
define_one_global_rw(allowed_mid_freq);
define_one_global_rw(allowed_low_high);
define_one_global_rw(allowed_low_low);
define_one_global_rw(allowed_low_freq);
define_one_global_rw(check_interval_ms);

static struct attribute *msm_thermal_attributes[] = {
        &shutdown_temp.attr,
	&allowed_max_high.attr,
	&allowed_max_low.attr,
	&allowed_max_freq.attr,
	&allowed_mid_high.attr,
	&allowed_mid_low.attr,
	&allowed_mid_freq.attr,
	&allowed_low_high.attr,
	&allowed_low_low.attr,
	&allowed_low_freq.attr,
	&check_interval_ms.attr,
	NULL
};


static struct attribute_group msm_thermal_attr_group = {
	.attrs = msm_thermal_attributes,
	.name = "conf",
};
/**************************** SYSFS END ****************************/

static int __init msm_thermal_init(void)
{
	int rc, ret = 0;

	enabled = 1;

	INIT_DELAYED_WORK(&check_temp_work, check_temp);
	queue_delayed_work(wq, &check_temp_work, HZ*30);
        check_temp_workq = alloc_workqueue(
                "msm_thermal", WQ_UNBOUND | WQ_RESCUER, 1);
        if (!check_temp_workq)
                BUG_ON(ENOMEM);
        INIT_DELAYED_WORK(&check_temp_work, check_temp);
        queue_delayed_work(check_temp_workq, &check_temp_work, 0);

	msm_thermal_kobject = kobject_create_and_add("msm_thermal", kernel_kobj);
	if (msm_thermal_kobject) {
		rc = sysfs_create_group(msm_thermal_kobject,
							&msm_thermal_attr_group);
		if (rc) {
			pr_warn("msm_thermal: sysfs: ERROR, could not create sysfs group");
		}
	} else
		pr_warn("msm_thermal: sysfs: ERROR, could not create sysfs kobj");

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
