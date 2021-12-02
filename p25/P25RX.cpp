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
*   Copyright (C) 2016,2017 by Jonathan Naylor G4KLX
*   Copyright (C) 2016,2017,2018 by Andy Uribe CA6JAU
*   Copyright (C) 2021 Bryan Biedenkapp N2PLL
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
#include "p25/P25RX.h"
#include "Utils.h"

using namespace p25;

// ---------------------------------------------------------------------------
//  Constants
// ---------------------------------------------------------------------------

const uint8_t MAX_SYNC_BYTES_START_ERRS = 2U;
const uint8_t MAX_SYNC_BYTES_ERRS = 4U;

const uint16_t MAX_SYNC_FRAMES = 7U;

const uint8_t CORRELATION_COUNTDOWN = 6U;

const uint16_t NOENDPTR = 9999U;

// ---------------------------------------------------------------------------
//  Public Class Members
// ---------------------------------------------------------------------------
/// <summary>
/// Initializes a new instance of the P25RX class.
/// </summary>
P25RX::P25RX() :
    m_bitBuffer(0x00U),
    m_buffer(),
    m_dataPtr(0U),
    m_minSyncPtr(0U),
    m_maxSyncPtr(NOENDPTR),
    m_startPtr(0U),
    m_endPtr(NOENDPTR),
    m_syncPtr(0U),
    m_lostCount(0U),
    m_countdown(0U),
    m_nac(0xF7EU),
    m_corrCountdown(CORRELATION_COUNTDOWN),
    m_state(P25RXS_NONE),
    m_duid(0xFFU)
{
    /* stub */
}

/// <summary>
/// Helper to reset data values to defaults.
/// </summary>
void P25RX::reset()
{
    m_bitBuffer = 0x00U;
 
    m_dataPtr = 0U;

    m_minSyncPtr = 0U;
    m_maxSyncPtr = NOENDPTR;

    m_startPtr = 0U;
    m_endPtr = NOENDPTR;
    m_syncPtr = 0U;

    m_lostCount = 0U;
    m_countdown = 0U;

    m_state = P25RXS_NONE;

    m_duid = 0xFFU;
}

/// <summary>
/// Sample P25 bits from the air interface.
/// </summary>
/// <param name="bit"></param>
void P25RX::databit(bool bit)
{
    _WRITE_BIT(m_buffer, m_dataPtr, bit);

    m_bitBuffer <<= 1;
    if (bit)
        m_bitBuffer |= 0x01U;

    if (m_state == P25RXS_SYNC) {
        processBit(bit);
    }
    else if (m_state == P25RXS_VOICE) {
        processVoice(bit);
    }
    else if (m_state == P25RXS_DATA) {
        processData(bit);
    }
    else {
        bool ret = correlateSync();
        if (ret) {
            // on the first sync, start the countdown to the state change
            if (m_countdown == 0U) {
                io.setDecode(true);

                m_countdown = m_corrCountdown;
                DEBUG2("P25RX: databit(): correlation countdown", m_countdown);
            }
        }

        if (m_countdown > 0U)
            m_countdown--;

        if (m_countdown == 1U) {
            m_minSyncPtr = m_syncPtr + P25_HDU_FRAME_LENGTH_BITS - 1U;
            if (m_minSyncPtr >= P25_LDU_FRAME_LENGTH_BITS)
                m_minSyncPtr -= P25_LDU_FRAME_LENGTH_BITS;

            m_maxSyncPtr = m_syncPtr + P25_HDU_FRAME_LENGTH_BITS + 1U;
            if (m_maxSyncPtr >= P25_LDU_FRAME_LENGTH_BITS)
                m_maxSyncPtr -= P25_LDU_FRAME_LENGTH_BITS;

            m_state = P25RXS_SYNC;
            m_countdown = 0U;
        }
    }

    m_dataPtr++;
    if (m_dataPtr >= P25_LDU_FRAME_LENGTH_BITS) {
        m_duid = 0xFFU;
        m_dataPtr = 0U;
    }
}

/// <summary>
/// Sets the P25 NAC.
/// </summary>
/// <param name="nac">NAC.</param>
void P25RX::setNAC(uint16_t nac)
{
    m_nac = nac;
}

