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
#include "BitBuffer.h"

// ---------------------------------------------------------------------------
//  Public Class Members
// ---------------------------------------------------------------------------

/// <summary>
/// Initializes a new instance of the BitBuffer class.
/// </summary>
BitBuffer::BitBuffer(uint16_t length) :
    m_length(length),
    m_bits(NULL),
    m_control(NULL),
    m_head(0U),
    m_tail(0U),
    m_full(false),
    m_overflow(false)
{
    m_bits = new uint8_t[length / 8U];
    m_control = new uint8_t[length / 8U];
}

/// <summary>
/// Helper to get how much space the ring buffer has for samples.
/// </summary>
/// <returns></returns>
uint16_t BitBuffer::getSpace() const
{
    uint16_t n = 0U;

    if (m_tail == m_head)
        n = m_full ? 0U : m_length;
    else if (m_tail < m_head)
        n = m_length - m_head + m_tail;
    else
        n = m_tail - m_head;

    if (n > m_length)
        n = 0U;

    return n;
}

/// <summary>
///
/// </summary>
/// <returns></returns>
uint16_t BitBuffer::getData() const
{
    if (m_tail == m_head)
        return m_full ? m_length : 0U;
    else if (m_tail < m_head)
        return m_head - m_tail;
    else
        return m_length - m_tail + m_head;
}

/// <summary>
///
/// </summary>
/// <param name="c"></param>
/// <returns></returns>
bool BitBuffer::put(uint8_t bit, uint8_t control)
{
    if (m_full) {
        m_overflow = true;
        return false;
    }

    _WRITE_BIT(m_bits, m_head, bit);
    _WRITE_BIT(m_control, m_head, control);

    m_head++;
    if (m_head >= m_length)
        m_head = 0U;

    if (m_head == m_tail)
        m_full = true;

    return true;
}

/// <summary>
///
/// </summary>
/// <returns></returns>
bool BitBuffer::get(uint8_t& bit, uint8_t& control)
{
    if (m_head == m_tail && !m_full)
        return false;

    bit = _READ_BIT(m_bits, m_tail);
    control = _READ_BIT(m_control, m_tail);

    m_full = false;

    m_tail++;
    if (m_tail >= m_length)
        m_tail = 0U;

    return true;
}

/// <summary>
/// 
/// </summary>
/// <returns></returns>
bool BitBuffer::hasOverflowed()
{
    bool overflow = m_overflow;
    m_overflow = false;

    return overflow;
}
