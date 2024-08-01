// SPDX-License-Identifier: GPL-2.0-only
/*
 * Digital Voice Modem - Hotspot Firmware
 * GPLv2 Open Source. Use is subject to license terms.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 *  Copyright (C) 2015,2016,2020 Jonathan Naylor, G4KLX
 *  Copyright (C) 2016,2017,2018,2019,2020 Andy Uribe, CA6JAU
 *  Copyright (C) 2017 Danilo, DB4PLE
 *  Copyright (C) 2017-2021 Bryan Biedenkapp, N2PLL
 *
 */
/**
 * @file IO.h
 * @ingroup hotspot_fw
 * @file IO.cpp
 * @ingroup hotspot_fw
 * @file IOSTM.cpp
 * @ingroup hotspot_fw
 * @file ADF7021.cpp
 * @ingroup hotspot_fw
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
#define CAL_DLY_LOOP 96100U
#else
#define CAL_DLY_LOOP 104600U
#endif

/**
 * @brief ADF7021 Gain Modes
 */
enum ADF_GAIN_MODE {
    ADF_GAIN_AUTO = 0U,         //! AGC automatic, default settings
    ADF_GAIN_AUTO_LIN = 1U,     //! AGC automatic with high LNA linearity
    ADF_GAIN_LOW = 2U,          //! AGC OFF, lowest gain
    ADF_GAIN_HIGH = 3U          //! AGC OFF, highest gain
};

// ---------------------------------------------------------------------------
//  Class Declaration
// ---------------------------------------------------------------------------

/**
 * @brief Implements the input/output data path with the radio air interface.
 * @ingroup hotspot_fw
 */
class DSP_FW_API IO {
public:
    /**
     * @brief Initializes a new instance of the IO class.
     */
    IO();

    /**
     * @brief Initializes the air interface sampler.
     */
    void init();
    /**
     * @brief Starts air interface sampler.
     */
    void start();

    /**
     * @brief Process bits from air interface.
     */
    void process();

    /**
     * @brief Write bits to air interface.
     * @param data Data to write.
     * @param length Length of data buffer.
     * @param control 
     */
    void write(uint8_t* data, uint16_t length, const uint8_t* control = NULL);

    /**
     * @brief Helper to get how much space the transmit ring buffer has for samples.
     * @returns uint16_t Amount of space in the transmit ring buffer for samples.
     */
    uint16_t getSpace(void) const;

    /**
     * @brief 
     * @param dcd 
     */
    void setDecode(bool dcd);

    /**
     * @brief Set modem mode.
     * @param modemState 
     */
    void setMode(DVM_STATE modemState);

    /**
     * @brief Hardware interrupt handler.
     */
    void interrupt1();
#if defined(DUPLEX)
    /**
     * @brief Hardware interrupt handler.
     */
    void interrupt2();
#endif

    /**
     * @brief Sets the ADF7021 RF configuration.
     * @param modemState 
     * @param reset 
     */
    void rf1Conf(DVM_STATE modemState, bool reset);
#if defined(DUPLEX)
    /**
     * @brief Sets the ADF7021 RF configuration.
     * @param modemState 
     */
    void rf2Conf(DVM_STATE modemState);
#endif

    /**
     * @brief Sets the deviation levels.
     * @param dmrTXLevel DMR Transmit Level.
     * @param p25TXLevel P25 Transmit Level.
     * @param nxdnTXLevel NXDN Transmit Level.
     */
    void setDeviations(uint8_t dmrTXLevel, uint8_t p25TXLevel, uint8_t nxdnTXLevel);
    /**
     * @brief Sets the RF parameters.
     * @param rxFreq Receive Frequency (hz).
     * @param txFreq Transmit Frequency (hz).
     * @param rfPower RF Power Level.
     * @param gainMode Gain Mode.
     * @returns uint8_t Reason code.
     */
    uint8_t setRFParams(uint32_t rxFreq, uint32_t txFreq, uint8_t rfPower, ADF_GAIN_MODE gainMode);
    /**
     * @brief Sets the RF adjustment parameters.
     * @param dmrDiscBWAdj DMR Discriminator Bandwidth Adjust.
     * @param p25DiscBWAdj P25 Discriminator Bandwidth Adjust.
     * @param nxdnDiscBWAdj NXDN Discriminator Bandwidth Adjust.
     * @param dmrPostBWAdj DMR Post Bandwidth Adjust.
     * @param p25PostBWAdj P25 Post Bandwidth Adjust.
     * @param nxdnPostBWAdj NXDN Post Bandwidth Adjust.
     */
    void setRFAdjust(int8_t dmrDiscBWAdj, int8_t p25DiscBWAdj, int8_t nxdnDiscBWAdj, int8_t dmrPostBWAdj, int8_t p25PostBWAdj, int8_t nxdnPostBWAdj);
    /**
     * @brief Sets the RF AFC adjustment parameters.
     * @param afcEnable Flag indicating the Automatic Frequency Correction is enabled.
     * @param afcKI 
     * @param afcKP 
     * @param afcRange 
     */
    void setAFCParams(bool afcEnable, uint8_t afcKI, uint8_t afcKP, uint8_t afcRange);