/// <summary>
/// Sets the P25 sync correlation countdown.
/// </summary>
/// <param name="count">Correlation Countdown Count.</param>
void P25RX::setCorrCount(uint8_t count)
{
    m_corrCountdown = count;
}

// ---------------------------------------------------------------------------
//  Private Class Members
// ---------------------------------------------------------------------------
/// <summary>
/// Helper to process P25 samples.
/// </summary>
/// <param name="bit"></param>
void P25RX::processBit(bool bit)
{
    if (m_minSyncPtr < m_maxSyncPtr) {
        if (m_dataPtr >= m_minSyncPtr && m_dataPtr <= m_maxSyncPtr)
            correlateSync();
    }
    else {
        if (m_dataPtr >= m_minSyncPtr || m_dataPtr <= m_maxSyncPtr)
            correlateSync();
    }

    // initial sample processing does not have an end pointer -- we simply wait till we've read
    // the bits up to the maximum sync pointer
    if (m_dataPtr == m_maxSyncPtr) {
        DEBUG4("P25RX: processBit(): dataPtr/startPtr/endPtr", m_dataPtr, m_startPtr, m_maxSyncPtr);
        DEBUG4("P25RX: processBit(): lostCount/maxSyncPtr/minSyncPtr", m_lostCount, m_maxSyncPtr, m_minSyncPtr);

        if (!decodeNid(m_startPtr)) {
            io.setDecode(false);

            serial.writeP25Lost();
            reset();
        }
        else {
            switch (m_duid) {
            case P25_DUID_HDU:
                {
                    DEBUG2("P25RX: processBit(): sync found in HDU pos", m_syncPtr);

                    uint8_t frame[P25_HDU_FRAME_LENGTH_BYTES + 1U];
                    bitsToBytes(m_startPtr + P25_NID_LENGTH_BITS, P25_HDU_FRAME_LENGTH_BITS, frame);

                    frame[0U] = 0x01U;
                    serial.writeP25Data(frame, P25_HDU_FRAME_LENGTH_BYTES + 1U);
                    reset();
                }
                break;
            case P25_DUID_TDU:
                {
                    DEBUG2("P25RX: processBit(): sync found in TDU pos", m_syncPtr);
                    
                    uint8_t frame[P25_TDU_FRAME_LENGTH_BYTES + 1U];
                    bitsToBytes(m_startPtr + P25_NID_LENGTH_BITS, P25_TDU_FRAME_LENGTH_BITS, frame);

                    frame[0U] = 0x01U;
                    serial.writeP25Data(frame, P25_TDU_FRAME_LENGTH_BYTES + 1U);
                    reset();
            }
                break;
            case P25_DUID_LDU1:
                m_state = P25RXS_VOICE;
                return;
            case P25_DUID_TSDU:
                {
                    DEBUG2("P25RX: processBit(): sync found in TSDU pos", m_syncPtr);

                    uint8_t frame[P25_TSDU_FRAME_LENGTH_BYTES + 1U];
                    bitsToBytes(m_startPtr + P25_NID_LENGTH_BITS, P25_TSDU_FRAME_LENGTH_BITS, frame);

                    frame[0U] = 0x01U;
                    serial.writeP25Data(frame, P25_TSDU_FRAME_LENGTH_BYTES + 1U);
                    reset();
            }
                break;
            case P25_DUID_LDU2:
                m_state = P25RXS_VOICE;
                return;
            case P25_DUID_PDU:
                m_state = P25RXS_DATA;
                return;
            case P25_DUID_TDULC:
                {
                    DEBUG2("P25RX: processBit(): sync found in TDULC pos", m_syncPtr);
            
                    uint8_t frame[P25_TDULC_FRAME_LENGTH_BYTES + 1U];
                    bitsToBytes(m_startPtr + P25_NID_LENGTH_BITS, P25_TDULC_FRAME_LENGTH_BITS, frame);

                    frame[0U] = 0x01U;
                    serial.writeP25Data(frame, P25_TDULC_FRAME_LENGTH_BYTES + 1U);
                    reset();
            }
                break;
            default:
                {
                    DEBUG3("P25RX: processBit(): illegal DUID in NID", m_nac, m_duid);
                    reset();
                }
                return;
            }
        }
    }

    m_minSyncPtr = m_syncPtr + P25_LDU_FRAME_LENGTH_BITS - 1U;
    if (m_minSyncPtr >= P25_LDU_FRAME_LENGTH_BITS)
        m_minSyncPtr -= P25_LDU_FRAME_LENGTH_BITS;

    m_maxSyncPtr = m_syncPtr + 1U;
    if (m_maxSyncPtr >= P25_LDU_FRAME_LENGTH_BITS)
        m_maxSyncPtr -= P25_LDU_FRAME_LENGTH_BITS;

    m_lostCount = MAX_SYNC_FRAMES;

    if (m_state == P25RXS_VOICE) {
        processVoice(bit);
    }

    if (m_state == P25RXS_DATA) {
        processData(bit);
    }
}

