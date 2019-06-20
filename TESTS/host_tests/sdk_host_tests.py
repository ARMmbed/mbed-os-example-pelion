## ----------------------------------------------------------------------------
## Copyright 2016-2018 ARM Ltd.
##
## SPDX-License-Identifier: Apache-2.0
##
## Licensed under the Apache License, Version 2.0 (the "License");
## you may not use this file except in compliance with the License.
## You may obtain a copy of the License at
##
##     http://www.apache.org/licenses/LICENSE-2.0
##
## Unless required by applicable law or agreed to in writing, software
## distributed under the License is distributed on an "AS IS" BASIS,
## WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
## See the License for the specific language governing permissions and
## limitations under the License.
## ----------------------------------------------------------------------------

from mbed_host_tests import BaseHostTest
from mbed_host_tests.host_tests_logger import HtrunLogger
from mbed_cloud.account_management import AccountManagementAPI
from mbed_cloud.device_directory import DeviceDirectoryAPI
from mbed_cloud.connect import ConnectAPI
import os
import time
import hashlib
import subprocess
import re
import signal
import pkg_resources


# Check for that manifest_tool.json exists
if not os.path.exists(".manifest_tool.json"):
    raise RuntimeError(
        'Cannot find "manifest_tool.json" to verify "mbed dm init..."')

# Check that user ran "mbed dm init..."
json_hash = hashlib.md5()
with open(".manifest_tool.json", "r") as fid:
    json_hash.update(fid.read())
if json_hash.hexdigest() == "de302858ac31a2d68123a18b57702777":
    err_msg = "".join([
        'Cannot detect change in manifest_tool.json ',
        '(md5={})'.format(json_hash.hexdigest()),
        ', initialize Pelion update with:\n',
        'mbed dm init -d "<your company name in Pelion DM>"',
        '--model-name "<product model identifier>" -q --force'])
    raise RuntimeError(err_msg)

# Check for old mbed-cloud-sdk version.
#   Prior to 2.0.6 the connectApi.set_resource_value_async()
#   required a string variable. This check is important because
#   mbed-os/requirements.txt also defines mbed-cloud-sdk as
#   a requiremnt also with too old version.
sdk_version = pkg_resources.get_distribution("mbed-cloud-sdk").version
sdk_version = pkg_resources.parse_version(sdk_version)
min_version = pkg_resources.parse_version("2.0.6")
if sdk_version < min_version:
    err_msg = "".join(
        ['SPDMC tests require mbed-cloud-sdk >= 2.0.6, ',
         'currently installed version is "{}"'.format(sdk_version)])
    raise ImportError(err_msg)

DEFAULT_CYCLE_PERIOD = 1.0

