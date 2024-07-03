// SPDX-License-Identifier: GPL-2.0-only
/*
 * Digital Voice Modem - Hotspot Firmware
 * GPLv2 Open Source. Use is subject to license terms.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 *  Copyright (C) 2009-2016 Jonathan Naylor, G4KLX
 *  Copyright (C) 2016,2017,2018 Andy Uribe, CA6JAU
 *  Copyright (C) 2021 Bryan Biedenkapp, N2PLL
 *
 */
#include "Globals.h"
#include "dmr/DMRDMORX.h"
#include "dmr/DMRSlotType.h"
#include "Utils.h"

using namespace dmr;

// ---------------------------------------------------------------------------
//  Constants
// ---------------------------------------------------------------------------

const uint8_t MAX_SYNC_BYTES_ERRS   = 2U;

const uint8_t MAX_SYNC_LOST_FRAMES  = 13U;

const uint16_t NOENDPTR = 9999U;

const uint8_t CONTROL_NONE  = 0x00U;
const uint8_t CONTROL_VOICE = 0x20U;
const uint8_t CONTROL_DATA  = 0x40U;

// ---------------------------------------------------------------------------
//  Public Class Members
// ---------------------------------------------------------------------------

/* Initializes a new instance of the DMRDMORX class. */

DMRDMORX::DMRDMORX() :
    m_bitBuffer(0x00U),
    m_buffer(),
    m_dataPtr(0U),
    m_syncPtr(0U),
    m_startPtr(0U),
    m_endPtr(NOENDPTR),
    m_control(CONTROL_NONE),
    m_syncCount(0U),
    m_colorCode(0U),
    m_state(DMORXS_NONE),
    m_n(0U),
    m_type(0U)
{
    /* stub */
}

/* Helper to reset data values to defaults. */

void DMRDMORX::reset()
{
    m_syncPtr = 0U;
    m_control = CONTROL_NONE;
    m_syncCount = 0U;
    m_state = DMORXS_NONE;
    m_startPtr = 0U;
    m_endPtr = NOENDPTR;
}

/* Sample DMR bits from the air interface. */

void DMRDMORX::databit(bool bit)
{
    _WRITE_BIT(m_buffer, m_dataPtr, bit);

    m_bitBuffer <<= 1;
    if (bit)
        m_bitBuffer |= 0x01U;

    if (m_state == DMORXS_NONE) {
        correlateSync();
    }
    else {
        uint16_t min = m_syncPtr + DMO_BUFFER_LENGTH_BITS - 2;
        uint16_t max = m_syncPtr + 2;

        if (min >= DMO_BUFFER_LENGTH_BITS)
            min -= DMO_BUFFER_LENGTH_BITS;
        if (max >= DMO_BUFFER_LENGTH_BITS)
            max -= DMO_BUFFER_LENGTH_BITS;

        if (min < max) {
            if (m_dataPtr >= min && m_dataPtr <= max)
                correlateSync();
        }
        else {
            if (m_dataPtr >= min || m_dataPtr <= max)
                correlateSync();
        }
    }

    if (m_dataPtr == m_endPtr) {
        frame[0U] = m_control;

        bitsToBytes(m_startPtr, DMR_FRAME_LENGTH_BYTES, frame + 1U);

        if (m_control == CONTROL_DATA) {
            // Data sync
            uint8_t colorCode;
            uint8_t dataType;
            DMRSlotType slotType;
            slotType.decode(frame + 1U, colorCode, dataType);

            if (colorCode == m_colorCode) {
                m_syncCount = 0U;
                m_n = 0U;

                frame[0U] |= dataType;

                switch (dataType) {
                case DT_DATA_HEADER:
                    DEBUG2("DMRDMORX::databit() data header found pos", m_syncPtr);
                    writeRSSIData(frame);
                    m_state = DMORXS_DATA;
                    m_type = 0x00U;
                    break;
                case DT_RATE_12_DATA:
                case DT_RATE_34_DATA:
                case DT_RATE_1_DATA:
                    if (m_state == DMORXS_DATA) {
                        DEBUG2("DMRDMORX::databit() data payload found pos", m_syncPtr);
                        writeRSSIData(frame);
                        m_type = dataType;
                    }
                    break;
                case DT_VOICE_LC_HEADER:
                    DEBUG2("DMRDMORX::databit() voice header found pos", m_syncPtr);
                    writeRSSIData(frame);
                    m_state = DMORXS_VOICE;
                    break;
                case DT_VOICE_PI_HEADER:
                    if (m_state == DMORXS_VOICE) {
                        DEBUG2("DMRDMORX::databit() voice pi header found pos", m_syncPtr);
                        writeRSSIData(frame);
                    }
                    m_state = DMORXS_VOICE;
                    break;
                case DT_TERMINATOR_WITH_LC:
                    if (m_state == DMORXS_VOICE) {
                        DEBUG2("DMRDMORX::databit() voice terminator found pos", m_syncPtr);
                        writeRSSIData(frame);
                        reset();
                    }
                    break;
                default:    // DT_CSBK
                    DEBUG2("DMRDMORX::databit() csbk found pos", m_syncPtr);
                    writeRSSIData(frame);
                    reset();
                    break;
                }
            }
        }
        else if (m_control == CONTROL_VOICE) {
            // Voice sync
            DEBUG2("DMRDMORX::databit() voice sync found pos", m_syncPtr);
            writeRSSIData(frame);

            m_state = DMORXS_VOICE;
            m_syncCount = 0U;
            m_n = 0U;
        }
        else {
            if (m_state != DMORXS_NONE) {
                m_syncCount++;
                if (m_syncCount >= MAX_SYNC_LOST_FRAMES) {
                    DEBUG1("DMRDMORX::databit() sync timeout, lost lock");
                    serial.writeDMRLost(true);
                    reset();
                }
            }

            if (m_state == DMORXS_VOICE) {
                if (m_n >= 5U) {
                    frame[0U] = CONTROL_VOICE;
                    m_n = 0U;
                }
                else {
                    frame[0U] = ++m_n;
                }
                serial.writeDMRData(true, frame, DMR_FRAME_LENGTH_BYTES + 1U);
            }
            else if (m_state == DMORXS_DATA) {
                if (m_type != 0x00U) {
                    frame[0U] = CONTROL_DATA | m_type;
                    writeRSSIData(frame);
                }
            }
        }

        // End of this slot, reset some items for the next slot.
        m_control = CONTROL_NONE;
    }

    m_dataPtr++;

    if (m_dataPtr >= DMO_BUFFER_LENGTH_BITS)
        m_dataPtr = 0U;

    io.setDecode(m_state != DMORXS_NONE);
}