/// <summary>
/// Helper to process LDU P25 bits.
/// </summary>
/// <param name="bit"></param>
void P25RX::processVoice(bool bit)
{
    if (m_minSyncPtr < m_maxSyncPtr) {
        if (m_dataPtr >= m_minSyncPtr && m_dataPtr <= m_maxSyncPtr)
            correlateSync();
    }
    else {
        if (m_dataPtr >= m_minSyncPtr || m_dataPtr <= m_maxSyncPtr)
            correlateSync();
    }

    if (m_dataPtr == m_endPtr) {
        if (m_lostCount == MAX_SYNC_FRAMES) {
            m_minSyncPtr = m_syncPtr + P25_LDU_FRAME_LENGTH_BITS - 1U;
            if (m_minSyncPtr >= P25_LDU_FRAME_LENGTH_BITS)
                m_minSyncPtr -= P25_LDU_FRAME_LENGTH_BITS;

            m_maxSyncPtr = m_syncPtr + 1U;
            if (m_maxSyncPtr >= P25_LDU_FRAME_LENGTH_BITS)
                m_maxSyncPtr -= P25_LDU_FRAME_LENGTH_BITS;
        }

        m_lostCount--;

        DEBUG4("P25RX: processVoice(): dataPtr/startPtr/endPtr", m_dataPtr, m_startPtr, m_endPtr);
        DEBUG4("P25RX: processVoice(): lostCount/maxSyncPtr/minSyncPtr", m_lostCount, m_maxSyncPtr, m_minSyncPtr);

        // we've not seen a data sync for too long, signal sync lost and change to P25RXS_NONE
        if (m_lostCount == 0U) {
            DEBUG1("P25RX: processVoice(): sync timeout in LDU, lost lock");

            io.setDecode(false);

            serial.writeP25Lost();
            reset();
        }
        else {
            if (!decodeNid(m_startPtr)) {
                io.setDecode(false);

                serial.writeP25Lost();
                reset();
            }
            else {
                if (m_duid == P25_DUID_TDU) {
                    DEBUG2("P25RX: processBit(): sync found in TDU pos", m_syncPtr);

                    uint8_t frame[P25_TDU_FRAME_LENGTH_BYTES + 1U];
                    bitsToBytes(m_startPtr + P25_NID_LENGTH_BITS, P25_TDU_FRAME_LENGTH_BITS, frame);

                    frame[0U] = 0x01U;
                    serial.writeP25Data(frame, P25_TDU_FRAME_LENGTH_BYTES + 1U);

                    io.setDecode(false);

                    reset();
                    return;
                }

                DEBUG2("P25RX: processVoice(): sync found in LDU pos", m_syncPtr);

                uint8_t frame[P25_LDU_FRAME_LENGTH_BYTES + 3U];
                bitsToBytes(m_startPtr + P25_NID_LENGTH_BITS, P25_LDU_FRAME_LENGTH_BITS, frame);

                frame[0U] = m_lostCount == (MAX_SYNC_FRAMES - 1U) ? 0x01U : 0x00U;
#if defined(SEND_RSSI_DATA)
                if (m_rssiCount > 0U) {
                    uint16_t rssi = m_rssiAccum / m_rssiCount;
                    frame[217U] = (rssi >> 8) & 0xFFU;
                    frame[218U] = (rssi >> 0) & 0xFFU;

                    serial.writeP25Data(false, frame, P25_LDU_FRAME_LENGTH_BYTES + 3U);
                }
                else {
                    serial.writeP25Data(false, frame, P25_LDU_FRAME_LENGTH_BYTES + 1U);
                }
#else
                serial.writeP25Data(frame, P25_LDU_FRAME_LENGTH_BYTES + 1U);
#endif
            }
        }
    }
}

