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
*   Copyright (C) 2020,2021 by Jonathan Naylor G4KLX
*   Copyright (C) 2016 by Jim McLaughlin KI6ZUM
*   Copyright (C) 2016,2017,2018,2019,2020 by Andy Uribe CA6JAU
*   Copyright (C) 2017 by Danilo DB4PLE
*   Copyright (C) 2021 Bryan Biedenkapp N2PLL
*
*   Some of the code is based on work of Guus Van Dooren PE1PLM:
*   https://github.com/ki6zum/gmsk-dstar/blob/master/firmware/dvmega/dvmega.ino
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
#include <math.h>

#include "Globals.h"
#include "ADF7021.h"

#if defined(ENABLE_ADF7021)

// ---------------------------------------------------------------------------
//  Globals
// ---------------------------------------------------------------------------

volatile bool toTxRequest = false;
volatile bool toRxRequest = false;
volatile bool even = true;
static uint32_t lastClk = 2U;

volatile uint32_t AD7021_CONTROL;
uint32_t ADF7021_RX_REG0;
uint32_t ADF7021_TX_REG0;
uint32_t ADF7021_REG1;
uint32_t ADF7021_REG2;
uint32_t ADF7021_REG3;
uint32_t ADF7021_REG4;
uint32_t ADF7021_REG10;
uint32_t ADF7021_REG13;

uint32_t div2;
uint32_t f_div;

uint8_t RX_N_Divider;   // Rx - 8-bit Integer_N
uint16_t RX_F_Divider;  // Rx - 15-bit Frational_N
uint8_t TX_N_Divider;   // Tx - 8-bit Integer_N
uint16_t TX_F_Divider;  // Tx - 15-bit Frational_N

uint16_t dmrDev;
uint16_t p25Dev;
uint16_t nxdnDev;

int8_t m_dmrDiscBWAdj;
int8_t m_p25DiscBWAdj;
int8_t m_nxdnDiscBWAdj;
int8_t m_dmrPostBWAdj;
int8_t m_p25PostBWAdj;
int8_t m_nxdnPostBWAdj;

// ---------------------------------------------------------------------------
//  Global Functions
// ---------------------------------------------------------------------------

/// <summary>
/// 
/// </summary>
static void AD7021_IOCTL_Shift()
{
    for (int i = 31; i >= 0; i--) {
        if (ADF_BIT_READ(AD7021_CONTROL, i) == HIGH)
            io.SDATA(HIGH);
        else
            io.SDATA(LOW);

        io.delayBit();
        io.SCLK(HIGH);
        io.delayBit();
        io.SCLK(LOW);
    }

    // to keep SDATA signal at defined level when idle (not required)
    io.SDATA(LOW);
}

/// <summary>
/// 
/// </summary>
static void AD7021_IOCTL_SLEPulse()
{
    io.SLE1(HIGH);
    io.delayBit();
    io.SLE1(LOW);
}

/// <summary>
/// 
/// </summary>
/// <param name="doSle"></param>
static void AD7021_1_IOCTL(bool doSle = true)
{
    AD7021_IOCTL_Shift();

    if (doSle)
        AD7021_IOCTL_SLEPulse();
}

#if defined(DUPLEX)
/// <summary>
/// 
/// </summary>
static void AD7021_2_IOCTL_SLEPulse()
{
    io.SLE2(HIGH);
    io.delayBit();
    io.SLE2(LOW);
}

/// <summary>
/// 
/// </summary>
/// <param name="doSle"></param>
static void AD7021_2_IOCTL(bool doSle = true)
{
    AD7021_IOCTL_Shift();

    if (doSle)
        AD7021_2_IOCTL_SLEPulse();
}
#endif

// ---------------------------------------------------------------------------
//  Public Class Members
// ---------------------------------------------------------------------------

/// <summary>
/// Hardware interrupt handler.
/// </summary>
void IO::interrupt1()
{
    uint8_t bit = 0U;

    if (!m_started)
        return;

    uint8_t clk = CLK();

    // this is to prevent activation by spurious interrupts
    // which seem to happen if you send out an control word
    // needs investigation
    // this workaround will fail if only rising or falling edge
    // is used to trigger the interrupt !!!!
    // TODO: figure out why sending the control word seems to issue interrupts
    // possibly this is a design problem of the RF7021 board or too long wires
    // on the breadboard build
    // but normally this will not hurt too much
    if (clk == lastClk)
        return;
    else
        lastClk = clk;

    // we set the TX bit at TXD low, sampling of ADF7021 happens at rising clock
    if (m_tx && clk == 0U) {
        m_txBuffer.get(bit, m_control);
        even = !even;

#if defined(BIDIR_DATA_PIN)
        if (bit)
            setRXDInt(HIGH);
        else
            setRXDInt(LOW);
#else
        if (bit)
            setTXDInt(HIGH);
        else
            setTXDInt(LOW);
#endif

        // wait a brief period before raising SLE
        if (toTxRequest == true) {
            asm volatile(
                "nop          \n\t"
                "nop          \n\t"
                "nop          \n\t"
            );

            // SLE Pulse, should be moved out of here into class method
            // according to datasheet in 4FSK we have to deliver this before 1/4 tbit == 26uS
            SLE1(HIGH);
            asm volatile(
                "nop          \n\t"
                "nop          \n\t"
                "nop          \n\t"
            );

            SLE1(LOW);
            SDATA(LOW);

            // now do housekeeping
            toTxRequest = false;

            // first tranmittted bit is always the odd bit
            even = ADF7021_EVEN_BIT;
        }
    }

    // we sample the RX bit at rising TXD clock edge, so TXD must be 1 and we are not in tx mode
    if (!m_tx && clk == 1U && !m_duplex) {
        if (RXD1())
            bit = 1U;
        else
            bit = 0U;

        m_rxBuffer.put(bit, m_control);
    }

    if (toRxRequest && even == ADF7021_EVEN_BIT && m_tx && clk == 0U) {
        // that is absolutely crucial in 4FSK, see datasheet:
        // enable sle after 1/4 tBit == 26uS when sending MSB (even == false) and clock is low
        delayUS(26U);

        // SLE Pulse, should be moved out of here into class method
        SLE1(HIGH);
        asm volatile(
            "nop          \n\t"
            "nop          \n\t"
            "nop          \n\t"
        );

        SLE1(LOW);
        SDATA(LOW);

        // now do housekeeping
        m_tx = false;
        toRxRequest = false;

        // last tranmittted bit is always the even bit
        // since the current bit is a transitional "don't care" bit, never transmitted
        even = !ADF7021_EVEN_BIT;
    }

    m_watchdog++;
    m_int1Counter++;
}

#if defined(DUPLEX)
/// <summary>
/// Hardware interrupt handler.
/// </summary>
void IO::interrupt2()
{
    uint8_t bit = 0U;

    if (m_duplex) {
        if (RXD2())
            bit = 1U;
        else
            bit = 0U;

        m_rxBuffer.put(bit, m_control);
    }

    m_int2Counter++;
}
#endif

