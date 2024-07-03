// SPDX-License-Identifier: GPL-2.0-only
/*
 * Digital Voice Modem - Hotspot Firmware
 * GPLv2 Open Source. Use is subject to license terms.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 *  Copyright (C) 2015,2016 Jonathan Naylor, G4KLX
 *  Copyright (C) 2017 Andy Uribe, CA6JAU
 *  Copyright (C) 2021 Bryan Biedenkapp, N2PLL
 *
 */
/**
 * @file DMRRX.h
 * @ingroup dmr_hfw
 * @file DMRRX.cpp
 * @ingroup dmr_hfw
 */
#if !defined(__DMR_RX_H__)
#define __DMR_RX_H__

#include "Defines.h"
#include "dmr/DMRSlotRX.h"

#if defined(DUPLEX)

namespace dmr
{
    // ---------------------------------------------------------------------------
    //  Class Declaration
    // ---------------------------------------------------------------------------

    /**
     * @brief Implements receiver logic for duplex DMR mode operation.
     * @ingroup dmr_hfw
     */
    class DSP_FW_API DMRRX {
    public:
        /**
         * @brief Initializes a new instance of the DMRRX class.
         */
        DMRRX();

        /**
         * @brief Helper to reset data values to defaults.
         */
        void reset();

        /**
         * @brief Sample DMR bits from the air interface.
         * @param bit 
         * @param[in] control 
         */
        void databit(bool bit, const uint8_t control);

        /**
         * @brief Sets the DMR color code.
         * @param colorCode 
         */
        void setColorCode(uint8_t colorCode);
        /**
         * @brief Sets the number of samples to delay before processing.
         * @param delay 
         */
        void setRxDelay(uint8_t delay);

    private:
        DMRSlotRX m_slot1RX;
        DMRSlotRX m_slot2RX;
    };
} // namespace dmr

#endif // DUPLEX
#endif // __DMR_RX_H__