/// <summary>
/// Helper to process PDU P25 bits.
/// </summary>
/// <param name="bit"></param>
void P25RX::processData(bool bit)
{
    if (m_minSyncPtr < m_maxSyncPtr) {
        if (m_dataPtr >= m_minSyncPtr && m_dataPtr <= m_maxSyncPtr)
            correlateSync();
    }
    else {
        if (m_dataPtr >= m_minSyncPtr || m_dataPtr <= m_maxSyncPtr)
            correlateSync();
    }

    if (m_dataPtr == m_endPtr) {
        if (m_lostCount == MAX_SYNC_FRAMES) {
            m_minSyncPtr = m_syncPtr + P25_LDU_FRAME_LENGTH_BITS - 1U;
            if (m_minSyncPtr >= P25_LDU_FRAME_LENGTH_BITS)
                m_minSyncPtr -= P25_LDU_FRAME_LENGTH_BITS;

            m_maxSyncPtr = m_syncPtr + 1U;
            if (m_maxSyncPtr >= P25_LDU_FRAME_LENGTH_BITS)
                m_maxSyncPtr -= P25_LDU_FRAME_LENGTH_BITS;
        }

        m_lostCount--;

        DEBUG4("P25RX: processData(): dataPtr/startPtr/endPtr", m_dataPtr, m_startPtr, m_endPtr);
        DEBUG4("P25RX: processData(): lostCount/maxSyncPtr/minSyncPtr", m_lostCount, m_maxSyncPtr, m_minSyncPtr);

        // we've not seen a data sync for too long, signal sync lost and change to P25RXS_NONE
        if (m_lostCount == 0U) {
            DEBUG1("P25RX: processData(): sync timeout in PDU, lost lock");

            io.setDecode(false);

            serial.writeP25Lost();
            reset();
        }
        else {
            if (!decodeNid(m_startPtr)) {
                io.setDecode(false);

                serial.writeP25Lost();
                reset();
            }
            else {
                DEBUG2("P25RX: processData(): sync found in PDU pos", m_syncPtr);

                uint8_t frame[P25_LDU_FRAME_LENGTH_BYTES + 3U];
                bitsToBytes(m_startPtr + P25_NID_LENGTH_BITS, P25_LDU_FRAME_LENGTH_BITS, frame);

                frame[0U] = m_lostCount == (MAX_SYNC_FRAMES - 1U) ? 0x01U : 0x00U;
#if defined(SEND_RSSI_DATA)
                if (m_rssiCount > 0U) {
                    uint16_t rssi = m_rssiAccum / m_rssiCount;
                    frame[217U] = (rssi >> 8) & 0xFFU;
                    frame[218U] = (rssi >> 0) & 0xFFU;

                    serial.writeP25Data(false, frame, P25_LDU_FRAME_LENGTH_BYTES + 3U);
                }
                else {
                    serial.writeP25Data(false, frame, P25_LDU_FRAME_LENGTH_BYTES + 1U);
                }
#else
                serial.writeP25Data(frame, P25_LDU_FRAME_LENGTH_BYTES + 1U);
#endif
            }
        }
    }
}

