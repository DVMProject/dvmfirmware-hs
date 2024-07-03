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
 * @file NXDNRX.h
 * @ingroup nxdn_hfw
 * @file NXDNRX.cpp
 * @ingroup nxdn_hfw
 */
#if !defined(__NXDN_RX_H__)
#define __NXDN_RX_H__

#include "Defines.h"
#include "nxdn/NXDNDefines.h"

namespace nxdn
{
    // ---------------------------------------------------------------------------
    //  Constants
    // ---------------------------------------------------------------------------

    /**
     * @brief NXDN Receiver State
     * @ingroup nxdn_hfw
     */
    enum NXDNRX_STATE {
        NXDNRXS_NONE,       //! None
        NXDNRXS_DATA        //! Data
    };

    // ---------------------------------------------------------------------------
    //  Class Declaration
    // ---------------------------------------------------------------------------

    /**
     * @brief Implements receiver logic for NXDN mode operation.
     * @ingroup nxdn_mfw
     */
    class DSP_FW_API NXDNRX {
    public:
        /**
         * @brief Initializes a new instance of the NXDNRX class.
         */
        NXDNRX();

        /**
         * @brief Helper to reset data values to defaults.
         */
        void reset();

        /**
         * @brief Sample NXDN bits from the air interface.
         * @param bit 
         */
        void databit(bool bit);

    private:
        uint64_t m_bitBuffer;
        uint8_t m_outBuffer[NXDN_FRAME_LENGTH_BYTES + 3U];
        uint8_t* m_buffer;

        uint16_t m_dataPtr;

        uint16_t m_lostCount;

        NXDNRX_STATE m_state;

        /**
         * @brief Helper to process NXDN data bits.
         * @param bit 
         */
        void processData(bool bit);

        /**
         * @brief Frame synchronization correlator.
         * @param first 
         */
        bool correlateSync(bool first = false);
    };
} // namespace nxdn

#endif // __NXDN_RX_H__
