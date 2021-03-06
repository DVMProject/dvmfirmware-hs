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
*   Copyright (C) 2015,2016,2017 by Jonathan Naylor G4KLX
*   Copyright (C) 2017,2018,2019,2020 by Andy Uribe CA6JAU
*   Copyright (C) 2021 Bryan Biedenkapp N2PLL
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
#if !defined(__DEFINES_H__)
#define __DEFINES_H__

#include <stdint.h>

// ---------------------------------------------------------------------------
//  Types
// ---------------------------------------------------------------------------

#ifndef _INT8_T_DECLARED
#ifndef __INT8_TYPE__
typedef signed char         int8_t;
#endif // __INT8_TYPE__
#endif // _INT8_T_DECLARED
#ifndef _INT16_T_DECLARED
#ifndef __INT16_TYPE__
typedef short               int16_t;
#endif // __INT16_TYPE__
#endif // _INT16_T_DECLARED
#ifndef _INT32_T_DECLARED
#ifndef __INT32_TYPE__
typedef int                 int32_t;
#endif // __INT32_TYPE__
#endif // _INT32_T_DECLARED
#ifndef _INT64_T_DECLARED
#ifndef __INT64_TYPE__
typedef long long           int64_t;
#endif // __INT64_TYPE__
#endif // _INT64_T_DECLARED
#ifndef _UINT8_T_DECLARED
#ifndef __UINT8_TYPE__
typedef unsigned char       uint8_t;
#endif // __UINT8_TYPE__
#endif // _UINT8_T_DECLARED
#ifndef _UINT16_T_DECLARED
#ifndef __UINT16_TYPE__
typedef unsigned short      uint16_t;
#endif // __UINT16_TYPE__
#endif // _UINT16_T_DECLARED
#ifndef _UINT32_T_DECLARED
#ifndef __UINT32_TYPE__
typedef unsigned int        uint32_t;
#endif // __UINT32_TYPE__
#endif // _UINT32_T_DECLARED
#ifndef _UINT64_T_DECLARED
#ifndef __UINT64_TYPE__
typedef unsigned long long  uint64_t;
#endif // __UINT64_TYPE__
#endif // _UINT64_T_DECLARED

#ifndef __LONG64_TYPE__
typedef long long           long64_t;
#endif // __LONG64_TYPE__
#ifndef __ULONG64_TYPE__
typedef unsigned long long  ulong64_t;
#endif // __ULONG64_TYPE__

// ---------------------------------------------------------------------------
//  Constants
// ---------------------------------------------------------------------------

#define DSP_FW_API 

// Allow the DMR protocol
#define ENABLE_DMR

// Allow the P25 protocol
#define ENABLE_P25

// Enable ADF7021 support
#define ENABLE_ADF7021

// Bidirectional Data pin (Enable Standard TX/RX Data Interface of ADF7021)
#define BIDIR_DATA_PIN

// TCXO of the ADF7021
// For 14.7456 MHz:
#define ADF7021_14_7456
// For 12.2880 MHz:
// #define ADF7021_12_2880

// Enable full duplex support with dual ADF7021 (valid for homebrew hotspots only)
// #define DUPLEX

// Support for ADF7021-N version: 
// #define ADF7021_N_VER

// Enable AFC support for DMR and P25 (experimental)
// #define ADF7021_ENABLE_4FSK_AFC

// Configure AFC with positive initial frequency offset
// #define ADF7021_AFC_POS

// Force the internal L (internal VCO inductor) for UHF 380 and T-band
#define FORCE_UHF_INTERAL_L

// Pass RSSI information to the host
// #define SEND_RSSI_DATA

// Enable for RPi 3B+, USB mode
#if defined(STM32_USB_HOST)
#define LONG_USB_RESET
#endif

const uint8_t BIT_MASK_TABLE[] = { 0x80U, 0x40U, 0x20U, 0x10U, 0x08U, 0x04U, 0x02U, 0x01U };

#define CPU_TYPE_STM32 0x02U

// ---------------------------------------------------------------------------
//  Macros
// ---------------------------------------------------------------------------

#define _WRITE_BIT(p, i, b) p[(i) >> 3] = (b) ? (p[(i) >> 3] | BIT_MASK_TABLE[(i) & 7]) : (p[(i)>>3] & ~BIT_MASK_TABLE[(i) & 7])
#define _READ_BIT(p, i)     ((p[(i) >> 3] & BIT_MASK_TABLE[(i) & 7]) >> (7 - ((i) & 7)))

#endif // __DEFINES_H__