    /**
     * @brief Flag indicating the TX ring buffer has overflowed.
     * @returns bool Flag indicating the TX ring buffer has overflowed.
     */
    bool hasTXOverflow(void);
    /**
     * @brief Flag indicating the RX ring buffer has overflowed.
     * @returns bool Flag indicating the RX ring buffer has overflowed.
     */
    bool hasRXOverflow(void);

    /**
     * @brief 
     */
    void resetWatchdog(void);
    /**
     * @brief 
     * @returns uint32_t 
     */
    uint32_t getWatchdog(void);

    /**
     * @brief Gets the CPU type the firmware is running on.
     * @returns uint8_t 
     */
    uint8_t getCPU() const;

    /**
     * @brief Gets the unique identifier for the air interface.
     * @param buffer 
     */
    void getUDID(uint8_t* buffer);

    /**
     * @brief 
     * @param modemState 
     */
    void updateCal(DVM_STATE modemState);

    /**
     * @brief 
     */
    void delayBit(void);

    /**
     * @brief 
     * @returns uint16_t 
     */
    uint16_t readRSSI(void);

    /**
     * @brief 
     */
    void selfTest();

    /**
     * @brief 
     */
    void resetMCU();

    /**
     * @brief 
     * @param[out] int1 
     * @param[out] int2 
     */
    void getIntCounter(uint16_t& int1, uint16_t& int2);
#if defined(ZUMSPOT_ADF7021) || defined(LONESTAR_USB) || defined(SKYBRIDGE_HS)
    /**
     * @brief 
     * @returns bool 
     */
    bool isDualBand();
#endif

    /**
     * @brief 
     * @param on 
     */
    void SCLK(bool on);
    /**
     * @brief 
     * @param on 
     */
    void SDATA(bool on);
    /**
     * @brief 
     * @returns bool 
     */
    bool SREAD();
    /**
     * @brief 
     * @param on 
     */
    void SLE1(bool on);

#if defined(DUPLEX)
    /**
     * @brief 
     * @param on 
     */
    void SLE2(bool on);
    /**
     * @brief 
     * @returns bool 
     */
    bool RXD2();
#endif
    /**
     * @brief 
     * @param on 
     */
    void CE(bool on);
    /**
     * @brief 
     * @returns bool 
     */
    bool RXD1();
    /**
     * @brief 
     * @returns bool 
     */
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

    /**
     * @brief Helper to check the frequencies are within band ranges of the ADF7021.
     * @param rxFreq Receive Frequency (hz).
     * @param txFreq Transmit Frequency (hz).
     */
    void checkBand(uint32_t rxFreq, uint32_t txFreq);
#if defined(ZUMSPOT_ADF7021) || defined(LONESTAR_USB) || defined(SKYBRIDGE_HS)
    /**
     * @brief 
     * @param enable 
     */
    void setBandVHF(bool enable);
    /**
     * @brief 
     * @returns bool 
     */
    bool hasSingleADF7021();
#endif
    /**
     * @brief 
     */
    void configureBand();
    /**
     * @brief 
     * @param modemState 
     */
    void configureTxRx(DVM_STATE modemState);

    /**
     * @brief 
     */
    void setTX();
    /**
     * @brief 
     * @param doSle 
     */
    void setRX(bool doSle = true);

    /**
     * @brief 
     */
    void delayIfCal();
    /**
     * @brief 
     */
    void delayReset();
    /**
     * @brief 
     * @param us 
     */
    void delayUS(uint32_t us);

    // Hardware specific routines
    /**
     * @brief Initializes hardware interrupts.
     */
    void initInt();
    /**
     * @brief Starts hardware interrupts.
     */
    void startInt();

    /**
     * @brief 
     * @param on 
     */
    void setTXDInt(bool on);
#if defined(BIDIR_DATA_PIN)
    /**
     * @brief 
     * @param dir 
     */
    void setDataDirOut(bool dir);
    /**
     * @brief 
     * @param on 
     */
    void setRXDInt(bool on);
#endif

    /**
     * @brief 
     * @param on 
     */
    void setLEDInt(bool on);
    /**
     * @brief 
     * @param on 
     */
    void setPTTInt(bool on);
    /**
     * @brief 
     * @param on 
     */
    void setCOSInt(bool on);

    /**
     * @brief 
     * @param on 
     */
    void setDMRInt(bool on);
    /**
     * @brief 
     * @param on 
     */
    void setP25Int(bool on);
    /**
     * @brief 
     * @param on 
     */
    void setNXDNInt(bool on);
};

#endif
