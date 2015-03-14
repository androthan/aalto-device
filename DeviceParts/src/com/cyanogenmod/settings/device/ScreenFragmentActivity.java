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

import android.content.Context;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.preference.CheckBoxPreference;
import android.preference.Preference;
import android.preference.PreferenceFragment;
import android.preference.PreferenceManager;
import android.preference.PreferenceScreen;

public class ScreenFragmentActivity extends PreferenceFragment {

    private TouchscreenSensitivity mTouchscreenSensitivity;
    private TouchkeysSensitivity mTouchkeysSensitivity;
    private TouchkeysTimeout mTouchkeysTimeout;
    private CheckBoxPreference mTouchkeysTimeoutDisabled;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        addPreferencesFromResource(R.xml.screen_preferences);

        mTouchscreenSensitivity = (TouchscreenSensitivity) findPreference(DeviceSettings.KEY_TOUCHSCREEN_SENSITIVITY);
        mTouchscreenSensitivity.setEnabled(TouchscreenSensitivity.isSupported());

        mTouchkeysSensitivity = (TouchkeysSensitivity) findPreference(DeviceSettings.KEY_TOUCHKEYS_SENSITIVITY);
        mTouchkeysSensitivity.setEnabled(TouchkeysSensitivity.isSupported());

        mTouchkeysTimeout = (TouchkeysTimeout) findPreference(DeviceSettings.KEY_TOUCHKEYS_TIMEOUT);
        mTouchkeysTimeoutDisabled = (CheckBoxPreference) findPreference(DeviceSettings.KEY_TOUCHKEYS_TIMEOUT_DISABLED);

        mTouchkeysTimeoutDisabled.setEnabled(TouchkeysTimeout.isSupported());

        if (!mTouchkeysTimeoutDisabled.isChecked() && TouchkeysTimeout.isSupported()) {
            mTouchkeysTimeout.setEnabled(true);
        } else {
            mTouchkeysTimeout.setEnabled(false);
        }

    }

    @Override
    public boolean onPreferenceTreeClick(PreferenceScreen preferenceScreen, Preference preference) {
        String key = preference.getKey();
        SharedPreferences sharedPrefs = PreferenceManager.getDefaultSharedPreferences(getActivity());

        if (key.compareTo(DeviceSettings.KEY_TOUCHKEYS_TIMEOUT_DISABLED) == 0) {
            if(((CheckBoxPreference)preference).isChecked()) {
                mTouchkeysTimeout.setEnabled(false);
                Utils.writeValue("/sys/class/misc/notification/bl_timeout", "0");
                Utils.writeValue("/sys/class/leds/button-backlight/brightness", "0"); //Turn LEDs off
            } else {
                mTouchkeysTimeout.setEnabled(true);
                Utils.writeValue("/sys/class/misc/notification/bl_timeout",
                    Integer.toString(sharedPrefs.getInt(DeviceSettings.KEY_TOUCHKEYS_TIMEOUT, 1600)));
            }
        }
        if (key.compareTo(DeviceSettings.KEY_TOUCHSCREEN_DISABLE_CALIBRATION) == 0) {
            if(((CheckBoxPreference)preference).isChecked()) {
                Utils.writeValue("/sys/touchscreen/disable_calibration", "1");
            } else {
                Utils.writeValue("/sys/touchscreen/disable_calibration", "0");
            }
        }
        return true;
    }

}
