/*
 * Copyright (C) 2012 The Android Open Source Project
 * Copyright (C) 2012 The CyanogenMod Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* Adapted from:
 * https://github.com/dhiru1602/android_device_samsung_galaxysl/blob/cm-11.0/power/power_latona.c
 * Thanks to:
 * dhiru1602
 * sconosciuto
 * The CyanogenMod Project
 */
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define LOG_TAG "YP-GS1 PowerHAL"
#include <utils/Log.h>

#include <hardware/hardware.h>
#include <hardware/power.h>

#define CPUFREQ_CPU0 "/sys/devices/system/cpu/cpu0/cpufreq/"
#define CPUFREQ_ONDEMAND "/sys/devices/system/cpu/cpufreq/ondemand/"
#define CPUFREQ_INTERACTIVE "/sys/devices/system/cpu/cpufreq/interactive/"

struct aalto_power_module {
    struct power_module base;
    pthread_mutex_t lock;
    int boostpulse_fd;
    int boostpulse_warned;
};

static char governor[20];

static int sysfs_read(char *path, char *s, int num_bytes)
{
    char buf[80];
    int count;
    int ret = 0;
    int fd = open(path, O_RDONLY);

    if (fd < 0) {
        strerror_r(errno, buf, sizeof(buf));
        ALOGE("Error opening %s: %s\n", path, buf);

        return -1;
    }

    if ((count = read(fd, s, num_bytes - 1)) < 0) {
        strerror_r(errno, buf, sizeof(buf));
        ALOGE("Error writing to %s: %s\n", path, buf);

        ret = -1;
    } else {
        s[count] = '\0';
    }

    close(fd);

    return ret;
}

static void sysfs_write(char *path, char *s)
{
    char buf[80];
    int len;
    int fd = open(path, O_WRONLY);

    if (fd < 0) {
        strerror_r(errno, buf, sizeof(buf));
        ALOGE("Error opening %s: %s\n", path, buf);
        return;
    }

    len = write(fd, s, strlen(s));
    if (len < 0) {
        strerror_r(errno, buf, sizeof(buf));
        ALOGE("Error writing to %s: %s\n", path, buf);
    }

    close(fd);
}

static int get_scaling_governor() {
    if (sysfs_read(CPUFREQ_CPU0 "scaling_governor", governor,
                sizeof(governor)) == -1) {
        // Can't obtain the scaling governor. Return.
        return -1;
    } else {
        // Strip newline at the end.
        int len = strlen(governor);

        len--;

        while (len >= 0 && (governor[len] == '\n' || governor[len] == '\r'))
            governor[len--] = '\0';
    }

    return 0;
}

static void aalto_power_set_interactive(struct power_module *module, int on)
{
    get_scaling_governor();
    if (strncmp(governor, "interactive", 11) == 0)
        sysfs_write(CPUFREQ_INTERACTIVE "timer_rate", on ? "30000" : "150000");
    if (strncmp(governor, "ondemand", 8) == 0)
        sysfs_write(CPUFREQ_ONDEMAND "sampling_rate", on ? "60000" : "150000");
}

static void configure_governor()
{
    aalto_power_set_interactive(NULL, 1);
    if (strncmp(governor, "interactive", 11) == 0) {
        sysfs_write(CPUFREQ_INTERACTIVE "min_sample_time", "90000");
        sysfs_write(CPUFREQ_INTERACTIVE "above_hispeed_delay", "30000");
        sysfs_write(CPUFREQ_INTERACTIVE "hispeed_freq", "600000");
    } else if (strncmp(governor, "ondemand", 8) == 0) {
        sysfs_write(CPUFREQ_ONDEMAND "io_is_busy", "1");
        sysfs_write(CPUFREQ_ONDEMAND "boostfreq", "600000");
    }
}

static int boostpulse_open(struct aalto_power_module *aalto)
{
    char buf[80];

    pthread_mutex_lock(&aalto->lock);

    if (aalto->boostpulse_fd < 0) {
        if (get_scaling_governor() < 0) {
            ALOGE("Can't read scaling governor.");
            aalto->boostpulse_warned = 1;
        } else {
            if (strncmp(governor, "ondemand", 8) == 0)
                aalto->boostpulse_fd = open(CPUFREQ_ONDEMAND "boostpulse", O_WRONLY);
            else if (strncmp(governor, "interactive", 11) == 0)
                aalto->boostpulse_fd = open(CPUFREQ_INTERACTIVE "boostpulse", O_WRONLY);

            if (aalto->boostpulse_fd < 0 && !aalto->boostpulse_warned) {
                strerror_r(errno, buf, sizeof(buf));
                ALOGE("Error opening %s boostpulse interface: %s\n", governor, buf);
                aalto->boostpulse_warned = 1;
            } else if (aalto->boostpulse_fd > 0) {
                configure_governor();
                ALOGD("Opened %s boostpulse interface", governor);
            }
        }
    }

    pthread_mutex_unlock(&aalto->lock);
    return aalto->boostpulse_fd;
}

static void aalto_power_hint(struct power_module *module, power_hint_t hint,
                            void *data)
{
    struct aalto_power_module *aalto = (struct aalto_power_module *) module;
    char buf[80];
    int len;
    int duration = 1;

    switch (hint) {
    case POWER_HINT_INTERACTION:
    case POWER_HINT_CPU_BOOST:
        if (boostpulse_open(aalto) >= 0) {
            if (data != NULL)
                duration = (int) data;

            snprintf(buf, sizeof(buf), "%d", duration);
            len = write(aalto->boostpulse_fd, buf, strlen(buf));

            if (len < 0) {
                strerror_r(errno, buf, sizeof(buf));
                ALOGE("Error writing to boostpulse: %s\n", buf);

                pthread_mutex_lock(&aalto->lock);
                close(aalto->boostpulse_fd);
                aalto->boostpulse_fd = -1;
                aalto->boostpulse_warned = 0;
                pthread_mutex_unlock(&aalto->lock);
            }
        }
        break;

    case POWER_HINT_VSYNC:
        break;

    default:
        break;
    }
}

static void aalto_power_init(struct power_module *module)
{
    get_scaling_governor();
    configure_governor();
    sysfs_write(CPUFREQ_CPU0 "screen_off_max_freq", "600000");
}

static struct hw_module_methods_t power_module_methods = {
    .open = NULL,
};

struct aalto_power_module HAL_MODULE_INFO_SYM = {
    base: {
        common: {
            tag: HARDWARE_MODULE_TAG,
            module_api_version: POWER_MODULE_API_VERSION_0_2,
            hal_api_version: HARDWARE_HAL_API_VERSION,
            id: POWER_HARDWARE_MODULE_ID,
            name: "YP-GS1 Power manager",
            author: "The CyanogenMod Project/Dhiru1602/Androthan",
            methods: &power_module_methods,
        },
       init: aalto_power_init,
       setInteractive: aalto_power_set_interactive,
       powerHint: aalto_power_hint,
    },

    lock: PTHREAD_MUTEX_INITIALIZER,
    boostpulse_fd: -1,
    boostpulse_warned: 0,
};
