// SPDX-License-Identifier: GPL-2.0-only
/*
 * Digital Voice Modem - Hotspot Firmware
 * GPLv2 Open Source. Use is subject to license terms.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 *  Copyright (C) 2015,2016 Jonathan Naylor, G4KLX
 *  Copyright (C) 2016,2017,2018 Andy Uribe, CA6JAU
 *  Copyright (C) 2021 Bryan Biedenkapp, N2PLL
 *
 */
/**
 * @file DMRDMORX.h
 * @ingroup dmr_hfw
 * @file DMRDMORX.cpp
 * @ingroup dmr_hfw
 */
#if !defined(__DMR_DMO_RX_H__)
#define __DMR_DMO_RX_H__

#include "Defines.h"
#include "dmr/DMRDefines.h"

namespace dmr
{
    // ---------------------------------------------------------------------------
    //  Constants
    // ---------------------------------------------------------------------------

    const uint16_t DMO_BUFFER_LENGTH_BITS = 576U;

    /**
     * @brief DMR DMO Receiver State
     * @ingroup dmr_hfw
     */
    enum DMORX_STATE {
        DMORXS_NONE,        //! None
        DMORXS_VOICE,       //! Voice Data
        DMORXS_DATA         //! PDU Data
    };

    // ---------------------------------------------------------------------------
    //  Class Declaration
    // ---------------------------------------------------------------------------

    /**
     * @brief Implements receiver logic for DMR DMO mode operation.
     * @ingroup dmr_hfw
     */
    class DSP_FW_API DMRDMORX {
    public:
        /**
         * @brief Initializes a new instance of the DMRDMORX class.
         */
        DMRDMORX();

        /**
         * @brief Helper to reset data values to defaults.
         */
        void reset();

        /**
         * @brief Sample DMR bits from the air interface.
         * @param bit 
         */
        void databit(bool bit);

        /**
         * @brief Sets the DMR color code.
         * @param colorCode 
         */
        void setColorCode(uint8_t colorCode);

    private:
        uint64_t m_bitBuffer;
        uint8_t m_buffer[DMO_BUFFER_LENGTH_BITS / 8U];  // 72 bytes

        uint8_t frame[DMR_FRAME_LENGTH_BYTES + 3U];

        uint16_t m_dataPtr;
        uint16_t m_syncPtr;
        uint16_t m_startPtr;
        uint16_t m_endPtr;

        uint8_t m_control;
        uint8_t m_syncCount;

        uint8_t m_colorCode;

        DMORX_STATE m_state;

        uint8_t m_n;

        uint8_t m_type;

        /**
         * @brief Frame synchronization correlator.
         */
        void correlateSync();
        
        /**
         * @brief 
         * @param start 
         * @param count
         * @param buffer
         */
        void bitsToBytes(uint16_t start, uint8_t count, uint8_t* buffer);
        /**
         * @brief 
         * @param frame 
         */
        void writeRSSIData(uint8_t* frame);
    };
} // namespace dmr

#endif // __DMR_DMO_RX_H__
