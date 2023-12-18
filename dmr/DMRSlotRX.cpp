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
*   Copyright (C) 2009-2017 by Jonathan Naylor G4KLX
*   Copyright (C) 2017,2018 by Andy Uribe CA6JAU
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
#include "dmr/DMRSlotRX.h"
#include "dmr/DMRSlotType.h"
#include "Utils.h"

using namespace dmr;

#if defined(DUPLEX)

// ---------------------------------------------------------------------------
//  Constants
// ---------------------------------------------------------------------------

const uint8_t MAX_SYNC_BYTES_ERRS = 3U;

const uint8_t MAX_SYNC_LOST_FRAMES = 13U;

const uint16_t NOENDPTR = 9999U;

const uint8_t CONTROL_NONE = 0x00U;
const uint8_t CONTROL_VOICE = 0x20U;
const uint8_t CONTROL_DATA = 0x40U;

// ---------------------------------------------------------------------------
//  Public Class Members
// ---------------------------------------------------------------------------

/// <summary>
/// Initializes a new instance of the DMRSlotRX class.
/// </summary>
DMRSlotRX::DMRSlotRX(bool slot) :
    m_slot(slot),
    m_bitBuffer(0x00U),
    m_buffer(),
    m_dataPtr(0U),
    m_syncPtr(0U),
    m_startPtr(0U),
    m_endPtr(NOENDPTR),
    m_delayPtr(0U),
    m_control(CONTROL_NONE),
    m_syncCount(0U),
    m_colorCode(0U),
    m_delay(0U),
    m_state(DMRRXS_NONE),
    m_n(0U),
    m_type(0U)
{
    /* stub */
}

/// <summary>
/// Helper to set data values for start of Rx.
/// </summary>
void DMRSlotRX::start()
{
    m_dataPtr = 0U;
    m_delayPtr = 0U;
    m_control = CONTROL_NONE;
}

/// <summary>
/// Helper to reset data values to defaults.
/// </summary>
void DMRSlotRX::reset()
{
    m_dataPtr = 0U;
    m_delayPtr = 0U;

    m_bitBuffer = 0U;

    resetSlot();
}

/// <summary>
/// Sample DMR bits from the air interface.
/// </summary>
/// <param name="bit"></param>
bool DMRSlotRX::databit(bool bit)
{
    uint16_t min, max;

    m_delayPtr++;
    if (m_delayPtr < m_delay)
        return m_state != DMRRXS_NONE;

    _WRITE_BIT(m_buffer, m_dataPtr, bit);

    m_bitBuffer <<= 1;
    if (bit)
        m_bitBuffer |= 0x01U;

    // Ensure that the buffer doesn't overflow
    if (m_dataPtr > m_endPtr || m_dataPtr >= 576U)
        return m_state != DMRRXS_NONE;

    if (m_state == DMRRXS_NONE) {
        correlateSync();
    }
    else {
        uint16_t min = m_syncPtr + DMR_BUFFER_LENGTH_BITS - 2U;
        uint16_t max = m_syncPtr + 2U;

        if (min >= DMR_BUFFER_LENGTH_BITS)
            min -= DMR_BUFFER_LENGTH_BITS;
        if (max >= DMR_BUFFER_LENGTH_BITS)
            max -= DMR_BUFFER_LENGTH_BITS;

        if (min < max) {
            if (m_dataPtr >= min && m_dataPtr <= max)
                correlateSync();
        } else {
            if (m_dataPtr >= min || m_dataPtr <= max)
                correlateSync();
        }        
    }

    if (m_dataPtr == m_endPtr) {
        uint8_t frame[DMR_FRAME_LENGTH_BYTES + 3U];
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
                    DEBUG3("DMRSlotRX: databit(): data header found slot/pos", m_slot ? 2U : 1U, m_syncPtr);
                    writeRSSIData(frame);
                    m_state = DMRRXS_DATA;
                    m_type = 0x00U;
                    break;
                case DT_RATE_12_DATA:
                case DT_RATE_34_DATA:
                case DT_RATE_1_DATA:
                    if (m_state == DMRRXS_DATA) {
                        DEBUG3("DMRSlotRX: databit(): data payload found slot/pos", m_slot ? 2U : 1U, m_syncPtr);
                        writeRSSIData(frame);
                        m_type = dataType;
                    }
                    break;
                case DT_VOICE_LC_HEADER:
                    DEBUG3("DMRSlotRX: databit(): voice header found slot/pos", m_slot ? 2U : 1U, m_syncPtr);
                    writeRSSIData(frame);
                    m_state = DMRRXS_VOICE;
                    break;
                case DT_VOICE_PI_HEADER:
                    if (m_state == DMRRXS_VOICE) {
                        DEBUG3("DMRSlotRX: databit(): voice pi header found slot/pos", m_slot ? 2U : 1U, m_syncPtr);
                        writeRSSIData(frame);
                    }
                    m_state = DMRRXS_VOICE;
                    break;
                case DT_TERMINATOR_WITH_LC:
                    if (m_state == DMRRXS_VOICE) {
                        DEBUG3("DMRSlotRX: databit(): voice terminator found slot/pos", m_slot ? 2U : 1U, m_syncPtr);
                        writeRSSIData(frame);
                        m_state = DMRRXS_NONE;
                        m_endPtr = NOENDPTR;
                    }
                    break;
                default:    // DT_CSBK
                    DEBUG3("DMRSlotRX: databit(): csbk found slot/pos", m_slot ? 2U : 1U, m_syncPtr);
                    writeRSSIData(frame);
                    m_state = DMRRXS_NONE;
                    m_endPtr = NOENDPTR;
                    break;
                }
            }
        }
        else if (m_control == CONTROL_VOICE) {
            // Voice sync
            DEBUG3("DMRSlotRX: databit(): voice sync found slot/pos", m_slot ? 2U : 1U, m_syncPtr);
            writeRSSIData(frame);
            m_state = DMRRXS_VOICE;
            m_syncCount = 0U;
            m_n = 0U;
        }
        else {
            if (m_state != DMRRXS_NONE) {
                m_syncCount++;
                if (m_syncCount >= MAX_SYNC_LOST_FRAMES) {
                    DEBUG1("DMRSlotRX: databit(): sync timeout, lost lock");
                    serial.writeDMRLost(m_slot);
                    resetSlot();
                }
            }

            if (m_state == DMRRXS_VOICE) {
                if (m_n >= 5U) {
                    frame[0U] = CONTROL_VOICE;
                    m_n = 0U;
                }
                else {
                    frame[0U] = ++m_n;
                }

                serial.writeDMRData(m_slot, frame, DMR_FRAME_LENGTH_BYTES + 1U);
            }
            else if (m_state == DMRRXS_DATA) {
                if (m_type != 0x00U) {
                    frame[0U] = CONTROL_DATA | m_type;
                    writeRSSIData(frame);
                }
            }
        }

        // end of this slot, reset some items for the next slot
        m_control = CONTROL_NONE;
    }

    m_dataPtr++;
    if (m_dataPtr >= DMR_BUFFER_LENGTH_BITS)
        m_dataPtr = 0U;

    return m_state != DMRRXS_NONE;
}