/// <summary>
/// Frame synchronization correlator.
/// </summary>
/// <returns></returns>
bool P25RX::correlateSync()
{
    uint8_t maxErrs;
    if (m_state == P25RXS_NONE)
        maxErrs = MAX_SYNC_BYTES_START_ERRS;
    else
        maxErrs = MAX_SYNC_BYTES_ERRS;

    uint8_t errs = countBits64((m_bitBuffer & P25_SYNC_BITS_MASK) ^ P25_SYNC_BITS);
    if (errs <= maxErrs) {
        DEBUG2("P25RX: correlateSync(): correlateSync errs", errs);

        m_syncPtr = m_dataPtr;

        m_startPtr = m_dataPtr + P25_LDU_FRAME_LENGTH_BITS - P25_SYNC_LENGTH_BITS + 1;
        if (m_startPtr >= P25_LDU_FRAME_LENGTH_BITS)
            m_startPtr -= P25_LDU_FRAME_LENGTH_BITS;

        m_endPtr = m_dataPtr + P25_LDU_FRAME_LENGTH_BITS - P25_SYNC_LENGTH_BITS - 1U;
        if (m_endPtr >= P25_LDU_FRAME_LENGTH_BITS)
            m_endPtr -= P25_LDU_FRAME_LENGTH_BITS;

        m_lostCount = MAX_SYNC_FRAMES;

        DEBUG4("P25RX: correlateSync(): dataPtr/startPtr/endPtr", m_dataPtr, m_startPtr, m_endPtr);

        return true;
    }

    return false;
}

/// <summary>
/// Helper to decode the P25 NID.
/// </summary>
/// <param name="start"></param>
bool P25RX::decodeNid(uint16_t start)
{
    uint16_t nidStartPtr = start + P25_SYNC_LENGTH_BITS;
    if (nidStartPtr >= P25_LDU_FRAME_LENGTH_BITS)
        nidStartPtr -= P25_LDU_FRAME_LENGTH_BITS;

    uint8_t nid[P25_NID_LENGTH_BYTES];
    bitsToBytes(m_startPtr, P25_NID_LENGTH_BITS, nid);

    if (m_nac == 0xF7EU) {
        m_duid = nid[1U] & 0x0FU;
        DEBUG2("P25RX: decodeNid(): DUID for xDU", m_duid);
        return true;
    }

    uint16_t nac = (nid[0U] << 4) | ((nid[1U] & 0xF0U) >> 4);
    if (nac == m_nac) {
        m_duid = nid[1U] & 0x0FU;
        DEBUG2("P25RX: decodeNid(): DUID for xDU", m_duid);
        return true;
    }
    else {
        DEBUG3("P25RX: decodeNid(): invalid NAC found; nac != m_nac", nac, m_nac);
    }

    return false;
}

/// <summary>
/// 
/// </summary>
/// <param name="start"></param>
/// <param name="count"></param>
/// <param name="buffer"></param>
void P25RX::bitsToBytes(uint16_t start, uint8_t count, uint8_t* buffer)
{
    for (uint8_t i = 0U; i < count; i++) {
        buffer[i] = 0U;
        buffer[i] |= _READ_BIT(m_buffer, start) << 7;
        start++;
        if (start >= P25_LDU_FRAME_LENGTH_BITS)
            start -= P25_LDU_FRAME_LENGTH_BITS;

        buffer[i] |= _READ_BIT(m_buffer, start) << 6;
        start++;
        if (start >= P25_LDU_FRAME_LENGTH_BITS)
            start -= P25_LDU_FRAME_LENGTH_BITS;

        buffer[i] |= _READ_BIT(m_buffer, start) << 5;
        start++;
        if (start >= P25_LDU_FRAME_LENGTH_BITS)
            start -= P25_LDU_FRAME_LENGTH_BITS;

        buffer[i] |= _READ_BIT(m_buffer, start) << 4;
        start++;
        if (start >= P25_LDU_FRAME_LENGTH_BITS)
            start -= P25_LDU_FRAME_LENGTH_BITS;

        buffer[i] |= _READ_BIT(m_buffer, start) << 3;
        start++;
        if (start >= P25_LDU_FRAME_LENGTH_BITS)
            start -= P25_LDU_FRAME_LENGTH_BITS;

        buffer[i] |= _READ_BIT(m_buffer, start) << 2;
        start++;
        if (start >= P25_LDU_FRAME_LENGTH_BITS)
            start -= P25_LDU_FRAME_LENGTH_BITS;

        buffer[i] |= _READ_BIT(m_buffer, start) << 1;
        start++;
        if (start >= P25_LDU_FRAME_LENGTH_BITS)
            start -= P25_LDU_FRAME_LENGTH_BITS;

        buffer[i] |= _READ_BIT(m_buffer, start) << 0;
        start++;
        if (start >= P25_LDU_FRAME_LENGTH_BITS)
            start -= P25_LDU_FRAME_LENGTH_BITS;
    }
}