/// <summary>
/// Sets the ADF7021 RF configuration.
/// </summary>
/// <param name="modemState"></param>
/// <param name="reset"></param>
void IO::rf1Conf(DVM_STATE modemState, bool reset)
{
    int32_t AFC_OFFSET = 0;

    uint32_t txFrequencyTmp, rxFrequencyTmp;

    DEBUG4("IO::rf1Conf(): configuring ADF for Tx/Rx; modemState/reset/rxGain", modemState, reset, m_gainMode);

#if defined (ZUMSPOT_ADF7021) || defined(SKYBRIDGE_HS)
    io.checkBand(m_rxFrequency, m_txFrequency);
#endif

    // Toggle CE pin for ADF7021 reset
    if (reset) {
        CE(LOW);
        delayReset();
        CE(HIGH);
        delayReset();
    }

    switch (modemState) {
    case STATE_DMR:
    case STATE_CW:
        AFC_OFFSET = AFC_OFFSET_DMR;
        break;
    case STATE_P25:
        AFC_OFFSET = AFC_OFFSET_P25;
        break;
    case STATE_NXDN:
        AFC_OFFSET = AFC_OFFSET_NXDN;
        break;
    default:
        break;
    }

    /*
    ** VCO/Oscillator (Register 1)
    */
    configureBand();

    /*
    ** Fractional-N Synthesizer (Register 0)
    */
    float divider = 0.0f;
    if (div2 == 1U)
        divider = (m_rxFrequency - 100000 + AFC_OFFSET) / (ADF7021_PFD / 2U);
    else
        divider = (m_rxFrequency - 100000 + (2 * AFC_OFFSET)) / ADF7021_PFD;

    // calculate Integer_N and Fractional_N divider values for Rx
    RX_N_Divider = floor(divider);
    divider = (divider - RX_N_Divider) * 32768;
    RX_F_Divider = floor(divider + 0.5);

    // setup rx register 0
    ADF7021_RX_REG0 = (uint32_t)ADF7021_REG0_ADDR;                      // Register Address 0
#if defined(BIDIR_DATA_PIN)
    ADF7021_RX_REG0 |= (uint32_t)0b01001 << 27;                         // Mux regulator/receive
#else
    ADF7021_RX_REG0 |= (uint32_t)0b01011 << 27;                         // Mux regulator/uart-spi enabled/receive
#endif
    ADF7021_RX_REG0 |= (uint32_t)RX_N_Divider << 19;                    // Frequency - 8-bit Int_N
    ADF7021_RX_REG0 |= (uint32_t)RX_F_Divider << 4;                     // Frequency - 15-bit Frac_N

    if (div2 == 1U)
        divider = m_txFrequency / (ADF7021_PFD / 2U);
    else
        divider = m_txFrequency / ADF7021_PFD;

    // calculate Integer_N and Fractional_N divider values for Tx
    TX_N_Divider = floor(divider);
    divider = (divider - TX_N_Divider) * 32768;
    TX_F_Divider = floor(divider + 0.5);

    // setup tx register 0
    ADF7021_TX_REG0 = (uint32_t)ADF7021_REG0_ADDR;                      // Register Address 0
#if defined(BIDIR_DATA_PIN)
    ADF7021_TX_REG0 |= (uint32_t)0b01000 << 27;                         // Mux regulator/transmit
#else
    ADF7021_TX_REG0 |= (uint32_t)0b01010 << 27;                         // Mux regulator/uart-spi enabled/transmit
#endif
    ADF7021_TX_REG0 |= (uint32_t)TX_N_Divider << 19;                    // Frequency - 8-bit Int_N
    ADF7021_TX_REG0 |= (uint32_t)TX_F_Divider << 4;                     // Frequency - 15-bit Frac_N

    // configure ADF Tx/RX
    configureTxRx(modemState);

    // write registers
    /*
    ** VCO/Oscillator (Register 1)
    */
    AD7021_CONTROL = ADF7021_REG1;
    AD7021_1_IOCTL();

    /*
    ** Tx/Rx Clock (Register 3)
    */
    AD7021_CONTROL = ADF7021_REG3;
    AD7021_1_IOCTL();

    /*
    ** Demodulator Setup (Register 4)
    */
    AD7021_CONTROL = ADF7021_REG4;
    AD7021_1_IOCTL();

    /*
    ** IF Fine Cal Setup (Register 6)
    */
    AD7021_CONTROL = ADF7021_REG6;
    AD7021_1_IOCTL();

    /*
    ** IF Coarse Cal Setup (Register 5)
    */
    AD7021_CONTROL = ADF7021_REG5;
    AD7021_1_IOCTL();

    // delay for filter calibration
    delayIfCal();

    /*
    ** N Register (Frequency) (Register 0)
    */
    setRX();

    /*
    ** Transmit Modulation (Register 2)
    */
    AD7021_CONTROL = ADF7021_REG2;
    AD7021_1_IOCTL();

    /*
    ** Test DAC (Register 14)
    */
#if defined(TEST_DAC)
    AD7021_CONTROL = 0x0000001E;
#else
    AD7021_CONTROL = 0x0000000E;
#endif
    AD7021_1_IOCTL();

    /*
    ** AGC (Register 9)
    */
   switch (m_gainMode) {
        case ADF_GAIN_AUTO_LIN:
            AD7021_CONTROL = 0x100231E9; // AGC ON, LNA high linearity
            break;
        case ADF_GAIN_LOW:
            AD7021_CONTROL = 0x120631E9; // AGC OFF, low gain, LNA high linearity
            break;
        case ADF_GAIN_HIGH:
            AD7021_CONTROL = 0x00A631E9; // AGC OFF, high gain
            break;
        case ADF_GAIN_AUTO:
        default:
            AD7021_CONTROL = 0x000231E9; // AGC ON, normal operation
            break;
    }
    AD7021_1_IOCTL();

    /*
    ** AFC (Register 10)
    */
    AD7021_CONTROL = ADF7021_REG10;
    AD7021_1_IOCTL();

    /*
    ** Sync Word Detect (Register 11)
    */
    AD7021_CONTROL = 0x0000003B;
    AD7021_1_IOCTL();

    /*
    ** SWD/Threshold Setup (Register 12)
    */
    AD7021_CONTROL = 0x0000010C;
    AD7021_1_IOCTL();

    /*
    ** 3FSK/4FSK Demod (Register 13)
    */
    AD7021_CONTROL = ADF7021_REG13;
    AD7021_1_IOCTL();

    /*
    ** Test Mode (Register 15)
    */
#if defined(TEST_TX)
    PTT(HIGH);
    AD7021_CONTROL = ADF7021_TX_REG0;
    AD7021_1_IOCTL();

    AD7021_CONTROL = 0x000E010F;
#else
    AD7021_CONTROL = 0x000E000F;
#endif
    AD7021_1_IOCTL();

#if defined(DUPLEX)
    // if duplex -- auto setup the second ADF7021
    if (m_duplex && (modemState != STATE_CW))
        rf2Conf(modemState);
#endif
}

#if defined(DUPLEX)
/// <summary>
/// Sets the ADF7021 RF configuration.
/// </summary>
/// <param name="modemState"></param>
/// <param name="reset"></param>
void IO::rf2Conf(DVM_STATE modemState)
{
    DEBUG3("IO::rf2Conf(): configuring 2nd ADF for Rx; modemState/rxGain", modemState, m_gainMode);

    // configure ADF Tx/RX
    configureTxRx(modemState);

    // write registers
    /*
    ** VCO/Oscillator (Register 1)
    */
    AD7021_CONTROL = ADF7021_REG1;
    AD7021_2_IOCTL();

    /*
    ** Tx/Rx Clock (Register 3)
    */
    AD7021_CONTROL = ADF7021_REG3;
    AD7021_2_IOCTL();

    /*
    ** Demodulator Setup (Register 4)
    */
    AD7021_CONTROL = ADF7021_REG4;
    AD7021_2_IOCTL();

    /*
    ** IF Fine Cal Setup (Register 6)
    */
    AD7021_CONTROL = ADF7021_REG6;
    AD7021_2_IOCTL();

    /*
    ** IF Coarse Cal Setup (Register 5)
    */
    AD7021_CONTROL = ADF7021_REG5;
    AD7021_2_IOCTL();

    // delay for filter calibration
    delayIfCal();

    /*
    ** N Register (Frequency) (Register 0)
    */
    // set to RX only
    AD7021_CONTROL = ADF7021_RX_REG0;
    AD7021_2_IOCTL();

    /*
    ** Transmit Modulation (Register 2)
    */
    AD7021_CONTROL = ADF7021_REG2;
    AD7021_2_IOCTL();

    /*
    ** Test DAC (Register 14)
    */
    AD7021_CONTROL = 0x0000000E;
    AD7021_2_IOCTL();

    /*
    ** AGC (Register 9)
    */
   switch (m_gainMode) {
        case ADF_GAIN_AUTO_LIN:
            AD7021_CONTROL = 0x100231E9; // AGC ON, LNA high linearity
            break;
        case ADF_GAIN_LOW:
            AD7021_CONTROL = 0x120631E9; // AGC OFF, low gain, LNA high linearity
            break;
        case ADF_GAIN_HIGH:
            AD7021_CONTROL = 0x00A631E9; // AGC OFF, high gain
            break;
        case ADF_GAIN_AUTO:
        default:
            AD7021_CONTROL = 0x000231E9; // AGC ON, normal operation
            break;
    }
    AD7021_2_IOCTL();

    /*
    ** AFC (Register 10)
    */
    AD7021_CONTROL = ADF7021_REG10;
    AD7021_2_IOCTL();

    /*
    ** Sync Word Detect (Register 11)
    */
    AD7021_CONTROL = 0x0000003B;
    AD7021_2_IOCTL();

    /*
    ** SWD/Threshold Setup (Register 12)
    */
    AD7021_CONTROL = 0x0000010C;
    AD7021_2_IOCTL();

    /*
    ** 3FSK/4FSK Demod (Register 13)
    */
    AD7021_CONTROL = ADF7021_REG13;
    AD7021_2_IOCTL();

    /*
    ** Test Mode (Register 15)
    */
    AD7021_CONTROL = 0x000E000F;
    AD7021_2_IOCTL();
}
#endif // DUPLEX

