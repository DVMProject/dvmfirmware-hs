// SPDX-License-Identifier: GPL-2.0-only
/*
 * Digital Voice Modem - Hotspot Firmware
 * GPLv2 Open Source. Use is subject to license terms.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 *  Copyright (C) 2015,2016,2017 Jonathan Naylor, G4KLX
 *  Copyright (C) 2016,2017,2018 Andy Uribe, CA6JAU
 *  Copyright (C) 2021-2024 Bryan Biedenkapp, N2PLL
 *
 */
/**
 * @file P25RX.h
 * @ingroup p25_hfw
 * @file P25RX.cpp
 * @ingroup p25_hfw
 */
#if !defined(__P25_RX_H__)
#define __P25_RX_H__

#include "Defines.h"
#include "p25/P25Defines.h"

namespace p25
{
    // ---------------------------------------------------------------------------
    //  Constants
    // ---------------------------------------------------------------------------

    /**
     * @brief P25 Receiver State
     * @ingroup p25_hfw
     */
    enum P25RX_STATE {
        P25RXS_NONE,        //! None
        P25RXS_SYNC,        //! Found Sync
        P25RXS_VOICE,       //! Voice Data
        P25RXS_DATA         //! PDU Data
    };

    // ---------------------------------------------------------------------------
    //  Class Declaration
    // ---------------------------------------------------------------------------

    /**
     * @brief Implements receiver logic for P25 mode operation.
     * @ingroup p25_hfw
     */
    class DSP_FW_API P25RX {
    public:
        /**
         * @brief Initializes a new instance of the P25RX class.
         */
        P25RX();

        /**
         * @brief Helper to reset data values to defaults.
         */
        void reset();

        /**
         * @brief Sample P25 bits from the air interface.
         * @param bit 
         */
        void databit(bool bit);

        /**
         * @brief Sets the P25 NAC.
         * @param nac Network Access Code.
         */
        void setNAC(uint16_t nac);

    private:
        uint64_t m_bitBuffer;
        uint8_t m_buffer[P25_PDU_FRAME_LENGTH_BYTES + 3U];

        uint16_t m_dataPtr;

        uint16_t m_endPtr;
        uint16_t m_pduEndPtr;

        uint16_t m_lostCount;

        uint16_t m_nac;

        P25RX_STATE m_state;

        uint8_t m_duid;

        /**
         * @brief Helper to process P25 bits.
         * @param bit 
         */
        void processBit(bool bit);
        /**
         * @brief Helper to process LDU P25 bits.
         * @param bit 
         */
        void processVoice(bool bit);
        /**
         * @brief Helper to process PDU P25 bits.
         * @param bit 
         */
        void processData(bool bit);

        /**
         * @brief Frame synchronization correlator.
         * @returns bool 
         */
        bool correlateSync();

        /**
         * @brief Helper to decode the P25 NID.
         * @returns bool True, if P25 NID was decoded, otherwise false.
         */
        bool decodeNid();
    };
} // namespace p25

#endif // __P25_RX_H__
