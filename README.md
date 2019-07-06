# Pelion Client Mbed OS Example

This is a simplified example with the following features:
- Mbed OS 5.13 and Pelion Device Management Client 3.3.0 
- Support for FW Update
- 200 lines of codes + credential sources

Note this is considered alpha with early access to partners.

## Supported platforms

This table shows a list of platforms that are supported.

Platform                          |  Connectivity      | Storage         | Notes          
----------------------------------| -------------------| ----------------| --------------  
NXP K64F                          | Ethernet           | Internal Flash  | 
NXP K66F                          | Ethernet           | Internal Flash  |   
ST NUCLEO_F429ZI                  | Ethernet           | Internal Flash  |   
ST NUCLEO_F411RE                  | WiFi IDW01M1       | SD card         |
Ublox UBLOX_EVK_ODIN_W2           | WiFi               | SD card         | 

<span class="notes">**(*) Note**: the platforms require further testing</span>

# Developer guide

This section is intended for developers to get started, import the example application, compile and get it running on their device.

## Requirements

- Mbed CLI >= 1.10.0
  
  For instruction on installing and using Mbed CLI, please see our [documentation](https://os.mbed.com/docs/mbed-os/latest/tools/developing-mbed-cli.html).
  
- Install the `CLOUD_SDK_API_KEY`

   `mbed config -G CLOUD_SDK_API_KEY ak_1MDE1...<snip>`

   You should generate your own API key. Pelion Device Management is available for any Mbed developer. Create a [free trial](https://os.mbed.com/pelion-free-tier).

   For instructions on how to generate your API key, please see our [documentation](https://cloud.mbed.com/docs/current/integrate-web-app/api-keys.html#generating-an-api-key). 

## Deploying

This repository is in the process of being updated and depends on few enhancements being deployed in mbed-cloud-client. In the meantime, follow these steps to import and apply the patches before compiling.

    mbed import mbed-os-pelion-example
    cd mbed-os-pelion-example
    ./patch-mbed-cloud-client.sh

## Compilation

    mbed target K64F
    mbed toolchain GCC_ARM
    mbed device-management init -d arm.com --model-name example-app --force -q
    mbed compile

## Program Flow

1. Bootstrap
1. Register
1. Open a serial terminal using putty/minicom (115200 bauds)
   - Press enter through putty/minicom to simulate button
   - Press 'i' to print endpoint name
   - Press Ctrl-C to to unregister
   - Press 'r' to reset storage and reboot (warning: it generates a new device ID!)
 
## Further information and requirements

Check the public tutorial for further information:

    https://www.pelion.com/docs/device-management/current/connecting/mbed-os.html

## Troubleshooting

- Device initializes but can't register to Pelion

  Error: `client_error(3) -> Bootstrap server URL is not correctly formed`
  
  Solution: Format the the storage by pressing 'r' in the serial terminal.
  
# Porting process for new platforms

## 1. Application configuration

### Recomentations:

Credentials should be installed on internal flash. Whenever possible, internal flash should also be used to store the candidate image for the FW update process, however, if there is no enough space, you may need to enable external storage (SD Card, SPI, etc).

Mbed OS platforms should have a default configuration for connectivity and storage in Mbed OS (`targets.json`).
You can extend or override the default configuration in `mbed_app.json`. Create a new entry under the target name for your device.

- **Connectivity** - Specify the default connectivity type for your target. It's essential with targets that lack default connectivity set in `targets.json` or for targets that support multiple connectivity options. For example:
   
   ```
            "target.network-default-interface-type" : "ETHERNET",
   ```
      
   The possible options are `ETHERNET`, `WIFI` and `CELLULAR`.
   
   Depending on connectivity type, you might have to specify more config options.

- **Storage** - Specify the storage block device type, which dynamically adds the block device driver you specified at compile time. For example:

   ```
            "target.components_add" : ["SD"],
   ```

   Valid options are `SD`, `SPIF`, `QSPIF` and `FLASHIAP` (not recommended). For more available options, please see the [block device components](https://github.com/ARMmbed/mbed-os/tree/master/components/storage/blockdevice).

   You also have to specify block device pin configuration, which may vary from one block device type to another. Here's an example for `SD`:
      
   ```
            "sd.SPI_MOSI"                      : "PE_6",
            "sd.SPI_MISO"                      : "PE_5",
            "sd.SPI_CLK"                       : "PE_2",
            "sd.SPI_CS"                        : "PE_4",
   ```
   
    If you are using SPI/QSPI flash, please make sure you have specified the correct SPI frequency by configuring `spif-driver.SPI_FREQ`. If it is not configured, 40Mhz will be applied by default.
   
- **Flash** - Define the basics for the microcontroller flash. For example:
   
   ```
            "device-management.flash-start-address"              : "0x08000000",
            "device-management.flash-size"                       : "(2048*1024)",
   ```
   
- **SOTP** --> **TODO: REPLACE by KVSTORE**

Define two SOTP or NVStore regions that Mbed OS Device Management will use to store its special keys, which encrypt the data stored. Use the last two Flash sectors (if possible) to ensure that they don't get overwritten when new firmware is applied. For example:

   ```
            "device-management.sotp-section-1-address"            : "(MBED_CONF_APP_FLASH_START_ADDRESS + MBED_CONF_APP_FLASH_SIZE - 2*(128*1024))",
            "device-management.sotp-section-1-size"               : "(128*1024)",
            "device-management.sotp-section-2-address"            : "(MBED_CONF_APP_FLASH_START_ADDRESS + MBED_CONF_APP_FLASH_SIZE - 1*(128*1024))",
            "device-management.sotp-section-2-size"               : "(128*1024)",
            "device-management.sotp-num-sections" : 2
   ```

`*-address` defines the start of the Flash sector, and `*-size` defines the actual sector size. `sotp-num-sections` should always be set to `2`.


## 2. Bootloader configuration

The bootloader is required to perform FW Updates. The steps below explain how to create a new configuration and binary for the bootloader.

1. Import as a new application the official [mbed-bootloader](https://github.com/ARMmbed/mbed-bootloader/) repository or the [mbed-bootloader-extended](https://github.com/ARMmbed/mbed-bootloader-extended/) repository that builds on top of `mbed-bootloader` and extends the support for file systems and storage drivers. You can do this with `mbed import mbed-bootloader-extended`.

1. Inside the imported bootloader application, and edit the application configuration, for example `mbed-bootloader-extended/mbed_app.json`. Add a new target entry similar to the step above, and specify:

   - **Flash** - Define the basics for the microcontroller flash (the same as in `mbed_app.json`). For example:
   
      ```
            "flash-start-address"              : "0x08000000",
            "flash-size"                       : "(2048*1024)",
      ```

   - **SOTP** --> **TODO: REPLACE by KVSTORE**
   
   Similar to the **SOTP** step above, specify the location of the SOTP key storage. In the bootloader, the variables are named differently. Try to use the last two Flash sectors (if possible) to ensure that they don't get overwritten when new firmware is applied. For example:
   
    ```
            "nvstore.area_1_address"           : "(MBED_CONF_APP_FLASH_START_ADDRESS + MBED_CONF_APP_FLASH_SIZE - 2*(128*1024))",
            "nvstore.area_1_size"              : "(128*1024)",
            "nvstore.area_2_address"           : "(MBED_CONF_APP_FLASH_START_ADDRESS + MBED_CONF_APP_FLASH_SIZE - 1*(128*1024))", "nvstore.area_2_size" : "(128*1024)",
    ```

    - **Application offset** - Specify start address for the application, and also the update-client meta information. As these are automatically calculated, you can copy the ones below:
    
    ```
            "update-client.application-details": "(MBED_CONF_APP_FLASH_START_ADDRESS + 64*1024)",
            "application-start-address"        : "(MBED_CONF_APP_FLASH_START_ADDRESS + 65*1024)",
            "max-application-size"             : "DEFAULT_MAX_APPLICATION_SIZE",
    ```
    
    - **Storage** - Specify the block device pin configuration, exactly as you defined it in the `mbed_app.json` file. For example:
    
    ```
            "target.components_add"            : ["SD"],
            "sd.SPI_MOSI"                      : "PE_6",
            "sd.SPI_MISO"                      : "PE_5",
            "sd.SPI_CLK"                       : "PE_2",
            "sd.SPI_CS"                        : "PE_4"
    ```
    
    If you are using SPI/QSPI flash, please make sure you have specified the correct SPI frequency by configuring `spif-driver.SPI_FREQ`. If it is not configured, 40Mhz will be applied by default.
    
1. Compile the bootloader using the `bootloader_app.json` configuration you just edited:

   ```
   $ mbed compile -t <TOOLCHAIN> -m <TARGET> --profile=tiny.json --app-config=<path to mbed-os-pelion-example/bootloader/bootloader_app.json>
   ```

<span class="notes">**Note:** `mbed-bootloader` is primarily optimized for `GCC_ARM`, so you may want to compile it with that toolchain.
Before jumping to the next step, you should compile and flash the bootloader and then connect over the virtual comport to ensure the bootloader is running correctly. You can ignore errors related to checksum verification or falure to jump to application - these are expected at this stage.</span>

### 3. Add the bootloader to your application

1. Copy the compiled bootloader from `mbed-bootloader/BUILDS/<TARGET>/<TOOLCHAIN>-TINY/mbed-bootloader.bin` to `<your_application_name>/bootloader/mbed-bootloader-<TARGET>.bin`.

1. Edit `<your_application_name>/mbed_app.json`, and modify the target entry to include:

   ```
            "target.features_add"              : ["BOOTLOADER"],
            "target.bootloader_img"            : "bootloader/mbed-bootloader-<TARGET>.bin",
            "target.app_offset"                : "0x10400",
            "target.header_offset"             : "0x10000",
            "update-client.application-details": "(MBED_CONF_APP_FLASH_START_ADDRESS + 64*1024)",
   ```
 
   <span class="notes">**Note:**    
      - `update-client.application-details` should be identical in both `bootloader_app.json` and `mbed_app.json`.
      - `target.app_offset` is relative offset to `flash-start-address` you specified in `mbed_app.json` and `bootloader_app.json`, and is the hex value of the offset specified by `application-start-address` in `bootloader_app.json`. For example,  `(MBED_CONF_APP_FLASH_START_ADDRESS+65*1024)` dec equals `0x10400` hex.
      - `target.header_offset` is also relative offset to the `flash-start-address` you specified in the `bootloader_app.json`, and is the hex value of the offset specified by `update-client.application-details`. For example, `(MBED_CONF_APP_FLASH_START_ADDRESS+64*1024)` dec equals `0x10000` hex.</span>

## Validation and testing

To confirm that the platform is working correcly, run the following tests:

- Mbed OS test

  `mbed test`

- Mbed OS integration tests

  See mbed-os/TESTS/integration/README.md (sip-workshop branch)
  
  `mbed test -t <toolchain> -m <platform> -n *integration-* -DINTEGRATION_TESTS -v `

- Pelion Client tests, including firmware update.

  See the [testing](./TESTS/Readme.md) documentation to validate the configuration on this example.

# Known-issues

Please review existing issues on github and report any problem you may see.