/// <summary>
/// 
/// </summary>
/// <param name="dmrTXLevel"></param>
/// <param name="p25TXLevel"></param>
/// <param name="nxdnTXLevel"></param>
void IO::setDeviations(uint8_t dmrTXLevel, uint8_t p25TXLevel, uint8_t nxdnTXLevel)
{
    dmrDev = uint16_t((ADF7021_DEV_DMR * uint16_t(dmrTXLevel)) / 128U);
    p25Dev = uint16_t((ADF7021_DEV_P25 * uint16_t(p25TXLevel)) / 128U);
    nxdnDev = uint16_t((ADF7021_DEV_NXDN  * uint16_t(nxdnTXLevel)) / 128U);
}

/// <summary>
/// Sets the RF adjustment parameters.
/// </summary>
/// <param name="dmrDevAdj"></param>
/// <param name="p25DevAdj"></param>
/// <param name="dmrDiscBWAdj"></param>
/// <param name="p25DiscBWAdj"></param>
/// <param name="nxdnDiscBWAdj"></param>
/// <param name="dmrPostBWAdj"></param>
/// <param name="p25PostBWAdj"></param>
/// <param name="nxdnPostBWAdj"></param>
void IO::setRFAdjust(int8_t dmrDiscBWAdj, int8_t p25DiscBWAdj, int8_t nxdnDiscBWAdj, int8_t dmrPostBWAdj, int8_t p25PostBWAdj, int8_t nxdnPostBWADJ)
{
    m_dmrDiscBWAdj = dmrDiscBWAdj;
    m_p25DiscBWAdj = p25DiscBWAdj;
    m_nxdnDiscBWAdj = nxdnDiscBWAdj;
    m_dmrPostBWAdj = dmrPostBWAdj;
    m_p25PostBWAdj = p25PostBWAdj;
    m_nxdnPostBWAdj = nxdnPostBWADJ;
}

/// <summary>
/// 
/// </summary>
void IO::updateCal(DVM_STATE modemState)
{
    uint32_t ADF7021_REG2;
    float divider;

    /*
    ** VCO/Oscillator (Register 1)
    */
    configureBand();

    AD7021_CONTROL = ADF7021_REG1;
    AD7021_1_IOCTL();

    // configure ADF Tx/RX
    configureTxRx(modemState);

    /*
    ** Demodulator Setup (Register 4)
    */
    AD7021_CONTROL = ADF7021_REG4;
    AD7021_1_IOCTL();

    /*
    ** Fractional-N Synthesizer (Register 0)
    */

    if (div2 == 1U)
        divider = m_txFrequency / (ADF7021_PFD / 2U);
    else
        divider = m_txFrequency / ADF7021_PFD;

    // calculate Integer_N and Fractional_N divider values for Rx
    TX_N_Divider = floor(divider);
    divider = (divider - TX_N_Divider) * 32768;
    TX_F_Divider = floor(divider + 0.5);

    // setup tx register 0
    ADF7021_TX_REG0 = (uint32_t)0b0000;                 // register 0
#if defined(BIDIR_DATA_PIN)
    ADF7021_TX_REG0 |= (uint32_t)0b01000 << 27;         // mux regulator/transmit
#else
    ADF7021_TX_REG0 |= (uint32_t)0b01010 << 27;         // mux regulator/uart-spi enabled/transmit
#endif
    ADF7021_TX_REG0 |= (uint32_t)TX_N_Divider << 19;    // frequency - 15-bit Frac_N
    ADF7021_TX_REG0 |= (uint32_t)TX_F_Divider << 4;     // frequency - 8-bit Int_N

    /*
    ** Transmit Modulation (Register 2)
    */
    AD7021_CONTROL = ADF7021_REG2;
    AD7021_1_IOCTL();

    DEBUG2("IO::updateCal(): updating ADF calibration; modemState", modemState);

    if (m_tx)
        setTX();
    else
        setRX();
}

/// <summary>
/// 
/// </summary>
/// <returns></returns>
uint16_t IO::readRSSI()
{
    uint32_t AD7021_RB;
    uint16_t RB_word = 0U;
    uint8_t RB_code, gainCode, gainCorr;

    // Register 7, readback enable, ADC RSSI mode
    AD7021_RB = 0x0147;

    // Send control register
    for (int i = 8; i >= 0; i--) {
        if (ADF_BIT_READ(AD7021_RB, i) == HIGH)
            SDATA(HIGH);
        else
            SDATA(LOW);

        delayBit();
        SCLK(HIGH);
        delayBit();
        SCLK(LOW);
    }

    SDATA(LOW);

#if defined(DUPLEX)
    if (m_duplex || m_modemState == STATE_RSSI_CAL)
        SLE2(HIGH);
    else
        SLE1(HIGH);
#else
    SLE1(HIGH);
#endif

    delayBit();

    // Read SREAD pin
    for (int i = 17; i >= 0; i--) {
        SCLK(HIGH);
        delayBit();

        if ((i != 17) && (i != 0))
            RB_word |= ((SREAD() & 0x01) << (i - 1));

        SCLK(LOW);
        delayBit();
    }

#if defined(DUPLEX)
    if (m_duplex || m_modemState == STATE_RSSI_CAL)
        SLE2(LOW);
    else
        SLE1(LOW);
#else
    SLE1(LOW);
#endif

    // Process RSSI code
    RB_code = RB_word & 0x7f;
    gainCode = (RB_word >> 7) & 0x0f;

    switch (gainCode) {
    case 0b1010:
        gainCorr = 0U;
        break;
    case 0b0110:
        gainCorr = 24U;
        break;
    case 0b0101:
        gainCorr = 38U;
        break;
    case 0b0100:
        gainCorr = 58U;
        break;
    case 0b0000:
        gainCorr = 86U;
        break;
    default:
        gainCorr = 0U;
        break;
    }

    return (130 - (RB_code + gainCorr) / 2);
}

// ---------------------------------------------------------------------------
//  Private Class Members
// ---------------------------------------------------------------------------

