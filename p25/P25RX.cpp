// SPDX-License-Identifier: GPL-2.0-only
/**
* Digital Voice Modem - Hotspot Firmware
* GPLv2 Open Source. Use is subject to license terms.
* DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
*
* @package DVM / Hotspot Firmware
* @derivedfrom MMDVM_HS (https://github.com/g4klx/MMDVM_HS)
* @license GPLv2 License (https://opensource.org/licenses/GPL-2.0)
*
*   Copyright (C) 2016,2017 Jonathan Naylor, G4KLX
*   Copyright (C) 2016,2017,2018 Andy Uribe, CA6JAU
*   Copyright (C) 2021-2024 Bryan Biedenkapp, N2PLL
*
*/
#include "Globals.h"
#include "p25/P25RX.h"
#include "Utils.h"

using namespace p25;

// ---------------------------------------------------------------------------
//  Constants
// ---------------------------------------------------------------------------

const uint8_t MAX_SYNC_BITS_START_ERRS = 2U;
const uint8_t MAX_SYNC_BITS_ERRS = 4U;

const uint16_t MAX_SYNC_FRAMES = 7U;

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
    m_endPtr(NOENDPTR),
    m_pduEndPtr(NOENDPTR),
    m_lostCount(0U),
    m_nac(0xF7EU),
    m_state(P25RXS_NONE),
    m_duid(0xFFU)
{
    ::memset(m_buffer, 0x00U, P25_LDU_FRAME_LENGTH_BYTES + 3U);
}

/// <summary>
/// Helper to reset data values to defaults.
/// </summary>
void P25RX::reset()
{
    m_bitBuffer = 0x00U;
    m_dataPtr = 0U;

    ::memset(m_buffer, 0x00U, P25_LDU_FRAME_LENGTH_BYTES + 3U);

    m_endPtr = NOENDPTR;
    m_pduEndPtr = NOENDPTR;

    m_lostCount = 0U;

    m_state = P25RXS_NONE;

    m_duid = 0xFFU;
}

/// <summary>
/// Sample P25 bits from the air interface.
/// </summary>
/// <param name="bit"></param>
void P25RX::databit(bool bit)
{
    m_bitBuffer <<= 1;
    if (bit)
        m_bitBuffer |= 0x01U;

    if (m_state != P25RXS_NONE) {
        _WRITE_BIT(m_buffer, m_dataPtr, bit);

        m_dataPtr++;
        if (m_state != P25RXS_DATA) {
            if (m_dataPtr > P25_LDU_FRAME_LENGTH_BITS) {
                reset();
            }
        } else {
            if (m_dataPtr > P25_PDU_FRAME_LENGTH_BITS) {
                reset();
            }
        }
    }

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
            DEBUG3("P25RX: databit(): dataPtr/endPtr", m_dataPtr, m_endPtr);
            m_state = P25RXS_SYNC;
        }

        io.setDecode(m_state != P25RXS_NONE);
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

// ---------------------------------------------------------------------------
//  Private Class Members
// ---------------------------------------------------------------------------

