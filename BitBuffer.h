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
*   Copyright (C) 2015,2016 by Jonathan Naylor G4KLX
*   Serial FIFO Control Copyright (C) 2015 by James McLaughlin KI6ZUM
*
*   This library is free software; you can redistribute it and/or
*   modify it under the terms of the GNU Library General Public
*   License as published by the Free Software Foundation; either
*   version 2 of the License, or (at your option) any later version.
*
*   This library is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*   Library General Public License for more details.
*
*   You should have received a copy of the GNU Library General Public
*   License along with this library; if not, write to the
*   Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
*   Boston, MA  02110-1301, USA.
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
