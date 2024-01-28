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
*
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
//      Implements a circular buffer for bit data.
// ---------------------------------------------------------------------------

class DSP_FW_API BitBuffer {
public:
    /// <summary>Initializes a new instance of the BitBuffer class.</summary>
    BitBuffer(uint16_t length);

    /// <summary>Helper to get how much space the ring buffer has for samples.</summary>
    uint16_t getSpace() const;

    /// <summary></summary>
    uint16_t getData() const;

    /// <summary></summary>
    bool put(uint8_t bit, uint8_t control);

    /// <summary></summary>
    bool get(uint8_t& bit, uint8_t& control);

    /// <summary></summary>
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
