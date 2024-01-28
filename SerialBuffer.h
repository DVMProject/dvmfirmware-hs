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
*   Copyright (C) 2015,2016 Jonathan Naylor, G4KLX
*   Serial FIFO Control Copyright (C) 2015 by James McLaughlin, KI6ZUM
*   Copyright (C) 2022 Bryan Biedenkapp, N2PLL
*
*/
#if !defined(__SERIAL_RB_H__)
#define __SERIAL_RB_H__

#if defined(STM32F10X_MD)
#include "stm32f10x.h"
#elif defined(STM32F4XX)
#include "stm32f4xx.h"
#include <cstddef>
#endif

#include "Defines.h"

// ---------------------------------------------------------------------------
//  Constants
// ---------------------------------------------------------------------------

const uint16_t SERIAL_RINGBUFFER_SIZE = 396U;

// ---------------------------------------------------------------------------
//  Class Declaration
//      Implements a circular buffer for serial data.
// ---------------------------------------------------------------------------

class DSP_FW_API SerialBuffer {
public:
    /// <summary>Initializes a new instance of the SerialBuffer class.</summary>
    SerialBuffer(uint16_t length = SERIAL_RINGBUFFER_SIZE);
    /// <summary>Finalizes a instance of the SerialBuffer class.</summary>
    ~SerialBuffer();

    /// <summary>Helper to get how much space the ring buffer has for samples.</summary>
    uint16_t getSpace() const;

    /// <summary></summary>
    uint16_t getData() const;

    /// <summary>Helper to reset data values to defaults.</summary>
    void reset();
    /// <summary>Helper to reset and reinitialize data values to defaults.</summary>
    void reinitialize(uint16_t length);

    /// <summary></summary>
    bool put(uint8_t c);

    /// <summary></summary>
    uint8_t peek() const;

    /// <summary></summary>
    uint8_t get();

private:
    uint16_t m_length;
    volatile uint8_t* m_buffer;

    volatile uint16_t m_head;
    volatile uint16_t m_tail;

    volatile bool m_full;
};

#endif // __SERIAL_RB_H__
