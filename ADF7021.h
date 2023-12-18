/**
* Digital Voice Modem - DSP Firmware (Hotspot)
* GPLv2 Open Source. Use is subject to license terms.
* DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
*
* @package DVM / DSP Firmware (Hotspot)
*
*/
//
// Based on code from the MMDVM_HS project. (https://github.com/juribeparada/MMDVM_HS)
// Licensed under the GPLv2 License (https://opensource.org/licenses/GPL-2.0)
//
/*
*   Copyright (C) 2020 by Jonathan Naylor G4KLX
*   Copyright (C) 2016 by Jim McLaughlin KI6ZUM
*   Copyright (C) 2016,2017,2018 by Andy Uribe CA6JAU
*   Copyright (C) 2017 by Danilo DB4PLE 
*   Copyright (C) 2021 Bryan Biedenkapp N2PLL
*
*   Some of the code is based on work of Guus Van Dooren PE1PLM:
*   https://github.com/ki6zum/gmsk-dstar/blob/master/firmware/dvmega/dvmega.ino
*
*   This program is free software; you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation; either version 2 of the License, or
*   (at your option) any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with this program; if not, write to the Free Software
*   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/
#if !defined(__ADF7021_H__)
#define __ADF7021_H__

#include "Defines.h"

// ---------------------------------------------------------------------------
//  Constants
// ---------------------------------------------------------------------------

#define LOW 0
#define HIGH 1

#define DEFAULT_FREQUENCY       433075000

/** Band Tables */
/** 136 - 174 mhz */
#define VHF_MIN                 136000000
#define VHF_MAX                 174000000

/** 216 - 225 mhz */
#define VHF_220_MIN             216000000
#define VHF_220_MAX             225000000

/** 380 - 431mhz */
#define UHF_380_MIN             380000000
#define UHF_380_MAX             431000000

/** 431 - 450mhz */
#define UHF_1_MIN               431000000
#define UHF_1_MAX               470000000

/** 450 - 470mhz */
#define UHF_2_MIN               450000000
#define UHF_2_MAX               470000000

/** 470 - 520mhz (T-band) */
#define UHF_T_MIN               470000000
#define UHF_T_MAX               520000000

/** 842 - 900mhz */
#define UHF_800_MIN             842000000
#define UHF_800_MAX             900000000

/** 900 - 950mhz */
#define UHF_900_MIN             900000000
#define UHF_900_MAX             950000000

#if defined(ENABLE_ADF7021)

#define ADF_BIT_READ(value, bit) (((value) >> (bit)) & 0x01)

#define ADF7021_EVEN_BIT        false

#define ADF7021_DISC_BW_MAX     660
#define ADF7021_POST_BW_MAX     1023

/*
    - Most of the registers values are obteined from ADI eval software:
        http://www.analog.com/en/products/rf-microwave/integrated-transceivers-transmitters-receivers/low-power-rf-transceivers/adf7021.html
    - or ADF7021 datasheet formulas:
        www.analog.com/media/en/technical-documentation/data-sheets/ADF7021.pdf
*/

/** Test modes */

// Enable SWD pin to access the demodulator output signal:
// (See application note AN-852 and ADF7021 datasheet, page 60)
// #define TEST_DAC

// Transmit the carrier frequency:
// #define TEST_TX

/** Support for 14.7456 MHz TCXO (modified RF7021SE boards) */
#if defined(ADF7021_14_7456)

// R = 2, DEMOD_CLK = 7.3728 MHz
// R = 3, DEMOD_CLK = 4.9152 MHz
// R = 4, DEMOD_CLK = 3.6864 MHz
#define ADF7021_PFD             3686400.0

/*
** VCO/Oscillator (Register 1)
*/
#define ADF7021_REG1_VHF        0x02175041  /** 136 - 174mhz */
#define ADF7021_REG1_VHF_220    0x021B5041  /** 219 - 225mhz */
#if defined(FORCE_UHF_INTERAL_L)
#define ADF7021_REG1_UHF_380    0x001B5041  /** 380 - 431mhz */
#else
#define ADF7021_REG1_UHF_380    0x021B5041  /** 380 - 431mhz */ // this could be problematic due to
                                                                // the external VCO control
