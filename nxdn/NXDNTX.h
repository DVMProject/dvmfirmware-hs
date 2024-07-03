// SPDX-License-Identifier: GPL-2.0-only
/*
 * Digital Voice Modem - Hotspot Firmware
 * GPLv2 Open Source. Use is subject to license terms.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 *  Copyright (C) 2015,2016,2017,2018,2020 Jonathan Naylor, G4KLX
 *  Copyright (C) 2022 Bryan Biedenkapp, N2PLL
 *
 */
/**
 * @file NXDNTX.h
 * @ingroup nxdn_hfw
 * @file NXDNTX.cpp
 * @ingroup nxdn_hfw
 */
#if !defined(__NXDN_TX_H__)
#define __NXDN_TX_H__

#include "Defines.h"
#include "SerialBuffer.h"

namespace nxdn
{
    // ---------------------------------------------------------------------------
    //  Constants
    // ---------------------------------------------------------------------------

    #define NXDN_FIXED_TX_HANG 600

    /**
     * @brief NXDN Transmitter States
     * @ingroup nxdn_hfw
     */
    enum NXDNTXSTATE {
        NXDNTXSTATE_NORMAL,     //! Normal
        NXDNTXSTATE_CAL         //! Calibration
    };

    // ---------------------------------------------------------------------------
    //  Class Declaration
    // ---------------------------------------------------------------------------

    /**
     * @brief Implements transmitter logic for NXDN mode operation.
     * @ingroup nxdn_hfw
     */
    class DSP_FW_API NXDNTX {
    public:
        /**
         * @brief Initializes a new instance of the NXDNTX class.
         */
        NXDNTX();

        /**
         * @brief Process local buffer and transmit on the air interface.
         */
        void process();

        /**
         * @brief Write data to the local buffer.
         * @param[in] data Buffer.
         * @param length Length of buffer.
         * @returns uint8_t Reason code.
         */
        uint8_t writeData(const uint8_t* data, uint16_t length);

        /**
         * @brief Clears the local buffer.
         */
        void clear();

        /**
         * @brief Sets the FDMA preamble count.
         * @param preambleCnt FDMA preamble count.
         */
        void setPreambleCount(uint8_t preambleCnt);
        /**
         * @brief Sets the transmit hang time.
         * @param txHang Transmit hang time.
         */
        void setTxHang(uint8_t txHang);
        /**
         * @brief Helper to set the calibration state for Tx.
         * @param start 
         */
        void setCal(bool start);

        /**
         * @brief Helper to resize the FIFO buffer.
         * @param size 
         */
        void resizeBuffer(uint16_t size);

        /**
         * @brief Helper to get how much space the ring buffer has for samples.
         * @returns uint8_t Amount of space in ring buffer for samples. 
         */
        uint8_t getSpace() const;

    private:
        SerialBuffer m_fifo;

        NXDNTXSTATE m_state;

        uint8_t m_poBuffer[60U];
        uint16_t m_poLen;
        uint16_t m_poPtr;

        uint16_t m_preambleCnt;
        uint32_t m_txHang;
        uint32_t m_tailCnt;

        /**
         * @brief Helper to generate data.
         */
        void createData();

        /**
         * @brief Helper to write a raw byte to the DAC.
         * @param c Byte.
         */
        void writeByte(uint8_t c);
        /**
         * @brief 
         */
        void writeSilence();
    };
} // namespace nxdn

#endif // __NXDN_TX_H__
