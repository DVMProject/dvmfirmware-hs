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
*   Copyright (C) 2013,2015,2016,2018,2020,2021 Jonathan Naylor, G4KLX
*   Copyright (C) 2016 Colin Durbridge, G4EML
*   Copyright (C) 2016,2017,2018,2019 Andy Uribe, CA6JAU
*   Copyright (C) 2019 Florian Wolters, DF2ET
*   Copyright (C) 2017-2024 Bryan Biedenkapp, N2PLL
*
*/
#include "Globals.h"
#include "SerialPort.h"

 // ---------------------------------------------------------------------------
 //  Constants
 // ---------------------------------------------------------------------------

#define concat(a, b, c) a " (build " b " " c ")"
const char HARDWARE[] = concat(DESCRIPTION, __TIME__, __DATE__);

const uint8_t PROTOCOL_VERSION   = 4U;

// ---------------------------------------------------------------------------
//  Public Class Members
// ---------------------------------------------------------------------------

/// <summary>
/// Initializes a new instance of the SerialPort class.
/// </summary>
SerialPort::SerialPort() :
    m_buffer(),
    m_ptr(0U),
    m_len(0U),
    m_dblFrame(false),
    m_debug(false)
{
    // stub
}

/// <summary>
/// Starts serial port communications.
/// </summary>
void SerialPort::start()
{
    beginInt(1U, SERIAL_SPEED);
}

