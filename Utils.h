// SPDX-License-Identifier: GPL-2.0-only
/*
 * Digital Voice Modem - Hotspot Firmware
 * GPLv2 Open Source. Use is subject to license terms.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 *  Copyright (C) 2015,2016,2020 Jonathan Naylor, G4KLX
 *  Copyright (C) 2017 Andy Uribe, CA6JAU
 *
 */
/**
 * @file Utils.h
 * @ingroup hotspot_fw
 * @file Utils.cpp
 * @ingroup hotspot_fw
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

/**
 * @brief Returns the count of bits in the passed 8 byte value.
 * @param bits uint8_t to count bits for.
 * @returns uint8_t Count of bits in passed value.
 */
DSP_FW_API uint8_t countBits8(uint8_t bits);
/**
 * @brief Returns the count of bits in the passed 16 byte value.
 * @param bits uint16_t to count bits for.
 * @returns uint8_t Count of bits in passed value.
 */
DSP_FW_API uint8_t countBits16(uint16_t bits);
/**
 * @brief Returns the count of bits in the passed 32 byte value.
 * @param bits uint32_t to count bits for.
 * @returns uint8_t Count of bits in passed value.
 */
DSP_FW_API uint8_t countBits32(uint32_t bits);
/**
 * @brief Returns the count of bits in the passed 64 byte value.
 * @param bits ulong64_t to count bits for.
 * @returns uint8_t Count of bits in passed value.
 */
DSP_FW_API uint8_t countBits64(ulong64_t bits);

#endif // __UTILS_H__
