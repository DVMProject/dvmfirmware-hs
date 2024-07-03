// SPDX-License-Identifier: GPL-2.0-only
/*
 * Digital Voice Modem - Hotspot Firmware
 * GPLv2 Open Source. Use is subject to license terms.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 *  Copyright (C) 2015,2016 Jonathan Naylor, G4KLX
 *  Serial FIFO Control Copyright (C) 2015 by James McLaughlin, KI6ZUM
 *
 */
/**
 * @file BitBuffer.h
 * @ingroup hotspot_fw
 * @file BitBuffer.cpp
 * @ingroup hotspot_fw
 */
#if !defined(__BIT_RB_H__)
#define __BIT_RB_H__

#if defined(STM32F10X_MD)
#include "stm32f10x.h"
#elif defined(STM32F4XX)
#include "stm32f4xx.h"
#include <cstddef>
#endif

#include "Defines.h"

// ---------------------------------------------------------------------------
//  Class Declaration
// ---------------------------------------------------------------------------

/**
 * @brief Implements a circular ring buffer for bit data.
 * @ingroup hotspot_fw
 */
class DSP_FW_API BitBuffer {
public:
    /**
     * @brief Initializes a new instance of the BitBuffer class.
     * @param length Length of buffer.
     */
    BitBuffer(uint16_t length);

    /**
     * @brief Helper to get how much space the ring buffer has for samples.
     * @returns uint16_t Amount of space remaining for data.
     */
    uint16_t getSpace() const;

    /**
     * @brief 
     * @returns uint16_t 
     */
    uint16_t getData() const;

    /**
     * @brief 
     * @param bit 
     * @param control
     * @returns bool 
     */
    bool put(uint8_t bit, uint8_t control);

    /**
     * @brief 
     * @param[out] bit 
     * @param[out] control 
     * @returns bool 
     */
    bool get(uint8_t& bit, uint8_t& control);

    /**
     * @brief 
     * @returns bool 
     */
    bool hasOverflowed();

private:
    uint16_t m_length;
    volatile uint8_t* m_bits;
    volatile uint8_t* m_control;

    volatile uint16_t m_head;
    volatile uint16_t m_tail;

    volatile bool m_full;

    bool m_overflow;
};

#endif // __BIT_RB_H__