/// <summary>
/// Process data from serial port.
/// </summary>
void SerialPort::process()
{
    while (availableInt(1U)) {
        uint8_t c = readInt(1U);

        if (m_ptr == 0U) {
            if (c == DVM_SHORT_FRAME_START) {
                // Handle the frame start correctly
                m_buffer[0U] = c;
                m_ptr = 1U;
                m_len = 0U;
                m_dblFrame = false;
            }
            else if (c == DVM_LONG_FRAME_START) {
                // Handle the frame start correctly
                m_buffer[0U] = c;
                m_ptr = 1U;
                m_len = 0U;
                m_dblFrame = true;
            }
            else {
                m_ptr = 0U;
                m_len = 0U;
            }
        }
        else if (m_ptr == 1U) {
            // Handle the frame length
            m_len = m_buffer[m_ptr] = c;
            m_ptr = 2U;
        }
        else if (m_ptr == 2U && m_dblFrame) {
            // Handle the frame length
            m_len = m_buffer[m_ptr] = (c + 255U);
            m_ptr = 3U;
        }
        else {
            // Any other bytes are added to the buffer
            m_buffer[m_ptr] = c;
            m_ptr++;

            // The full packet has been received, process it
            if (m_ptr == m_len) {
                uint8_t err = 2U;

                switch (m_buffer[2U]) {
                case CMD_GET_STATUS:
                    getStatus();
                    break;

                case CMD_GET_VERSION:
                    getVersion();
                    break;

                case CMD_SET_CONFIG:
                    err = setConfig(m_buffer + 3U, m_len - 3U);
                    if (err == RSN_OK)
                        sendACK();
                    else
                        sendNAK(err);
                    break;

                case CMD_SET_MODE:
                    err = setMode(m_buffer + 3U, m_len - 3U);
                    if (err == RSN_OK)
                        sendACK();
                    else
                        sendNAK(err);
                    break;

                case CMD_SET_SYMLVLADJ:
                    sendACK(); // CMD_SET_RXLEVEL not supported by HS
                    break;

                case CMD_SET_RXLEVEL:
                    sendACK(); // CMD_SET_RXLEVEL not supported by HS
                    break;

                case CMD_SET_RFPARAMS:
                    err = setRFParams(m_buffer + 3U, m_len - 3U);
                    if (err == RSN_OK)
                        sendACK();
                    else
                        sendNAK(err);
                    break;

                case CMD_CAL_DATA:
                    if (m_modemState == STATE_DMR_DMO_CAL_1K || m_modemState == STATE_DMR_CAL_1K ||
                        m_modemState == STATE_DMR_LF_CAL || m_modemState == STATE_DMR_CAL)
                        err = calDMR.write(m_buffer + 3U, m_len - 3U);
                    if (m_modemState == STATE_P25_CAL_1K || m_modemState == STATE_P25_CAL)
                        err = calP25.write(m_buffer + 3U, m_len - 3U);
                    if (m_modemState == STATE_NXDN_CAL)
                        err = calNXDN.write(m_buffer + 3U, m_len - 3U);
                    if (err == RSN_OK)
                    {
                        sendACK();
                    }
                    else {
                        DEBUG2("SerialPort::process() received invalid calibration data", err);
                        sendNAK(err);
                    }
                    break;

                case CMD_FLSH_READ:
                    flashRead();
                    break;

                case CMD_FLSH_WRITE:
                    err = flashWrite(m_buffer + 3U, m_len - 3U);
                    if (err == RSN_OK) {
                        sendACK();
                    }
                    else {
                        DEBUG2("SerialPort::process() received invalid data to write to flash", err);
                        sendNAK(err);
                    }
                    break;

                case CMD_SET_BUFFERS:
                    err = setBuffers(m_buffer + 3U, m_len - 3U);
                    if (err == RSN_OK) {
                        sendACK();
                    }
                    else {
                        DEBUG2("SerialPort::process() received invalid data to set buffers", err);
                        sendNAK(err);
                    }
                    break;

                /** CW */
                case CMD_SEND_CWID:
                    err = RSN_RINGBUFF_FULL;
                    if (m_modemState == STATE_IDLE) {
                        m_cwIdState = true;
                        
                        DEBUG2("SerialPort::process() setting modem state", STATE_CW);
                        io.rf1Conf(STATE_CW, true);
                        
                        err = cwIdTX.write(m_buffer + 3U, m_len - 3U);
                    }
                    if (err != RSN_OK) {
                        DEBUG2("SerialPort::process() invalid CW Id data", err);
                        sendNAK(err);
                    }
                    break;

                /** Digital Mobile Radio */
                case CMD_DMR_DATA1:
#if defined(DUPLEX)
                    if (m_dmrEnable) {
                        if (m_modemState == STATE_IDLE || m_modemState == STATE_DMR) {
                            if (m_duplex)
                                err = dmrTX.writeData1(m_buffer + 3U, m_len - 3U);
                        }
                    }
                    if (err == RSN_OK) {
                        if (m_modemState == STATE_IDLE)
                            setMode(STATE_DMR);
                    }
                    else {
                        DEBUG2("SerialPort::process() received invalid DMR data", err);
                        sendNAK(err);
                    }
#else
                    sendNAK(RSN_INVALID_REQUEST);
#endif
                    break;

                case CMD_DMR_DATA2:
                    if (m_dmrEnable) {
                        if (m_modemState == STATE_IDLE || m_modemState == STATE_DMR) {
#if defined(DUPLEX)
                            if (m_duplex)
                                err = dmrTX.writeData2(m_buffer + 3U, m_len - 3U);
                            else
                                err = dmrDMOTX.writeData(m_buffer + 3U, m_len - 3U);
#else
                            err = dmrDMOTX.writeData(m_buffer + 3U, m_len - 3U);
#endif
                        }
                    }
                    if (err == RSN_OK) {
                        if (m_modemState == STATE_IDLE)
                            setMode(STATE_DMR);
                    }
                    else {
                        DEBUG2("SerialPort::process() received invalid DMR data", err);
                        sendNAK(err);
                    }
                    break;

                case CMD_DMR_START:
#if defined(DUPLEX)
                    if (m_dmrEnable) {
                        err = RSN_INVALID_DMR_START;
                        if (m_len == 4U) {
                            if (m_buffer[3U] == 0x01U && m_modemState == STATE_DMR) {
                                if (!m_tx)
                                    dmrTX.setStart(true);
                                err = RSN_OK;
                            }
                            else if (m_buffer[3U] == 0x00U && m_modemState == STATE_DMR) {
                                if (m_tx)
                                    dmrTX.setStart(false);
                                err = RSN_OK;
                            }
                        }
                    }
                    if (err != RSN_OK) {
                        DEBUG3("SerialPort::process() received invalid DMR start", err, m_len);
                        sendNAK(err);
                    }
#else
                    sendNAK(RSN_INVALID_REQUEST);
#endif
                    break;

                case CMD_DMR_SHORTLC:
#if defined(DUPLEX)
                    if (m_dmrEnable)
                        err = dmrTX.writeShortLC(m_buffer + 3U, m_len - 3U);
                    if (err != RSN_OK) {
                        DEBUG2("SerialPort::process() received invalid DMR Short LC", err);
                        sendNAK(err);
                    }
#else
                    sendNAK(RSN_INVALID_REQUEST);
#endif
                    break;

                case CMD_DMR_ABORT:
#if defined(DUPLEX)
                    if (m_dmrEnable)
                        err = dmrTX.writeAbort(m_buffer + 3U, m_len - 3U);
                    if (err != RSN_OK) {
                        DEBUG2("SerialPort::process() received invalid DMR Abort", err);
                        sendNAK(err);
                    }
#else
                    sendNAK(RSN_INVALID_REQUEST);
#endif
                    break;

                case CMD_DMR_CACH_AT_CTRL:
#if defined(DUPLEX)
                    if (m_dmrEnable) {
                        err = RSN_INVALID_REQUEST;
                        if (m_len == 4U) {
                            dmrTX.setIgnoreCACH_AT(m_buffer[3U]);
                            err = RSN_OK;
                        }
                    }
                    if (err != RSN_OK) {
                        DEBUG2("SerialPort::process() received invalid DMR CACH AT Control", err);
                        sendNAK(err);
                    }
#else
                    sendNAK(RSN_INVALID_REQUEST);
#endif
                    break;

                case CMD_DMR_CLEAR1:
#if defined(DUPLEX)
                    if (m_dmrEnable) {
                        if (m_modemState == STATE_IDLE || m_modemState == STATE_P25)
                            dmrTX.resetFifo1();
                    }
#else
                    sendNAK(RSN_INVALID_REQUEST);
#endif
                    break;
                case CMD_DMR_CLEAR2:
#if defined(DUPLEX)
                    if (m_dmrEnable) {
                        if (m_modemState == STATE_IDLE || m_modemState == STATE_P25)
                            dmrTX.resetFifo2();
                    }
#else
                    sendNAK(RSN_INVALID_REQUEST);
#endif
                    break;


                /** Project 25 */
                case CMD_P25_DATA:
                    if (m_p25Enable) {
                        if (m_modemState == STATE_IDLE || m_modemState == STATE_P25)
                            err = p25TX.writeData(m_buffer + 3U, m_len - 3U);
                    }
                    if (err == RSN_OK) {
                        if (m_modemState == STATE_IDLE)
                            setMode(STATE_P25);
                    }
                    else {
                        DEBUG2("SerialPort::process() received invalid P25 data", err);
                        sendNAK(err);
                    }
                    break;

                case CMD_P25_CLEAR:
                    if (m_p25Enable) {
                        if (m_modemState == STATE_IDLE || m_modemState == STATE_P25)
                            p25TX.clear();
                    }
                    break;

                /** Next Generation Digital Narrowband */
                case CMD_NXDN_DATA:
                    if (m_nxdnEnable) {
                        if (m_modemState == STATE_IDLE || m_modemState == STATE_NXDN)
                            err = nxdnTX.writeData(m_buffer + 3U, m_len - 3U);
                    }
                    if (err == RSN_OK) {
                        if (m_modemState == STATE_IDLE)
                            setMode(STATE_NXDN);
                    }
                    else {
                        DEBUG2("SerialPort::process() received invalid NXDN data", err);
                        sendNAK(err);
                    }
                    break;
                case CMD_NXDN_CLEAR:
                    if (m_nxdnEnable) {
                        if (m_modemState == STATE_IDLE || m_modemState == STATE_P25)
                            nxdnTX.clear();
                    }
                    break;

                default:
                    // Handle this, send a NAK back
                    sendNAK(RSN_NAK);
                    break;
                }

                m_ptr = 0U;
                m_len = 0U;
                m_dblFrame = false;
            }
        }
    }

    if (io.getWatchdog() >= 48000U) {
        m_ptr = 0U;
        m_len = 0U;
        m_dblFrame = false;
    }
}

