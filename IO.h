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
*   Copyright (C) 2017-2021 Bryan Biedenkapp N2PLL
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
#if !defined(__IO_H__)
#define __IO_H__

#include "Defines.h"
#include "Globals.h"
#include "BitBuffer.h"

// ---------------------------------------------------------------------------
//  Constants
// ---------------------------------------------------------------------------

#if defined(DUPLEX)
#if defined(STM32_USB_HOST)
#define CAL_DLY_LOOP 98950U
#else
#define CAL_DLY_LOOP 96100U
#endif
#else
#if defined(STM32_USB_HOST)
#define CAL_DLY_LOOP 110850U
#else
#define CAL_DLY_LOOP 104600U
#endif
#endif

enum ADF_GAIN_MODE {
    // AGC automatic, default settings
    ADF_GAIN_AUTO = 0U,
    // AGC automatic with high LNA linearity
    ADF_GAIN_AUTO_LIN = 1U,
    // AGC OFF, lowest gain
    ADF_GAIN_LOW = 2U,
    // AGC OFF, highest gain
    ADF_GAIN_HIGH = 3U
};

// ---------------------------------------------------------------------------
//  Class Declaration
//      Implements the input/output data path with the radio air interface.
// ---------------------------------------------------------------------------

class DSP_FW_API IO {
public:
    /// <summary>Initializes a new instance of the IO class.</summary>
    IO();

    /// <summary>Starts air interface sampler.</summary>
    void start();

    /// <summary>Process bits from air interface.</summary>
    void process();

    /// <summary>Write bits to air interface.</summary>
    void write(uint8_t* data, uint16_t length, const uint8_t* control = NULL);

    /// <summary>Helper to get how much space the transmit ring buffer has for samples.</summary>
    uint16_t getSpace(void) const;

    /// <summary></summary>
    void setDecode(bool dcd);

    /// <summary>Set modem mode.</summary>
    void setMode(DVM_STATE modemState);

    /// <summary>Hardware interrupt handler.</summary>
    void interrupt1();
#if defined(DUPLEX)
    /// <summary>Hardware interrupt handler.</summary>
    void interrupt2();
#endif

    /// <summary>Sets the ADF7021 RF configuration.</summary>
    void rf1Conf(DVM_STATE modemState, bool reset);
#if defined(DUPLEX)
    /// <summary>Sets the ADF7021 RF configuration.</summary>
    void rf2Conf(DVM_STATE modemState);
#endif

    /// <summary></summary>
    void setDeviations(uint8_t dmrTXLevel, uint8_t p25TXLevel);
    /// <summary>Sets the RF parameters.</summary>
    uint8_t setRFParams(uint32_t rxFreq, uint32_t txFreq, uint8_t rfPower, ADF_GAIN_MODE gainMode);
    /// <summary>Sets the RF adjustment parameters.</summary>
    void setRFAdjust(int8_t dmrDiscBWAdj, int8_t p25DiscBWAdj, int8_t dmrPostBWAdj, int8_t p25PostBWAdj);

    /// <summary>Flag indicating the TX ring buffer has overflowed.</summary>
    bool hasTXOverflow(void);
    /// <summary>Flag indicating the RX ring buffer has overflowed.</summary>
    bool hasRXOverflow(void);

    /// <summary></summary>
    void resetWatchdog(void);
    /// <summary></summary>
    uint32_t getWatchdog(void);

    /// <summary>Gets the CPU type the firmware is running on.</summary>
    uint8_t getCPU() const;

    /// <summary>Gets the unique identifier for the air interface.</summary>
    void getUDID(uint8_t* buffer);

    /// <summary></summary>
    void updateCal(DVM_STATE modemState);

    /// <summary></summary>
    void delayBit(void);

    /// <summary></summary>
    uint16_t readRSSI(void);

    /// <summary></summary>
    void selfTest();

    /// <summary></summary>
    void getIntCounter(uint16_t& int1, uint16_t& int2);
#if defined(ZUMSPOT_ADF7021) || defined(LONESTAR_USB) || defined(SKYBRIDGE_HS)
    /// <summary></summary>
    bool isDualBand();
#endif

    /// <summary></summary>
    void SCLK(bool on);
    /// <summary></summary>
    void SDATA(bool on);
    /// <summary></summary>
    bool SREAD();
    /// <summary></summary>
    void SLE1(bool on);

#if defined(DUPLEX)
    /// <summary></summary>
    void SLE2(bool on);
    /// <summary></summary>
    bool RXD2();
#endif
    /// <summary></summary>
    void CE(bool on);
    /// <summary></summary>
    bool RXD1();
    /// <summary></summary>
    bool CLK();

private:
    bool m_started;

    BitBuffer m_rxBuffer;
    BitBuffer m_txBuffer;

    uint32_t m_ledCount;
    bool m_ledValue;

    volatile uint32_t m_watchdog;

    volatile uint16_t m_int1Counter;
    volatile uint16_t m_int2Counter;

    uint32_t m_rxFrequency;
    uint32_t m_txFrequency;
    uint8_t m_rfPower;
    ADF_GAIN_MODE m_gainMode;

    /// <summary>Helper to check the frequencies are within band ranges of the ADF7021.</summary>
    void checkBand(uint32_t rxFreq, uint32_t txFreq);
#if defined(ZUMSPOT_ADF7021) || defined(LONESTAR_USB) || defined(SKYBRIDGE_HS)
    /// <summary></summary>
    void setBandVHF(bool enable);
    /// <summary></summary>
    bool hasSingleADF7021();
#endif
    /// <summary></summary>
    void configureBand();
    /// <summary></summary>
    void configureTxRx(DVM_STATE modemState);

    /// <summary></summary>
    void setTX();
    /// <summary></summary>
    void setRX(bool doSle = true);

    /// <summary></summary>
    void delayIfCal();
    /// <summary></summary>
    void delayReset();
    /// <summary></summary>
    void delayUS(uint32_t us);

    // Hardware specific routines
    /// <summary>Initializes hardware interrupts.</summary>
    void initInt();
    /// <summary>Starts hardware interrupts.</summary>
    void startInt();

    /// <summary></summary>
    void setTXDInt(bool on);
#if defined(BIDIR_DATA_PIN)
    /// <summary></summary>
    void setDataDirOut(bool dir);
    /// <summary></summary>
    void setRXDInt(bool on);
#endif

    /// <summary></summary>
    void setLEDInt(bool on);
    /// <summary></summary>
    void setPTTInt(bool on);
    /// <summary></summary>
    void setCOSInt(bool on);

    /// <summary></summary>
    void setDMRInt(bool on);
    /// <summary></summary>
    void setP25Int(bool on);
};

#endif
