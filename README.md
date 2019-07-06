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

TBD

# Known-issues

Please review existing issues on github and report any problem you may see.
