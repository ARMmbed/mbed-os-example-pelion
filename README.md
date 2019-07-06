# Pelion Client Mbed OS Example

Features:
- Mbed OS 5.13
- Supports Update
- 200 lines of codes + credential sources

# Deploying

Because the point of this repository is to propose the change ideas, mbed-cloud-client patching is required before compilation.

    mbed import mbed-os-pelion-example
    cd mbed-os-pelion-example
    ./patch-mbed-cloud-client.sh

# Compilation

    mbed target K64F
    mbed toolchain GCC_ARM
    mbed device-management init -d arm.com --model-name example-app --force -q
    mbed compile

# Program Flow

1. Bootstrap
1. Register
1. Send enter through putty/minicom to simulate button
1. Send 'i' to print endpoint name
1. Send Ctrl-C through putty/minicom to unregister

# Further information and requirements

Check the public tutorial for further information:

    https://www.pelion.com/docs/device-management/current/connecting/mbed-os.html

# Troubleshooting

- Device initializes but can't register to Pelion

  Error: `client_error(3) -> Bootstrap server URL is not correctly formed`
  
  Solution: Format the the storage by pressing 'r' in the serial terminal.
