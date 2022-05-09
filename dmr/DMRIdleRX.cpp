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
*   Copyright (C) 2009-2017 by Jonathan Naylor G4KLX
*   Copyright (C) 2017,2018 by Andy Uribe CA6JAU
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
#include "Globals.h"
#include "dmr/DMRIdleRX.h"
#include "dmr/DMRSlotType.h"
#include "Utils.h"

using namespace dmr;

#if defined(DUPLEX)

// ---------------------------------------------------------------------------
//  Constants
// ---------------------------------------------------------------------------

const uint8_t MAX_SYNC_BYTES_ERRS = 2U;

const uint16_t NOENDPTR = 9999U;

const uint8_t CONTROL_IDLE = 0x80U;
const uint8_t CONTROL_DATA = 0x40U;

// ---------------------------------------------------------------------------
//  Public Class Members
// ---------------------------------------------------------------------------

/// <summary>
/// Initializes a new instance of the DMRIdleRX class.
/// </summary>
DMRIdleRX::DMRIdleRX() :
    m_bitBuffer(0U),
    m_buffer(),
    m_dataPtr(0U),
    m_endPtr(NOENDPTR),
    m_colorCode(0U)
{
    /* stub */
}

/// <summary>
/// Helper to reset data values to defaults.
/// </summary>
void DMRIdleRX::reset()
{
    m_dataPtr = 0U;
    m_endPtr = NOENDPTR;
}

/// <summary>
/// Sample DMR bits from the air interface.
/// </summary>
/// <param name="bit"></param>
void DMRIdleRX::databit(bool bit)
{
    _WRITE_BIT(m_buffer, m_dataPtr, bit);

    m_bitBuffer <<= 1;
    if (bit)
        m_bitBuffer |= 0x01U;

    if (countBits64((m_bitBuffer & DMR_SYNC_BITS_MASK) ^ DMR_MS_DATA_SYNC_BITS) <= MAX_SYNC_BYTES_ERRS) {
        m_endPtr = m_dataPtr + DMR_SLOT_TYPE_LENGTH_BITS / 2U + DMR_INFO_LENGTH_BITS / 2U;
        if (m_endPtr >= DMR_IDLE_LENGTH_BITS)
            m_endPtr -= DMR_IDLE_LENGTH_BITS;

        DEBUG3("DMRIdleRx: databit(): dataPtr/endPtr", m_dataPtr, m_endPtr);
    }

    if (m_dataPtr == m_endPtr) {
        uint16_t ptr = m_endPtr + DMR_IDLE_LENGTH_BITS - DMR_FRAME_LENGTH_BITS + 1;
        if (ptr >= DMR_IDLE_LENGTH_BITS)
            ptr -= DMR_IDLE_LENGTH_BITS;

        uint8_t frame[DMR_FRAME_LENGTH_BYTES + 1U];
        bitsToBytes(ptr, DMR_FRAME_LENGTH_BYTES, frame + 1U);

        uint8_t colorCode;
        uint8_t dataType;
        DMRSlotType slotType;
        slotType.decode(frame + 1U, colorCode, dataType);

        if (colorCode == m_colorCode && dataType == DT_CSBK) {
            frame[0U] = CONTROL_IDLE | CONTROL_DATA | DT_CSBK;
            serial.writeDMRData(false, frame, DMR_FRAME_LENGTH_BYTES + 1U);
        }

        m_endPtr = NOENDPTR;
    }

    m_dataPtr++;
    if (m_dataPtr >= DMR_IDLE_LENGTH_BITS)
        m_dataPtr = 0U;
}

/// <summary>
/// Sets the DMR color code.
/// </summary>
/// <param name="colorCode">Color code.</param>
void DMRIdleRX::setColorCode(uint8_t colorCode)
{
    m_colorCode = colorCode;
}

// ---------------------------------------------------------------------------
//  Private Class Members
// ---------------------------------------------------------------------------

/// <summary>
/// 
/// </summary>
/// <param name="start"></param>
/// <param name="count"></param>
/// <param name="buffer"></param>
void DMRIdleRX::bitsToBytes(uint16_t start, uint8_t count, uint8_t* buffer)
{
  for (uint8_t i = 0U; i < count; i++) {
    buffer[i]  = 0U;
    buffer[i] |= _READ_BIT(m_buffer, start) << 7;
    start++;
    if (start >= DMR_IDLE_LENGTH_BITS)
      start -= DMR_IDLE_LENGTH_BITS;

    buffer[i] |= _READ_BIT(m_buffer, start) << 6;
    start++;
    if (start >= DMR_IDLE_LENGTH_BITS)
      start -= DMR_IDLE_LENGTH_BITS;

    buffer[i] |= _READ_BIT(m_buffer, start) << 5;
    start++;
    if (start >= DMR_IDLE_LENGTH_BITS)
      start -= DMR_IDLE_LENGTH_BITS;

    buffer[i] |= _READ_BIT(m_buffer, start) << 4;
    start++;
    if (start >= DMR_IDLE_LENGTH_BITS)
      start -= DMR_IDLE_LENGTH_BITS;

    buffer[i] |= _READ_BIT(m_buffer, start) << 3;
    start++;
    if (start >= DMR_IDLE_LENGTH_BITS)
      start -= DMR_IDLE_LENGTH_BITS;

    buffer[i] |= _READ_BIT(m_buffer, start) << 2;
    start++;
    if (start >= DMR_IDLE_LENGTH_BITS)
      start -= DMR_IDLE_LENGTH_BITS;

    buffer[i] |= _READ_BIT(m_buffer, start) << 1;
    start++;
    if (start >= DMR_IDLE_LENGTH_BITS)
      start -= DMR_IDLE_LENGTH_BITS;

    buffer[i] |= _READ_BIT(m_buffer, start) << 0;
    start++;
    if (start >= DMR_IDLE_LENGTH_BITS)
      start -= DMR_IDLE_LENGTH_BITS;
  }
}

#endif // DUPLEX
