# Running Device Management Client example on the PSoC® 64 Secure Boot Wi-Fi BT Pioneer Kit (CYTFM_064B0S2_4343W)

This document guides you through all of the steps required to run Device Management Client example on the CYTFM_064B0S2_4343W target.

- [Prerequisites](#prerequisites).
- [Cloning the example](#cloning-the-example).
- [Provisioning the device with initial credentials](#provisioning-the-device-with-initial-credentials).
- [Generating and provisioning Device Management credentials](#generating-and-provisioning-device-management-credentials).
- [Building and running the example](#building-and-running-the-example).
- [Updating firmware](#updating-firmware).

## Prerequisites

-	[Python 3.7](https://www.python.org/downloads/release/python-378)
-	Run `pip install mbed-cli cysecuretools pyopenssl` to install:
    - Mbed CLI 1.10.0 or higher
    - cysecuretools (you need 2.0.0 or higher)
    -	pyopenssl
- Install the `libusb` dependency for pyOCD based on the [Cypress documentation](https://www.cypress.com/file/502721/download#page=19&zoom=100,96,382).

    **Note:** Due to a known issue, Cypress recommends using [`libusb` version 1.0.21](https://github.com/libusb/libusb/releases/tag/v1.0.21) on Windows instead of the most recent version.

## Cloning the example

1. Clone the `mbed-os-example-pelion` repository:

    ```
    git clone https://github.com/ARMmbed/mbed-os-example-pelion
    ```

1. Check out the `cytfm-064b0s2-4343w` branch:

    ```
    cd mbed-os-example-pelion
    git checkout cytfm-064b0s2-4343w
    ```

1. Deploy dependencies:  

   ```
   mbed deploy
   ```

## Provisioning the device with initial credentials

You need to carry out this step only once on each board to be able to re-provision later with your root CA and device certificate.

1. Set up your project workspace for CySecureTools and create keys for provisioning:

    ```
    cd ./mbed-os/targets/TARGET_Cypress/TARGET_PSOC6/TARGET_CYTFM_064B0S2_4343W
    cysecuretools -t cy8ckit-064b0s2-4343w init
    cysecuretools -t cy8ckit-064b0s2-4343w -p policy/policy_multi_CM0_CM4_tfm.json create-keys
    ```

    You will be prompted to overwrite existing files. Type `y` to continue.

1. 	Unplug your device from the power supply.
1. 	Remove the jumper shunt from J26.
1. 	Plug in power.
1. 	Press the Mode button until the LED stops blinking to put the device in KitProg3 mode.
1. 	To provision the board with basic configuration, run:

    ```
    cysecuretools -t cy8ckit-064b0s2-4343w -p policy/policy_multi_CM0_CM4_tfm.json provision-device
    ```
1. 	Unplug your device from the power supply.
1. 	Put back the jumper shunt back in J26.
1. 	Plug in power.
1. 	Press the Mode button until the LED stops blinking to put the device in DAPLink mode.

For more information about the initial provisioning process, please see ["Provision the Device" section of the CY8CKIT-064B0S2-4343W PSoC 64 Secure Boot Wi-Fi BT Pioneer Kit Guide](https://www.cypress.com/file/502721/download#page=30&zoom=100,96,382).

## Generating and provisioning Device Management credentials

1. Navigate to the `mbed-os-example-pelion/TARGET_CYTFM_064B0S2_4343W` directory.

1. Create a `certificates` directory:

    ```
    mkdir certificates
    ```  

1. Name your own root CA private key and certificate `rootCA.key` and `rootCA.pem` respectively, and place them in the `TARGET_CYTFM_064B0S2_4343W/certificates` directory.

    Alternatively, if you don't have a root CA, you can generate a root CA private key and certificate using the [OpenSSL toolkit](https://www.openssl.org/):

    ```
    openssl ecparam -out certificates/rootCA.key -name prime256v1 -genkey
    (echo '[ req ]'; echo 'distinguished_name=dn'; echo 'prompt = no'; echo '[ ext ]'; echo "basicConstraints = CA:TRUE"; echo "keyUsage = digitalSignature, keyCertSign, cRLSign"; echo '[ dn ]'; echo 'CN = ROOT_CA') > certificates/root.cnf
    openssl req -key certificates/rootCA.key -new -x509 -days 7300 -sha256 -out certificates/rootCA.pem -config certificates/root.cnf -extensions ext
    ```

1. [Upload the root CA certificate to the portal](https://www.pelion.com/docs/device-management/latest/provisioning-process/managing-ca-certificates.html#uploading-a-ca-certificate-or-certificate-chain).

    **Important:** When you upload your root CA certificate to Device Management Portal, you must select **Enrollment - I received this certificate from the device manufacturer or a supplier** from the **How will devices use this certificate?** dropdown.

1. Set up your project workspace for CySecureTools and create keys based on the `cytfm_pelion_policy.json` policy:

    ```
    cd ./TARGET_CYTFM_064B0S2_4343W
    cysecuretools -t cy8ckit-064b0s2-4343w init
    cysecuretools -t cy8ckit-064b0s2-4343w -p policy/cytfm_pelion_policy.json create-keys
    ```

    **Note:** You use these keys to sign future application images and the device uses the keys to verify the application images. Therefore, if you lose the keys, you need to re-provision the board with new keys.

1. Provision the device with your root CA, app keys and device certificate:
    ```
    python ../mbed-os/targets/TARGET_Cypress/TARGET_PSOC6/TARGET_CYTFM_064B0S2_4343W/reprov_helper.py -d cy8ckit-064b0s2-4343w -p policy/cytfm_pelion_policy.json -existing-keys --serial <device's unique serial number> -y
    cd ../
    ```

## Building and running the example

1. Build the example:

    ```
    mbed compile -m CYTFM_064B0S2_4343W -t GCC_ARM
    ```

1. Flash and run the application:

    1. Drag and drop the hex output file (`BUILD/CYTFM_064B0S2_4343W/GCC_ARM/mbed-os-example-pelion.hex`) to the mounted drive for the board.
    1. Reset the board.
    1. Copy the enrollment ID from the terminal output.
    1. Upload the enrollment ID to Device Management:
        1. In Device Management Portal, click **Device directory** > **Enrolling devices**.
        1. Enter the enrollment ID in the **Device enrollment key** field.
        1. Click **Add single device**.
    1. Go back to the terminal, press **C** to continue, and check that the device connects to Device Management.

**Note:** For development purposes, you can reset the Device Management credentials by running `pyocd erase -s 0x101C0000-0x101C9000`.

## Updating firmware

The `CYTFM_064B0S2_4343W` target board has two cores - CM0 for the T-FM firmware, CM4 for the example application.

We currently support updating the example application in the CM4 core.

**To update the example application:**

1. Install manifest-tool v2.0 or higher:

    ```
    pip install --upgrade manifest-tool
    ```
    The Cypress update flow requires the newest version of the manifest-tool.

1. Initialize the environment:

    ```
    manifest-dev-tool init --force -a [access key from Device Management Portal]
    ```
    For information about access keys, please see [Application access keys](https://www.pelion.com/docs/device-management/latest/user-account/application-access-keys.html).

1. Update the firmware version in the `cytfm_pelion_policy.json` file:

    1. Go to `"id": 16` in the file.

       This section of the file holds the CM4 core configuration.

    1. Update `"version": "<new firmware version>",`.

        Where `<new firmware version>` is a string in MSB.LSB (Most Significant Byte/Least Significant Byte) format.

1. Build the upgraded signed image:

    ```
    mbed compile -m CYTFM_064B0S2_4343W -t GCC_ARM
    ```
    This creates a `./BUILD/CYTFM_064B0S2_4343W/GCC_ARM/mbed-os-example-pelion-psoc64_upgrade.hex` file.

    The manifest tool does not currently support hex files; therefore, you must convert the image to bin format.

1. To convert the upgrade image from hex to bin format:

    ```
    python inthex2bin.py BUILD/CYTFM_064B0S2_4343W/GCC_ARM/mbed-os-example-pelion-psoc64_upgrade.hex
    ```
    This creates the `./BUILD/CYTFM_064B0S2_4343W/GCC_ARM/mbed-os-example-pelion-psoc64_upgrade.bin` file.

1. Perform the update:

    ```
    manifest-dev-tool update-v1 --payload-path BUILD/CYTFM_064B0S2_4343W/GCC_ARM/mbed-os-example-pelion-psoc64_upgrade.bin --fw-version <new firmware version> --device-id <device ID> --start-campaign --wait-for-completion --timeout 3600
    ```

    Where:

    - `<new firmware version>` is a 64-bit unsigned integer, where 32 MSBs represent the major version and 32 LSBs represent the minor. For example, version 1.0 is represented as `4294967296` and version 1.1 as `4294967297`.
    - `<device ID>` is the ID of the device to be updated.

    You can validate that the device is using the upgraded firmware in the serial monitor printout:

    ```
    Current FW image version: <new firmware version>
    ```