/// <summary>
/// Sets the DMR color code.
/// </summary>
/// <param name="colorCode">Color code.</param>
void DMRSlotRX::setColorCode(uint8_t colorCode)
{
    m_colorCode = colorCode;
}

/// <summary>
/// Sets the number of samples to delay before processing.
/// </summary>
/// <param name="delay">Number of samples to delay.</param>
void DMRSlotRX::setRxDelay(uint8_t delay)
{
    m_delay = delay;
}

// ---------------------------------------------------------------------------
//  Private Class Members
// ---------------------------------------------------------------------------

/// <summary>
/// Frame synchronization correlator.
/// </summary>
/// <param name="first"></param>
void DMRSlotRX::correlateSync()
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

    if (countBits64((m_bitBuffer & DMR_SYNC_BITS_MASK) ^ DMR_MS_DATA_SYNC_BITS) <= MAX_SYNC_BYTES_ERRS) {
        uint8_t errs = 0U;
        for (uint8_t i = 0U; i < DMR_SYNC_BYTES_LENGTH; i++)
            errs += countBits8((sync[i] & DMR_SYNC_BYTES_MASK[i]) ^ DMR_MS_DATA_SYNC_BYTES[i]);

        DEBUG2("DMRSlotRX: correlateSync(): correlateSync errs", errs);

        DEBUG4("DMRSlotRX: correlateSync(): sync [b0 - b2]", sync[0], sync[1], sync[2]);
        DEBUG4("DMRSlotRX: correlateSync(): sync [b3 - b5]", sync[3], sync[4], sync[5]);
        DEBUG2("DMRSlotRX: correlateSync(): sync [b6]", sync[6]);

        m_control = CONTROL_DATA;
        m_syncPtr = m_dataPtr;

        m_startPtr = m_dataPtr + DMR_BUFFER_LENGTH_BITS - DMR_SLOT_TYPE_LENGTH_BITS / 2U - DMR_INFO_LENGTH_BITS / 2U - DMR_SYNC_LENGTH_BITS + 1;
        if (m_startPtr >= DMR_BUFFER_LENGTH_BITS)
            m_startPtr -= DMR_BUFFER_LENGTH_BITS;

        m_endPtr = m_dataPtr + DMR_SLOT_TYPE_LENGTH_BITS / 2U + DMR_INFO_LENGTH_BITS / 2U;
        if (m_endPtr >= DMR_BUFFER_LENGTH_BITS)
            m_endPtr -= DMR_BUFFER_LENGTH_BITS;

        DEBUG4("DMRSlotRX: correlateSync(): dataPtr/startPtr/endPtr", m_dataPtr, m_startPtr, m_endPtr);
    } else if (countBits64((m_bitBuffer & DMR_SYNC_BITS_MASK) ^ DMR_MS_VOICE_SYNC_BITS) <= MAX_SYNC_BYTES_ERRS) {
        uint8_t errs = 0U;
        for (uint8_t i = 0U; i < DMR_SYNC_BYTES_LENGTH; i++)
            errs += countBits8((sync[i] & DMR_SYNC_BYTES_MASK[i]) ^ DMR_MS_VOICE_SYNC_BYTES[i]);

        DEBUG2("DMRSlotRX: correlateSync(): correlateSync errs", errs);

        DEBUG4("DMRSlotRX: correlateSync(): sync [b0 - b2]", sync[0], sync[1], sync[2]);
        DEBUG4("DMRSlotRX: correlateSync(): sync [b3 - b5]", sync[3], sync[4], sync[5]);
        DEBUG2("DMRSlotRX: correlateSync(): sync [b6]", sync[6]);

        m_control = CONTROL_VOICE;
        m_syncPtr = m_dataPtr;

        m_startPtr = m_dataPtr + DMR_BUFFER_LENGTH_BITS - DMR_SLOT_TYPE_LENGTH_BITS / 2U - DMR_INFO_LENGTH_BITS / 2U - DMR_SYNC_LENGTH_BITS + 1;
        if (m_startPtr >= DMR_BUFFER_LENGTH_BITS)
            m_startPtr -= DMR_BUFFER_LENGTH_BITS;

        m_endPtr = m_dataPtr + DMR_SLOT_TYPE_LENGTH_BITS / 2U + DMR_INFO_LENGTH_BITS / 2U;
        if (m_endPtr >= DMR_BUFFER_LENGTH_BITS)
            m_endPtr -= DMR_BUFFER_LENGTH_BITS;

        DEBUG4("DMRSlotRX: correlateSync(): dataPtr/startPtr/endPtr", m_dataPtr, m_startPtr, m_endPtr);
    }
}

