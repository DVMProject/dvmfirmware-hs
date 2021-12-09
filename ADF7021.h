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

#if defined(ADF7021_DISABLE_RC_4FSK)
#define ADF7021_EVEN_BIT        true
#else
#define ADF7021_EVEN_BIT        false
#endif // ADF7021_DISABLE_RC_4FSK

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

// R = 4
// DEMOD_CLK = 2.4576 MHz (DEFAULT
// DEMOD_CLK = 4.9152 MHz (DMR, P25)
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

#if defined(ENABLE_P25_WIDE)
#define ADF7021_DEV_P25         32U
#else
#define ADF7021_DEV_P25         22U
#endif // ENABLE_P25_WIDE

/*
** Tx/Rx Clock (Register 3)
*/
#define ADF7021_REG3_DEFAULT    0x2A4C4193
#if defined(TEST_DAC)
#define ADF7021_REG3_DMR        0x2A4C04D3
#define ADF7021_REG3_P25        0x2A4C04D3
#else
#define ADF7021_REG3_DMR        0x2A4C80D3
#define ADF7021_REG3_P25        0x2A4C80D3
#endif // TEST_DAC

/*
** Demodulator Setup (Register 4)
*/
// Discriminator bandwith, demodulator
// Bug in ADI evaluation software, use datasheet formula (4FSK)
#define ADF7021_DISC_BW_DEFAULT 522U // K=85
#define ADF7021_DISC_BW_DMR     393U // K=32
#define ADF7021_DISC_BW_P25     394U // K=32

// Post demodulator bandwith
#define ADF7021_POST_BW_DEFAULT 10U
#define ADF7021_POST_BW_DMR     80U
#define ADF7021_POST_BW_P25     6U

/*
** IF Coarse Cal Setup (Register 5)
*/
#define ADF7021_REG5            0x000024F5

/*
** IF Fine Cal Setup (Register 6)
*/
#define ADF7021_REG6            0x05070E16

/*
** AFC (Register 10)
*/
#define ADF7021_REG10_DEFAULT   0x0C96473A

#if defined(ADF7021_ENABLE_4FSK_AFC)

#define ADF7021_REG10_DMR       0x01FE473A
#define ADF7021_REG10_P25       0x01FE473A

#if defined(ADF7021_AFC_POS)

#define AFC_OFFSET_DMR          -250
#define AFC_OFFSET_P25          -250

#else

#define AFC_OFFSET_DMR          250
#define AFC_OFFSET_P25          250

#endif // ADF7021_AFC_POS

#else

#define ADF7021_REG10_DMR       0x049E472A
#define ADF7021_REG10_P25       0x049E472A
#define AFC_OFFSET_DMR          0
#define AFC_OFFSET_P25          0

#endif // ADF7021_ENABLE_4FSK_AFC

/** Support for 12.2880 MHz TCXO */
#elif defined(ADF7021_12_2880)

// R = 2
// DEMOD_CLK = 2.4576 MHz (DEFAULT)
// DEMOD_CLK = 6.1440 MHz (DMR, P25)
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

#if defined(ENABLE_P25_WIDE)
#define ADF7021_DEV_P25         19U
#else
#define ADF7021_DEV_P25         13U
#endif // ENABLE_P25_WIDE

/*
** Tx/Rx Clock (Register 3)
*/
#define ADF7021_REG3_DEFAULT    0x29EC4153
#if defined(TEST_DAC)
#define ADF7021_REG3_DMR        0x29EC0493
#define ADF7021_REG3_P25        0x29EC0493
#else
#define ADF7021_REG3_DMR        0x29ECA093
#define ADF7021_REG3_P25        0x29ECA093
#endif // TEST_DAC

/*
** Demodulator Setup (Register 4)
*/
// Discriminator bandwith, demodulator
// Bug in ADI evaluation software, use datasheet formula (4FSK)
#define ADF7021_DISC_BW_DEFAULT 522U // K=85
#define ADF7021_DISC_BW_DMR     491U // K=32
#define ADF7021_DISC_BW_P25     493U // K=32

// Post demodulator bandwith
#define ADF7021_POST_BW_DEFAULT 10U
#define ADF7021_POST_BW_DMR     80U
#define ADF7021_POST_BW_P25     6U

/*
** IF Coarse Cal Setup (Register 5)
*/
#define ADF7021_REG5            0x00001ED5

/*
** IF Fine Cal Setup (Register 6)
*/
#define ADF7021_REG6            0x0505EBB6

/*
** AFC (Register 10)
*/
#define ADF7021_REG10_DEFAULT   0x0C96557A

#if defined(ADF7021_ENABLE_4FSK_AFC)

#define ADF7021_REG10_DMR       0x01FE557A
#define ADF7021_REG10_P25       0x01FE557A

#if defined(ADF7021_AFC_POS)

#define AFC_OFFSET_DMR          -250
#define AFC_OFFSET_P25          -250

#else

#define AFC_OFFSET_DMR          250
#define AFC_OFFSET_P25          250

#endif // ADF7021_AFC_POS

#else

#define ADF7021_REG10_DMR       0x049E556A
#define ADF7021_REG10_P25       0x049E556A
#define AFC_OFFSET_DMR          0
#define AFC_OFFSET_P25          0

#endif // ADF7021_ENABLE_4FSK_AFC

#endif // ADF7021_12_2880

/*
** 3FSK/4FSK Demod (Register 13)
*/
// Slicer threshold for 4FSK demodulator
#define ADF7021_SLICER_TH_DEFAULT 0U

#if defined(ADF7021_N_VER)

#define ADF7021_SLICER_TH_DMR    51U
#define ADF7021_SLICER_TH_P25    43U

#else

#define ADF7021_SLICER_TH_DMR    57U
#define ADF7021_SLICER_TH_P25    47U

#endif // ADF7021_N_VER

#endif // ENABLE_ADF7021
#endif // __ADF7021_H__
