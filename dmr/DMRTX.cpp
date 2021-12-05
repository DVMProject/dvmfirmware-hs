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
*   Copyright (C) 2016 by Colin Durbridge G4EML
*   Copyright (C) 2017 by Andy Uribe CA6JAU
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
#include "dmr/DMRSlotType.h"

using namespace dmr;

// ---------------------------------------------------------------------------
//  Constants
// ---------------------------------------------------------------------------

// The PR FILL and BS Data Sync pattern.
const uint8_t IDLE_DATA[] = {
    0x53U, 0xC2U, 0x5EU, 0xABU, 0xA8U, 0x67U, 0x1DU, 0xC7U, 0x38U, 0x3BU, 0xD9U,
    0x36U, 0x00U, 0x0DU, 0xFFU, 0x57U, 0xD7U, 0x5DU, 0xF5U, 0xD0U, 0x03U, 0xF6U,
    0xE4U, 0x65U, 0x17U, 0x1BU, 0x48U, 0xCAU, 0x6DU, 0x4FU, 0xC6U, 0x10U, 0xB4U
};

const uint8_t CACH_INTERLEAVE[] = {
    1U,  2U,  3U,  5U,  6U,  7U,  9U, 10U, 11U, 13U, 15U, 16U, 17U, 19U, 20U, 21U, 23U,
    25U, 26U, 27U, 29U, 30U, 31U, 33U, 34U, 35U, 37U, 39U, 40U, 41U, 43U, 44U, 45U, 47U,
    49U, 50U, 51U, 53U, 54U, 55U, 57U, 58U, 59U, 61U, 63U, 64U, 65U, 67U, 68U, 69U, 71U,
    73U, 74U, 75U, 77U, 78U, 79U, 81U, 82U, 83U, 85U, 87U, 88U, 89U, 91U, 92U, 93U, 95U
};

const uint8_t EMPTY_SHORT_LC[] = {
    0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U
};

const uint32_t STARTUP_COUNT = 20U;
const uint32_t ABORT_COUNT = 6U;

// ---------------------------------------------------------------------------
//  Public Class Members
// ---------------------------------------------------------------------------
/// <summary>
/// Initializes a new instance of the DMRTX class.
/// </summary>
DMRTX::DMRTX() :
    m_fifo(),
    m_state(DMRTXSTATE_IDLE),
    m_idle(),
    m_cachPtr(0U),
    m_shortLC(),
    m_newShortLC(),
    m_markBuffer(),
    m_poBuffer(),
    m_poLen(0U),
    m_poPtr(0U),
    m_frameCount(0U),
    m_abortCount(),
    m_abort(),
    m_control_old(0U)
{
    ::memcpy(m_newShortLC, EMPTY_SHORT_LC, 12U);
    ::memcpy(m_shortLC, EMPTY_SHORT_LC, 12U);

    m_abort[0U] = false;
    m_abort[1U] = false;

    m_abortCount[0U] = 0U;
    m_abortCount[1U] = 0U;
}

/// <summary>
/// Process local buffer and transmit on the air interface.
/// </summary>
void DMRTX::process()
{
    if (m_state == DMRTXSTATE_IDLE)
        return;

    if (m_poLen == 0U) {
        switch (m_state) {
        case DMRTXSTATE_SLOT1:
            createData(0U);
            m_state = DMRTXSTATE_CACH2;
            break;

        case DMRTXSTATE_CACH2:
            createCACH(1U, 0U);
            m_state = DMRTXSTATE_SLOT2;
            break;

        case DMRTXSTATE_SLOT2:
            createData(1U);
            m_state = DMRTXSTATE_CACH1;
            break;

        case DMRTXSTATE_CAL:
            createCal();
            break;

        default:
            createCACH(0U, 1U);
            m_state = DMRTXSTATE_SLOT1;
            break;
        }

        DEBUG2("DMRTX: process(): poLen", m_poLen);
    }

    if (m_poLen > 0U) {
        uint16_t space = io.getSpace();

        while (space > 8U) {
            uint8_t c = m_poBuffer[m_poPtr];
            uint8_t m = m_markBuffer[m_poPtr];
            m_poPtr++;

            writeByte(c, m);

            space -= 8U;

            if (m_poPtr >= m_poLen) {
                m_poPtr = 0U;
                m_poLen = 0U;
                return;
            }
        }
    }
}

