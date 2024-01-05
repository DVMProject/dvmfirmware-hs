/**
* Digital Voice Modem - DSP Firmware (Hotspot)
* GPLv2 Open Source. Use is subject to license terms.
* DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
*
* @package DVM / DSP Firmware (Hotspot)
*
*/
//
// Based on code from the MMDVM project. (https://github.com/g4klx/MMDVM)
// Licensed under the GPLv2 License (https://opensource.org/licenses/GPL-2.0)
//
/*
 *   Copyright (C) 2009-2018,2020 by Jonathan Naylor G4KLX
 *   Copyright (C) 2017 by Andy Uribe CA6JAU
 *   Copyright (C) 2022 by Bryan Biedenkapp N2PLL
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
#include "nxdn/NXDNTX.h"
#include "nxdn/NXDNDefines.h"

using namespace nxdn;

// ---------------------------------------------------------------------------
//  Public Class Members
// ---------------------------------------------------------------------------

/// <summary>
/// Initializes a new instance of the NXDNTX class.
/// </summary>
NXDNTX::NXDNTX() :
    m_fifo(NXDN_TX_BUFFER_LEN),
    m_state(NXDNTXSTATE_NORMAL),
    m_poBuffer(),
    m_poLen(0U),
    m_poPtr(0U),
    m_preambleCnt(240U), // 200ms
    m_txHang(3000U),     // 5s
    m_tailCnt(0U)
{
    /* stub */
}

/// <summary>
/// Process local buffer and transmit on the air interface.
/// </summary>
void NXDNTX::process()
{
    if (m_fifo.getData() == 0U && m_poLen == 0U && m_tailCnt > 0U &&
        m_state != NXDNTXSTATE_CAL) {
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
        if (m_state == NXDNTXSTATE_CAL)
            m_tailCnt = 0U;

        if (m_fifo.getData() == 0U)
            return;

        createData();
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
uint8_t NXDNTX::writeData(const uint8_t* data, uint16_t length)
{
    if (length != (NXDN_FRAME_LENGTH_BYTES + 1U))
        return RSN_ILLEGAL_LENGTH;

    uint16_t space = m_fifo.getSpace();
    DEBUG3("NXDNTX::writeData() dataLength/fifoLength", length, space);
    if (space < NXDN_FRAME_LENGTH_BYTES)
        return RSN_RINGBUFF_FULL;

    for (uint8_t i = 0U; i < NXDN_FRAME_LENGTH_BYTES; i++)
        m_fifo.put(data[i + 1U]);

    return RSN_OK;
}

/// <summary>
/// Clears the local buffer.
/// </summary>
void NXDNTX::clear()
{
    m_fifo.reset();
}

/// <summary>
/// Sets the FDMA preamble count.
/// </summary>
/// <param name="preambleCnt">Count of preambles.</param>
void NXDNTX::setPreambleCount(uint8_t preambleCnt)
{
    m_preambleCnt = 300U + uint16_t(preambleCnt) * 6U; // 500ms + tx delay

    // clamp preamble count
    if (m_preambleCnt > 60U)
        m_preambleCnt = 60U;
}

/// <summary>
/// Sets the Tx hang time.
/// </summary>
/// <param name="txHang">Transmit hang time in seconds.</param>
void NXDNTX::setTxHang(uint8_t txHang)
{
    m_txHang = txHang * NXDN_FIXED_TX_HANG;
}

/// <summary>
/// Helper to set the calibration state for Tx.
/// </summary>
/// <param name="start"></param>
void NXDNTX::setCal(bool start)
{
    m_state = start ? NXDNTXSTATE_CAL : NXDNTXSTATE_NORMAL;
}

/// <summary>
/// Helper to resize the FIFO buffer.
/// </summary>
/// <param name="size"></param>
void NXDNTX::resizeBuffer(uint16_t size)
{
    m_fifo.reset();
    m_fifo.reinitialize(size);
}

/// <summary>
/// Helper to get how much space the ring buffer has for samples.
/// </summary>
/// <returns></returns>
uint8_t NXDNTX::getSpace() const
{
    return m_fifo.getSpace() / NXDN_FRAME_LENGTH_BYTES;
}

// ---------------------------------------------------------------------------
//  Private Class Members
// ---------------------------------------------------------------------------

/// <summary>
///
/// </summary>
void NXDNTX::createData()
{
    if (!m_tx) {
        for (uint16_t i = 0U; i < m_preambleCnt; i++)
            m_poBuffer[m_poLen++] = NXDN_SYNC;

        m_poBuffer[m_poLen++] = NXDN_PREAMBLE[0U];
        m_poBuffer[m_poLen++] = NXDN_PREAMBLE[1U];
        m_poBuffer[m_poLen++] = NXDN_PREAMBLE[2U];
    }
    else {
        DEBUG2("NXDNTX::createData() fifoSpace", m_fifo.getSpace());
        for (uint8_t i = 0U; i < NXDN_FRAME_LENGTH_BYTES; i++) {
            m_poBuffer[m_poLen++] = m_fifo.get();
        }
    }

    m_poPtr = 0U;
}

/// <summary>
///
/// </summary>
/// <param name="c"></param>
void NXDNTX::writeByte(uint8_t c)
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
void NXDNTX::writeSilence()
{
    uint8_t bit;
    for (uint8_t i = 0U; i < 4U; i++) {
        bit = 0U;
        io.write(&bit, 1);
    }
}
