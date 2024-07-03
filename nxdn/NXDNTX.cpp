// SPDX-License-Identifier: GPL-2.0-only
/*
 * Digital Voice Modem - Hotspot Firmware
 * GPLv2 Open Source. Use is subject to license terms.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 *  Copyright (C) 2009-2018,2020 Jonathan Naylor, G4KLX
 *  Copyright (C) 2017 Andy Uribe, CA6JAU
 *  Copyright (C) 2022 Bryan Biedenkapp, N2PLL
 *
 */
#include "Globals.h"
#include "nxdn/NXDNTX.h"
#include "nxdn/NXDNDefines.h"

using namespace nxdn;

// ---------------------------------------------------------------------------
//  Public Class Members
// ---------------------------------------------------------------------------

/* Initializes a new instance of the NXDNTX class. */

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

/* Process local buffer and transmit on the air interface. */

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

/* Write data to the local buffer. */

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

/* Clears the local buffer. */

void NXDNTX::clear()
{
    m_fifo.reset();
}

/* Sets the FDMA preamble count. */

void NXDNTX::setPreambleCount(uint8_t preambleCnt)
{
    m_preambleCnt = 300U + uint16_t(preambleCnt) * 6U; // 500ms + tx delay

    // clamp preamble count
    if (m_preambleCnt > 60U)
        m_preambleCnt = 60U;
}

/* Sets the Tx hang time. */

void NXDNTX::setTxHang(uint8_t txHang)
{
    m_txHang = txHang * NXDN_FIXED_TX_HANG;
}

/* Helper to set the calibration state for Tx. */

void NXDNTX::setCal(bool start)
{
    m_state = start ? NXDNTXSTATE_CAL : NXDNTXSTATE_NORMAL;
}

/* Helper to resize the FIFO buffer. */

void NXDNTX::resizeBuffer(uint16_t size)
{
    m_fifo.reset();
    m_fifo.reinitialize(size);
}

/* Helper to get how much space the ring buffer has for samples. */

uint8_t NXDNTX::getSpace() const
{
    return m_fifo.getSpace() / NXDN_FRAME_LENGTH_BYTES;
}

// ---------------------------------------------------------------------------
//  Private Class Members
// ---------------------------------------------------------------------------

/* Helper to generate data. */

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

/* Helper to write a raw byte to the DAC. */

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

/* */

void NXDNTX::writeSilence()
{
    uint8_t bit;
    for (uint8_t i = 0U; i < 4U; i++) {
        bit = 0U;
        io.write(&bit, 1);
    }
}