/// <summary>
/// Write slot 1 data to the local buffer.
/// </summary>
/// <param name="data"></param>
/// <param name="length"></param>
/// <returns></returns>
uint8_t DMRTX::writeData1(const uint8_t* data, uint8_t length)
{
    if (length != (DMR_FRAME_LENGTH_BYTES + 1U))
        return RSN_ILLEGAL_LENGTH;

    uint16_t space = m_fifo[0U].getSpace();
    DEBUG3("DMRTX: writeData1(): dataLength/fifoLength", length, space);
    if (space < DMR_FRAME_LENGTH_BYTES)
        return RSN_RINGBUFF_FULL;

    if (m_abort[0U]) {
        m_fifo[0U].reset();
        m_abort[0U] = false;
    }

    for (uint8_t i = 0U; i < DMR_FRAME_LENGTH_BYTES; i++)
        m_fifo[0U].put(data[i + 1U]);

    // Start the TX if it isn't already on
    if (!m_tx)
        m_state = DMRTXSTATE_SLOT1;

    return RSN_OK;
}

/// <summary>
/// Write slot 2 data to the local buffer.
/// </summary>
/// <param name="data"></param>
/// <param name="length"></param>
/// <returns></returns>
uint8_t DMRTX::writeData2(const uint8_t* data, uint8_t length)
{
    if (length != (DMR_FRAME_LENGTH_BYTES + 1U))
        return RSN_ILLEGAL_LENGTH;

    uint16_t space = m_fifo[1U].getSpace();
    DEBUG3("DMRTX: writeData2(): dataLength/fifoLength", length, space);
    if (space < DMR_FRAME_LENGTH_BYTES)
        return RSN_RINGBUFF_FULL;

    if (m_abort[1U]) {
        m_fifo[1U].reset();
        m_abort[1U] = false;
    }

    for (uint8_t i = 0U; i < DMR_FRAME_LENGTH_BYTES; i++)
        m_fifo[1U].put(data[i + 1U]);

    // Start the TX if it isn't already on
    if (!m_tx)
        m_state = DMRTXSTATE_SLOT1;

    return RSN_OK;
}

/// <summary>
/// Write short LC data to the local buffer.
/// </summary>
/// <param name="data"></param>
/// <param name="length"></param>
/// <returns></returns>
uint8_t DMRTX::writeShortLC(const uint8_t* data, uint8_t length)
{
    if (length != 9U)
        return RSN_ILLEGAL_LENGTH;

    ::memset(m_newShortLC, 0x00U, 12U);

    for (uint8_t i = 0U; i < 68U; i++) {
        bool b = _READ_BIT(data, i);
        uint8_t n = CACH_INTERLEAVE[i];
        _WRITE_BIT(m_newShortLC, n, b);
    }

    return RSN_OK;
}

/// <summary>
/// Write abort data to the local buffer.
/// </summary>
/// <param name="data"></param>
/// <param name="length"></param>
/// <returns></returns>
uint8_t DMRTX::writeAbort(const uint8_t* data, uint8_t length)
{
    if (length != 1U)
        return RSN_ILLEGAL_LENGTH;

    switch (data[0U]) {
    case 1U:
        m_abort[0U] = true;
        m_abortCount[0U] = 0U;
        return RSN_OK;

    case 2U:
        m_abort[1U] = true;
        m_abortCount[1U] = 0U;
        return RSN_OK;

    default:
        return RSN_INVALID_DMR_SLOT;
    }
}