/// <summary>
/// 
/// </summary>
void DMRSlotRX::resetSlot()
{
    m_syncPtr = 0U;

    m_control = CONTROL_NONE;
    m_syncCount = 0U;
    m_state = DMRRXS_NONE;
    m_startPtr = 0U;
    m_endPtr = NOENDPTR;
    m_type = 0U;
    m_n = 0U;
}

/// <summary>
/// 
/// </summary>
/// <param name="start"></param>
/// <param name="count"></param>
/// <param name="buffer"></param>
void DMRSlotRX::bitsToBytes(uint16_t start, uint8_t count, uint8_t* buffer)
{
    for (uint8_t i = 0U; i < count; i++) {
        buffer[i] = 0U;
        buffer[i] |= _READ_BIT(m_buffer, start) << 7;
        start++;
        if (start >= DMR_BUFFER_LENGTH_BITS)
            start -= DMR_BUFFER_LENGTH_BITS;

        buffer[i] |= _READ_BIT(m_buffer, start) << 6;
        start++;
        if (start >= DMR_BUFFER_LENGTH_BITS)
            start -= DMR_BUFFER_LENGTH_BITS;

        buffer[i] |= _READ_BIT(m_buffer, start) << 5;
        start++;
        if (start >= DMR_BUFFER_LENGTH_BITS)
            start -= DMR_BUFFER_LENGTH_BITS;

        buffer[i] |= _READ_BIT(m_buffer, start) << 4;
        start++;
        if (start >= DMR_BUFFER_LENGTH_BITS)
            start -= DMR_BUFFER_LENGTH_BITS;

        buffer[i] |= _READ_BIT(m_buffer, start) << 3;
        start++;
        if (start >= DMR_BUFFER_LENGTH_BITS)
            start -= DMR_BUFFER_LENGTH_BITS;

        buffer[i] |= _READ_BIT(m_buffer, start) << 2;
        start++;
        if (start >= DMR_BUFFER_LENGTH_BITS)
            start -= DMR_BUFFER_LENGTH_BITS;

        buffer[i] |= _READ_BIT(m_buffer, start) << 1;
        start++;
        if (start >= DMR_BUFFER_LENGTH_BITS)
            start -= DMR_BUFFER_LENGTH_BITS;

        buffer[i] |= _READ_BIT(m_buffer, start) << 0;
        start++;
        if (start >= DMR_BUFFER_LENGTH_BITS)
            start -= DMR_BUFFER_LENGTH_BITS;
    }
}

/// <summary>
///
/// </summary>
/// <param name="frame"></param>
void DMRSlotRX::writeRSSIData(uint8_t* frame)
{
#if defined(SEND_RSSI_DATA)
    uint16_t rssi = io.readRSSI();

    frame[34U] = (rssi >> 8) & 0xFFU;
    frame[35U] = (rssi >> 0) & 0xFFU;

    serial.writeDMRData(m_slot, frame, DMR_FRAME_LENGTH_BYTES + 3U);
#else
    serial.writeDMRData(m_slot, frame, DMR_FRAME_LENGTH_BYTES + 1U);
#endif
}

#endif // DUPLEX
