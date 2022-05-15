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
*   Copyright (C) 2016 by Jonathan Naylor G4KLX
*   Copyright (C) 2016,2017 by Andy Uribe CA6JAU
*   Copyright (C) 2021-2022 by Bryan Biedenkapp N2PLL
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
#include "p25/P25TX.h"
#include "p25/P25Defines.h"

using namespace p25;

// ---------------------------------------------------------------------------
//  Public Class Members
// ---------------------------------------------------------------------------

/// <summary>
/// Initializes a new instance of the P25TX class.
/// </summary>
P25TX::P25TX() :
    m_fifo(P25_TX_BUFFER_LEN),
    m_state(P25TXSTATE_NORMAL),
    m_poBuffer(),
    m_poLen(0U),
    m_poPtr(0U),
    m_preambleCnt(P25_FIXED_DELAY),
    m_txHang(P25_FIXED_TX_HANG),
    m_tailCnt(0U)
{
    /* stub */
}

/// <summary>
/// Process local buffer and transmit on the air interface.
/// </summary>
void P25TX::process()
{
    if (m_fifo.getData() == 0U && m_poLen == 0U && m_tailCnt > 0U &&
        m_state != P25TXSTATE_CAL) {
        // transmit silence until the hang timer has expired
        uint16_t space = io.getSpace();

        while (space > 8U) {
            writeSilence();

            space -= 8U;
            m_tailCnt--;

            if (m_tailCnt == 0U)
                return;
            if (m_fifo.getData() > 0U) {
                m_tailCnt = 0U;
                return;
            }
        }

        if (m_fifo.getData() == 0U && m_poLen == 0U)
            return;
    }

    if (m_poLen == 0U) {
        if (m_state == P25TXSTATE_CAL) {
            m_tailCnt = 0U;
            createCal();
        }
        else {
            if (m_fifo.getData() == 0U)
                return;

            createData();
        }

        DEBUG2("P25TX: process(): poLen", m_poLen);
    }

    if (m_poLen > 0U) {
        uint16_t space = io.getSpace();

        while (space > 8U) {
            uint8_t c = m_poBuffer[m_poPtr++];

            writeByte(c);

            space -= 8U;
            m_tailCnt = m_txHang;

            if (m_poPtr >= m_poLen) {
                m_poPtr = 0U;
                m_poLen = 0U;
                return;
            }
        }
    }
}

/// <summary>
/// Write data to the local buffer.
/// </summary>
/// <param name="data"></param>
/// <param name="length"></param>
/// <returns></returns>
uint8_t P25TX::writeData(const uint8_t* data, uint8_t length)
{
    if (length < (P25_TDU_FRAME_LENGTH_BYTES + 1U))
        return RSN_ILLEGAL_LENGTH;

    uint16_t space = m_fifo.getSpace();
    DEBUG3("P25TX: writeData(): dataLength/fifoLength", length, space);
    if (space < length) {
        m_fifo.reset();
        return RSN_RINGBUFF_FULL;
    }

    m_fifo.put(length - 1U);
    for (uint8_t i = 0U; i < (length - 1U); i++)
        m_fifo.put(data[i + 1U]);

    return RSN_OK;
}

/// <summary>
/// Clears the local buffer.
/// </summary>
void P25TX::clear()
{
    m_fifo.reset();
}

/// <summary>
/// Sets the FDMA preamble count.
/// </summary>
/// <param name="preambleCnt">Count of preambles.</param>
void P25TX::setPreambleCount(uint8_t preambleCnt)
{
    m_preambleCnt = P25_FIXED_DELAY + preambleCnt;

    // clamp preamble count to 250ms maximum
    if (m_preambleCnt > 1200U)
        m_preambleCnt = 1200U;
}

/// <summary>
/// Sets the Tx hang time.
/// </summary>
/// <param name="txHang">Transmit hang time in seconds.</param>
void P25TX::setTxHang(uint8_t txHang)
{
    if (txHang > 0U)
        m_txHang = txHang * 1200U;
    else
        m_txHang = P25_FIXED_TX_HANG;

    // clamp tx hang count to 13s maximum
    if (txHang > 13U)
        m_txHang = 13U * 1200U;
}

/// <summary>
/// Helper to set the calibration state for Tx.
/// </summary>
/// <param name="start"></param>
void P25TX::setCal(bool start)
{
    m_state = start ? P25TXSTATE_CAL : P25TXSTATE_NORMAL;
}

/// <summary>
/// Helper to get how much space the ring buffer has for samples.
/// </summary>
/// <returns></returns>
uint8_t P25TX::getSpace() const
{
    return m_fifo.getSpace() / P25_LDU_FRAME_LENGTH_BYTES;
}

// ---------------------------------------------------------------------------
//  Private Class Members
// ---------------------------------------------------------------------------

/// <summary>
///
/// </summary>
void P25TX::createData()
{
    if (!m_tx) {
        for (uint16_t i = 0U; i < m_preambleCnt; i++)
            m_poBuffer[m_poLen++] = P25_START_SYNC;
    }
    else {
        uint8_t length = m_fifo.get();
        DEBUG3("P25TX: createData(): dataLength/fifoSpace", length, m_fifo.getSpace());
        for (uint8_t i = 0U; i < length; i++) {
            m_poBuffer[m_poLen++] = m_fifo.get();
        }
    }

    m_poPtr = 0U;
}

/// <summary>
///
/// </summary>
void P25TX::createCal()
{
    // 1.2 kHz sine wave generation
    if (m_modemState == STATE_P25_CAL) {
        for (unsigned int i = 0U; i < P25_LDU_FRAME_LENGTH_BYTES; i++) {
            m_poBuffer[i] = P25_START_SYNC;
        }

        m_poLen = P25_LDU_FRAME_LENGTH_BYTES;
    }

    // 80 Hz square wave generation
    if (m_modemState == STATE_P25_LF_CAL) {
        for (unsigned int i = 0U; i < 108U; i++) {
            m_poBuffer[i] = 0x55U; // +3, +3, ... pattern
        }

        m_poBuffer[109U] = 0x5FU; // +3, +3, -3, -3 pattern

        for (unsigned int i = 110U; i < 216U; i++) {
            m_poBuffer[i] = 0xFFU; // -3, -3, ... pattern
        }

        m_poLen = P25_LDU_FRAME_LENGTH_BYTES;
    }

    m_poLen = P25_LDU_FRAME_LENGTH_BYTES;
    m_poPtr = 0U;
}

/// <summary>
/// 
/// </summary>
/// <param name="c"></param>
void P25TX::writeByte(uint8_t c)
{
    uint8_t bit;
    uint8_t mask = 0x80U;

    for (uint8_t i = 0U; i < 8U; i++, c <<= 1) {
        if ((c & mask) == mask)
            bit = 1U;
        else
            bit = 0U;

        io.write(&bit, 1);
    }
}

/// <summary>
///
/// </summary>
void P25TX::writeSilence()
{
    uint8_t bit;
    for (uint8_t i = 0U; i < 4U; i++) {
        bit = 0U;
        io.write(&bit, 1);
    }
}
