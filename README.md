# Pelion Client Mbed OS Example

This is a simplified example for Mbed OS with the following features:
- Mbed OS 5.14.1 and Pelion Device Management Client 4.0.0
- Support for FW Update
- 200 lines of code + credential sources

There is an example of the client for multiple operating systems in [mbed-cloud-client-example](https://github.com/ARMmbed/mbed-cloud-client-example) repository. The underlying client library is the same for both. This Mbed OS only example is simpler as it only supports one OS, but if you want to do development in Linux and Mbed OS at the same time - you should use the [mbed-cloud-client-example](https://github.com/ARMmbed/mbed-cloud-client-example).

## Supported platforms

This table shows a list of platforms that are supported.

Platform                          |  Connectivity     | Storage for credentials and FW candidate | Notes
----------------------------------| ------------------| ------------------------| --------------
NXP K64F                          | Ethernet          | Internal Flash          |
NXP K66F                          | Ethernet          | Internal Flash          |
ST NUCLEO_F429ZI                  | Ethernet          | Internal Flash          |
ST NUCLEO_F411RE                  | Wi-Fi ESP8266     | SD card                 |
Ublox UBLOX_EVK_ODIN_W2           | Wi-Fi             | SD card                 |
ST DISCO_L475VG_IOT01A            | Wi-Fi             | QSPIF                   |
Embedded Planet Agora             | Cellular          | SPIF                    | Build-only

Build-only = This target is currently verified only via compilation, and is not verified at runtime.

# Developer guide

This section is intended for developers to get started, import the example application, compile and get it running on their device.

## Requirements

- Mbed CLI >= 1.10.0

  For instructions on installing and using Mbed CLI, please see our [documentation](https://os.mbed.com/docs/mbed-os/latest/tools/developing-mbed-cli.html).

- Install the `CLOUD_SDK_API_KEY`

   `mbed config -G CLOUD_SDK_API_KEY ak_1MDE1...<snip>`

   You should generate your own API key. Pelion Device Management is available for any Mbed developer. Create a [free trial](https://os.mbed.com/pelion-free-tier).

   For instructions on how to generate your API key, please see our [documentation](https://cloud.mbed.com/docs/current/integrate-web-app/api-keys.html#generating-an-api-key).

## Deploying

This repository is in the process of being updated and depends on few enhancements being deployed in mbed-cloud-client. In the meantime, follow these steps to import and apply the patches before compiling.

    ```
    mbed import mbed-os-pelion-example
    cd mbed-os-pelion-example
    ```

## Compiling

    mbed target K64F
    mbed toolchain GCC_ARM
    mbed device-management init -d arm.com --model-name example-app --force -q
    mbed compile

## Program Flow

1. Initialize, connect and register to Pelion DM
1. Interact with the user through the serial port (115200 bauds)
   - Press enter through putty/minicom to simulate button
   - Press 'i' to print endpoint name
   - Press Ctrl-C to to unregister
   - Press 'r' to reset storage and reboot (warning: it generates a new device ID!)

## Further information and requirements

Check the public tutorial for further information:

  [https://www.pelion.com/docs/device-management/current/connecting/mbed-os.html](https://www.pelion.com/docs/device-management/current/connecting/mbed-os.html)

## Troubleshooting

- Device initializes but can't register to Pelion

  Error: `client_error(3) -> Bootstrap server URL is not correctly formed`

  Solution: Format the the storage by pressing 'r' in the serial terminal.

# Porting process to add support for an Mbed Enabled board

  There are many steps involved in this process. We generally recomend the following steps:

  1. Configure the application using `mbed_app.json`
      - Configure the default connectivity
      - Configure the KVSTORE area to store credentials (internal or external memory)
      - Build the application, program the board and observe whether the application can connect to Pelion DM by using a serial terminal.
  1. Configure the bootloader using `bootloader_app.json`
      - Configure the KVSTORE area
      - Configure the FW Candidate Storage
      - Build bootloader application, program the board and observe whether this is able to boot.
  1. Enable application with bootloader using `mbed_app.json`
      - Enable the usage of the bootloader
      - Ensure the KVSTORE addresses and FW Candidate storage addresses match with the bootloader configuration
      - Build the application again (this time combined with bootloader) and check whether it can boot and connect to Pelion DM.
      - Perform a FW Update and check whether the process can be completed succesfully.

## 1. Application configuration

<span class="notes">**Note**: consider allocating the credentials on internal flash to simplify the application setup process. In addition, consider the use of internal flash to store the firmware candidate image for the FW update process as this would remove the need to use external components. If there isn't enough space, you may need to enable external storage (SD Card, SPI, etc).</span>

Mbed OS boards should have a default configuration for connectivity and storage in Mbed OS (`targets.json`).
You can extend or override the default configuration using `mbed_app.json` in this application. Create a new entry under the target name for your device.

### a. Connectivity

  Specify the default IP connectivity type for your target. It's essential with targets that lack default connectivity set in `targets.json` or for targets that support multiple connectivity options. For example:

      "target.network-default-interface-type" : "ETHERNET",

  The possible options are `ETHERNET`, `WIFI` and `CELLULAR`.

  Depending on connectivity type, you might have to specify more configuration options. Review the [documentation](https://os.mbed.com/docs/mbed-os/latest/porting/porting-connectivity.html) for further information.

### b. Storage for credentials

  Start by getting familiar with the multiple [storage options](https://os.mbed.com/docs/mbed-os/latest/reference/storage.html) and configurations supported in Mbed OS.

  Then start designing the system memory map, the location of components (whether they are on internal or external memory), and the corresponding base addresses and sizes. You may want to create a diagram similar to the one below to help you to make design decisions:

    +--------------------------+
    |                          |
    |                          |
    |                          |
    |Firmware Candidate Storage|
    |                          |
    |                          |
    |                          |
    +--------------------------+ <-+ update-client.storage-address
    |                          |
    |         KVSTORE          |
    |                          |
    +--------------------------+ <-+ storage_tdb_internal.internal_base_address
    |                          |
    |        Free space        |
    |                          |
    +--------------------------+
    |                          |
    |                          |
    |        Active App        |
    |                          |
    |                          |
    |                          |
    +--------------------------+ <-+ mbed-bootloader.application-start-address
    |Active App Metadata Header|
    +--------------------------+ <-+ update-client.application-details
    |                          |
    |        Bootloader        |
    |                          |
    +--------------------------+ <-+ 0

  In cases where the MCU has two separate memory banks, it's appropiate to allocate the bootloader and base application in one bank, and KVSTORE storage at the begining of the second bank followed by a firmware candidate storage.

  - **Option 1:** Allocating credentials in internal memory

    **This is the preferred option whenever possible**. Make sure `TDB_INTERNAL` is the type of storage selected in `mbed_app.json`. Specify the base address depending on the available memory in the system. The size of this section should be aligned with the flash erase sector. The value should be multiple of 4 with a minimum of 24KB and upwards depending on the use case (for example the usage of certificate chain will increase the need of storage to hold those certificates). An example of this configuration can be seen for the `NUCLEO_F429ZI` platform in this application.

        "storage.storage_type"                      : "TDB_INTERNAL"
        "storage_tdb_internal.internal_base_address": "(MBED_ROM_START+1024*1024)",
        "storage_tdb_internal.internal_size"        : "(128*1024)",

  - **Option 2:** Allocating credentials in external memory:

    This is possible when the platform has an storage device wired to the MCU (could be on-board or external component). Make sure `FILESYSTEM` is specified as type of storage. The blockdevice and filesystem should be one of the supported in Mbed OS (see [docs](https://os.mbed.com/docs/mbed-os/latest/porting/blockdevice-port.html)).

    An example of this configuration can be seen for the `K64F` platform in the [mbed-cloud-client-example](https://github.com/ARMmbed/mbed-cloud-client-example/blob/master/mbed_app.json#L32)

        "storage.storage_type"                      : "FILESYSTEM",
        "storage_filesystem.blockdevice"            : "SD",
        "storage_filesystem.filesystem"             : "LITTLE",
        "storage_filesystem.internal_base_address"  : "(32*1024)",
        "storage_filesystem.rbp_internal_size"      : "(8*1024)",
        "storage_filesystem.external_base_address"  : "(0x0)",
        "storage_filesystem.external_size"          : "(1024*1024*64)",

### c. Storage for firmware updates

  Before enabling FW updates, it's recomended that the application is able to initialize the network and connect to Pelion DM.

  Once the connection is successfull, you can follow the steps below to enable the platform to receive FW updates. Note the configuration for the application in this section should match with the one on  the bootloader - see section below.

  - Common configuration

    Regardless of where the firmware candidate is located (internal or external), there is a need to have a bootloader in place. The binary of the booloader can be specified with the `bootloader_img` option. The address and size of the bootloader determines the `application-details` and `bootloader-details` options. The value of `bootloader-details` can be obtained by running the binary on the target and observing the serial output.

    Review the [mbed-bootloader](https://github.com/ARMmbed/mbed-bootloader#configurations) guidelines on how these options should be selected. Review the [bootloader configuration](2.-Bootloader-configuration) section below for more information.

    Copy the compiled bootloader from `mbed-bootloader/BUILDS/<TARGET>/<TOOLCHAIN>-TINY/mbed-bootloader.bin` to `bootloader/mbed-bootloader-<TARGET>.bin`.

    Edit `mbed-os-pelion-example/mbed_app.json` and modify the target configuration to match with the one in `bootloader_app.json`.

   <span class="notes">**Note:**

  - `update-client.application-details` should be identical in both `bootloader_app.json` and `mbed_app.json`.

  - `target.app_offset` is relative offset to `flash-start-address` you specified in `mbed_app.json` and `bootloader_app.json`, and is the hex value of the offset specified by `application-start-address` in `bootloader_app.json`. For example,  `(MBED_CONF_APP_FLASH_START_ADDRESS+65*1024)` dec equals `0x10400` hex.

  - `target.header_offset` is also relative offset to the `flash-start-address` you specified in the `bootloader_app.json`, and is the hex value of the offset specified by `update-client.application-details`. For example, `(MBED_CONF_APP_FLASH_START_ADDRESS+64*1024)` dec equals `0x10000` hex.</span>

  An example of this configuration can be seen for the `NUCLEO_F429ZI` platform.

        "update-client.application-details"         : "(MBED_ROM_START + MBED_BOOTLOADER_SIZE)",
        "update-client.bootloader-details"          : "0x08007300",
        "target.bootloader_img"                     : "bootloader/mbed-bootloader-<target>",
        "target.header_offset"                      : "0x8000",
        "target.app_offset"                         : "0x8400",

  - **Option 1:** Allocating the firmware update candidate in internal memory

    **This is the preferred option whenever possible**. Make sure `ARM_UCP_FLASHIAP` is selected in `update-storage` in `mbed_app.json`. This area should be located at the end of the flash after the KVSTORE area. Specify the `storage-address`, `storage-size` and `storage-page` as required. The `application-details` option should point at the end of the bootloader area. An example of this configuration can be seen for the `NUCLEO_F429ZI` platform.

        "mbed-cloud-client.update-storage"          : "ARM_UCP_FLASHIAP",
        "update-client.storage-address"             : "(MBED_CONF_STORAGE_TDB_INTERNAL_INTERNAL_BASE_ADDRESS+MBED_CONF_STORAGE_TDB_INTERNAL_INTERNAL_SIZE)",
        "update-client.storage-size"                : "(1024*1024-MBED_CONF_STORAGE_TDB_INTERNAL_INTERNAL_SIZE)",
        "update-client.storage-page"                : 1,

  - **Option 2:** Allocating the firmware update candidate in external memory

  When using an external device to the MCU to store the firmware candidate, make sure `ARM_UCP_FLASHIAP_BLOCKDEVICE` is specified as type of `update-storage`. Specify the `storage-address`, `storage-size` and `storage-page` as required.

  An example of this configuration can be seen for the `K64F` platform in the [mbed-cloud-client-example](https://github.com/ARMmbed/mbed-cloud-client-example/blob/master/mbed_app.json#L32)

        "mbed-cloud-client.update-storage"          : "ARM_UCP_FLASHIAP_BLOCKDEVICE",
        "update-client.storage-address"             : "(1024*1024*64)",
        "update-client.storage-size"                : "((MBED_ROM_START + MBED_ROM_SIZE - APPLICATION_ADDR) * MBED_CONF_UPDATE_CLIENT_STORAGE_LOCATIONS)",

## 2. Bootloader configuration

The bootloader is required to perform FW Updates. The steps below explain how to create a new configuration and binary for the bootloader.

1. Import as a new application the [mbed-bootloader](https://github.com/ARMmbed/mbed-bootloader/) repository.

1. Edit the bootloader application configuration in this example (`bootloader/bootloader_app.json`) and add a new target entry. An example of this configuration can be seen for the `NUCLEO_F429ZI` platform:


       "update-client.firmware-header-version"    : "2",
       "mbed-bootloader.use-kvstore-rot"          : 0,
       "mbed-bootloader.bootloader-size"          : "APPLICATION_SIZE",
       "update-client.application-details"        : "(MBED_ROM_START + MBED_BOOTLOADER_SIZE)",
       "mbed-bootloader.application-start-address": "(MBED_CONF_UPDATE_CLIENT_APPLICATION_DETAILS + MBED_BOOTLOADER_ACTIVE_HEADER_REGION_SIZE)",
       "mbed-bootloader.max-application-size"     : "(MBED_ROM_START + MBED_BOOTLOADER_FLASH_BANK_SIZE - MBED_CONF_MBED_BOOTLOADER_APPLICATION_START_ADDRESS)",
       "update-client.storage-address"            : "(MBED_ROM_START + MBED_BOOTLOADER_FLASH_BANK_SIZE + KVSTORE_SIZE)",
       "update-client.storage-size"               : "(MBED_BOOTLOADER_FLASH_BANK_SIZE - KVSTORE_SIZE)",
       "update-client.storage-locations"          : 1,
       "kvstore-size"                             : "2*64*1024",
       "update-client.storage-page"               : 1

1. Compile the bootloader using the `bootloader_app.json` configuration you've just edited:

    `mbed compile -t <TOOLCHAIN> -m <TARGET> --profile=tiny.json --app-config=.../mbed-os-pelion-example/bootloader/bootloader_app.json>`

<span class="notes">**Note:** `mbed-bootloader` is primarily optimized for `GCC_ARM`, so you may want to compile it with that toolchain.
Before jumping to the next step, you should compile and flash the bootloader and then connect over the virtual serial port to ensure the bootloader is running correctly. You can ignore errors related to checksum verification or failure to jump to application - these are expected at this stage.</span>

## Validation and testing

In addition to having an example application succesfully connected to Pelion DM, it's required to ensure that the application is working correcly in multiple situations. This can be achived by running the following tests:

- Mbed OS tests (as described in our [documentation](https://os.mbed.com/docs/mbed-os/latest/porting/testing.html))

  `mbed test`

- Mbed OS integration tests

  See [mbed-os/TESTS/integration/README.md](https://github.com/ARMmbed/mbed-os/blob/sip-workshop/TESTS/integration/README.md) (sip-workshop branch)

  `mbed test -t <toolchain> -m <platform> -n *integration-* -DINTEGRATION_TESTS -v`

- Pelion Client tests, including firmware update.

  See the [testing](./TESTS/README.md) documentation to validate the configuration in this example.

# Known-issues

Please review existing issues on github and report any problem you may see.