/// <summary>
/// Helper to set the start state for Tx.
/// </summary>
/// <param name="start"></param>
void DMRTX::setStart(bool start)
{
    m_state = start ? DMRTXSTATE_SLOT1 : DMRTXSTATE_IDLE;

    m_frameCount = 0U;
    m_abortCount[0U] = 0U;
    m_abortCount[1U] = 1U;

    m_abort[0U] = false;
    m_abort[1U] = false;
}

/// <summary>
/// Helper to set the calibration state for Tx.
/// </summary>
/// <param name="start"></param>
void DMRTX::setCal(bool start)
{
    m_state = start ? DMRTXSTATE_CAL : DMRTXSTATE_IDLE;
}

/// <summary>
/// Helper to get how much space the slot 1 ring buffer has for samples.
/// </summary>
/// <returns></returns>
uint8_t DMRTX::getSpace1() const
{
    return m_fifo[0U].getSpace() / (DMR_FRAME_LENGTH_BYTES + 2U);
}

/// <summary>
/// Helper to get how much space the slot 2 ring buffer has for samples.
/// </summary>
/// <returns></returns>
uint8_t DMRTX::getSpace2() const
{
    return m_fifo[1U].getSpace() / (DMR_FRAME_LENGTH_BYTES + 2U);
}

/// <summary>
/// Sets the DMR color code.
/// </summary>
/// <param name="colorCode">Color code.</param>
void DMRTX::setColorCode(uint8_t colorCode)
{
    ::memcpy(m_idle, IDLE_DATA, DMR_FRAME_LENGTH_BYTES);

    DMRSlotType slotType;
    slotType.encode(colorCode, DT_IDLE, m_idle);
}

/// <summary>
/// Helper to reset data values to defaults for slot 1 FIFO.
/// </summary>
void DMRTX::resetFifo1()
{
    m_fifo[0U].reset();
}

/// <summary>
/// Helper to reset data values to defaults for slot 2 FIFO.
/// </summary>
void DMRTX::resetFifo2()
{
    m_fifo[1U].reset();
}

/// <summary>
///
/// </summary>
/// <returns></returns>
uint32_t DMRTX::getFrameCount()
{
    return m_frameCount;
}

// ---------------------------------------------------------------------------
//  Private Class Members
// ---------------------------------------------------------------------------
/// <summary>
///
/// </summary>
/// <param name="slotIndex"></param>
void DMRTX::createData(uint8_t slotIndex)
{
    if (m_fifo[slotIndex].getData() > 0U && m_frameCount >= STARTUP_COUNT) {
        for (unsigned int i = 0U; i < DMR_FRAME_LENGTH_BYTES; i++) {
            m_poBuffer[i] = m_fifo[slotIndex].get();
            if (i == 8U)
                m_markBuffer[i] = slotIndex == 0U ? MARK_SLOT1 : MARK_SLOT2;
            else
                m_markBuffer[i] = MARK_NONE;
        }
    }
    else {
        m_abort[slotIndex] = false;
        // Transmit an idle message
        for (unsigned int i = 0U; i < DMR_FRAME_LENGTH_BYTES; i++) {
            m_poBuffer[i] = m_idle[i];
            if (i == 8U)
                m_markBuffer[i] = slotIndex == 0U ? MARK_SLOT1 : MARK_SLOT2;
            else
                m_markBuffer[i] = MARK_NONE;
        }
    }

    m_poLen = DMR_FRAME_LENGTH_BYTES;
    m_poPtr = 0U;
}

