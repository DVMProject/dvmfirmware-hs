// SPDX-License-Identifier: GPL-2.0-only
/*
 * Digital Voice Modem - Hotspot Firmware
 * GPLv2 Open Source. Use is subject to license terms.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 *  Copyright (C) 2009-2017 Jonathan Naylor, G4KLX
 *  Copyright (C) 2017,2018 Andy Uribe, CA6JAU
 *
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

/* Initializes a new instance of the DMRIdleRX class. */

DMRIdleRX::DMRIdleRX() :
    m_bitBuffer(0U),
    m_buffer(),
    m_dataPtr(0U),
    m_endPtr(NOENDPTR),
    m_colorCode(0U)
{
    /* stub */
}

/* Helper to reset data values to defaults. */

void DMRIdleRX::reset()
{
    m_dataPtr = 0U;
    m_endPtr = NOENDPTR;
}

/* Sample DMR bits from the air interface. */

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

        DEBUG3("DMRIdleRx::databit() dataPtr/endPtr", m_dataPtr, m_endPtr);
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

/* Sets the DMR color code. */

void DMRIdleRX::setColorCode(uint8_t colorCode)
{
    m_colorCode = colorCode;
}

// ---------------------------------------------------------------------------
//  Private Class Members
// ---------------------------------------------------------------------------

/* */

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
