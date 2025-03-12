### Directory Breakdown:

- baseline_sampler.ino
    - This is the main file that should be uploaded to the ESP32 for collecting baseline data. It initalizes, collects, and sends data to the Raspberry Pi Server
- tapo_device.h
    - Contains the on/off/energy commands to control the Tapo
    - Modifications to original library include ability to poll energy data, nonblocking delays, and a priority system for commands
- tapo_protocol.h
    - Contains functions to send the actual requests to and from the Tapo device
    - Also manages handshaking
    - largely unmodified from original tapo-esp32 library
- tapo_cipher.h
    - Contains functions that implement the KLAP encryption scheme used by Tapo devices
    - unmodified from original tapo-esp32 library