/// <summary>
/// Helper to check if the modem is in a calibration state.
/// </summary>
/// <param name="state"></param>
/// <returns></returns>
bool SerialPort::isCalState(DVM_STATE state)
{
    // calibration mode check
    if (state == STATE_P25_CAL_1K ||
        state == STATE_DMR_DMO_CAL_1K || state == STATE_DMR_CAL_1K ||
        state == STATE_DMR_LF_CAL ||
        state == STATE_RSSI_CAL ||
        state == STATE_P25_CAL || state == STATE_DMR_CAL || state == STATE_NXDN_CAL ||
        state == STATE_INT_CAL) {
        return true;
    }
    
    return false;
}

/// <summary>
/// Helper to determine digital mode if the modem is in a calibration state.
/// </summary>
/// <param name="state"></param>
/// <returns></returns>
DVM_STATE SerialPort::calRelativeState(DVM_STATE state)
{
    if (isCalState(state)) {
        if (state == STATE_DMR_DMO_CAL_1K || state == STATE_DMR_CAL_1K ||
            state == STATE_DMR_LF_CAL || state == STATE_DMR_CAL ||
            state == STATE_RSSI_CAL || state == STATE_INT_CAL) {
            return STATE_DMR;
        } else if (state == STATE_P25_CAL_1K ||
            state == STATE_P25_CAL) {
            return STATE_P25;
        } else if (state == STATE_NXDN_CAL) {
            return STATE_NXDN;
        }
    }
    
    return STATE_CW;
}

/// <summary>
/// Write DMR frame data to serial port.
/// </summary>
/// <param name="slot"></param>
/// <param name="data"></param>
/// <param name="length"></param>
void SerialPort::writeDMRData(bool slot, const uint8_t* data, uint8_t length)
{
    if (m_modemState != STATE_DMR && m_modemState != STATE_IDLE)
        return;

    if (!m_dmrEnable)
        return;

    if (length + 3U > 40U) {
        m_buffer[2U] = slot ? CMD_DMR_DATA2 : CMD_DMR_DATA1;
        sendNAK(RSN_ILLEGAL_LENGTH);
        return;
    }

    uint8_t reply[40U];
    ::memset(reply, 0x00U, 40U);

    reply[0U] = DVM_SHORT_FRAME_START;
    reply[1U] = length + 3U;
    reply[2U] = slot ? CMD_DMR_DATA2 : CMD_DMR_DATA1;

    ::memcpy(reply + 3U, data, length);

    writeInt(1U, reply, length + 3U);
}

/// <summary>
/// Write lost DMR frame data to serial port.
/// </summary>
/// <param name="slot"></param>
void SerialPort::writeDMRLost(bool slot)
{
    if (m_modemState != STATE_DMR && m_modemState != STATE_IDLE)
        return;

    if (!m_dmrEnable)
        return;

    uint8_t reply[3U];

    reply[0U] = DVM_SHORT_FRAME_START;
    reply[1U] = 3U;
    reply[2U] = slot ? CMD_DMR_LOST2 : CMD_DMR_LOST1;

    writeInt(1U, reply, 3);
}

/// <summary>
/// Write P25 frame data to serial port.
/// </summary>
/// <param name="data"></param>
/// <param name="length"></param>
void SerialPort::writeP25Data(const uint8_t* data, uint16_t length)
{
    if (m_modemState != STATE_P25 && m_modemState != STATE_IDLE)
        return;

    if (!m_p25Enable)
        return;

    if (length + 4U > 520U) {
        m_buffer[2U] = CMD_P25_DATA;
        sendNAK(RSN_ILLEGAL_LENGTH);
        return;
    }

    uint8_t reply[520U];
    ::memset(reply, 0x00U, 520U);

    if (length < 252U) {
        reply[0U] = DVM_SHORT_FRAME_START;
        reply[1U] = length + 3U;
        reply[2U] = CMD_P25_DATA;
        ::memcpy(reply + 3U, data, length);

        writeInt(1U, reply, length + 3U);
    }
    else {
        length += 4U;
        reply[0U] = DVM_LONG_FRAME_START;
        reply[1U] = (length >> 8U) & 0xFFU;
        reply[2U] = (length & 0xFFU);
        reply[3U] = CMD_P25_DATA;
        ::memcpy(reply + 4U, data, length);

        writeInt(1U, reply, length + 4U);
    }
}

