// SPDX-License-Identifier: GPL-2.0-only
/*
 * Digital Voice Modem - Hotspot Firmware
 * GPLv2 Open Source. Use is subject to license terms.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 *  Copyright (C) 2009-2016 Jonathan Naylor, G4KLX
 *  Copyright (C) 2016 Colin Durbridge, G4EML
 *  Copyright (C) 2016,2017,2018 Andy Uribe, CA6JAU
 *  Copyright (C) 2021 Bryan Biedenkapp, N2PLL
 *
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

/* Initializes a new instance of the DMRDMOTX class. */

DMRDMOTX::DMRDMOTX() :
    m_fifo(DMR_TX_BUFFER_LEN),
    m_poBuffer(),
    m_poLen(0U),
    m_poPtr(0U),
    m_preambleCnt(DMRDMO_FIXED_DELAY)
{
    /* stub */
}

/* Process local buffer and transmit on the air interface. */

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

/* Write data to the local buffer. */

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

/* Sets the FDMA preamble count. */

void DMRDMOTX::setPreambleCount(uint8_t preambleCnt)
{
    uint32_t preambles = (uint32_t)((float)preambleCnt / 0.2083F);
    m_preambleCnt = DMRDMO_FIXED_DELAY + preambles;

    // clamp preamble count to 16ms maximum
    if (m_preambleCnt > 80U)
        m_preambleCnt = 80U;
}

/* Helper to resize the FIFO buffer. */

void DMRDMOTX::resizeBuffer(uint16_t size)
{
    m_fifo.reset();
    m_fifo.reinitialize(size);
}

/* Helper to get how much space the ring buffer has for samples. */

uint16_t DMRDMOTX::getSpace() const
{
    return m_fifo.getSpace() / (DMR_FRAME_LENGTH_BYTES + 2U);
}

// ---------------------------------------------------------------------------
//  Private Class Members
// ---------------------------------------------------------------------------

/* Helper to write a raw byte to the DAC. */

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