/// <summary>
/// Helper to process P25 samples.
/// </summary>
/// <param name="bit"></param>
void P25RX::processBit(bool bit)
{
    // process NID
    if (m_dataPtr == P25_SYNC_LENGTH_BITS + P25_NID_LENGTH_BITS + 1) {
        DEBUG3("P25RX::processBit() dataPtr/endPtr", m_dataPtr, m_endPtr);

        if (!decodeNid()) {
            io.setDecode(false);

            serial.writeP25Lost();
            reset();
        }
        else {
            switch (m_duid) {
            case P25_DUID_HDU:
                {
                    DEBUG2("P25RX::processBit() sync found in HDU pos", m_dataPtr);
                    m_endPtr = P25_HDU_FRAME_LENGTH_BITS;
                }
                break;
            case P25_DUID_TDU:
                {
                    DEBUG2("P25RX::processBit() sync found in TDU pos", m_dataPtr);
                    m_endPtr = P25_TDU_FRAME_LENGTH_BITS;
                }
                break;
            case P25_DUID_LDU1:
            case P25_DUID_VSELP1:
                m_state = P25RXS_VOICE;
                m_endPtr = P25_LDU_FRAME_LENGTH_BITS;
                return;
            case P25_DUID_TSDU:
                {
                    DEBUG2("P25RX::processBit() sync found in TSDU pos", m_dataPtr);
                    m_endPtr = P25_TSDU_FRAME_LENGTH_BITS;
                }
                break;
            case P25_DUID_LDU2:
            case P25_DUID_VSELP2:
                m_state = P25RXS_VOICE;
                m_endPtr = P25_LDU_FRAME_LENGTH_BITS;
                return;
            case P25_DUID_PDU:
                m_state = P25RXS_DATA;
                m_endPtr = m_pduEndPtr = P25_PDU_FRAME_LENGTH_BITS;
                return;
            case P25_DUID_TDULC:
                {
                    DEBUG2("P25RX::processBit() sync found in TDULC pos", m_dataPtr);
                    m_endPtr = P25_TDULC_FRAME_LENGTH_BITS;
                }
                break;
            default:
                {
                    DEBUG3("P25RX::processBit() illegal DUID in NID", m_nac, m_duid);
                    reset();
                }
                return;
            }
        }
    }

    if (m_state == P25RXS_SYNC) {
        // only search for a sync in the right place +-2 bits
        if (m_dataPtr >= (P25_SYNC_LENGTH_BITS - 2U) && m_dataPtr <= (P25_SYNC_LENGTH_BITS + 2U)) {
            correlateSync();
        }

        // since we aren't processing voice or data -- simply wait till we've reached the end pointer
        if (m_dataPtr == m_endPtr) {
            // DEBUG3("P25RX: m_buffer dump endPtr/endPtrB", m_endPtr, m_endPtr / 8U);
            // DEBUG_DUMP(m_buffer, P25_LDU_FRAME_LENGTH_BYTES + 3U);

            uint8_t frame[P25_HDU_FRAME_LENGTH_BYTES + 1U];
            ::memcpy(frame + 1U, m_buffer, m_endPtr / 8U);

            frame[0U] = 0x01U; // has sync
            serial.writeP25Data(frame, (m_endPtr / 8U) + 1U);
            reset();
        }
    }
    else {
        if (m_state == P25RXS_VOICE) {
            m_lostCount = MAX_SYNC_FRAMES;
            processVoice(bit);
        }

        if (m_state == P25RXS_DATA) {
            m_lostCount = MAX_SYNC_FRAMES;
            processData(bit);
        }
    }
}