/// <summary>
/// Write lost P25 frame data to serial port.
/// </summary>
void SerialPort::writeP25Lost()
{
    if (m_modemState != STATE_P25 && m_modemState != STATE_IDLE)
        return;

    if (!m_p25Enable)
        return;

    uint8_t reply[3U];

    reply[0U] = DVM_SHORT_FRAME_START;
    reply[1U] = 3U;
    reply[2U] = CMD_P25_LOST;

    writeInt(1U, reply, 3);
}

/// <summary>
/// Write NXDN frame data to serial port.
/// </summary>
/// <param name="data"></param>
/// <param name="length"></param>
void SerialPort::writeNXDNData(const uint8_t* data, uint8_t length)
{
    if (m_modemState != STATE_NXDN && m_modemState != STATE_IDLE)
        return;

    if (!m_nxdnEnable)
        return;

    if (length + 3U > 130U) {
        m_buffer[2U] = CMD_NXDN_DATA;
        sendNAK(RSN_ILLEGAL_LENGTH);
        return;
    }

    uint8_t reply[130U];
    ::memset(reply, 0x00U, 130U);

    reply[0U] = DVM_SHORT_FRAME_START;
    reply[1U] = length + 3U;
    reply[2U] = CMD_NXDN_DATA;

    ::memcpy(reply + 3U, data, length);

    writeInt(1U, reply, length + 3U);
}

/// <summary>
/// Write lost NXDN frame data to serial port.
/// </summary>
void SerialPort::writeNXDNLost()
{
    if (m_modemState != STATE_NXDN && m_modemState != STATE_IDLE)
        return;

    if (!m_nxdnEnable)
        return;

    uint8_t reply[3U];

    reply[0U] = DVM_SHORT_FRAME_START;
    reply[1U] = 3U;
    reply[2U] = CMD_NXDN_LOST;

    writeInt(1U, reply, 3);
}

/// <summary>
/// Write calibration frame data to serial port.
/// </summary>
/// <param name="data"></param>
/// <param name="length"></param>
void SerialPort::writeCalData(const uint8_t* data, uint8_t length)
{
    if (length + 3U > 130U) {
        m_buffer[2U] = CMD_CAL_DATA;
        sendNAK(RSN_ILLEGAL_LENGTH);
        return;
    }

    uint8_t reply[130U];
    ::memset(reply, 0x00U, 130U);

    reply[0U] = DVM_SHORT_FRAME_START;
    reply[1U] = length + 3U;
    reply[2U] = CMD_CAL_DATA;

    ::memcpy(reply + 3U, data, length);

    writeInt(1U, reply, length + 3U);
}

/// <summary>
/// Write RSSI frame data to serial port.
/// </summary>
/// <param name="data"></param>
/// <param name="length"></param>
void SerialPort::writeRSSIData(const uint8_t* data, uint8_t length)
{
    if (m_modemState != STATE_RSSI_CAL)
        return;

    if (length + 3U > 30U) {
        m_buffer[2U] = CMD_RSSI_DATA;
        sendNAK(RSN_ILLEGAL_LENGTH);
        return;
    }

    uint8_t reply[30U];
    ::memset(reply, 0x00U, 30U);

    reply[0U] = DVM_SHORT_FRAME_START;
    reply[1U] = length + 3U;
    reply[2U] = CMD_RSSI_DATA;

    ::memcpy(reply + 3U, data, length);

    writeInt(1U, reply, length + 3U);
}

/// <summary>
///
/// </summary>
/// <param name="text"></param>
void SerialPort::writeDebug(const char* text)
{
    if (!m_debug)
        return;

    uint8_t reply[130U];
    ::memset(reply, 0x00U, 130U);

    reply[0U] = DVM_SHORT_FRAME_START;
    reply[1U] = 0U;
    reply[2U] = CMD_DEBUG1;

    uint8_t count = 3U;
    for (uint8_t i = 0U; text[i] != '\0'; i++, count++)
        reply[count] = text[i];

    reply[1U] = count;

    writeInt(1U, reply, count, true);
}

/// <summary>
///
/// </summary>
/// <param name="text"></param>
/// <param name="n1"></param>
void SerialPort::writeDebug(const char* text, int16_t n1)
{
    if (!m_debug)
        return;

    uint8_t reply[130U];
    ::memset(reply, 0x00U, 130U);

    reply[0U] = DVM_SHORT_FRAME_START;
    reply[1U] = 0U;
    reply[2U] = CMD_DEBUG2;

    uint8_t count = 3U;
    for (uint8_t i = 0U; text[i] != '\0'; i++, count++)
        reply[count] = text[i];

    reply[count++] = (n1 >> 8) & 0xFF;
    reply[count++] = (n1 >> 0) & 0xFF;

    reply[1U] = count;

    writeInt(1U, reply, count, true);
}