/// <summary>
///
/// </summary>
void IO::configureBand()
{
    /*
    ** VCO/Oscillator (Register 1)
    */
    /** 136 - 174mhz */
    if ((m_txFrequency >= VHF_MIN) && (m_txFrequency < VHF_MAX)) {
        ADF7021_REG1 = ADF7021_REG1_VHF;            // VHF (80 - 200), external VCO
        div2 = 1U;
    }

    /** 216 - 225mhz */
    if ((m_txFrequency >= VHF_220_MIN) && (m_txFrequency < VHF_220_MAX)) {
        ADF7021_REG1 = ADF7021_REG1_VHF_220;        // VHF (200 - 450), external VCO
        div2 = 1U; // should be 2U?
    }

    /** 380 - 431mhz */
    if ((m_txFrequency >= UHF_380_MIN) && (m_txFrequency < UHF_380_MAX)) {
        // NOTE: I've included support for this band, but, this could be problematic due to
        // the external VCO control on most (if not all) hotspots
        ADF7021_REG1 = ADF7021_REG1_UHF_380;        // UHF (200 - 450), external VCO
        div2 = 1U;
    }

    /** 431 - 450mhz */
    if ((m_txFrequency >= UHF_1_MIN) && (m_txFrequency < UHF_1_MAX)) {
        ADF7021_REG1 = ADF7021_REG1_UHF_1;          // UHF (431 - 450), internal VCO
        div2 = 1U;
    }

    /** 450 - 470mhz */
    if ((m_txFrequency >= UHF_2_MIN) && (m_txFrequency < UHF_2_MAX)) {
        ADF7021_REG1 = ADF7021_REG1_UHF_2;          // UHF (450 - 470), internal VCO
        div2 = 1U;
    }

    /** 470 - 520mhz */
    if ((m_txFrequency >= UHF_T_MIN) && (m_txFrequency < UHF_T_MAX)) {
        // NOTE: I've included support for this band, but, this could be problematic due to
        // the external VCO control on most (if not all) hotspots
        ADF7021_REG1 = ADF7021_REG1_UHF_T;          // UHF (470 - 520), external VCO
#if defined(FORCE_UHF_INTERAL_L)
        div2 = 2U;
#else
        div2 = 1U;
#endif
    }

    /** 842 - 900mhz */
    if ((m_txFrequency >= UHF_800_MIN) && (m_txFrequency < UHF_800_MAX)) {
        ADF7021_REG1 = ADF7021_REG1_800;            // UHF (862 - 900), internal VCO
        div2 = 2U;
    }

    /** 900 - 950mhz */
    if ((m_txFrequency >= UHF_900_MIN) && (m_txFrequency < UHF_900_MAX)) {
        ADF7021_REG1 = ADF7021_REG1_900;            // UHF (900 - 950), internal VCO
        div2 = 2U;
    }

    if (div2 == 1U)
        f_div = 2U;
    else
        f_div = 1U;

    DEBUG3("IO::configureBand(): configuring ADF freq band; reg1/f_div", ADF7021_REG1, f_div);
}