/// <summary>
/// Helper to process LDU P25 bits.
/// </summary>
/// <param name="bit"></param>
void P25RX::processVoice(bool bit)
{
    // only search for a sync in the right place +-2 bits
    if (m_dataPtr >= (P25_SYNC_LENGTH_BITS - 2U) && m_dataPtr <= (P25_SYNC_LENGTH_BITS + 2U)) {
        correlateSync();
    }

    // process NID
    if (m_dataPtr == P25_SYNC_LENGTH_BITS + P25_NID_LENGTH_BITS + 1) {
        DEBUG3("P25RX::processVoice() dataPtr/endPtr", m_dataPtr, m_endPtr);

        if (!decodeNid()) {
            io.setDecode(false);

            serial.writeP25Lost();
            reset();
        }
        else {
            switch (m_duid) {
            case P25_DUID_TDU:
                {
                    DEBUG2("P25RX::processVoice() sync found in TDU pos", m_dataPtr);
                    m_endPtr = P25_TDU_FRAME_LENGTH_BITS;
                }
                break;
            case P25_DUID_LDU1:
                m_endPtr = P25_LDU_FRAME_LENGTH_BITS;
                return;
            case P25_DUID_LDU2:
                m_endPtr = P25_LDU_FRAME_LENGTH_BITS;
                return;
            default:
                {
                    DEBUG3("P25RX::processVoice() illegal DUID in NID", m_nac, m_duid);
                    reset();
                }
                return;
            }
        }
    }

    // if we've reached the end pointer and the DUID is a TDU; send it
    if (m_dataPtr == m_endPtr && m_duid == P25_DUID_TDU)
    {
        DEBUG2("P25RX::processVoice() sync found in TDU pos", m_dataPtr);

        uint8_t frame[P25_TDU_FRAME_LENGTH_BYTES + 1U];
        ::memcpy(frame + 1U, m_buffer, m_endPtr / 8U);

        frame[0U] = 0x01U; // has sync
        serial.writeP25Data(frame, P25_TDU_FRAME_LENGTH_BYTES + 1U);

        io.setDecode(false);

        reset();
        return;
    }

    // process voice frame
    if (m_dataPtr == m_endPtr) {
        m_lostCount--;
        
        // we've not seen a data sync for too long, signal sync lost and change to P25RXS_NONE
        if (m_lostCount == 0U) {
            DEBUG1("P25RX::processVoice() sync timeout in LDU, lost lock");

            io.setDecode(false);

            serial.writeP25Lost();
            reset();
        }
        else {
            DEBUG2("P25RX::processVoice() sync found in LDU pos", m_dataPtr);

            uint8_t frame[P25_LDU_FRAME_LENGTH_BYTES + 3U];
            ::memcpy(frame + 1U, m_buffer, m_endPtr / 8U);

            frame[0U] = m_lostCount == (MAX_SYNC_FRAMES - 1U) ? 0x01U : 0x00U; // set sync flag
#if defined(SEND_RSSI_DATA)
            uint16_t rssi = io.readRSSI();
            frame[217U] = (rssi >> 8) & 0xFFU;
            frame[218U] = (rssi >> 0) & 0xFFU;

            serial.writeP25Data(false, frame, P25_LDU_FRAME_LENGTH_BYTES + 3U);
#else
            serial.writeP25Data(frame, P25_LDU_FRAME_LENGTH_BYTES + 1U);
#endif

        }
    }
}

