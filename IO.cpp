// SPDX-License-Identifier: GPL-2.0-only
/*
 * Digital Voice Modem - Hotspot Firmware
 * GPLv2 Open Source. Use is subject to license terms.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 *  Copyright (C) 2015,2016,2020 Jonathan Naylor, G4KLX
 *  Copyright (C) 2016,2017,2018,2019,2020 Andy Uribe, CA6JAU
 *  Copyright (C) 2017 Danilo, DB4PLE
 *  Copyright (C) 2021,2024 Bryan Biedenkapp, N2PLL
 *
 */
#include "Globals.h"
#include "ADF7021.h"
#include "IO.h"

// ---------------------------------------------------------------------------
//  Public Class Members
// ---------------------------------------------------------------------------

/* Initializes a new instance of the IO class. */

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
    /* stub */
}

/* Initializes the air interface sampler. */

void IO::init()
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

/* Starts air interface sampler. */

void IO::start()
{
    if (m_started)
        return;

    startInt();

    m_started = true;
}

/* Process samples from air interface. */

void IO::process()
{
    uint8_t bit;
    uint8_t control;

    m_ledCount++;
    if (m_started) {
        // Two seconds timeout
        if (m_watchdog >= 19200U) {
            if (m_modemState == STATE_DMR || m_modemState == STATE_P25 || m_modemState == STATE_NXDN) {
#if defined(DUPLEX)
                if (m_modemState == STATE_DMR && m_tx)
                    dmrTX.setStart(false);
#endif
            }

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
            DEBUG2("IO::process() setting modem state", m_modemState);
            io.rf1Conf(m_modemState, true);
        }

        setRX(false);
    }

    if (m_rxBuffer.getData() >= 1U) {
        m_rxBuffer.get(bit, control);

        if (m_modemState == STATE_DMR) {
            /** Digital Mobile Radio */
#if defined(DUPLEX)
            if (m_duplex && !m_forceDMO) {
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
            /** Next Generation Digital Narrowband */
            nxdnRX.databit(bit);
        }
    }
}

/* Write bits to air interface. */

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

/* Helper to get how much space the transmit ring buffer has for samples. */

uint16_t IO::getSpace() const
{
    return m_txBuffer.getSpace();
}

/* */

void IO::setDecode(bool dcd)
{
    if (dcd != m_dcd)
        setCOSInt(dcd ? true : false);

    m_dcd = dcd;
}

/* Helper to set the modem air interface state. */

void IO::setMode(DVM_STATE modemState)
{
    DVM_STATE relativeState = modemState;

    if (serial.isCalState(modemState)) {
        relativeState = serial.calRelativeState(modemState);
    }

    DEBUG3("IO::setMode() setting modem state", modemState, relativeState);
    rf1Conf(relativeState, true);

    DEBUG4("IO::setMode() setting lights", relativeState == STATE_DMR, relativeState == STATE_P25, relativeState == STATE_NXDN);
    setDMRInt(relativeState == STATE_DMR);
    setP25Int(relativeState == STATE_P25);
    setNXDNInt(relativeState == STATE_NXDN);
}

/* Sets the RF parameters. */

uint8_t IO::setRFParams(uint32_t rxFreq, uint32_t txFreq, uint8_t rfPower, ADF_GAIN_MODE gainMode)
{
    m_rfPower = rfPower >> 2;
    m_gainMode = gainMode;

    // check frequency ranges
    if (!(
            /* 136 - 174 mhz */
            ((rxFreq >= VHF_MIN) && (rxFreq < VHF_MAX)) || ((txFreq >= VHF_MIN) && (txFreq < VHF_MAX)) ||
            /* 216 - 225 mhz */
            ((rxFreq >= VHF_220_MIN) && (rxFreq < VHF_220_MAX)) || ((txFreq >= VHF_220_MIN) && (txFreq < VHF_220_MAX)) ||
            /* 380 - 431 mhz */
            ((rxFreq >= UHF_380_MIN) && (rxFreq < UHF_380_MAX)) || ((txFreq >= UHF_380_MIN) && (txFreq < UHF_380_MAX)) ||
            /* 431 - 450 mhz */
            ((rxFreq >= UHF_1_MIN) && (rxFreq < UHF_1_MAX)) || ((txFreq >= UHF_1_MIN) && (txFreq < UHF_1_MAX)) ||
            /* 450 - 470 mhz */
            ((rxFreq >= UHF_2_MIN) && (rxFreq < UHF_2_MAX)) || ((txFreq >= UHF_2_MIN) && (txFreq < UHF_2_MAX)) ||
            /* 470 - 520 mhz */
            ((rxFreq >= UHF_T_MIN) && (rxFreq < UHF_T_MAX)) || ((txFreq >= UHF_T_MIN) && (txFreq < UHF_T_MAX)) ||
            /* 842 - 900 mhz */
            ((rxFreq >= UHF_800_MIN) && (rxFreq < UHF_800_MAX)) || ((txFreq >= UHF_800_MIN) && (txFreq < UHF_800_MAX)) ||
            /* 900 - 950 mhz */
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

    DEBUG5("IO::setRFParams() setting RF params", m_rxFrequency, m_txFrequency, m_rfPower, m_gainMode);

    return RSN_OK;
}

/* Flag indicating the TX ring buffer has overflowed. */

bool IO::hasTXOverflow()
{
    return m_txBuffer.hasOverflowed();
}

/* Flag indicating the RX ring buffer has overflowed. */

bool IO::hasRXOverflow()
{
    return m_rxBuffer.hasOverflowed();
}

/* */

void IO::resetWatchdog()
{
    m_watchdog = 0U;
}

/* */

uint32_t IO::getWatchdog()
{
    return m_watchdog;
}

/* */

void IO::selfTest()
{
    bool ledValue = false;

    for (uint8_t i = 0; i < 6; i++) {
        ledValue = !ledValue;

        // We exclude PTT to avoid trigger the transmitter
        setLEDInt(ledValue);
        setCOSInt(ledValue);

        setDMRInt(ledValue);
        setP25Int(ledValue);
        setNXDNInt(ledValue);

        delayUS(125000);
    }

    // blinkin lights
    setLEDInt(false);
    setCOSInt(false);
    setDMRInt(false);
    setP25Int(false);
    setNXDNInt(false);
    delayUS(125000U);

    setLEDInt(true);
    setCOSInt(false);
    setDMRInt(false);
    setP25Int(false);
    delayUS(125000U);

    setLEDInt(false);
    setCOSInt(true);
    setDMRInt(false);
    setP25Int(false);
    delayUS(125000U);

    setLEDInt(false);
    setCOSInt(false);
    setDMRInt(true);
    setP25Int(false);
    delayUS(125000U);

    setLEDInt(false);
    setCOSInt(false);
    setDMRInt(false);
    setP25Int(true);
    delayUS(125000U);

    setLEDInt(false);
    setCOSInt(false);
    setDMRInt(false);
    setP25Int(false);
    setNXDNInt(true);
    delayUS(125000U);

    setLEDInt(false);
    setCOSInt(false);
    setDMRInt(false);
    setP25Int(true);
    setNXDNInt(false);
    delayUS(125000U);

    setLEDInt(false);
    setCOSInt(false);
    setDMRInt(true);
    setP25Int(false);
    setNXDNInt(false);
    delayUS(125000U);

    setLEDInt(false);
    setCOSInt(true);
    setDMRInt(false);
    setP25Int(false);
    setNXDNInt(false);
    delayUS(125000U);

    setLEDInt(true);
    setCOSInt(false);
    setDMRInt(false);
    setP25Int(false);
    setNXDNInt(false);
    delayUS(125000U);

    setLEDInt(false);
    setCOSInt(false);
    setDMRInt(false);
    setP25Int(false);
    setNXDNInt(false);
    delayUS(125000U);
}

/* */

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
/* */

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