/// <summary>
///
/// </summary>
/// <param name="text"></param>
/// <param name="n1"></param>
/// <param name="n2"></param>
void SerialPort::writeDebug(const char* text, int16_t n1, int16_t n2)
{
    if (!m_debug)
        return;

    uint8_t reply[130U];
    ::memset(reply, 0x00U, 130U);

    reply[0U] = DVM_SHORT_FRAME_START;
    reply[1U] = 0U;
    reply[2U] = CMD_DEBUG3;

    uint8_t count = 3U;
    for (uint8_t i = 0U; text[i] != '\0'; i++, count++)
        reply[count] = text[i];

    reply[count++] = (n1 >> 8) & 0xFF;
    reply[count++] = (n1 >> 0) & 0xFF;

    reply[count++] = (n2 >> 8) & 0xFF;
    reply[count++] = (n2 >> 0) & 0xFF;

    reply[1U] = count;

    writeInt(1U, reply, count, true);
}

/// <summary>
///
/// </summary>
/// <param name="text"></param>
/// <param name="n1"></param>
/// <param name="n2"></param>
/// <param name="n3"></param>
void SerialPort::writeDebug(const char* text, int16_t n1, int16_t n2, int16_t n3)
{
    if (!m_debug)
        return;

    uint8_t reply[130U];
    ::memset(reply, 0x00U, 130U);

    reply[0U] = DVM_SHORT_FRAME_START;
    reply[1U] = 0U;
    reply[2U] = CMD_DEBUG4;

    uint8_t count = 3U;
    for (uint8_t i = 0U; text[i] != '\0'; i++, count++)
        reply[count] = text[i];

    reply[count++] = (n1 >> 8) & 0xFF;
    reply[count++] = (n1 >> 0) & 0xFF;

    reply[count++] = (n2 >> 8) & 0xFF;
    reply[count++] = (n2 >> 0) & 0xFF;

    reply[count++] = (n3 >> 8) & 0xFF;
    reply[count++] = (n3 >> 0) & 0xFF;

    reply[1U] = count;

    writeInt(1U, reply, count, true);
}

/// <summary>
///
/// </summary>
/// <param name="text"></param>
/// <param name="n1"></param>
/// <param name="n2"></param>
/// <param name="n3"></param>
/// <param name="n4"></param>
void SerialPort::writeDebug(const char* text, int16_t n1, int16_t n2, int16_t n3, int16_t n4)
{
    if (!m_debug)
        return;

    uint8_t reply[130U];
    ::memset(reply, 0x00U, 130U);

    reply[0U] = DVM_SHORT_FRAME_START;
    reply[1U] = 0U;
    reply[2U] = CMD_DEBUG5;

    uint8_t count = 3U;
    for (uint8_t i = 0U; text[i] != '\0'; i++, count++)
        reply[count] = text[i];

    reply[count++] = (n1 >> 8) & 0xFF;
    reply[count++] = (n1 >> 0) & 0xFF;

    reply[count++] = (n2 >> 8) & 0xFF;
    reply[count++] = (n2 >> 0) & 0xFF;

    reply[count++] = (n3 >> 8) & 0xFF;
    reply[count++] = (n3 >> 0) & 0xFF;

    reply[count++] = (n4 >> 8) & 0xFF;
    reply[count++] = (n4 >> 0) & 0xFF;

    reply[1U] = count;

    writeInt(1U, reply, count, true);
}

/// <summary>
///
/// </summary>
/// <param name="data"></param>
/// <param name="length"></param>
void SerialPort::writeDump(const uint8_t* data, uint16_t length)
{
    if (!m_debug)
        return;

    if (length + 4U > 516U) {
        m_buffer[2U] = CMD_DEBUG_DUMP;
        sendNAK(RSN_ILLEGAL_LENGTH);
        return;
    }

    uint8_t reply[516U];
    ::memset(reply, 0x00U, 516U);

    if (length > 252U) {
        reply[0U] = DVM_LONG_FRAME_START;
        reply[1U] = (length >> 8U) & 0xFFU;
        reply[2U] = (length & 0xFFU);
        reply[3U] = CMD_DEBUG_DUMP;

        for (uint8_t i = 0U; i < length; i++)
            reply[i + 4U] = data[i];

        writeInt(1U, reply, length + 4U);
    }
    else {
        reply[0U] = DVM_SHORT_FRAME_START;
        reply[1U] = length + 3U;
        reply[2U] = CMD_DEBUG_DUMP;

        for (uint8_t i = 0U; i < length; i++)
            reply[i + 3U] = data[i];

        writeInt(1U, reply, length + 3U);
    }
}

// ---------------------------------------------------------------------------
//  Private Class Members
// ---------------------------------------------------------------------------

/// <summary>
/// Write acknowlegement.
/// </summary>
void SerialPort::sendACK()
{
    uint8_t reply[4U];

    reply[0U] = DVM_SHORT_FRAME_START;
    reply[1U] = 4U;
    reply[2U] = CMD_ACK;
    reply[3U] = m_buffer[2U];

    writeInt(1U, reply, 4);
}

/// <summary>
/// Write negative acknowlegement.
/// </summary>
/// <param name="err"></param>
void SerialPort::sendNAK(uint8_t err)
{
    uint8_t reply[5U];

    reply[0U] = DVM_SHORT_FRAME_START;
    reply[1U] = 5U;
    reply[2U] = CMD_NAK;
    reply[3U] = m_buffer[2U];
    reply[4U] = err;

    writeInt(1U, reply, 5);
}