#endif // FORCE_UHF_INTERAL_L
#define ADF7021_REG1_UHF_1      0x00575041  /** 431 - 450mhz */
#define ADF7021_REG1_UHF_2      0x01D75041  /** 450 - 470mhz */
#if defined(FORCE_UHF_INTERAL_L)
#define ADF7021_REG1_UHF_T      0x00235041  /** 470 - 520mhz */
#else
#define ADF7021_REG1_UHF_T      0x02235041  /** 470 - 520mhz */ // this could be problematic due to
                                                                // the external VCO control
#endif // FORCE_UHF_INTERAL_L
#define ADF7021_REG1_800        0x00535041  /** 842 - 900mhz */
#define ADF7021_REG1_900        0x01D35041  /** 900 - 950mhz */

/*
** Transmit Modulation (Register 2)
*/
#define ADF7021_DEV_DEFAULT     43U
#define ADF7021_DEV_DMR         23U
#if defined(P25_ALTERNATE_DEV_LEVEL)
#define ADF7021_DEV_P25         24U
#else
#define ADF7021_DEV_P25         22U
#endif
#define ADF7021_DEV_NXDN        13U

/*
** Demodulator Setup (Register 4)
*/
// Discriminator bandwith, demodulator
// Bug in ADI evaluation software, use datasheet formula (4FSK)
#define ADF7021_DISC_BW_DEFAULT 522U // K=85
#define ADF7021_DISC_BW_DMR     393U // K=32
#define ADF7021_DISC_BW_P25     393U // K=32
#define ADF7021_DISC_BW_NXDN    295U // K=32

// Post demodulator bandwith
#define ADF7021_POST_BW_DEFAULT 10U
#define ADF7021_POST_BW_DMR     80U
#define ADF7021_POST_BW_P25     6U
#define ADF7021_POST_BW_NXDN    7U

/*
** IF Coarse Cal Setup (Register 5)
*/
#define ADF7021_REG5            0x000024F5

/*
** IF Fine Cal Setup (Register 6)
*/
#define ADF7021_REG6            0x05070E16

/** Support for 12.2880 MHz TCXO */
#elif defined(ADF7021_12_2880)

// R = 2, DEMOD_CLK = 6.1440 MHz
// R = 3, DEMOD_CLK = 4.0960 MHz
// R = 4, DEMOD_CLK = 3.0720 MHz
#define ADF7021_PFD             6144000.0

/*
** VCO/Oscillator (Register 1)
*/
#define ADF7021_REG1_VHF        0x02175021  /** 136 - 174mhz */
#define ADF7021_REG1_VHF_220    0x021B5021  /** 219 - 225mhz */
#if defined(FORCE_UHF_INTERAL_L)
#define ADF7021_REG1_UHF_380    0x001B5021  /** 380 - 431mhz */
#else
#define ADF7021_REG1_UHF_380    0x021B5021  /** 380 - 431mhz */ // this could be problematic due to
                                                                // the external VCO control
#endif // FORCE_UHF_INTERAL_L
#define ADF7021_REG1_UHF_1      0x00575021  /** 431 - 450mhz */
#define ADF7021_REG1_UHF_2      0x01D75021  /** 450 - 470mhz */
#if defined(FORCE_UHF_INTERAL_L)
#define ADF7021_REG1_UHF_T      0x00235021  /** 470 - 520mhz */
#else
#define ADF7021_REG1_UHF_T      0x02235021  /** 470 - 520mhz */ // this could be problematic due to
                                                                // the external VCO control
#endif // FORCE_UHF_INTERAL_L
#define ADF7021_REG1_800        0x00535021  /** 842 - 900mhz */
#define ADF7021_REG1_900        0x01D35021  /** 900 - 950mhz */

/*
** Transmit Modulation (Register 2)
*/
#define ADF7021_DEV_DEFAULT     26U
#define ADF7021_DEV_DMR         14U
#if defined(P25_ALTERNATE_DEV_LEVEL)
#define ADF7021_DEV_P25         15U
#else
#define ADF7021_DEV_P25         13U
#endif
#define ADF7021_DEV_NXDN        8U