/// <summary>
///
/// </summary>
/// <param name="modemState"></param>
void IO::configureTxRx(DVM_STATE modemState)
{
    uint16_t dmrDiscBW = ADF7021_DISC_BW_DMR, dmrPostBW = ADF7021_POST_BW_DMR;
    uint16_t p25DiscBW = ADF7021_DISC_BW_P25, p25PostBW = ADF7021_POST_BW_P25;
    uint16_t nxdnDiscBW = ADF7021_DISC_BW_NXDN, nxdnPostBW = ADF7021_POST_BW_NXDN;

    // configure DMR discriminator and post demodulator BW
    if (dmrDiscBW + m_dmrDiscBWAdj < 0U)
        dmrDiscBW = 0U;
    else
        dmrDiscBW = ADF7021_DISC_BW_DMR + m_dmrDiscBWAdj;
    if (dmrDiscBW > ADF7021_DISC_BW_MAX)
        dmrDiscBW = ADF7021_DISC_BW_MAX;

    if (dmrPostBW + m_dmrPostBWAdj < 0)
        dmrPostBW = 0U;
    else
        dmrPostBW = ADF7021_POST_BW_DMR + m_dmrPostBWAdj;
    if (dmrPostBW > ADF7021_POST_BW_MAX)
        dmrPostBW = ADF7021_POST_BW_MAX;

    // configure P25 discriminator and post demodulator BW
    if (p25DiscBW + m_p25DiscBWAdj < 0U)
        p25DiscBW = 0U;
    else
        p25DiscBW = ADF7021_DISC_BW_P25 + m_p25DiscBWAdj;
    if (p25DiscBW > ADF7021_DISC_BW_MAX)
        p25DiscBW = ADF7021_DISC_BW_MAX;

    if (p25PostBW + m_p25PostBWAdj < 0)
        p25PostBW = 0U;
    else
        p25PostBW = ADF7021_POST_BW_P25 + m_p25PostBWAdj;
    if (p25PostBW > ADF7021_POST_BW_MAX)
        p25PostBW = ADF7021_POST_BW_MAX;

    // configure NXDN discriminator and post demodulator BW
    if (nxdnDiscBW + m_nxdnDiscBWAdj < 0U)
        nxdnDiscBW = 0U;
    else
        nxdnDiscBW = ADF7021_DISC_BW_NXDN + m_nxdnDiscBWAdj;
    if (nxdnDiscBW > ADF7021_DISC_BW_MAX)
        nxdnDiscBW = ADF7021_DISC_BW_MAX;

    if (nxdnPostBW + m_nxdnPostBWAdj < 0)
        nxdnPostBW = 0U;
    else
        nxdnPostBW = ADF7021_POST_BW_NXDN + m_nxdnPostBWAdj;
    if (nxdnPostBW > ADF7021_POST_BW_MAX)
        nxdnPostBW = ADF7021_POST_BW_MAX;

    /*
    ** Configure the remaining registers based on modem state.
    */

    switch (modemState) {
    case STATE_CW: // 4FSK
        {
            // Dev: +1 symb (variable), symb rate = 4800

            /*
            ** Tx/Rx Clock (Register 3)
            */
/** Support for 14.7456 MHz TCXO (modified RF7021SE boards) */
#if defined(ADF7021_14_7456)
            ADF7021_REG3 = (uint32_t)ADF7021_REG3_ADDR;                 // Register Address 3
            ADF7021_REG3 |= (uint32_t)ADF7021_REG3_BBOS_DIV_8 << 4;     // Base Band Clock Divider
            ADF7021_REG3 |= (uint32_t)(3 & 0xFU) << 6;                  // Demodulator Clock Divider
            ADF7021_REG3 |= (uint32_t)(32 & 0xFFU) << 10;               // Data/Clock Recovery Divider (CDR)
            ADF7021_REG3 |= (uint32_t)(147 & 0xFFU) << 18;              // Sequencer Clock Divider
            ADF7021_REG3 |= (uint32_t)(10 & 0x3FU) << 26;               // AGC Clock Divider

/** Support for 12.2880 MHz TCXO */
#elif defined(ADF7021_12_2880)
            ADF7021_REG3 = (uint32_t)ADF7021_REG3_ADDR;                 // Register Address 3
            ADF7021_REG3 |= (uint32_t)ADF7021_REG3_BBOS_DIV_8 << 4;     // Base Band Clock Divider
            ADF7021_REG3 |= (uint32_t)(2 & 0xFU) << 6;                  // Demodulator Clock Divider
            ADF7021_REG3 |= (uint32_t)(40 & 0xFFU) << 10;               // Data/Clock Recovery Divider (CDR)
            ADF7021_REG3 |= (uint32_t)(123 & 0xFFU) << 18;              // Sequencer Clock Divider
            ADF7021_REG3 |= (uint32_t)(10 & 0x3FU) << 26;               // AGC Clock Divider
#endif

            /*
            ** AFC (Register 10)
            */
/** Support for 14.7456 MHz TCXO (modified RF7021SE boards) */
#if defined(ADF7021_14_7456)
            ADF7021_REG10 = (uint32_t)ADF7021_REG10_ADDR;               // Register Address 10
#if defined(ADF7021_ENABLE_4FSK_AFC)
            ADF7021_REG10 |= (uint32_t)ADF7021_REG10_AFC_ENABLE << 4;   // AFC Enable/Disable
#else
            ADF7021_REG10 |= (uint32_t)ADF7021_REG10_AFC_DISABLE << 4;  // AFC Enable/Disable
#endif
            ADF7021_REG10 |= (uint32_t)(569 & 0xFFFU) << 5;             // AFC Scaling Factor
            ADF7021_REG10 |= (uint32_t)(15 & 0xFU) << 17;               // KI
            ADF7021_REG10 |= (uint32_t)(4 & 0x7U) << 21;                // KP
            ADF7021_REG10 |= (uint32_t)(4 & 0xFFU) << 24;               // Maximum AFC Range

/** Support for 12.2880 MHz TCXO */
#elif defined(ADF7021_12_2880)
            ADF7021_REG10 = (uint32_t)ADF7021_REG10_ADDR;               // Register Address 10
#if defined(ADF7021_ENABLE_4FSK_AFC)
            ADF7021_REG10 |= (uint32_t)ADF7021_REG10_AFC_ENABLE << 4;   // AFC Enable/Disable
#else
            ADF7021_REG10 |= (uint32_t)ADF7021_REG10_AFC_DISABLE << 4;  // AFC Enable/Disable
#endif
            ADF7021_REG10 |= (uint32_t)(683 & 0xFFFU) << 5;             // AFC Scaling Factor
            ADF7021_REG10 |= (uint32_t)(15 & 0xFU) << 17;               // KI
            ADF7021_REG10 |= (uint32_t)(4 & 0x7U) << 21;                // KP
            ADF7021_REG10 |= (uint32_t)(4 & 0xFFU) << 24;               // Maximum AFC Range

#endif

            /*
            ** Demodulator Setup (Register 4)
            */
            // K=32
            ADF7021_REG4 = (uint32_t)ADF7021_REG4_ADDR;                 // Register Address 4
            ADF7021_REG4 |= (uint32_t)ADF7021_REG4_MODE_4FSK << 4;      // Demodulation Scheme
            ADF7021_REG4 |= (uint32_t)ADF7021_REG4_CROSS_PROD << 7;     // Dot Product
            ADF7021_REG4 |= (uint32_t)ADF7021_REG4_INV_CLKDAT << 8;     // Clock/Data Inversion
            ADF7021_REG4 |= (uint32_t)(dmrDiscBW & 0x3FFU) << 10;       // Discriminator BW
            ADF7021_REG4 |= (uint32_t)(dmrPostBW & 0xFFFU) << 20;       // Post Demod BW
            ADF7021_REG4 |= (uint32_t)ADF7021_REG4_IF_25K << 30;        // IF Filter

            /*
            ** 3FSK/4FSK Demod (Register 13)
            */
            ADF7021_REG13 = (uint32_t)0b1101 << 0;                      // Register Address 13
            ADF7021_REG13 |= (uint32_t)ADF7021_SLICER_TH_DMR << 4;      // Slicer Threshold

            /*
            ** Transmit Modulation (Register 2)
            */
            ADF7021_REG2 = (uint32_t)ADF7021_REG2_ADDR;                 // Register Address 2
            ADF7021_REG2 |= (uint32_t)ADF7021_REG2_MOD_4FSKRC << 4;     // Modulation Scheme
            ADF7021_REG2 |= (uint32_t)ADF7021_REG2_PA_DEF << 7;         // PA Enable & PA Bias
            ADF7021_REG2 |= (uint32_t)(m_rfPower & 0x3FU) << 13;        // PA Level (0 - Off, 63 - 13 dBm)
            ADF7021_REG2 |= (uint32_t)(m_cwIdTXLevel / div2) << 19;     // Freq. Deviation
            ADF7021_REG2 |= (uint32_t)ADF7021_REG2_INV_DATA << 28;      // Clock/Data Inversion
            ADF7021_REG2 |= (uint32_t)ADF7021_REG2_RC_5 << 30;          // R-Cosine Alpha
        }
        break;

    case STATE_DMR: // 4FSK
        {
            // Dev: +1 symb 648 Hz, symb rate = 4800

            /*
            ** Tx/Rx Clock (Register 3)
            */
/** Support for 14.7456 MHz TCXO (modified RF7021SE boards) */
#if defined(ADF7021_14_7456)
            ADF7021_REG3 = (uint32_t)ADF7021_REG3_ADDR;                 // Register Address 3
            ADF7021_REG3 |= (uint32_t)ADF7021_REG3_BBOS_DIV_8 << 4;     // Base Band Clock Divider
            ADF7021_REG3 |= (uint32_t)(3 & 0xFU) << 6;                  // Demodulator Clock Divider
            ADF7021_REG3 |= (uint32_t)(32 & 0xFFU) << 10;               // Data/Clock Recovery Divider (CDR)
            ADF7021_REG3 |= (uint32_t)(147 & 0xFFU) << 18;              // Sequencer Clock Divider
            ADF7021_REG3 |= (uint32_t)(10 & 0x3FU) << 26;               // AGC Clock Divider

/** Support for 12.2880 MHz TCXO */
#elif defined(ADF7021_12_2880)
            ADF7021_REG3 = (uint32_t)ADF7021_REG3_ADDR;                 // Register Address 3
            ADF7021_REG3 |= (uint32_t)ADF7021_REG3_BBOS_DIV_8 << 4;     // Base Band Clock Divider
            ADF7021_REG3 |= (uint32_t)(2 & 0xFU) << 6;                  // Demodulator Clock Divider
            ADF7021_REG3 |= (uint32_t)(40 & 0xFFU) << 10;               // Data/Clock Recovery Divider (CDR)
            ADF7021_REG3 |= (uint32_t)(123 & 0xFFU) << 18;              // Sequencer Clock Divider
            ADF7021_REG3 |= (uint32_t)(10 & 0x3FU) << 26;               // AGC Clock Divider
#endif

            /*
            ** AFC (Register 10)
            */
/** Support for 14.7456 MHz TCXO (modified RF7021SE boards) */
#if defined(ADF7021_14_7456)
            ADF7021_REG10 = (uint32_t)ADF7021_REG10_ADDR;               // Register Address 10
#if defined(ADF7021_ENABLE_4FSK_AFC)
            ADF7021_REG10 |= (uint32_t)ADF7021_REG10_AFC_ENABLE << 4;   // AFC Enable/Disable
#else
            ADF7021_REG10 |= (uint32_t)ADF7021_REG10_AFC_DISABLE << 4;  // AFC Enable/Disable
#endif
            ADF7021_REG10 |= (uint32_t)(569 & 0xFFFU) << 5;             // AFC Scaling Factor
            ADF7021_REG10 |= (uint32_t)(15 & 0xFU) << 17;               // KI
            ADF7021_REG10 |= (uint32_t)(4 & 0x7U) << 21;                // KP
            ADF7021_REG10 |= (uint32_t)(4 & 0xFFU) << 24;               // Maximum AFC Range

/** Support for 12.2880 MHz TCXO */
#elif defined(ADF7021_12_2880)
            ADF7021_REG10 = (uint32_t)ADF7021_REG10_ADDR;               // Register Address 10
#if defined(ADF7021_ENABLE_4FSK_AFC)
            ADF7021_REG10 |= (uint32_t)ADF7021_REG10_AFC_ENABLE << 4;   // AFC Enable/Disable
#else
            ADF7021_REG10 |= (uint32_t)ADF7021_REG10_AFC_DISABLE << 4;  // AFC Enable/Disable
#endif
            ADF7021_REG10 |= (uint32_t)(683 & 0xFFFU) << 5;             // AFC Scaling Factor
            ADF7021_REG10 |= (uint32_t)(15 & 0xFU) << 17;               // KI
            ADF7021_REG10 |= (uint32_t)(4 & 0x7U) << 21;                // KP
            ADF7021_REG10 |= (uint32_t)(4 & 0xFFU) << 24;               // Maximum AFC Range

#endif

            /*
            ** Demodulator Setup (Register 4)
            */
            // K=32
            ADF7021_REG4 = (uint32_t)ADF7021_REG4_ADDR;                 // Register Address 4
            ADF7021_REG4 |= (uint32_t)ADF7021_REG4_MODE_4FSK << 4;      // Demodulation Scheme
            ADF7021_REG4 |= (uint32_t)ADF7021_REG4_CROSS_PROD << 7;     // Dot Product
            ADF7021_REG4 |= (uint32_t)ADF7021_REG4_INV_CLKDAT << 8;     // Clock/Data Inversion
            ADF7021_REG4 |= (uint32_t)(dmrDiscBW & 0x3FFU) << 10;       // Discriminator BW
            ADF7021_REG4 |= (uint32_t)(dmrPostBW & 0xFFFU) << 20;       // Post Demod BW
            ADF7021_REG4 |= (uint32_t)ADF7021_REG4_IF_125K << 30;       // IF Filter

            /*
            ** 3FSK/4FSK Demod (Register 13)
            */
            ADF7021_REG13 = (uint32_t)0b1101 << 0;                      // Register Address 13
            ADF7021_REG13 |= (uint32_t)ADF7021_SLICER_TH_DMR << 4;      // Slicer Threshold

            /*
            ** Transmit Modulation (Register 2)
            */
            ADF7021_REG2 = (uint32_t)ADF7021_REG2_ADDR;                 // Register Address 2
            ADF7021_REG2 |= (uint32_t)ADF7021_REG2_MOD_4FSKRC << 4;     // Modulation Scheme
            ADF7021_REG2 |= (uint32_t)ADF7021_REG2_PA_DEF << 7;         // PA Enable & PA Bias
            ADF7021_REG2 |= (uint32_t)(m_rfPower & 0x3FU) << 13;        // PA Level (0 - Off, 63 - 13 dBm)
            ADF7021_REG2 |= (uint32_t)(dmrDev / div2) << 19;            // Freq. Deviation
            ADF7021_REG2 |= (uint32_t)ADF7021_REG2_INV_DATA << 28;      // Clock/Data Inversion
            ADF7021_REG2 |= (uint32_t)ADF7021_REG2_RC_5 << 30;          // R-Cosine Alpha
        }
        break;

    case STATE_P25: // 4FSK
        {
            // Dev: +1 symb 600 Hz, symb rate = 4800

            /*
            ** Tx/Rx Clock (Register 3)
            */
/** Support for 14.7456 MHz TCXO (modified RF7021SE boards) */
#if defined(ADF7021_14_7456)
            ADF7021_REG3 = (uint32_t)ADF7021_REG3_ADDR;                 // Register Address 3
            ADF7021_REG3 |= (uint32_t)ADF7021_REG3_BBOS_DIV_8 << 4;     // Base Band Clock Divider
            ADF7021_REG3 |= (uint32_t)(3 & 0xFU) << 6;                  // Demodulator Clock Divider
            ADF7021_REG3 |= (uint32_t)(32 & 0xFFU) << 10;               // Data/Clock Recovery Divider (CDR)
            ADF7021_REG3 |= (uint32_t)(147 & 0xFFU) << 18;              // Sequencer Clock Divider
            ADF7021_REG3 |= (uint32_t)(10 & 0x3FU) << 26;               // AGC Clock Divider

/** Support for 12.2880 MHz TCXO */
#elif defined(ADF7021_12_2880)
            ADF7021_REG3 = (uint32_t)ADF7021_REG3_ADDR;                 // Register Address 3
            ADF7021_REG3 |= (uint32_t)ADF7021_REG3_BBOS_DIV_8 << 4;     // Base Band Clock Divider
            ADF7021_REG3 |= (uint32_t)(2 & 0xFU) << 6;                  // Demodulator Clock Divider
            ADF7021_REG3 |= (uint32_t)(40 & 0xFFU) << 10;               // Data/Clock Recovery Divider (CDR)
            ADF7021_REG3 |= (uint32_t)(123 & 0xFFU) << 18;              // Sequencer Clock Divider
            ADF7021_REG3 |= (uint32_t)(10 & 0x3FU) << 26;               // AGC Clock Divider
#endif

            /*
            ** AFC (Register 10)
            */
/** Support for 14.7456 MHz TCXO (modified RF7021SE boards) */
#if defined(ADF7021_14_7456)
            ADF7021_REG10 = (uint32_t)ADF7021_REG10_ADDR;               // Register Address 10
#if defined(ADF7021_ENABLE_4FSK_AFC)
            ADF7021_REG10 |= (uint32_t)ADF7021_REG10_AFC_ENABLE << 4;   // AFC Enable/Disable
#else
            ADF7021_REG10 |= (uint32_t)ADF7021_REG10_AFC_DISABLE << 4;  // AFC Enable/Disable
#endif
            ADF7021_REG10 |= (uint32_t)(569 & 0xFFFU) << 5;             // AFC Scaling Factor
            ADF7021_REG10 |= (uint32_t)(15 & 0xFU) << 17;               // KI
            ADF7021_REG10 |= (uint32_t)(4 & 0x7U) << 21;                // KP
            ADF7021_REG10 |= (uint32_t)(4 & 0xFFU) << 24;               // Maximum AFC Range

/** Support for 12.2880 MHz TCXO */
#elif defined(ADF7021_12_2880)
            ADF7021_REG10 = (uint32_t)ADF7021_REG10_ADDR;               // Register Address 10
#if defined(ADF7021_ENABLE_4FSK_AFC)
            ADF7021_REG10 |= (uint32_t)ADF7021_REG10_AFC_ENABLE << 4;   // AFC Enable/Disable
#else
            ADF7021_REG10 |= (uint32_t)ADF7021_REG10_AFC_DISABLE << 4;  // AFC Enable/Disable
#endif
            ADF7021_REG10 |= (uint32_t)(683 & 0xFFFU) << 5;             // AFC Scaling Factor
            ADF7021_REG10 |= (uint32_t)(15 & 0xFU) << 17;               // KI
            ADF7021_REG10 |= (uint32_t)(4 & 0x7U) << 21;                // KP
            ADF7021_REG10 |= (uint32_t)(4 & 0xFFU) << 24;               // Maximum AFC Range

#endif

            /*
            ** Demodulator Setup (Register 4)
            */
            // K=32
            ADF7021_REG4 = (uint32_t)ADF7021_REG4_ADDR;                 // Register Address 4
            ADF7021_REG4 |= (uint32_t)ADF7021_REG4_MODE_4FSK << 4;      // Demodulation Scheme
            ADF7021_REG4 |= (uint32_t)ADF7021_REG4_CROSS_PROD << 7;     // Dot Product
            ADF7021_REG4 |= (uint32_t)ADF7021_REG4_INV_CLKDAT << 8;     // Clock/Data Inversion
            ADF7021_REG4 |= (uint32_t)(p25DiscBW & 0x3FFU) << 10;       // Discriminator BW
            ADF7021_REG4 |= (uint32_t)(p25PostBW & 0xFFFU) << 20;       // Post Demod BW
            ADF7021_REG4 |= (uint32_t)ADF7021_REG4_IF_125K << 30;       // IF Filter

            /*
            ** 3FSK/4FSK Demod (Register 13)
            */
            ADF7021_REG13 = (uint32_t)ADF70210_REG13_ADDR;              // Register Address 13
            ADF7021_REG13 |= (uint32_t)ADF7021_SLICER_TH_P25 << 4;      // Slicer Threshold

            /*
            ** Transmit Modulation (Register 2)
            */
            ADF7021_REG2 = (uint32_t)ADF7021_REG2_ADDR;                 // Register Address 2
            ADF7021_REG2 |= (uint32_t)ADF7021_REG2_MOD_4FSKRC << 4;     // Modulation Scheme
            ADF7021_REG2 |= (uint32_t)ADF7021_REG2_PA_DEF << 7;         // PA Enable & PA Bias
            ADF7021_REG2 |= (uint32_t)(m_rfPower & 0x3FU) << 13;        // PA Level (0 - Off, 63 - 13 dBm)
            ADF7021_REG2 |= (uint32_t)(p25Dev / div2) << 19;            // Freq. Deviation
            ADF7021_REG2 |= (uint32_t)ADF7021_REG2_INV_DATA << 28;      // Clock/Data Inversion
            ADF7021_REG2 |= (uint32_t)ADF7021_REG2_RC_5 << 30;          // R-Cosine Alpha
        }
        break;
    case STATE_NXDN: // 4FSK
        {
            // Dev: +1 symb 350 Hz, symb rate = 2400
            // Dev: +1 symb 800 Hz, symb rate = 4800

            /*
            ** Tx/Rx Clock (Register 3)
            */
/** Support for 14.7456 MHz TCXO (modified RF7021SE boards) */
#if defined(ADF7021_14_7456)
            ADF7021_REG3 = (uint32_t)ADF7021_REG3_ADDR;                 // Register Address 3
            ADF7021_REG3 |= (uint32_t)ADF7021_REG3_BBOS_DIV_8 << 4;     // Base Band Clock Divider
#if defined(NXDN_9600_BAUD)
            ADF7021_REG3 |= (uint32_t)(3 & 0xFU) << 6;                  // Demodulator Clock Divider
            ADF7021_REG3 |= (uint32_t)(32 & 0xFFU) << 10;               // Data/Clock Recovery Divider (CDR)
#else
            ADF7021_REG3 |= (uint32_t)(4 & 0xFU) << 6;                  // Demodulator Clock Divider
            ADF7021_REG3 |= (uint32_t)(48 & 0xFFU) << 10;               // Data/Clock Recovery Divider (CDR)
#endif
            ADF7021_REG3 |= (uint32_t)(147 & 0xFFU) << 18;              // Sequencer Clock Divider
            ADF7021_REG3 |= (uint32_t)(10 & 0x3FU) << 26;               // AGC Clock Divider

/** Support for 12.2880 MHz TCXO */
#elif defined(ADF7021_12_2880)
            ADF7021_REG3 = (uint32_t)ADF7021_REG3_ADDR;                 // Register Address 3
            ADF7021_REG3 |= (uint32_t)ADF7021_REG3_BBOS_DIV_8 << 4;     // Base Band Clock Divider
#if defined(NXDN_9600_BAUD)
            ADF7021_REG3 |= (uint32_t)(2 & 0xFU) << 6;                  // Demodulator Clock Divider
            ADF7021_REG3 |= (uint32_t)(40 & 0xFFU) << 10;               // Data/Clock Recovery Divider (CDR)
#else
            ADF7021_REG3 |= (uint32_t)(4 & 0xFU) << 6;                  // Demodulator Clock Divider
            ADF7021_REG3 |= (uint32_t)(40 & 0xFFU) << 10;               // Data/Clock Recovery Divider (CDR)
#endif
            ADF7021_REG3 |= (uint32_t)(123 & 0xFFU) << 18;              // Sequencer Clock Divider
            ADF7021_REG3 |= (uint32_t)(10 & 0x3FU) << 26;               // AGC Clock Divider
#endif

            /*
            ** AFC (Register 10)
            */
/** Support for 14.7456 MHz TCXO (modified RF7021SE boards) */
#if defined(ADF7021_14_7456)
            ADF7021_REG10 = (uint32_t)ADF7021_REG10_ADDR;               // Register Address 10
#if defined(ADF7021_ENABLE_4FSK_AFC)
            ADF7021_REG10 |= (uint32_t)ADF7021_REG10_AFC_ENABLE << 4;   // AFC Enable/Disable
#else
            ADF7021_REG10 |= (uint32_t)ADF7021_REG10_AFC_DISABLE << 4;  // AFC Enable/Disable
#endif
            ADF7021_REG10 |= (uint32_t)(569 & 0xFFFU) << 5;             // AFC Scaling Factor
            ADF7021_REG10 |= (uint32_t)(15 & 0xFU) << 17;               // KI
            ADF7021_REG10 |= (uint32_t)(4 & 0x7U) << 21;                // KP
            ADF7021_REG10 |= (uint32_t)(4 & 0xFFU) << 24;               // Maximum AFC Range

/** Support for 12.2880 MHz TCXO */
#elif defined(ADF7021_12_2880)
            ADF7021_REG10 = (uint32_t)ADF7021_REG10_ADDR;               // Register Address 10
#if defined(ADF7021_ENABLE_4FSK_AFC)
            ADF7021_REG10 |= (uint32_t)ADF7021_REG10_AFC_ENABLE << 4;   // AFC Enable/Disable
#else
            ADF7021_REG10 |= (uint32_t)ADF7021_REG10_AFC_DISABLE << 4;  // AFC Enable/Disable
#endif
            ADF7021_REG10 |= (uint32_t)(683 & 0xFFFU) << 5;             // AFC Scaling Factor
            ADF7021_REG10 |= (uint32_t)(15 & 0xFU) << 17;               // KI
            ADF7021_REG10 |= (uint32_t)(4 & 0x7U) << 21;                // KP
            ADF7021_REG10 |= (uint32_t)(4 & 0xFFU) << 24;               // Maximum AFC Range
#endif

            /*
            ** Demodulator Setup (Register 4)
            */
            // K=32
            ADF7021_REG4 = (uint32_t)ADF7021_REG4_ADDR;                 // Register Address 4
            ADF7021_REG4 |= (uint32_t)ADF7021_REG4_MODE_4FSK << 4;      // Demodulation Scheme
            ADF7021_REG4 |= (uint32_t)ADF7021_REG4_CROSS_PROD << 7;     // Dot Product
            ADF7021_REG4 |= (uint32_t)ADF7021_REG4_INV_CLKDAT << 8;     // Clock/Data Inversion
            ADF7021_REG4 |= (uint32_t)(nxdnDiscBW & 0x3FFU) << 10;      // Discriminator BW
            ADF7021_REG4 |= (uint32_t)(nxdnPostBW & 0xFFFU) << 20;      // Post Demod BW
            ADF7021_REG4 |= (uint32_t)ADF7021_REG4_IF_125K << 30;       // IF Filter

            /*
            ** 3FSK/4FSK Demod (Register 13)
            */
            ADF7021_REG13 = (uint32_t)ADF70210_REG13_ADDR;              // Register Address 13
            ADF7021_REG13 |= (uint32_t)ADF7021_SLICER_TH_NXDN << 4;     // Slicer Threshold

            /*
            ** Transmit Modulation (Register 2)
            */
            ADF7021_REG2 = (uint32_t)ADF7021_REG2_ADDR;                 // Register Address 2
            ADF7021_REG2 |= (uint32_t)ADF7021_REG2_MOD_4FSKRC << 4;     // Modulation Scheme
            ADF7021_REG2 |= (uint32_t)ADF7021_REG2_PA_DEF << 7;         // PA Enable & PA Bias
            ADF7021_REG2 |= (uint32_t)(m_rfPower & 0x3FU) << 13;        // PA Level (0 - Off, 63 - 13 dBm)
            ADF7021_REG2 |= (uint32_t)(nxdnDev / div2) << 19;           // Freq. Deviation
            ADF7021_REG2 |= (uint32_t)ADF7021_REG2_INV_DATA << 28;      // Clock/Data Inversion
            ADF7021_REG2 |= (uint32_t)ADF7021_REG2_RC_5 << 30;          // R-Cosine Alpha
        }
        break;
    default: // GMSK
        {
            // Dev: 1200 Hz, symb rate = 4800

            /*
            ** Tx/Rx Clock (Register 3)
            */
/** Support for 14.7456 MHz TCXO (modified RF7021SE boards) */
#if defined(ADF7021_14_7456)
            ADF7021_REG3 = (uint32_t)ADF7021_REG3_ADDR;                 // Register Address 3
            ADF7021_REG3 |= (uint32_t)ADF7021_REG3_BBOS_DIV_8 << 4;     // Base Band Clock Divider
            ADF7021_REG3 |= (uint32_t)(6 & 0xFU) << 6;                  // Demodulator Clock Divider
            ADF7021_REG3 |= (uint32_t)(8 & 0xFFU) << 10;                // Data/Clock Recovery Divider (CDR)
            ADF7021_REG3 |= (uint32_t)(147 & 0xFFU) << 18;              // Sequencer Clock Divider
            ADF7021_REG3 |= (uint32_t)(10 & 0x3FU) << 26;               // AGC Clock Divider

/** Support for 12.2880 MHz TCXO */
#elif defined(ADF7021_12_2880)
            ADF7021_REG3 = (uint32_t)ADF7021_REG3_ADDR;                 // Register Address 3
            ADF7021_REG3 |= (uint32_t)ADF7021_REG3_BBOS_DIV_8 << 4;     // Base Band Clock Divider
            ADF7021_REG3 |= (uint32_t)(5 & 0xFU) << 6;                  // Demodulator Clock Divider
            ADF7021_REG3 |= (uint32_t)(16 & 0xFFU) << 10;               // Data/Clock Recovery Divider (CDR)
            ADF7021_REG3 |= (uint32_t)(123 & 0xFFU) << 18;              // Sequencer Clock Divider
            ADF7021_REG3 |= (uint32_t)(10 & 0x3FU) << 26;               // AGC Clock Divider
#endif

            /*
            ** AFC (Register 10)
            */
/** Support for 14.7456 MHz TCXO (modified RF7021SE boards) */
#if defined(ADF7021_14_7456)
            ADF7021_REG10 = (uint32_t)ADF7021_REG10_ADDR;               // Register Address 10
            ADF7021_REG10 |= (uint32_t)ADF7021_REG10_AFC_ENABLE << 4;   // AFC Enable/Disable
            ADF7021_REG10 |= (uint32_t)(569 & 0xFFFU) << 5;             // AFC Scaling Factor
            ADF7021_REG10 |= (uint32_t)(11 & 0xFU) << 17;               // KI
            ADF7021_REG10 |= (uint32_t)(4 & 0x7U) << 21;                // KP
            ADF7021_REG10 |= (uint32_t)(12 & 0xFFU) << 24;              // Maximum AFC Range

/** Support for 12.2880 MHz TCXO */
#elif defined(ADF7021_12_2880)
            ADF7021_REG10 = (uint32_t)ADF7021_REG10_ADDR;               // Register Address 10
            ADF7021_REG10 |= (uint32_t)ADF7021_REG10_AFC_ENABLE << 4;   // AFC Enable/Disable
            ADF7021_REG10 |= (uint32_t)(683 & 0xFFFU) << 5;             // AFC Scaling Factor
            ADF7021_REG10 |= (uint32_t)(11 & 0xFU) << 17;               // KI
            ADF7021_REG10 |= (uint32_t)(4 & 0x7U) << 21;                // KP
            ADF7021_REG10 |= (uint32_t)(12 & 0xFFU) << 24;              // Maximum AFC Range

#endif

            /*
            ** Demodulator Setup (Register 4)
            */
            // K=32
            ADF7021_REG4 = (uint32_t)ADF7021_REG4_ADDR;                 // Register Address 4
            ADF7021_REG4 |= (uint32_t)ADF7021_REG4_MODE_GMSK << 4;      // Demodulation Scheme
            ADF7021_REG4 |= (uint32_t)ADF7021_REG4_DOT_PROD << 7;       // Dot Product
            ADF7021_REG4 |= (uint32_t)ADF7021_REG4_INV_DATA << 8;       // Clock/Data Inversion
            ADF7021_REG4 |= (uint32_t)ADF7021_DISC_BW_DEFAULT << 10;    // Discriminator BW
            ADF7021_REG4 |= (uint32_t)ADF7021_POST_BW_DEFAULT << 20;    // Post Demod BW
            ADF7021_REG4 |= (uint32_t)ADF7021_REG4_IF_125K << 30;       // IF Filter

            /*
            ** 3FSK/4FSK Demod (Register 13)
            */
            ADF7021_REG13 = (uint32_t)ADF70210_REG13_ADDR;              // Register Address 13
            ADF7021_REG13 |= (uint32_t)ADF7021_SLICER_TH_DEFAULT << 4;  // Slicer Threshold

            /*
            ** Transmit Modulation (Register 2)
            */
            ADF7021_REG2 = (uint32_t)ADF7021_REG2_ADDR;                 // Register Address 2
            ADF7021_REG2 |= (uint32_t)ADF7021_REG2_MOD_GMSK << 4;       // Modulation Scheme
            ADF7021_REG2 |= (uint32_t)ADF7021_REG2_PA_DEF << 7;         // PA Enable & PA Bias
            ADF7021_REG2 |= (uint32_t)(m_rfPower & 0x3FU) << 13;        // PA Level (0 - Off, 63 - 13 dBm)
            ADF7021_REG2 |= (uint32_t)(ADF7021_DEV_DEFAULT / div2) << 19; // Freq. Deviation
            ADF7021_REG2 |= (uint32_t)ADF7021_REG2_INV_NORM << 28;      // Clock/Data Inversion
            ADF7021_REG2 |= (uint32_t)ADF7021_REG2_RC_5 << 30;          // R-Cosine Alpha
        }
        break;
    }

    DEBUG5("IO::configureTxRx(): configuring ADF Tx/Rx values; dmrDiscBW/dmrPostBW/p25DiscBW/p25PostBW", dmrDiscBW, dmrPostBW, p25DiscBW, p25PostBW);
    DEBUG3("IO::configureTxRx(): configuring ADF Tx/Rx values; nxdnDiscBW/nxdnPostBW", nxdnDiscBW, nxdnPostBW);
    DEBUG5("IO::configureTxRx(): configuring ADF Tx/Rx values; dmrSymDev/p25SymDev/nxdnSymDev/rfPower", (uint16_t)((ADF7021_PFD * dmrDev) / (f_div * 65536)),
        (uint16_t)((ADF7021_PFD * p25Dev) / (f_div * 65536)), (uint16_t)((ADF7021_PFD * nxdnDev) / (f_div * 65536)), m_rfPower);
}

/// <summary>
/// 
/// </summary>
void IO::setTX()
{
    // PTT pin on (doing it earlier helps to measure timing impact)
    setPTTInt(HIGH);

    // Send register 0 for TX operation, but do not activate yet.
    // This is done in the interrupt at the correct time
    AD7021_CONTROL = ADF7021_TX_REG0;
    AD7021_1_IOCTL(false);

#if defined(BIDIR_DATA_PIN)
    setDataDirOut(true);  // data pin output mode
#endif

    toTxRequest = true;
    while(CLK());
}

/// <summary>
/// 
/// </summary>
/// <param name="doSle"></param>
void IO::setRX(bool doSle)
{
    // PTT pin off (doing it earlier helps to measure timing impact)
    setPTTInt(LOW);

    // Send register 0 for RX operation, but do not activate yet.
    // This is done in the interrupt at the correct time
    AD7021_CONTROL = ADF7021_RX_REG0;
    AD7021_1_IOCTL(doSle);

#if defined(BIDIR_DATA_PIN)
    setDataDirOut(false);  // data pin output mode
#endif

    if (!doSle) {
        toRxRequest = true;
        while(toRxRequest) {
            asm volatile ("nop"); 
        }
    }
}

#endif // ENABLE_ADF7021