/// <summary>
/// Write modem DSP status.
/// </summary>
void SerialPort::getStatus()
{
    io.resetWatchdog();

    uint8_t reply[15U];

    // send all sorts of interesting internal values
    reply[0U] = DVM_SHORT_FRAME_START;
    reply[1U] = 12U;
    reply[2U] = CMD_GET_STATUS;

    reply[3U] = 0x01U;
    if (m_dmrEnable)
        reply[3U] |= 0x02U;
    if (m_p25Enable)
        reply[3U] |= 0x08U;
    if (m_nxdnEnable)
        reply[3U] |= 0x10U;

    reply[4U] = uint8_t(m_modemState);

    reply[5U] = m_tx ? 0x01U : 0x00U;

    if (io.hasRXOverflow())
        reply[5U] |= 0x04U;

    if (io.hasTXOverflow())
        reply[5U] |= 0x08U;

    reply[5U] |= m_dcd ? 0x40U : 0x00U;

    reply[6U] = 0U;

    if (m_dmrEnable) {
#if defined(DUPLEX)
        if (m_duplex) {
            reply[7U] = dmrTX.getSpace1();
            reply[8U] = dmrTX.getSpace2();
        } else {
            reply[7U] = 10U;
            reply[8U] = dmrDMOTX.getSpace();
        }
#else
        reply[7U] = 10U;
        reply[8U] = dmrDMOTX.getSpace();
#endif
    } else {
        reply[7U] = 0U;
        reply[8U] = 0U;
    }

    reply[9U] = 0U;

    if (m_p25Enable)
        reply[10U] = p25TX.getSpace();
    else
        reply[10U] = 0U;

    if (m_nxdnEnable)
        reply[11U] = nxdnTX.getSpace();
    else
        reply[11U] = 0U;

    writeInt(1U, reply, 12);
}

/// <summary>
/// Write modem DSP version.
/// </summary>
void SerialPort::getVersion()
{
    uint8_t reply[200U];

    reply[0U] = DVM_SHORT_FRAME_START;
    reply[1U] = 0U;
    reply[2U] = CMD_GET_VERSION;

    reply[3U] = PROTOCOL_VERSION;

    reply[4U] = io.getCPU();

    // Reserve 16 bytes for the UDID
    ::memcpy(reply + 5U, 0x00U, 16U);
    io.getUDID(reply + 5U);

    uint8_t count = 21U;
    for (uint8_t i = 0U; HARDWARE[i] != 0x00U; i++, count++)
        reply[count] = HARDWARE[i];

    reply[1U] = count;

    writeInt(1U, reply, count);
}

/// <summary>
/// Helper to validate the passed modem state is valid.
/// </summary>
/// <param name="state"></param>
/// <returns></returns>
uint8_t SerialPort::modemStateCheck(DVM_STATE state)
{
    // invalid mode check
    if (state != STATE_IDLE && state != STATE_DMR && state != STATE_P25 && state != STATE_NXDN && 
        state != STATE_P25_CAL_1K &&
        state != STATE_DMR_DMO_CAL_1K && state != STATE_DMR_CAL_1K &&
        state != STATE_DMR_LF_CAL &&
        state != STATE_RSSI_CAL &&
        state != STATE_P25_CAL && state != STATE_DMR_CAL &&
        state != STATE_NXDN_CAL)
        return RSN_INVALID_MODE;
/*
    // DMR without DMR being enabled
    if (state == STATE_DMR && !m_dmrEnable)
        return RSN_DMR_DISABLED;
    // P25 without P25 being enabled
    if (state == STATE_P25 && !m_p25Enable)
        return RSN_P25_DISABLED;
    // NXDN without NXDN being enabled
    if (state == STATE_NXDN && !m_nxdnEnable)
        return RSN_NXDN_DISABLED;
*/
    return RSN_OK;
}

/// <summary>
/// Set modem DSP configuration from serial port data.
/// </summary>
/// <param name="data"></param>
/// <param name="length"></param>
/// <returns></returns>
uint8_t SerialPort::setConfig(const uint8_t* data, uint8_t length)
{
    if (length < 15U)
        return RSN_ILLEGAL_LENGTH;

    bool simplex = (data[0U] & 0x80U) == 0x80U;

    m_forceDMO = (data[0U] & 0x40U) == 0x40U;
    m_debug = (data[0U] & 0x10U) == 0x10U;

    bool dmrEnable = (data[1U] & 0x02U) == 0x02U;
    bool p25Enable = (data[1U] & 0x08U) == 0x08U;
    bool nxdnEnable = (data[1U] & 0x10U) == 0x10U;

    uint8_t fdmaPreamble = data[2U];
    if (fdmaPreamble > 255U)
        return RSN_INVALID_FDMA_PREAMBLE;

    DVM_STATE modemState = DVM_STATE(data[3U]);

    uint8_t ret = modemStateCheck(modemState);
    if (ret != RSN_OK)
        return ret;

    uint8_t colorCode = data[6U];
    if (colorCode > 15U)
        return RSN_INVALID_DMR_CC;

#if defined(DUPLEX)
    uint8_t dmrRxDelay = data[7U];
    if (dmrRxDelay > 255U)
        return RSN_INVALID_DMR_RX_DELAY;
#endif

    uint16_t nac = (data[8U] << 4) + (data[9U] >> 4);

    m_cwIdTXLevel = data[5U] >> 2; // ??

    uint8_t dmrTXLevel = data[10U];
    uint8_t p25TXLevel = data[12U];
    uint8_t nxdnTXLevel = data[15U];

    m_modemState = modemState;

    m_dmrEnable = dmrEnable;
    m_p25Enable = p25Enable;
    m_nxdnEnable = nxdnEnable;

    if (m_dmrEnable && m_p25Enable)
        return RSN_HS_NO_DUAL_MODE;
    if (m_dmrEnable && m_nxdnEnable)
        return RSN_HS_NO_DUAL_MODE;
    if (m_p25Enable && m_nxdnEnable)
        return RSN_HS_NO_DUAL_MODE;

    m_duplex = !simplex;

#if !defined(DUPLEX)
    if (m_duplex) {
        DEBUG1("SerialPort::setConfig() Full duplex not supported with this firmware");
        return RSN_INVALID_REQUEST;
    }
#elif defined(DUPLEX) && (defined(ZUMSPOT_ADF7021) || defined(LONESTAR_USB) || defined(SKYBRIDGE_HS))
    if (io.isDualBand() && m_duplex) {
        DEBUG1("SerialPort::setConfig() Full duplex is not supported on this board");
        return RSN_INVALID_REQUEST;
    }
#endif

    io.setDeviations(dmrTXLevel, p25TXLevel, nxdnTXLevel);

    p25TX.setPreambleCount(fdmaPreamble);
    dmrDMOTX.setPreambleCount(fdmaPreamble);
    //nxdnTX.setPreambleCount(fdmaPreamble);

    p25RX.setNAC(nac);

#if defined(DUPLEX)
    dmrTX.setColorCode(colorCode);
    dmrRX.setColorCode(colorCode);
    dmrRX.setRxDelay(dmrRxDelay);
    dmrIdleRX.setColorCode(colorCode);
#endif

    dmrDMORX.setColorCode(colorCode);

    if (m_modemState != STATE_IDLE && isCalState(m_modemState)) {
        io.updateCal(calRelativeState(m_modemState));
    }

    setMode(m_modemState);

    io.start();

    return RSN_OK;
}