class SDKTests(BaseHostTest):
    __result = None
    deviceApi = None
    connectApi = None
    deviceID = None
    post_timeout = None
    firmware_proc = None
    firmware_sent = False
    firmware_file = None
    iteration = 0
    boot_cycles = 0

    def send_safe(self, key, value):
        #self.send_kv('dummy_start', 0)
        self.send_kv(key, value)
        self.send_kv(key, value)
        self.send_kv(key, value)
        self.send_kv(key, value)
        self.send_kv(key, value)
        #self.send_kv('dummy_end', 1)

    def _callback_device_booted(self, key, value, timestamp): 
        # This is used to let the device boot normally
        self.send_safe('__sync', 0)

    def _callback_device_ready(self, key, value, timestamp):
        # Send device iteration number after a reset
        self.boot_cycles += 1
        # Prevent boot loop due to Mbed OS crash
        if self.boot_cycles <= 5:
            self.send_safe('iteration', self.iteration)

    def _callback_test_advance(self, key, value, timestamp):
        # Advance test sequence
        self.iteration = self.iteration + 1
        self.send_safe('reset', 0)

    def _callback_test_failed(self, key, value, timestamp):
        # Test failed. End it.
        self.notify_complete(False)

    """
    Device Register routines
    """
    def _callback_verify_registration(self, key, value, timestamp):
        try:
            #set value for later use
            self.deviceID = value

            # Check if device is in Mbed Cloud Device Directory
            device = self.deviceApi.get_device(value)

            # Send registraton status to device
            self.send_safe("registered", 1 if device.state == "registered" else 0)
        except:
            # SDK throws an exception if the device is not found (unsuccessful registration) or times out
            self.send_safe("registered", 0)

    def _callback_verify_identity(self, key, value, timestamp):
        # Send true if old DeviceID is the same as current device is
        self.send_safe("verified", 1 if self.deviceID == value else 0)

    """
    Device Connect routines
    """
    def _callback_verify_lwm2m_get(self, key, value, timestamp):
        timeout = 0

        # Get resource value from device
        async_response = self.connectApi.get_resource_value_async(self.deviceID, value)

        # Set a 30 second timeout here.
        while not async_response.is_done and timeout <= 50:
            time.sleep(0.1)
            timeout += 1

        if not async_response.is_done:
            # Kick the REST API
            timeout = 0
            async_response = self.connectApi.get_resource_value_async(self.deviceID, value)
            while not async_response.is_done and timeout <= 250:
                time.sleep(0.1)
                timeout += 1

        if async_response.is_done:
            # Send resource value back to device
            self.send_safe("get_value", async_response.value)
        else:
            # Request timed out.
            self.send_safe("timeout", 0)

    def _callback_verify_lwm2m_set(self, key, value, timestamp):
        timeout = 0

        # Get resource value from device
        async_response = self.connectApi.get_resource_value_async(self.deviceID, value)

        # Set a 30 second timeout here.
        while not async_response.is_done and timeout <= 300:
            time.sleep(0.1)
            timeout += 1

        if async_response.is_done:
            # Send resource value back to device
            self.send_safe("set_value", async_response.value)
        else:
            # Request timed out.
            self.send_safe("timeout", 0)

    def _callback_verify_lwm2m_put(self, key, value, timestamp):
        timeout = 0

        # Get resource value from device and increment it
        resource_value = self.connectApi.get_resource_value_async(self.deviceID, value)

        # Set a 30 second timeout here.
        while not resource_value.is_done and timeout <= 300:
            time.sleep(0.1)
            timeout += 1

        if not resource_value.is_done:
            self.send_safe("timeout", 0)
            return

        updated_value = int(resource_value.value) + 5

        # Set new resource value from cloud
        async_response = self.connectApi.set_resource_value_async(self.deviceID, value, updated_value)

        # Set a 30 second timeout here.
        while not async_response.is_done and timeout <= 300:
            time.sleep(0.1)
            timeout += 1

        if not async_response.is_done:
            self.send_safe("timeout", 0)
        else:
            # Send new resource value to device for verification.
            self.send_safe("res_set", updated_value);

    def _callback_verify_lwm2m_post(self, key, value, timestamp):
        timeout = 0

        # Execute POST function on device
        resource_value = self.connectApi.execute_resource_async(self.deviceID, value)

        # Set a 30 second timeout here.
        while not resource_value.is_done and timeout <= 300:
            time.sleep(0.1)
            timeout += 1

        if not resource_value.is_done:
            self.send_safe("timeout", 0)
            self.post_timeout = 1

    def _callback_verify_lwm2m_post_result(self, key, value, timestamp):

        # Called from callback function on device, POST function working as expected.
        # If post_timeout is not none, the request took longer than 30 seconds, which is
        # a failure. Don't send this value.
        if not self.post_timeout:
            self.send_safe("post_test_executed", 0)

    """
    Device Firmware update routines
    """
    def firmware_campaign_cleanup(self):
        if self.firmware_proc:
            if os.name == 'nt':
                os.kill(self.firmware_proc.pid, signal.CTRL_C_EVENT)
                os.kill(self.firmware_proc.pid, signal.CTRL_BREAK_EVENT)
            self.firmware_proc.terminate()
            outs, errs = self.firmware_proc.communicate()
            self.logger.prn_inf('Firmware campaign process killed: PID %s' % self.firmware_proc.pid)
            self.firmware_proc = None

            try:
                time.sleep(1) # let the manifest-tool sub-process die gracefully
                if self.firmware_file:
                    os.remove(self.firmware_file)
                    self.firmware_file = None
            except Exception, e:
                pass

    def _callback_firmware_ready(self, key, value, timestamp):
        if self.firmware_sent:
            # Firmware was sent, but wasn't applied if this callback is called
            self.firmware_campaign_cleanup()
            self.notify_complete(False)
        else:
            # Send device iteration number after a reset
            self.send_safe('iteration', self.iteration)

    def _callback_firmware_prepare(self, key, value, timestamp):
        if not self.deviceID:
            self.logger.prn_err("ERROR: No DeviceID")
            self.notify_complete(False)
            return -1

        target = self.get_config_item('platform_name')
        image = self.get_config_item('image_path')
        update_image = re.sub(r'(.+)\.([a-z0-9]+)$', r'\1_update.bin', image if image else "")
        if not image or not os.path.exists(update_image):
            self.logger.prn_err("ERROR: No main or update image")
            self.notify_complete(False)
            return -1
        self.logger.prn_inf('Found FW update image: "%s"' % update_image)

        try:
            # Open the firmware update image as provided by the build system
            with open(update_image, 'rb') as f:
                raw = f.read()
            # Modify the initial "spdmc_ready_chk" sequence into "firmware_update" 
            # (matching the string length) as an indication that the firmware was changed/updated
            raw = re.sub(r'spdmc_ready_chk', r'firmware_update', raw)

            # Save the firmware into a temp place. Manifest tool has issues handling very long paths even if -n is specified
            update_mod_image = ".%s.%s.%s" % (target, re.sub(r'.*[\\/](.+)\.([a-z0-9]+)$', r'\1_update_mod.\2', image), time.time())
            with open(update_mod_image, 'wb') as f:
                f.write(raw)
        except Exception, e:
            self.logger.prn_err("ERROR: While preparing modified image")
            self.notify_complete(False)
            return -1
        self.logger.prn_inf('Modified FW update image: "%s"' % update_mod_image)

        # Use non-blocking call, but remember the process, so we can kill it later
        try:
            spargs = dict()
            if os.name == 'posix':
                spargs['preexec_fn'] = os.setpgrp
            elif os.name == 'nt':
                spargs['creationflags'] = subprocess.CREATE_NEW_PROCESS_GROUP
            self.firmware_proc = subprocess.Popen(["mbed", "dm", "update", "device", "-p", update_mod_image, "-D", self.deviceID], stderr=subprocess.STDOUT, **spargs)
            self.firmware_file = update_mod_image
        except Exception, e:
            self.logger.prn_err("ERROR: Unable to execute 'mbed dm' sub-command")
            self.firmware_campaign_cleanup()
            self.notify_complete(False)
            return -1

        # At this point the firmware should be on it's way to the device
        self.firmware_sent = True
        self.send_safe('firmware_sent', 1)
        self.logger.prn_inf("Firmware sent and update campaign started. Check for download progress.")

    def _callback_firmware_update(self, key, value, timestamp):
        self.logger.prn_inf("Firmware successfully updated!")
        self.firmware_campaign_cleanup()
        self.iteration = self.iteration + 1
        self.send_safe('iteration', self.iteration)


    def _check_account_id_match(self, api_config):
        accountApi = AccountManagementAPI(api_config)

        # Check that API key matches mbed_cloud_dev_credentials.c
        re_account_id = re.compile(r"^const char MBED_CLOUD_DEV_ACCOUNT_ID\[\] = \"(?P<account_id>[0-9a-f]{32})\";$")
        credential_account_id = None
        with open("mbed_cloud_dev_credentials.c", "r") as fid:
            for line in fid:
                match = re_account_id.match(line)
                if match:
                    credential_account_id = match.group("account_id")
                    break
            else:
                # Coult not find account_id line from "mbed_cloud_dev_credentials.c"
                self.logger.prn_wrn("WARNING: mbed_cloud_dev_credentials.c parsing failed, cannot verify matching account IDs.")
                return

        # Get API key account ID
        api_key_account_id = accountApi.get_account().id

        # Compare keys
        if credential_account_id != api_key_account_id:
            self.logger.prn_err('ERROR: "mbed_cloud_dev_credentials.c" account ID of "{}" does not match CLOUD_SDK_API_KEY account ID of "{}"'.format(
                credential_account_id, api_key_account_id))
            raise KeyError

    """
    Host setup routines
    """
    def setup(self):
        # Generic test routines
        self.register_callback('device_booted', self._callback_device_booted)
        self.register_callback('device_ready', self._callback_device_ready)
        self.register_callback('test_advance', self._callback_test_advance)
        self.register_callback('test_failed', self._callback_test_failed)

        # Callbacks from device registration tests
        self.register_callback('verify_registration', self._callback_verify_registration)
        self.register_callback('verify_identity', self._callback_verify_identity)

        # Callbacks from LWM2M tests
        self.register_callback('verify_lwm2m_get_test', self._callback_verify_lwm2m_get)
        self.register_callback('verify_lwm2m_set_test', self._callback_verify_lwm2m_set)
        self.register_callback('verify_lwm2m_put_test', self._callback_verify_lwm2m_put)
        self.register_callback('verify_lwm2m_post_test', self._callback_verify_lwm2m_post)
        self.register_callback('verify_lwm2m_post_test_result', self._callback_verify_lwm2m_post_result)

        # Callbacks from FW update tests
        self.register_callback('spdmc_ready_chk', self._callback_firmware_ready)
        self.register_callback('firmware_prepare', self._callback_firmware_prepare)
        self.register_callback('firmware_update', self._callback_firmware_update)

        # Setup API config
        try:
            result = subprocess.check_output(["mbed", "config", "--list"], stderr=subprocess.STDOUT)
        except Exception, e:
            self.logger.prn_err("ERROR: CLOUD_SDK_API_KEY global config is not set: " + str(e))
            return -1

        match = re.search(r'CLOUD_SDK_API_KEY=(.*)\n', result)
        if match == None:
            self.logger.prn_err("ERROR: CLOUD_SDK_API_KEY global config is not set.")
            return -1

        api_key_val = match.group(1).strip()
        api_config = {"api_key" : api_key_val, "host" : "https://api.us-east-1.mbedcloud.com"}

        self.iteration = 0
        self.boot_cycles = 0

        # Instantiate Device and Connect API
        self.deviceApi = DeviceDirectoryAPI(api_config)
        self.connectApi = ConnectAPI(api_config)

        # Check matching account ID between CLOUD_SDK_API_KEY and "mbed_cloud_dev_credentials.c"
        try:
            self._check_account_id_match(api_config)
        except KeyError:
            return -1

    def result(self):
        return self.__result

    def teardown(self):
        # Delete device from directory so as not to hit device allocation quota.
        if self.deviceID:
            self.deviceApi.delete_device(self.deviceID)
        self.firmware_campaign_cleanup()

        pass

    def __init__(self):
        super(SDKTests, self).__init__()
        self.logger = HtrunLogger('TEST')
