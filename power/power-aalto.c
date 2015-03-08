/*
 * Copyright (C) 2012 The Android Open Source Project
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
#define BOOSTPULSE_ONDEMAND (CPUFREQ_ONDEMAND "boostpulse")

#define MAX_BUF_SZ  10

/* initialize to something safe */
static char screen_off_max_freq[MAX_BUF_SZ] = "600000";
static char scaling_max_freq[MAX_BUF_SZ] = "1100000";

struct aalto_power_module {
    struct power_module base;
    pthread_mutex_t lock;
    int boostpulse_fd;
    int boostpulse_warned;
};

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


int sysfs_read(const char *path, char *buf, size_t size)
{
    int fd, len;

    fd = open(path, O_RDONLY);
    if (fd < 0)
        return -1;

    do {
        len = read(fd, buf, size);
    } while (len < 0 && errno == EINTR);

    close(fd);

    return len;
}

static void aalto_power_init(struct power_module *module)
{
    sysfs_write(CPUFREQ_ONDEMAND "boostfreq", "800000");
}

static int boostpulse_open(struct aalto_power_module *aalto)
{
    char buf[80];

    pthread_mutex_lock(&aalto->lock);

    if (aalto->boostpulse_fd < 0) {
        aalto->boostpulse_fd = open(BOOSTPULSE_ONDEMAND, O_WRONLY);

        if (aalto->boostpulse_fd < 0 && !aalto->boostpulse_warned) {
            strerror_r(errno, buf, sizeof(buf));
            ALOGE("Error opening boostpulse: %s\n", buf);
            aalto->boostpulse_warned = 1;
        } else if (aalto->boostpulse_fd > 0)
            ALOGD("Opened boostpulse interface\n");
    }

    pthread_mutex_unlock(&aalto->lock);
    return aalto->boostpulse_fd;
}

static void aalto_power_set_interactive(struct power_module *module, int on)
{
    return;

    int len;

    char buf[MAX_BUF_SZ];

    /*
     * Lower maximum frequency when screen is off.  
     */
    if (!on) {
        /* read the current scaling max freq and save it before updating */
        len = sysfs_read(CPUFREQ_CPU0 "scaling_max_freq", buf, sizeof(buf));

        /* make sure it's not the screen off freq, if the "on"
         * call is skipped (can happen if you press the power
         * button repeatedly) we might have read it. We should
         * skip it if that's the case
         */
        if (len != -1 && strncmp(buf, screen_off_max_freq,
                strlen(screen_off_max_freq)) != 0)
            memcpy(scaling_max_freq, buf, sizeof(buf));
        sysfs_write(CPUFREQ_CPU0 "scaling_max_freq", screen_off_max_freq);
    } else
        sysfs_write(CPUFREQ_CPU0 "scaling_max_freq", scaling_max_freq);

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
            len = write(aalto->boostpulse_fd, "1", 1);

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
            name: "Power HAL for Aalto board",
            author: "Sconosciuto/Androthan",
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
