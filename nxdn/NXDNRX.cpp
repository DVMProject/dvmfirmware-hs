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
 *   Copyright (C) 2022 Bryan Biedenkapp N2PLL
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

/// <summary>
/// Initializes a new instance of the NXDNRX class.
/// </summary>
NXDNRX::NXDNRX() :
    m_bitBuffer(0x00U),
    m_outBuffer(),
    m_buffer(NULL),
    m_dataPtr(0U),
    m_endPtr(NOENDPTR),
    m_lostCount(0U),
    m_state(NXDNRXS_NONE)
{
    ::memset(m_outBuffer, 0x00U, NXDN_FRAME_LENGTH_BYTES + 3U);
    m_buffer = m_outBuffer + 1U;
}

/// <summary>
/// Helper to reset data values to defaults.
/// </summary>
void NXDNRX::reset()
{
    m_bitBuffer = 0x00U;
    m_dataPtr = 0U;

    m_endPtr = NOENDPTR;

    m_lostCount = 0U;

    m_state = NXDNRXS_NONE;
}

/// <summary>
/// Sample NXDN bits from the air interface.
/// </summary>
/// <param name="bit"></param>
void NXDNRX::databit(bool bit)
{
    m_bitBuffer <<= 1;
    if (bit)
        m_bitBuffer |= 0x01U;

    if (m_state != NXDNRXS_NONE) {
        _WRITE_BIT(m_buffer, m_dataPtr, bit);

        m_dataPtr++;
        if (m_dataPtr > NXDN_FRAME_LENGTH_BITS) {
            reset();
        }
    }

    if (m_state == NXDNRXS_DATA) {
        processData(bit);
    }
    else {

        bool ret = correlateSync(true);
        if (ret) {
            DEBUG3("NXDNRX: databit(): dataPtr/endPtr", m_dataPtr, m_endPtr);
            m_state = NXDNRXS_DATA;
        }

        io.setDecode(m_state != NXDNRXS_NONE);
    }
}

// ---------------------------------------------------------------------------
//  Private Class Members
// ---------------------------------------------------------------------------

/// <summary>
/// Helper to process NXDN data bits.
/// </summary>
/// <param name="bit"></param>
void NXDNRX::processData(bool bit)
{
    // only search for a sync in the right place +-2 bits
    if (m_dataPtr >= (NXDN_FSW_LENGTH_BITS - 2U) && m_dataPtr <= (NXDN_FSW_LENGTH_BITS + 2U)) {
        correlateSync();
    }

    // process frame
    if (m_dataPtr == m_endPtr) {
        m_lostCount--;
        
        // we've not seen a data sync for too long, signal sync lost and change to NXDNRXS_NONE
        if (m_lostCount == 0U) {
            DEBUG1("NXDNRX: processData(): sync timed out, lost lock");

            io.setDecode(false);

            serial.writeNXDNLost();
            reset();
        }
        else {
            DEBUG2("NXDNRX: processData(): sync found pos", m_dataPtr);

            m_outBuffer[0U] = m_lostCount == (MAX_FSW_FRAMES - 1U) ? 0x01U : 0x00U; // set sync flag
            serial.writeNXDNData(m_outBuffer, NXDN_FRAME_LENGTH_BYTES + 1U);

            ::memset(m_outBuffer, 0x00U, NXDN_FRAME_LENGTH_BYTES + 3U);
            m_dataPtr = 0U;
        }
    }
}

/// <summary>
/// Frame synchronization correlator.
/// </summary>
/// <param name="first"></param>
/// <returns></returns>
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
        DEBUG2("NXDNRX: correlateSync(): correlateSync errs", errs);

        if (first) {
            // unpack sync bytes
            uint8_t sync[NXDN_FSW_BYTES_LENGTH];
            sync[0U] = (uint8_t)((m_bitBuffer >> 16) & NXDN_FSW_BYTES_MASK[0U]);
            sync[1U] = (uint8_t)((m_bitBuffer >> 8) & NXDN_FSW_BYTES_MASK[1U]);
            sync[2U] = (uint8_t)((m_bitBuffer >> 0) & NXDN_FSW_BYTES_MASK[2U]);

            for (uint8_t i = 0U; i < NXDN_FSW_BYTES_LENGTH; i++)
                m_buffer[i] = sync[i];

            DEBUG4("NXDNRX: correlateSync(): sync [b0 - b2]", m_buffer[0], m_buffer[1], m_buffer[2]);
        }

        m_lostCount = MAX_FSW_FRAMES;
        m_dataPtr = NXDN_FSW_LENGTH_BITS;
        m_endPtr = NXDN_FRAME_LENGTH_BITS;

        DEBUG3("NXDNRX: correlateSync(): dataPtr/endPtr", m_dataPtr, m_endPtr);

        return true;
    }

    return false;
}