/* Sets the DMR color code. */

void DMRDMORX::setColorCode(uint8_t colorCode)
{
    m_colorCode = colorCode;
}

// ---------------------------------------------------------------------------
//  Private Class Members
// ---------------------------------------------------------------------------

/* Frame synchronization correlator. */

void DMRDMORX::correlateSync()
{
    // unpack sync bytes
    uint8_t sync[DMR_SYNC_BYTES_LENGTH];
    sync[0U] = (uint8_t)((m_bitBuffer >> 48) & 0xFFU);
    sync[1U] = (uint8_t)((m_bitBuffer >> 40) & 0xFFU);
    sync[2U] = (uint8_t)((m_bitBuffer >> 32) & 0xFFU);
    sync[3U] = (uint8_t)((m_bitBuffer >> 24) & 0xFFU);
    sync[4U] = (uint8_t)((m_bitBuffer >> 16) & 0xFFU);
    sync[5U] = (uint8_t)((m_bitBuffer >> 8) & 0xFFU);
    sync[6U] = (uint8_t)((m_bitBuffer >> 0) & 0xFFU);

    if ((countBits64((m_bitBuffer & DMR_SYNC_BITS_MASK) ^ DMR_MS_DATA_SYNC_BITS) <= MAX_SYNC_BYTES_ERRS) || 
        (countBits64((m_bitBuffer & DMR_SYNC_BITS_MASK) ^ DMR_S2_DATA_SYNC_BITS) <= MAX_SYNC_BYTES_ERRS)) {
        uint8_t errs = 0U;
        for (uint8_t i = 0U; i < DMR_SYNC_BYTES_LENGTH; i++)
            errs += countBits8((sync[i] & DMR_SYNC_BYTES_MASK[i]) ^ DMR_MS_DATA_SYNC_BYTES[i]);

        DEBUG2("DMRDMORX::correlateSync() sync errs", errs);

        DEBUG4("DMRDMORX::correlateSync() sync [b0 - b2]", sync[0], sync[1], sync[2]);
        DEBUG4("DMRDMORX::correlateSync() sync [b3 - b5]", sync[3], sync[4], sync[5]);
        DEBUG2("DMRDMORX::correlateSync() sync [b6]", sync[6]);

        m_control = CONTROL_DATA;
        m_syncPtr = m_dataPtr;

        m_startPtr = m_dataPtr + DMO_BUFFER_LENGTH_BITS - DMR_SLOT_TYPE_LENGTH_BITS / 2U - DMR_INFO_LENGTH_BITS / 2U - DMR_SYNC_LENGTH_BITS + 1;
        if (m_startPtr >= DMO_BUFFER_LENGTH_BITS)
            m_startPtr -= DMO_BUFFER_LENGTH_BITS;

        m_endPtr = m_dataPtr + DMR_SLOT_TYPE_LENGTH_BITS / 2U + DMR_INFO_LENGTH_BITS / 2U;
        if (m_endPtr >= DMO_BUFFER_LENGTH_BITS)
            m_endPtr -= DMO_BUFFER_LENGTH_BITS;

        DEBUG4("DMRDMORX::correlateSync() dataPtr/startPtr/endPtr", m_dataPtr, m_startPtr, m_endPtr);
    } else if ((countBits64((m_bitBuffer & DMR_SYNC_BITS_MASK) ^ DMR_MS_VOICE_SYNC_BITS) <= MAX_SYNC_BYTES_ERRS) || 
        (countBits64((m_bitBuffer & DMR_SYNC_BITS_MASK) ^ DMR_S2_VOICE_SYNC_BITS) <= MAX_SYNC_BYTES_ERRS)) {
        uint8_t errs = 0U;
        for (uint8_t i = 0U; i < DMR_SYNC_BYTES_LENGTH; i++)
            errs += countBits8((sync[i] & DMR_SYNC_BYTES_MASK[i]) ^ DMR_MS_VOICE_SYNC_BYTES[i]);

        DEBUG2("DMRDMORX::correlateSync() correlateSync errs", errs);

        DEBUG4("DMRDMORX::correlateSync() sync [b0 - b2]", sync[0], sync[1], sync[2]);
        DEBUG4("DMRDMORX::correlateSync() sync [b3 - b5]", sync[3], sync[4], sync[5]);
        DEBUG2("DMRDMORX::correlateSync() sync [b6]", sync[6]);

        m_control  = CONTROL_VOICE;
        m_syncPtr  = m_dataPtr;

        m_startPtr = m_dataPtr + DMO_BUFFER_LENGTH_BITS - DMR_SLOT_TYPE_LENGTH_BITS / 2U - DMR_INFO_LENGTH_BITS / 2U - DMR_SYNC_LENGTH_BITS + 1;
        if (m_startPtr >= DMO_BUFFER_LENGTH_BITS)
            m_startPtr -= DMO_BUFFER_LENGTH_BITS;

        m_endPtr   = m_dataPtr + DMR_SLOT_TYPE_LENGTH_BITS / 2U + DMR_INFO_LENGTH_BITS / 2U;
        if (m_endPtr >= DMO_BUFFER_LENGTH_BITS)
            m_endPtr -= DMO_BUFFER_LENGTH_BITS;

        DEBUG4("DMRDMORX::correlateSync() dataPtr/startPtr/endPtr", m_dataPtr, m_startPtr, m_endPtr);
    }    
}