/// <summary>
/// Set modem DSP mode from serial port data.
/// </summary>
/// <param name="data"></param>
/// <param name="length"></param>
/// <returns></returns>
uint8_t SerialPort::setMode(const uint8_t* data, uint8_t length)
{
    if (length < 1U)
        return RSN_ILLEGAL_LENGTH;

    DVM_STATE modemState = DVM_STATE(data[0U]);

    if (modemState == m_modemState)
        return RSN_OK;

    uint8_t ret = modemStateCheck(modemState);
    if (ret != RSN_OK)
        return ret;

    setMode(modemState);

    return RSN_OK;
}

/// <summary>
/// Sets the modem state.
/// </summary>
/// <param name="modemState"></param>
void SerialPort::setMode(DVM_STATE modemState)
{
    switch (modemState) {
    case STATE_DMR:
        DEBUG1("SerialPort::setMode() mode set to DMR");
        p25RX.reset();
        nxdnRX.reset();
        cwIdTX.reset();
        break;
    case STATE_P25:
        DEBUG1("SerialPort::setMode() mode set to P25");
#if defined(DUPLEX)
        dmrIdleRX.reset();
        dmrRX.reset();
#endif
        dmrDMORX.reset();
        nxdnRX.reset();
        cwIdTX.reset();
        break;
    case STATE_NXDN:
        DEBUG1("SerialPort::setMode() mode set to NXDN");
#if defined(DUPLEX)
        dmrIdleRX.reset();
        dmrRX.reset();
#endif
        dmrDMORX.reset();
        p25RX.reset();
        cwIdTX.reset();
        break;
    case STATE_DMR_CAL:
        DEBUG1("SerialPort::setMode() mode set to DMR Calibrate");
#if defined(DUPLEX)
        dmrIdleRX.reset();
        dmrRX.reset();
#endif
        dmrDMORX.reset();
        p25RX.reset();
        nxdnRX.reset();
        cwIdTX.reset();
        break;
    case STATE_P25_CAL:
        DEBUG1("SerialPort::setMode() mode set to P25 Calibrate");
#if defined(DUPLEX)
        dmrIdleRX.reset();
        dmrRX.reset();
#endif
        dmrDMORX.reset();
        p25RX.reset();
        nxdnRX.reset();
        cwIdTX.reset();
        break;
    case STATE_NXDN_CAL:
        DEBUG1("SerialPort::setMode() mode set to NXDN Calibrate");
#if defined(DUPLEX)
        dmrIdleRX.reset();
        dmrRX.reset();
#endif
        dmrDMORX.reset();
        p25RX.reset();
        nxdnRX.reset();
        cwIdTX.reset();
        break;
    case STATE_RSSI_CAL:
        DEBUG1("SerialPort::setMode() mode set to RSSI Calibrate");
#if defined(DUPLEX)
        dmrIdleRX.reset();
        dmrRX.reset();
#endif
        dmrDMORX.reset();
        p25RX.reset();
        nxdnRX.reset();
        cwIdTX.reset();
        break;
    case STATE_DMR_LF_CAL:
        DEBUG1("SerialPort::setMode() mode set to DMR 80Hz Calibrate");
#if defined(DUPLEX)
        dmrIdleRX.reset();
        dmrRX.reset();
#endif
        dmrDMORX.reset();
        p25RX.reset();
        nxdnRX.reset();
        cwIdTX.reset();
        break;
    case STATE_DMR_CAL_1K:
        DEBUG1("SerialPort::setMode() mode set to DMR BS 1031Hz Calibrate");
#if defined(DUPLEX)
        dmrIdleRX.reset();
        dmrRX.reset();
#endif
        dmrDMORX.reset();
        p25RX.reset();
        nxdnRX.reset();
        cwIdTX.reset();
        break;
    case STATE_DMR_DMO_CAL_1K:
        DEBUG1("SerialPort::setMode() mode set to DMR MS 1031Hz Calibrate");
#if defined(DUPLEX)
        dmrIdleRX.reset();
        dmrRX.reset();
#endif
        dmrDMORX.reset();
        p25RX.reset();
        nxdnRX.reset();
        cwIdTX.reset();
        break;
    case STATE_P25_CAL_1K:
        DEBUG1("SerialPort::setMode() mode set to P25 1011Hz Calibrate");
#if defined(DUPLEX)
        dmrIdleRX.reset();
        dmrRX.reset();
#endif
        dmrDMORX.reset();
        p25RX.reset();
        nxdnRX.reset();
        cwIdTX.reset();
        break;
    default:
        DEBUG1("SerialPort::setMode() mode set to Idle");
        // STATE_IDLE
        break;
    }

    m_modemState = modemState;

    io.setMode(m_modemState);
}