/*
** Demodulator Setup (Register 4)
*/
// Discriminator bandwith, demodulator
// Bug in ADI evaluation software, use datasheet formula (4FSK)
#define ADF7021_DISC_BW_DEFAULT 522U // K=85
#define ADF7021_DISC_BW_DMR     491U // K=32
#define ADF7021_DISC_BW_P25     491U // K=32
#define ADF7021_DISC_BW_NXDN    246U // K=32

// Post demodulator bandwith
#define ADF7021_POST_BW_DEFAULT 10U
#define ADF7021_POST_BW_DMR     80U
#define ADF7021_POST_BW_P25     6U
#define ADF7021_POST_BW_NXDN    8U

/*
** IF Coarse Cal Setup (Register 5)
*/
#define ADF7021_REG5            0x00001ED5

/*
** IF Fine Cal Setup (Register 6)
*/
#define ADF7021_REG6            0x0505EBB6

#endif // ADF7021_12_2880

/*
** N Register (Register 0)
*/
#define ADF7021_REG0_ADDR       0b0000

/*
** Transmit Modulation (Register 2)
*/
#define ADF7021_REG2_ADDR       0b0010

#define ADF7021_REG2_MOD_GMSK   0b001
#define ADF7021_REG2_MOD_4FSK   0b011
#define ADF7021_REG2_MOD_4FSKRC 0b111

#define ADF7021_REG2_PA_DEF     0b110001

#define ADF7021_REG2_INV_NORM   0b00
#define ADF7021_REG2_INV_CLK    0b01
#define ADF7021_REG2_INV_DATA   0b10
#define ADF7021_REG2_INV_CLKDAT 0b11

#define ADF7021_REG2_RC_5       0b0
#define ADF7021_REG2_RC_7       0b1

/*
** Tx/Rx Clock (Register 3)
*/
#define ADF7021_REG3_ADDR       0b0011

// Baseband Offset Clock Divider
#define ADF7021_REG3_BBOS_DIV_4 0b00
#define ADF7021_REG3_BBOS_DIV_8 0b01
#define ADF7021_REG3_BBOS_DIV_16 0b10
#define ADF7021_REG3_BBOS_DIV_32 0b11

/*
** Demodulator Setup (Register 4)
*/
#define ADF7021_REG4_ADDR       0b0100

#define ADF7021_REG4_MODE_GMSK  0b001
#define ADF7021_REG4_MODE_4FSK  0b011

#define ADF7021_REG4_CROSS_PROD 0b0
#define ADF7021_REG4_DOT_PROD   0b1

#define ADF7021_REG4_INV_NORM   0b00
#define ADF7021_REG4_INV_CLK    0b01
#define ADF7021_REG4_INV_DATA   0b10
#define ADF7021_REG4_INV_CLKDAT 0b11

#define ADF7021_REG4_IF_125K    0b00
#define ADF7021_REG4_IF_1875K   0b01
#define ADF7021_REG4_IF_25K     0b10

/*
** AFC (Register 10)
*/
#define ADF7021_REG10_ADDR      0b1010

#define ADF7021_REG10_AFC_DISABLE 0b0   
#define ADF7021_REG10_AFC_ENABLE 0b1

/*
** 3FSK/4FSK Demod (Register 13)
*/
#define ADF7021_REG13_ADDR     0b1101

// Slicer threshold for 4FSK demodulator
#define ADF7021_SLICER_TH_DEFAULT 0U

#if defined(ADF7021_N_VER)

#define ADF7021_SLICER_TH_DMR   51U
#define ADF7021_SLICER_TH_P25   43U
#define ADF7021_SLICER_TH_NXDN  26U

#else

#define ADF7021_SLICER_TH_DMR   57U
#define ADF7021_SLICER_TH_P25   47U
#define ADF7021_SLICER_TH_NXDN  26U


#endif // ADF7021_N_VER

#endif // ENABLE_ADF7021
#endif // __ADF7021_H__
