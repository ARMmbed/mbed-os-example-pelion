# Pelion Client Mbed OS Example

Features:
- Mbed OS 5.13
- Supports Update
- 200 lines of codes + credential sources

# Deploying

Because the point of this repository is to propose the change ideas, mbed-cloud-client patching is required before compilation.

    mbed import new-mbed-os-pelion-example
    cd new-mbed-os-pelion-example
    git checkout -b reduced-configuration -t origin/reduced-configuration
    ./patch-mbed-cloud-client.sh

# Compilation

    mbed target K64F
    mbed toolchain GCC_ARM
    mbed compile
    

# Program Flow

1. Bootstrap
1. Register
1. Send enter through putty/minicom to simulate button
1. Send 'i' to print endpoint name
1. Send Ctrl-C through putty/minicom to unregister
