#
# Copyright (C) 2008 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import os, sys

LOCAL_DIR = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..'))
RELEASETOOLS_DIR = os.path.abspath(os.path.join(LOCAL_DIR, '../../../build/tools/releasetools'))
VENDOR_SAMSUNG_DIR = os.path.abspath(os.path.join(LOCAL_DIR, '../../../vendor/samsung'))

import edify_generator

class EdifyGenerator(edify_generator.EdifyGenerator):
    def AssertDevice(self, device):
      edify_generator.EdifyGenerator.AssertDevice(self, device)

      self.script.append('ui_print("Android 4.1.2 - Samsung YP-GS1");')

    def RunBackup(self, command):
      edify_generator.EdifyGenerator.RunBackup(self, command)

    def BMLWriteRawImage(self, partition, image):
      """Write the given package file into the given partition."""

      args = {'partition': partition, 'image': image}

      self.script.append(
            ('assert(package_extract_file("%(image)s", "/tmp/%(image)s"),\n'
             '       write_raw_image("/tmp/%(image)s", "%(partition)s"),\n'
             '       delete("/tmp/%(image)s"));') % args)
