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
*   Copyright (C) 2015,2016,2020 by Jonathan Naylor G4KLX
*   Copyright (C) 2016,2017,2018,2019,2020 by Andy Uribe CA6JAU
*   Copyright (C) 2017 by Danilo DB4PLE
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
#include "ADF7021.h"
#include "IO.h"

// ---------------------------------------------------------------------------
//  Public Class Members
// ---------------------------------------------------------------------------

/// <summary>
/// Initializes a new instance of the IO class.
/// </summary>
IO::IO():
    m_started(false),
    m_rxBuffer(1024U),
    m_txBuffer(1024U),
    m_ledCount(0U),
    m_ledValue(true),
    m_watchdog(0U),
    m_int1Counter(0U),
    m_int2Counter(0U),
    m_rxFrequency(DEFAULT_FREQUENCY),
    m_txFrequency(DEFAULT_FREQUENCY),
    m_rfPower(0U),
    m_gainMode(ADF_GAIN_AUTO)    
{
    initInt();

    CE(HIGH);

    setLEDInt(HIGH);
    setPTTInt(LOW);
    setDMRInt(LOW);
    setP25Int(LOW);
    setNXDNInt(LOW);
    setCOSInt(LOW);

#if !defined(BIDIR_DATA_PIN)
    setTXDInt(LOW);
#endif

    SCLK(LOW);
    SDATA(LOW);
    SLE1(LOW);
#if defined(DUPLEX)
    SLE2(LOW);
#endif

    selfTest();
}

/// <summary>
/// Starts air interface sampler.
/// </summary>
void IO::start()
{
    if (m_started)
        return;

    startInt();

    m_started = true;
}

/// <summary>
/// Process samples from air interface.
/// </summary>
void IO::process()
{
    uint8_t bit;
    uint32_t scanTime;
    uint8_t control;

    m_ledCount++;
    if (m_started) {
        // Two seconds timeout
        if (m_watchdog >= 19200U) {
/*            
            if (m_modemState == STATE_DMR || m_modemState == STATE_P25) {
                m_modemState = STATE_IDLE;
                setMode(m_modemState);
            }
*/
            m_watchdog = 0U;
        }

        if (m_ledCount >= 48000U) {
            m_ledCount = 0U;
            m_ledValue = !m_ledValue;
            setLEDInt(m_ledValue);
        }
    }
    else {
        if (m_ledCount >= 480000U) {
            m_ledCount = 0U;
            m_ledValue = !m_ledValue;
            setLEDInt(m_ledValue);
        }
        return;
    }

    // switch off the transmitter if needed
    if (m_txBuffer.getData() == 0U && m_tx) {
        if (m_cwIdState) { 
            // check for CW ID end of transmission
            m_cwIdState = false;
            io.rf1Conf(m_modemState, true);
        }

        setRX(false);
    }

    if (m_rxBuffer.getData() >= 1U) {
        m_rxBuffer.get(bit, control);

        if (m_modemState == STATE_DMR) {
            /** Digital Mobile Radio */
#if defined(DUPLEX)
            if (m_duplex) {
                if (m_tx)
                    dmrRX.databit(bit, control);
                else
                    dmrIdleRX.databit(bit);
            }
            else
                dmrDMORX.databit(bit);
#else
            dmrDMORX.databit(bit);
#endif
        }
        else if (m_modemState == STATE_P25) {
            /** Project 25 */
            p25RX.databit(bit);
        }
        else if (m_modemState == STATE_NXDN) {
            /* Next Generation Digital Narrowband */
            nxdnRX.databit(bit);
        }
    }
}

/// <summary>
/// Write bits to air interface.
/// </summary>
/// <param name="mode"></param>
/// <param name="samples"></param>
/// <param name="length"></param>
/// <param name="control"></param>
void IO::write(uint8_t* data, uint16_t length, const uint8_t* control)
{
    if (!m_started)
        return;

    for (uint16_t i = 0U; i < length; i++) {
        if (control == NULL)
            m_txBuffer.put(data[i], MARK_NONE);
        else
            m_txBuffer.put(data[i], control[i]);
    }

    // switch the transmitter on if needed
    if (!m_tx) {
        setTX();
        m_tx = true;
    }
}

/// <summary>
/// Helper to get how much space the transmit ring buffer has for samples.
/// </summary>
/// <returns></returns>
uint16_t IO::getSpace() const
{
    return m_txBuffer.getSpace();
}

/// <summary>
///
/// </summary>
/// <param name="dcd"></param>
void IO::setDecode(bool dcd)
{
    if (dcd != m_dcd)
        setCOSInt(dcd ? true : false);

    m_dcd = dcd;
}

/// <summary>
/// Helper to set the modem air interface state.
/// </summary>
void IO::setMode(DVM_STATE modemState)
{
    DVM_STATE relativeState = modemState;

    if (serial.isCalState(modemState)) {
        relativeState = serial.calRelativeState(modemState);
    }

    DEBUG3("IO::setMode(): setting RF hardware", modemState, relativeState);

    rf1Conf(relativeState, true);

    setDMRInt(relativeState == STATE_DMR);
    setP25Int(relativeState == STATE_P25);
    setNXDNInt(relativeState == STATE_NXDN);
}

