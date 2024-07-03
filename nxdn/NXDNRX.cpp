// SPDX-License-Identifier: GPL-2.0-only
/*
 * Digital Voice Modem - Hotspot Firmware
 * GPLv2 Open Source. Use is subject to license terms.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 *  Copyright (C) 2009-2018,2020 Jonathan Naylor, G4KLX
 *  Copyright (C) 2022 Bryan Biedenkapp, N2PLL
 *
 */
#include "Globals.h"
#include "nxdn/NXDNRX.h"
#include "Utils.h"

using namespace nxdn;

// ---------------------------------------------------------------------------
//  Constants
// ---------------------------------------------------------------------------

const uint8_t MAX_FSW_BIT_START_ERRS = 0U;
const uint8_t MAX_FSW_BIT_RUN_ERRS   = 3U;

const unsigned int MAX_FSW_FRAMES = 5U + 1U;

const uint16_t NOENDPTR = 9999U;

// ---------------------------------------------------------------------------
//  Public Class Members
// ---------------------------------------------------------------------------

/* Initializes a new instance of the NXDNRX class. */

NXDNRX::NXDNRX() :
    m_bitBuffer(0x00U),
    m_outBuffer(),
    m_buffer(NULL),
    m_dataPtr(0U),
    m_lostCount(0U),
    m_state(NXDNRXS_NONE)
{
    ::memset(m_outBuffer, 0x00U, NXDN_FRAME_LENGTH_BYTES + 3U);
    m_buffer = m_outBuffer + 1U;
}

/* Helper to reset data values to defaults. */

void NXDNRX::reset()
{
    m_bitBuffer = 0x00U;
    m_dataPtr = 0U;

    m_lostCount = 0U;

    m_state = NXDNRXS_NONE;
}

/* Sample NXDN bits from the air interface. */

void NXDNRX::databit(bool bit)
{
    if (m_state == NXDNRXS_DATA) {
        processData(bit);
    }
    else {
        m_bitBuffer <<= 1;
        if (bit)
            m_bitBuffer |= 0x01U;

        bool ret = correlateSync(true);
        if (ret) {
            m_state = NXDNRXS_DATA;
        }

        io.setDecode(m_state != NXDNRXS_NONE);
    }
}

// ---------------------------------------------------------------------------
//  Private Class Members
// ---------------------------------------------------------------------------

/* Helper to process NXDN data bits. */

void NXDNRX::processData(bool bit)
{
    m_bitBuffer <<= 1;
    if (bit)
        m_bitBuffer |= 0x01U;

    _WRITE_BIT(m_buffer, m_dataPtr, bit);

    m_dataPtr++;
    if (m_dataPtr > NXDN_FRAME_LENGTH_BITS) {
        reset();
    }

    // only search for a sync in the right place +-2 bits
    if (m_dataPtr >= (NXDN_FSW_LENGTH_BITS - 2U) && m_dataPtr <= (NXDN_FSW_LENGTH_BITS + 2U)) {
        if (correlateSync()) {
            DEBUG2("NXDNRX::processData() sync found pos", m_dataPtr - NXDN_FSW_LENGTH_BITS);
        }
    }

    // process frame
    if (m_dataPtr == NXDN_FRAME_LENGTH_BITS) {
        m_lostCount--;
        
        // we've not seen a data sync for too long, signal sync lost and change to NXDNRXS_NONE
        if (m_lostCount == 0U) {
            DEBUG1("NXDNRX::processData() sync timed out, lost lock");

            io.setDecode(false);

            serial.writeNXDNLost();
            reset();
        }
        else {
            m_outBuffer[0U] = m_lostCount == (MAX_FSW_FRAMES - 1U) ? 0x01U : 0x00U; // set sync flag
            serial.writeNXDNData(m_outBuffer, NXDN_FRAME_LENGTH_BYTES + 1U);

            ::memset(m_outBuffer, 0x00U, NXDN_FRAME_LENGTH_BYTES + 3U);
            m_dataPtr = 0U;
        }
    }
}

/* Frame synchronization correlator. */

bool NXDNRX::correlateSync(bool first)
{
    uint8_t maxErrs;
    if (m_state == NXDNRXS_NONE)
        maxErrs = MAX_FSW_BIT_START_ERRS;
    else
        maxErrs = MAX_FSW_BIT_RUN_ERRS;

    // fuzzy matching of the data sync bit sequence
    uint8_t errs = countBits64((m_bitBuffer & NXDN_FSW_BITS_MASK) ^ NXDN_FSW_BITS);
    if (errs <= maxErrs) {
        DEBUG2("NXDNRX::correlateSync() sync errs", errs);

        if (first) {
            // unpack sync bytes
            uint8_t sync[NXDN_FSW_BYTES_LENGTH];
            sync[0U] = (uint8_t)((m_bitBuffer >> 16) & NXDN_FSW_BYTES_MASK[0U]);
            sync[1U] = (uint8_t)((m_bitBuffer >> 8) & NXDN_FSW_BYTES_MASK[1U]);
            sync[2U] = (uint8_t)((m_bitBuffer >> 0) & NXDN_FSW_BYTES_MASK[2U]);

            for (uint8_t i = 0U; i < NXDN_FSW_BYTES_LENGTH; i++)
                m_buffer[i] = sync[i];

            DEBUG4("NXDNRX::correlateSync() sync [b0 - b2]", m_buffer[0], m_buffer[1], m_buffer[2]);
        }

        m_lostCount = MAX_FSW_FRAMES;
        m_dataPtr = NXDN_FSW_LENGTH_BITS;

        DEBUG2("NXDNRX::correlateSync() dataPtr", m_dataPtr - NXDN_FSW_LENGTH_BITS);

        return true;
    }

    return false;
}
