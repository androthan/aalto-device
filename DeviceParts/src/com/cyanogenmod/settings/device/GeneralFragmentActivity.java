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

import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.os.Bundle;
import android.preference.Preference;
import android.preference.PreferenceFragment;
import android.preference.PreferenceScreen;

import com.cyanogenmod.settings.device.R;

public class GeneralFragmentActivity extends PreferenceFragment {

    private Hspa mHspa;
    private VibratorIntensity mVibratorIntensity;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        addPreferencesFromResource(R.xml.general_preferences);

        mHspa = (Hspa) findPreference(DeviceSettings.KEY_HSPA);
        mHspa.setEnabled(Hspa.isSupported());

        mVibratorIntensity = (VibratorIntensity) findPreference(DeviceSettings.KEY_VIBRATOR_INTENSITY);
        mVibratorIntensity.setEnabled(VibratorIntensity.isSupported());
    }

    @Override
    public boolean onPreferenceTreeClick(PreferenceScreen preferenceScreen, Preference preference) {

        String key = preference.getKey();

        if (key.compareTo(DeviceSettings.KEY_ACCELEROMETER_CALIBRATION) == 0) {
        	Utils.writeValue("/sys/devices/virtual/sensors/accelerometer_sensor/calibration", "1");
        	AlertDialog.Builder builder = new AlertDialog.Builder((Context)getActivity());
            builder.setTitle((R.string.accelerometer_dialog_head))
            .setMessage((R.string.accelerometer_dialog_message))
            .setPositiveButton("OK", new DialogInterface.OnClickListener() {
                public void onClick(DialogInterface dialog, int id) {
                    dialog.dismiss();
                }
            });
            AlertDialog alert = builder.create();
            alert.show();
        }

        return true;
    }

    public static boolean isSupported(String FILE) {
        return Utils.fileExists(FILE);
    }

}