/// <summary>
/// Sets the RF parameters.
/// </summary>
/// <param name="rxFreq"></param>
/// <param name="txFreq"></param>
/// <param name="rfPower"></param>
/// <param name="gainMode"></param>
uint8_t IO::setRFParams(uint32_t rxFreq, uint32_t txFreq, uint8_t rfPower, ADF_GAIN_MODE gainMode)
{
    m_rfPower = rfPower >> 2;
    m_gainMode = gainMode;

    // check frequency ranges
    if (!(
        /** 136 - 174 mhz */
        ((rxFreq >= VHF_MIN) && (rxFreq < VHF_MAX)) || ((txFreq >= VHF_MIN) && (txFreq < VHF_MAX)) ||
        /** 216 - 225 mhz */
        ((rxFreq >= VHF_220_MIN) && (rxFreq < VHF_220_MAX)) || ((txFreq >= VHF_220_MIN) && (txFreq < VHF_220_MAX)) ||
        /** 380 - 431 mhz */
        ((rxFreq >= UHF_380_MIN) && (rxFreq < UHF_380_MAX)) || ((txFreq >= UHF_380_MIN) && (txFreq < UHF_380_MAX)) ||
        /** 431 - 450 mhz */
        ((rxFreq >= UHF_1_MIN) && (rxFreq < UHF_1_MAX)) || ((txFreq >= UHF_1_MIN) && (txFreq < UHF_1_MAX)) ||
        /** 450 - 470 mhz */
        ((rxFreq >= UHF_2_MIN) && (rxFreq < UHF_2_MAX)) || ((txFreq >= UHF_2_MIN) && (txFreq < UHF_2_MAX)) ||
        /** 470 - 520 mhz */
        ((rxFreq >= UHF_T_MIN) && (rxFreq < UHF_T_MAX)) || ((txFreq >= UHF_T_MIN) && (txFreq < UHF_T_MAX)) ||
        /** 842 - 900 mhz */
        ((rxFreq >= UHF_800_MIN) && (rxFreq < UHF_800_MAX)) || ((txFreq >= UHF_800_MIN) && (txFreq < UHF_800_MAX)) ||
        /** 900 - 950 mhz */
        ((rxFreq >= UHF_900_MIN) && (rxFreq < UHF_900_MAX)) || ((txFreq >= UHF_900_MIN) && (txFreq < UHF_900_MAX))
        ))
        return RSN_INVALID_REQUEST;

#if defined(ZUMSPOT_ADF7021) || defined(LONESTAR_USB) || defined(SKYBRIDGE_HS)
    // check hotspot configuration for single or dual ADF7021
    if (!(io.hasSingleADF7021())) {
        // there are two ADF7021s on the board -- are they dual-band?
        if (io.isDualBand()) {
            // dual band
            if ((txFreq <= VHF_220_MAX) && (rxFreq <= VHF_220_MAX)) {
                // turn on VHF side
                io.setBandVHF(true);
            }
            else if ((txFreq >= UHF_1_MIN) && (rxFreq >= UHF_1_MIN)) {
                // turn on UHF side
                io.setBandVHF(false);
            }
        }
        else if (!io.isDualBand()) {
            // duplex board
            if ((txFreq < UHF_1_MIN) || (rxFreq < UHF_1_MIN)) {
                // Reject VHF frequencies
                return RSN_INVALID_REQUEST;
            }
        }
    }
#endif

    m_rxFrequency = rxFreq;
    m_txFrequency = txFreq;

    DEBUG5("IO::setRFParams(): setting RF params", m_rxFrequency, m_txFrequency, m_rfPower, m_gainMode);

    return RSN_OK;
}

/// <summary>
/// Flag indicating the TX ring buffer has overflowed.
/// </summary>
/// <returns></returns>
bool IO::hasTXOverflow()
{
    return m_txBuffer.hasOverflowed();
}

/// <summary>
/// Flag indicating the RX ring buffer has overflowed.
/// </summary>
/// <returns></returns>
bool IO::hasRXOverflow()
{
    return m_rxBuffer.hasOverflowed();
}

/// <summary>
///
/// </summary>
void IO::resetWatchdog()
{
    m_watchdog = 0U;
}

/// <summary>
///
/// </summary>
/// <returns></returns>
uint32_t IO::getWatchdog()
{
    return m_watchdog;
}

/// <summary>
///
/// </summary>
void IO::selfTest()
{
    bool ledValue = false;
    uint32_t ledCount = 0U;
    uint32_t blinks = 0U;

    while (true) {
        ledCount++;
        delayUS(1000U);

        if (ledCount >= 125U) {
            ledCount = 0U;
            ledValue = !ledValue;

            setLEDInt(!ledValue);
            setPTTInt(ledValue);
            setDMRInt(ledValue);
            setP25Int(ledValue);
            setNXDNInt(ledValue);
            setCOSInt(ledValue);

            blinks++;

            if (blinks > 5U)
                break;
        }
    }
}

/// <summary>
/// 
/// </summary>
/// <param name="int1"></param>
/// <param name="int2"></param>
void IO::getIntCounter(uint16_t& int1, uint16_t& int2)
{
    int1 = m_int1Counter;
    int2 = m_int2Counter;
    m_int1Counter = 0U;
    m_int2Counter = 0U;
}

// ---------------------------------------------------------------------------
//  Private Class Members
// ---------------------------------------------------------------------------

#if defined(ZUMSPOT_ADF7021) || defined(LONESTAR_USB) || defined(SKYBRIDGE_HS)
/// <summary>
/// 
/// </summary>
/// <param name="rxFreq"></param>
/// <param name="txFreq"></param>
void IO::checkBand(uint32_t rxFreq, uint32_t txFreq)
{
    // check hotspot configuration for single or dual ADF7021
    if (!(io.hasSingleADF7021())) {
        // there are two ADF7021s on the board -- are they dual-band?
        if (io.isDualBand()) {
            // dual band
            if ((txFreq <= VHF_220_MAX) && (rxFreq <= VHF_220_MAX)) {
                // turn on VHF side
                io.setBandVHF(true);
            }
            else if ((txFreq >= UHF_1_MIN) && (rxFreq >= UHF_1_MIN)) {
                // turn on UHF side
                io.setBandVHF(false);
            }
        }
    }
}
#endif