/// <summary>
///
/// </summary>
void DMRTX::createCal()
{
    // 1.2 kHz sine wave generation
    if (m_modemState == STATE_DMR_CAL) {
        for (unsigned int i = 0U; i < DMR_FRAME_LENGTH_BYTES; i++) {
            m_poBuffer[i] = DMR_START_SYNC;
            m_markBuffer[i] = MARK_NONE;
        }

        m_poLen = DMR_FRAME_LENGTH_BYTES;
    }

    // 80 Hz square wave generation
    if (m_modemState == STATE_DMR_LF_CAL) {
        for (unsigned int i = 0U; i < 7U; i++) {
            m_poBuffer[i] = 0x55U; // +3, +3, ... pattern
            m_markBuffer[i] = MARK_NONE;
        }

        m_poBuffer[7U] = 0x5FU; // +3, +3, -3, -3 pattern

        for (unsigned int i = 8U; i < 15U; i++) {
            m_poBuffer[i] = 0xFFU; // -3, -3, ... pattern
            m_markBuffer[i] = MARK_NONE;
        }

        m_poLen = 15U;
    }

    m_poLen = DMR_FRAME_LENGTH_BYTES;
    m_poPtr = 0U;
}

/// <summary>
///
/// </summary>
/// <param name="txSlotIndex"></param>
/// <param name="rxSlotIndex"></param>
void DMRTX::createCACH(uint8_t txSlotIndex, uint8_t rxSlotIndex)
{
    m_frameCount++;
    m_abortCount[0U]++;
    m_abortCount[1U]++;

    if (m_cachPtr >= 12U)
        m_cachPtr = 0U;

    if (m_cachPtr == 0U) {
        if (m_fifo[0U].getData() == 0U && m_fifo[1U].getData() == 0U)
            ::memcpy(m_shortLC, EMPTY_SHORT_LC, 12U);
        else
            ::memcpy(m_shortLC, m_newShortLC, 12U);
    }

    ::memcpy(m_poBuffer, m_shortLC + m_cachPtr, 3U);
    m_markBuffer[0U] = MARK_NONE;
    m_markBuffer[1U] = MARK_NONE;
    m_markBuffer[2U] = rxSlotIndex == 1U ? MARK_SLOT1 : MARK_SLOT2;

    bool at = false;
    if (m_frameCount >= STARTUP_COUNT)
        at = m_fifo[rxSlotIndex].getData() > 0U;
    bool tc = txSlotIndex == 1U;
    bool ls0 = true;            // For 1 and 2
    bool ls1 = true;

    if (m_cachPtr == 0U)          // For 0
        ls1 = false;
    else if (m_cachPtr == 9U)     // For 3
        ls0 = false;

    bool h0 = at ^ tc ^ ls1;
    bool h1 = tc ^ ls1 ^ ls0;
    bool h2 = at ^ tc ^ ls0;

    m_poBuffer[0U] |= at ? 0x80U : 0x00U;
    m_poBuffer[0U] |= tc ? 0x08U : 0x00U;
    m_poBuffer[1U] |= ls1 ? 0x80U : 0x00U;
    m_poBuffer[1U] |= ls0 ? 0x08U : 0x00U;
    m_poBuffer[1U] |= h0 ? 0x02U : 0x00U;
    m_poBuffer[2U] |= h1 ? 0x20U : 0x00U;
    m_poBuffer[2U] |= h2 ? 0x02U : 0x00U;

    m_poLen = DMR_CACH_LENGTH_BYTES;
    m_poPtr = 0U;

    m_cachPtr += 3U;
}

/// <summary>
///
/// </summary>
/// <param name="c"></param>
/// <param name="control"></param>
void DMRTX::writeByte(uint8_t c, uint8_t control)
{
    uint8_t bit;
    uint8_t mask = 0x80U;
    uint8_t control_tmp = m_control_old;

    for (uint8_t i = 0U; i < 8U; i++, c <<= 1) {
        if ((c & mask) == mask)
            bit = 1U;
        else
            bit = 0U;

        if (i == 7U) {
            if (control == MARK_SLOT2)
                control_tmp = true;
            else if (control == MARK_SLOT1)
                control_tmp = false;

            m_control_old = control_tmp;
        }

        io.write(&bit, 1, &control_tmp);
    }
}
