"""
Copyright 2019 ARM Limited
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
"""

# pylint: disable=missing-docstring,useless-super-delegation
# pylint: disable=method-hidden,relative-import

import time
import json
from pelion_helper import PelionBase


class Testcase(PelionBase):
    def __init__(self):
        PelionBase.__init__(self,
                            name="register_and_read_id",
                            title="Example application can register and prints out its device ID",
                            status="released",
                            type="acceptance",
                            component=["mbed_cloud_client_example"],
                            requirements={
                                "duts": {  # default requirements for all nodes
                                    '*': {
                                        "count": 1,
                                        "type": "hardware",
                                        "application": {
                                            "init_cli_cmds": [],
                                            "post_cli_cmds": []
                                        }
                                    }
                                }
                            })
                                
    def setup(self):
        super(Testcase, self).setup()
        self.__timeout = 120
        self.__endtime = time.time() + self.__timeout


    def case(self):

        while True:
            try:
                self.verify_trace(1, ['Device ID'], True)
                break
            except:
                if time.time() > self.__endtime:
                    raise TestStepFail('Timeout: Did not find Endpoint Name within %d seconds' % self.__timeout)
                else:
                    pass

        # Get the endpoint from the logs
        self.logger.info("Reading device ID from console.")
        dev_id_raw = ""
        dev_id_raw = list(filter(lambda x: "Device ID" in x, self.duts[0].traces))
        device_id = dev_id_raw[0].split()[2]
        self.logger.info("Writing Device ID %s to pelion.tc_cfg", device_id)
        # Store the Device ID to pelion.tc_cfg
        with open("TESTS/pelion.tc_cfg") as stream:
            data = json.load(stream)
            data["device_id"] = device_id

        with open("TESTS/pelion.tc_cfg", 'w') as stream:
            json.dump(data, stream)

        self.verify_registration("registered")

    def teardown(self):
        self.connect_api.stop_notifications()