/// <summary>
/// Sets the RF parameters.
/// </summary>
/// <param name="data"></param>
/// <param name="length"></param>
/// <returns></returns>
uint8_t SerialPort::setRFParams(const uint8_t* data, uint8_t length)
{
    if (length < 17U)
        return RSN_ILLEGAL_LENGTH;

    uint32_t rxFreq, txFreq;
    uint8_t rfPower;
    ADF_GAIN_MODE gainMode;

    rxFreq = data[1U] << 0;
    rxFreq |= data[2U] << 8;
    rxFreq |= data[3U] << 16;
    rxFreq |= data[4U] << 24;

    txFreq = data[5U] << 0;
    txFreq |= data[6U] << 8;
    txFreq |= data[7U] << 16;
    txFreq |= data[8U] << 24;

    rfPower = data[9U];

    int8_t dmrDiscBWAdj = int8_t(data[10U]) - 128;
    if (dmrDiscBWAdj > 128)
        return RSN_INVALID_REQUEST;
    if (dmrDiscBWAdj < -128)
        return RSN_INVALID_REQUEST;

    int8_t p25DiscBWAdj = int8_t(data[11U]) - 128;
    if (p25DiscBWAdj > 128)
        return RSN_INVALID_REQUEST;
    if (p25DiscBWAdj < -128)
        return RSN_INVALID_REQUEST;

    int8_t nxdnDiscBWAdj = int8_t(data[15U]) - 128;
    if (nxdnDiscBWAdj > 128)
        return RSN_INVALID_REQUEST;
    if (nxdnDiscBWAdj < -128)
        return RSN_INVALID_REQUEST;

    int8_t dmrPostBWAdj = int8_t(data[12U]) - 128;
    if (dmrPostBWAdj > 128)
        return RSN_INVALID_REQUEST;
    if (dmrPostBWAdj < -128)
        return RSN_INVALID_REQUEST;

    int8_t p25PostBWAdj = int8_t(data[13U]) - 128;
    if (p25PostBWAdj > 128)
        return RSN_INVALID_REQUEST;
    if (p25PostBWAdj < -128)
        return RSN_INVALID_REQUEST;

    int8_t nxdnPostBWAdj = int8_t(data[16U]) - 128;
    if (nxdnPostBWAdj > 128)
        return RSN_INVALID_REQUEST;
    if (nxdnPostBWAdj < -128)
        return RSN_INVALID_REQUEST;

    // support optional AFC parameters
    if (length > 17U) {
        if (length < 19U)
            return RSN_ILLEGAL_LENGTH;

        bool afcEnable = (data[17U] & 0x80U) == 0x80U;
        uint8_t afcKI = data[17U] & 0x0FU;
        uint8_t afcKP = (data[17U] >> 4) & 0x07U;
        uint8_t afcRange = data[18U];

        io.setAFCParams(afcEnable, afcKI, afcKP, afcRange);
    } else {
        io.setAFCParams(false, 11, 4, 1);
    }

    gainMode = (ADF_GAIN_MODE)data[14U];

    io.setRFAdjust(dmrDiscBWAdj, p25DiscBWAdj, nxdnDiscBWAdj, dmrPostBWAdj, p25PostBWAdj, nxdnPostBWAdj);

    return io.setRFParams(rxFreq, txFreq, rfPower, gainMode);
}

/// <summary>
/// Sets the protocol ring buffer sizes.
/// </summary>
/// <param name="data"></param>
/// <param name="length"></param>
/// <returns></returns>
uint8_t SerialPort::setBuffers(const uint8_t* data, uint8_t length)
{
    if (length < 1U)
        return RSN_ILLEGAL_LENGTH;
    if (m_modemState != STATE_IDLE)
        return RSN_INVALID_MODE;

    uint16_t dmrBufSize = dmr::DMR_TX_BUFFER_LEN;
    uint16_t p25BufSize = p25::P25_TX_BUFFER_LEN;
    uint16_t nxdnBufSize = nxdn::NXDN_TX_BUFFER_LEN;

    dmrBufSize = (data[0U] << 8) + (data[1U]);
    p25BufSize = (data[2U] << 8) + (data[3U]);
    nxdnBufSize = (data[4U] << 8) + (data[5U]);

    p25TX.resizeBuffer(p25BufSize);
    nxdnTX.resizeBuffer(nxdnBufSize);

    dmrTX.resizeBuffer(dmrBufSize);
    dmrDMOTX.resizeBuffer(dmrBufSize);

    return RSN_OK;
}