/// <summary>
/// Helper to process PDU P25 bits.
/// </summary>
/// <param name="bit"></param>
void P25RX::processData(bool bit)
{
    // only search for a sync in the right place +-2 bits
    if (m_dataPtr >= (P25_SYNC_LENGTH_BITS - 2U) && m_dataPtr <= (P25_SYNC_LENGTH_BITS + 2U)) {
        correlateSync();
    }

    // process NID
    if (m_dataPtr == P25_SYNC_LENGTH_BITS + P25_NID_LENGTH_BITS + 1) {
        DEBUG3("P25RX::processData() dataPtr/pduEndPtr", m_dataPtr, m_pduEndPtr);

        if (!decodeNid()) {
            io.setDecode(false);

            serial.writeP25Lost();
            reset();
        }
        else {
            switch (m_duid) {
            case P25_DUID_PDU:
                m_endPtr = m_pduEndPtr = P25_PDU_FRAME_LENGTH_BITS;
                return;
            default:
                {
                    DEBUG3("P25RX::processData() illegal DUID in NID", m_nac, m_duid);
                    reset();
                }
                return;
            }
        }
    }

    // process data frame
    if (m_dataPtr == m_pduEndPtr) {
        m_lostCount--;
        
        // we've not seen a data sync for too long, signal sync lost and change to P25RXS_NONE
        if (m_lostCount == 0U) {
            DEBUG1("P25RX::processData() sync timeout in PDU, lost lock");

            io.setDecode(false);

            serial.writeP25Lost();
            reset();
        }
        else {
            DEBUG2("P25RX::processData() sync found in PDU pos", m_dataPtr);

            uint8_t frame[P25_PDU_FRAME_LENGTH_BYTES + 1U];
            ::memcpy(frame + 1U, m_buffer, m_pduEndPtr / 8U);

            frame[0U] = m_lostCount == (MAX_SYNC_FRAMES - 1U) ? 0x01U : 0x00U; // set sync flag
            serial.writeP25Data(frame, P25_PDU_FRAME_LENGTH_BYTES + 1U);
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
        maxErrs = MAX_SYNC_BITS_START_ERRS;
    else
        maxErrs = MAX_SYNC_BITS_ERRS;

    // fuzzy matching of the data sync bit sequence
    uint8_t errs = countBits64((m_bitBuffer & P25_SYNC_BITS_MASK) ^ P25_SYNC_BITS);
    if (errs <= maxErrs) {
        ::memset(m_buffer, 0x00U, P25_LDU_FRAME_LENGTH_BYTES + 3U);

        DEBUG2("P25RX::correlateSync() sync errs", errs);

        // unpack sync bytes
        uint8_t sync[P25_SYNC_BYTES_LENGTH];
        sync[0U] = (uint8_t)((m_bitBuffer >> 40) & 0xFFU);
        sync[1U] = (uint8_t)((m_bitBuffer >> 32) & 0xFFU);
        sync[2U] = (uint8_t)((m_bitBuffer >> 24) & 0xFFU);
        sync[3U] = (uint8_t)((m_bitBuffer >> 16) & 0xFFU);
        sync[4U] = (uint8_t)((m_bitBuffer >> 8) & 0xFFU);
        sync[5U] = (uint8_t)((m_bitBuffer >> 0) & 0xFFU);

        DEBUG4("P25RX::correlateSync() sync [b0 - b2]", sync[0], sync[1], sync[2]);
        DEBUG4("P25RX::correlateSync() sync [b3 - b5]", sync[3], sync[4], sync[5]);

        for (uint8_t i = 0U; i < P25_SYNC_BYTES_LENGTH; i++)
            m_buffer[i] = sync[i];

        // DEBUG1("P25RX: m_buffer dump");
        // DEBUG_DUMP(m_buffer, P25_LDU_FRAME_LENGTH_BYTES);

        m_endPtr = m_dataPtr + P25_LDU_FRAME_LENGTH_BITS - P25_SYNC_LENGTH_BITS;
        if (m_endPtr >= P25_LDU_FRAME_LENGTH_BITS)
            m_endPtr -= P25_LDU_FRAME_LENGTH_BITS;
        m_pduEndPtr = m_dataPtr + P25_PDU_FRAME_LENGTH_BITS - P25_SYNC_LENGTH_BITS;
        if (m_pduEndPtr >= P25_PDU_FRAME_LENGTH_BITS)
            m_pduEndPtr -= P25_PDU_FRAME_LENGTH_BITS;

        m_lostCount = MAX_SYNC_FRAMES;
        m_dataPtr = P25_SYNC_LENGTH_BITS;

        DEBUG4("P25RX::correlateSync() dataPtr/endPtr/pduEndPtr", m_dataPtr, m_endPtr, m_pduEndPtr);

        return true;
    }

    return false;
}

/// <summary>
/// Helper to decode the P25 NID.
/// </summary>
bool P25RX::decodeNid()
{
    uint8_t nid[P25_NID_LENGTH_BYTES];
    for (int i = 6U; i < 8U; i++)
        nid[i - 6U] = m_buffer[i];

    if (m_nac == 0xF7EU) {
        m_duid = nid[1U] & 0x0FU;
        DEBUG2("P25RX::decodeNid() DUID for xDU", m_duid);
        return true;
    }

    uint16_t nac = (nid[0U] << 4) | ((nid[1U] & 0xF0U) >> 4);
    if (nac == m_nac) {
        m_duid = nid[1U] & 0x0FU;
        DEBUG2("P25RX::decodeNid() DUID for xDU", m_duid);
        return true;
    }
    else {
        DEBUG3("P25RX::decodeNid() invalid NAC found; nac != m_nac", nac, m_nac);
    }

    return false;
}
