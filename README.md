# Digital Voice Modem Firmware (Hotspot)

The DVM firmware provides the embedded microcontroller implementation of a mixed-mode DMR/P25 or dedicated-mode DMR or P25 repeater system. The firmware; is the portion of a complete Over-The-Air modem implementation that uses an ADF7021 to provide a raw RF interface.

This project is a direct fork of the MMDVM_HS (https://github.com/juribeparada/MMDVM_HS) project.

## Building

Please see the various Makefile's included in the project for more information. This project includes a few Makefiles to target different hardware. (All following information assumes familiarity with the standard Linux make system.)

* Makefile.STM32FX - This makefile is used for targeting a generic STM32F103 with an ADF7021 RF SoC device.

* For STM32F103 using Ubuntu OS install the standard ARM embedded toolchain (typically arm-gcc-none-eabi).
  - Make sure to clone this repository with the ```--recurse-submodules``` option, otherwise the STM32 platform files will be missing! ```git clone --recurse-submodules https://github.com/DVMProject/dvmfirmware-hs.git```

Use the ```make``` command to build the firmware, choosing the appropriate makefile with the -F switch.

## License

This project is licensed under the GPLv2 License - see the [LICENSE.md](LICENSE.md) file for details. Use of this project is intended, strictly for amateur and educational use ONLY. Any other use is at the risk of user and all commercial purposes are strictly forbidden.

