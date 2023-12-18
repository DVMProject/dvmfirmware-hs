/**
* Digital Voice Modem - DSP Firmware
* GPLv2 Open Source. Use is subject to license terms.
* DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
*
* @package DVM / DSP Firmware
*
*/
//
// Based on code from the MMDVM project. (https://github.com/g4klx/MMDVM)
// Licensed under the GPLv2 License (https://opensource.org/licenses/GPL-2.0)
//
/*
*   Copyright (C) 2009-2016 by Jonathan Naylor G4KLX
*   Copyright (C) 2016 by Colin Durbridge G4EML
*   Copyright (C) 2016,2017,2018 by Andy Uribe CA6JAU
*   Copyright (C) 2021 by Bryan Biedenkapp N2PLL
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

// PR FILL pattern
const uint8_t PR_FILL[] =
        { 0x63U, 0xEAU, 0x00U, 0x76U, 0x6CU, 0x76U, 0xC4U, 0x52U, 0xC8U, 0x78U,
          0x09U, 0x2DU, 0xB8U, 0x79U, 0x27U, 0x57U, 0x9BU, 0x31U, 0xBCU, 0x3EU,
          0xEAU, 0x45U, 0xC3U, 0x30U, 0x49U, 0x17U, 0x93U, 0xAEU, 0x8BU, 0x6DU,
          0xA4U, 0xA5U, 0xADU, 0xA2U, 0xF1U, 0x35U, 0xB5U, 0x3CU, 0x1EU };

// ---------------------------------------------------------------------------
//  Public Class Members
// ---------------------------------------------------------------------------

/// <summary>
/// Initializes a new instance of the DMRDMOTX class.
/// </summary>
DMRDMOTX::DMRDMOTX() :
    m_fifo(DMR_TX_BUFFER_LEN),
    m_poBuffer(),
    m_poLen(0U),
    m_poPtr(0U),
    m_preambleCnt(DMRDMO_FIXED_DELAY)
{
    /* stub */
}

/// <summary>
/// Process local buffer and transmit on the air interface.
/// </summary>
void DMRDMOTX::process()
{
    if (m_poLen == 0U && m_fifo.getData() > 0U) {
        if (!m_tx) {
            for (uint16_t i = 0U; i < m_preambleCnt; i++)
                m_poBuffer[i] = DMR_START_SYNC;

            m_poLen = m_preambleCnt;
        }
        else {
            for (unsigned int i = 0U; i < DMR_FRAME_LENGTH_BYTES; i++)
                    m_poBuffer[i] = m_fifo.get();

            for (unsigned int i = 0U; i < 39U; i++)
                    m_poBuffer[i + DMR_FRAME_LENGTH_BYTES] = PR_FILL[i];                    

            m_poLen = 72U;
        }

        m_poPtr = 0U;
    }

    if (m_poLen > 0U) {
        uint16_t space = io.getSpace();

        while (space > 8U) {
            uint8_t c = m_poBuffer[m_poPtr++];

            writeByte(c);

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
/// Write data to the local buffer.
/// </summary>
/// <param name="data"></param>
/// <param name="length"></param>
/// <returns></returns>
uint8_t DMRDMOTX::writeData(const uint8_t* data, uint8_t length)
{
    if (length != (DMR_FRAME_LENGTH_BYTES + 1U))
        return RSN_ILLEGAL_LENGTH;

    uint16_t space = m_fifo.getSpace();
    DEBUG3("DMRDMOTX::writeData() dataLength/fifoLength", length, space);
    if (space < DMR_FRAME_LENGTH_BYTES)
        return RSN_RINGBUFF_FULL;

    for (uint8_t i = 0U; i < DMR_FRAME_LENGTH_BYTES; i++)
        m_fifo.put(data[i + 1U]);

    return RSN_OK;
}

/// <summary>
/// Sets the FDMA preamble count.
/// </summary>
/// <param name="preambleCnt">Count of preambles.</param>
void DMRDMOTX::setPreambleCount(uint8_t preambleCnt)
{
    uint32_t preambles = (uint32_t)((float)preambleCnt / 0.2083F);
    m_preambleCnt = DMRDMO_FIXED_DELAY + preambles;

    // clamp preamble count to 250ms maximum
    if (m_preambleCnt > 1200U)
        m_preambleCnt = 1200U;
}

/// <summary>
/// Helper to resize the FIFO buffer.
/// </summary>
/// <param name="size"></param>
void DMRDMOTX::resizeBuffer(uint16_t size)
{
    m_fifo.reset();
    m_fifo.reinitialize(size);
}

/// <summary>
/// Helper to get how much space the ring buffer has for samples.
/// </summary>
/// <returns></returns>
uint16_t DMRDMOTX::getSpace() const
{
    return m_fifo.getSpace() / (DMR_FRAME_LENGTH_BYTES + 2U);
}

// ---------------------------------------------------------------------------
//  Private Class Members
// ---------------------------------------------------------------------------

/// <summary>
///
/// </summary>
/// <param name="c"></param>
void DMRDMOTX::writeByte(uint8_t c)
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
