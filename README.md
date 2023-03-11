# Digital Voice Modem Firmware (Hotspot)

The DVM hotspot firmware provides the embedded microcontroller implementation of a dedicated-mode DMR, P25 or NXDN hotspot system. The firmware; is the portion of a complete Over-The-Air modem implementation that uses an ADF7021 to provide a raw RF interface.

This project is a direct fork of the MMDVM_HS (https://github.com/juribeparada/MMDVM_HS) project.

Please feel free to reach out to us for help, comments or otherwise, on our Discord: https://discord.gg/3pBe8xgrEz

## Building

Please see the various Makefile's included in the project for more information. This project includes a few Makefiles to target different hardware. (All following information assumes familiarity with the standard Linux make system.)

* Makefile.STM32FX - This makefile is used for targeting a generic STM32F103 with an ADF7021 RF SoC device.

* For STM32F103 using Ubuntu OS install the standard ARM embedded toolchain (typically arm-gcc-none-eabi).
  - Make sure to clone this repository with the ```--recurse-submodules``` option, otherwise the STM32 platform files will be missing! ```git clone --recurse-submodules https://github.com/DVMProject/dvmfirmware-hs.git```

To build the firmware, use the ```make``` command, followed by -f and the correct makefile, followed by the type of board you are using. 

An example of this would be ```make -f Makefile.STM32FX mmdvm-hs-hat-dual``` for a full duplex modem hotspot, attached to GPIO.

## Firmware installation

The device can be used on top on a RPi attached via the GPIO port or standalone and connected via USB (see usb-support branch). Both variants require different handling of compiling and uploading the firmware, examples on flashing devices are mostly not included here because the methods to flash vary from device to device.

### Install the firmware via GPIO on Raspberry Pi

> **_NOTE:_**  Your mileage may vary with these instructions, the hotspot boards are loosely designed around a common factor but not all are created equally.

First you will need to disable the serial console and disable bluetooth. Edit ```/boot/cmdline.txt``` and remove the line ```console=serial0, 115200```.
Next, you will need to disable bluetooth on the board. Edit ```/boot/config.txt``` and add a line containing ```dtoverlay=disable-bt```. Reboot.

> Most sets of instructions reccomend to download stm32flash from online, however we have found the prepackaged version to work fine.

Once the hotspot is back on, navigate to the build folder where you compiled the firmware. Put a jumper across the J1 points on the board, and the RED heartbeat LED should stop flashing. Run the below command to flash.

```stm32flash -v -w dvm-firmware-hs_f1.bin -i 20,-21,21,-20 -R /dev/ttyAMA0```

You should see the below output if the board flashed successfully.
```
Wrote and verified address 0x0800be40 (100.00%) Done.

Resetting device... 
Reset done.
```

## Notes

**USB Support Note**: See the usb-support branch for the version of this firmware that supports USB.
**NXDN Support Note**: NXDN support is currently experimental.

## License

This project is licensed under the GPLv2 License - see the [LICENSE.md](LICENSE.md) file for details. Use of this project is intended, for amateur and/or educational use ONLY. Any other use is at the risk of user and all commercial purposes is strictly discouraged.

