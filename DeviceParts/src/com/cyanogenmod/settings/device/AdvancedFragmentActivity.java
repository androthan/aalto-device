/*
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

package com.cyanogenmod.settings.device;

import android.os.Bundle;
import android.preference.Preference;
import android.preference.PreferenceFragment;
import android.preference.PreferenceScreen;

import com.cyanogenmod.settings.device.R;

public class AdvancedFragmentActivity extends PreferenceFragment {

    private FSync mFSync;
    private BatteryWorkDelay mBatteryWorkDelay;
    private CpuBoostFreq mCpuBoostFreq;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        addPreferencesFromResource(R.xml.advanced_preferences);

        mFSync = (FSync) findPreference(DeviceSettings.KEY_FSYNC);
        mFSync.setEnabled(FSync.isSupported());

        mBatteryWorkDelay = (BatteryWorkDelay) findPreference(DeviceSettings.KEY_BATTERY_WORK_DELAY);
        mBatteryWorkDelay.setEnabled(BatteryWorkDelay.isSupported());

        mCpuBoostFreq = (CpuBoostFreq) findPreference(DeviceSettings.KEY_CPU_BOOST_FREQ);
        if(!CpuBoostFreq.isSupported()) {
            mCpuBoostFreq.setTitle(R.string.cpu_boost_freq_notsupported);
            mCpuBoostFreq.setSummary("");
            mCpuBoostFreq.setEnabled(false);
        }
    }

}
