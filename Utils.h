// SPDX-License-Identifier: GPL-2.0-only
/**
* Digital Voice Modem - Hotspot Firmware
* GPLv2 Open Source. Use is subject to license terms.
* DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
*
* @package DVM / Hotspot Firmware
* @derivedfrom MMDVM_HS (https://github.com/g4klx/MMDVM_HS)
* @license GPLv2 License (https://opensource.org/licenses/GPL-2.0)
*
*   Copyright (C) 2015,2016,2020 Jonathan Naylor, G4KLX
*   Copyright (C) 2017 Andy Uribe, CA6JAU
*
*/
#if !defined(__UTILS_H__)
#define __UTILS_H__

#if defined(STM32F10X_MD)
#include "stm32f10x.h"
#elif defined(STM32F4XX)
#include "stm32f4xx.h"
#elif defined(STM32F7XX)
#include "stm32f7xx.h"
#endif

#include "Defines.h"

// ---------------------------------------------------------------------------
//  Global Functions
// ---------------------------------------------------------------------------

DSP_FW_API uint8_t countBits8(uint8_t bits);
DSP_FW_API uint8_t countBits16(uint16_t bits);
DSP_FW_API uint8_t countBits32(uint32_t bits);
DSP_FW_API uint8_t countBits64(ulong64_t bits);

#endif // __UTILS_H__