/* */

void DMRDMORX::bitsToBytes(uint16_t start, uint8_t count, uint8_t* buffer)
{
    for (uint8_t i = 0U; i < count; i++) {
        buffer[i] = 0U;
        buffer[i] |= _READ_BIT(m_buffer, start) << 7;
        start++;
        if (start >= DMO_BUFFER_LENGTH_BITS)
            start -= DMO_BUFFER_LENGTH_BITS;
        
        buffer[i] |= _READ_BIT(m_buffer, start) << 6;
        start++;
        if (start >= DMO_BUFFER_LENGTH_BITS)
            start -= DMO_BUFFER_LENGTH_BITS;
        
        buffer[i] |= _READ_BIT(m_buffer, start) << 5;
        start++;
        if (start >= DMO_BUFFER_LENGTH_BITS)
            start -= DMO_BUFFER_LENGTH_BITS;
        
        buffer[i] |= _READ_BIT(m_buffer, start) << 4;
        start++;
        if (start >= DMO_BUFFER_LENGTH_BITS)
            start -= DMO_BUFFER_LENGTH_BITS;
        
        buffer[i] |= _READ_BIT(m_buffer, start) << 3;
        start++;
        if (start >= DMO_BUFFER_LENGTH_BITS)
            start -= DMO_BUFFER_LENGTH_BITS;
        
        buffer[i] |= _READ_BIT(m_buffer, start) << 2;
        start++;
        if (start >= DMO_BUFFER_LENGTH_BITS)
            start -= DMO_BUFFER_LENGTH_BITS;
        
        buffer[i] |= _READ_BIT(m_buffer, start) << 1;
        start++;
        if (start >= DMO_BUFFER_LENGTH_BITS)
            start -= DMO_BUFFER_LENGTH_BITS;

        buffer[i] |= _READ_BIT(m_buffer, start) << 0;
        start++;
        if (start >= DMO_BUFFER_LENGTH_BITS)
            start -= DMO_BUFFER_LENGTH_BITS;
    }
}

/* */

void DMRDMORX::writeRSSIData(uint8_t* frame)
{
#if defined(SEND_RSSI_DATA)
    uint16_t rssi = io.readRSSI();

    frame[34U] = (rssi >> 8) & 0xFFU;
    frame[35U] = (rssi >> 0) & 0xFFU;

    serial.writeDMRData(true, frame, DMR_FRAME_LENGTH_BYTES + 3U);
#else
    serial.writeDMRData(true, frame, DMR_FRAME_LENGTH_BYTES + 1U);
#endif
}
