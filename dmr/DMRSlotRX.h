// SPDX-License-Identifier: GPL-2.0-only
/*
 * Digital Voice Modem - Hotspot Firmware
 * GPLv2 Open Source. Use is subject to license terms.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 *  Copyright (C) 2015,2016,2017 Jonathan Naylor, G4KLX
 *  Copyright (C) 2017,2018 Andy Uribe, CA6JAU
 *  Copyright (C) 2021 Bryan Biedenkapp, N2PLL
 *
 */
/**
 * @file DMRSlotRX.h
 * @ingroup dmr_hfw
 * @file DMRSlotRX.cpp
 * @ingroup dmr_hfw
 */
#if !defined(__DMR_SLOT_RX_H__)
#define __DMR_SLOT_RX_H__

#include "Defines.h"
#include "dmr/DMRDefines.h"

#if defined(DUPLEX)

namespace dmr
{
    // ---------------------------------------------------------------------------
    //  Constants
    // ---------------------------------------------------------------------------

    const uint16_t DMR_BUFFER_LENGTH_BITS = 576U;

    /**
     * @brief DMR Slot Receiver State
     * @ingroup dmr_hfw
     */
    enum DMRRX_STATE {
        DMRRXS_NONE,        //! None
        DMRRXS_VOICE,       //! Voice Data
        DMRRXS_DATA         //! PDU Data
    };

    // ---------------------------------------------------------------------------
    //  Class Declaration
    // ---------------------------------------------------------------------------

    /**
     * @brief Implements receiver logic for DMR slots.
     * @ingroup dmr_hfw
     */
    class DSP_FW_API DMRSlotRX {
    public:
        /**
         * @brief Initializes a new instance of the DMRSlotRX class.
         * @param slot 
         */
        DMRSlotRX(bool slot);

        /**
         * @brief Helper to set data values for start of Rx.
         */
        void start();
        /**
         * @brief Helper to reset data values to defaults.
         */
        void reset();

        /**
         * @brief Sample DMR bits from the air interface.
         * @param bit 
         */
        bool databit(bool bit);

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
        bool m_slot;

        uint64_t m_bitBuffer;
        uint8_t m_buffer[DMR_BUFFER_LENGTH_BITS / 8U];  // 72 bytes
        
        uint16_t m_dataPtr;
        uint16_t m_syncPtr;
        uint16_t m_startPtr;
        uint16_t m_endPtr;
        uint16_t m_delayPtr;

        uint8_t m_control;
        uint8_t m_syncCount;

        uint8_t m_colorCode;

        uint16_t m_delay;

        DMRRX_STATE m_state;

        uint8_t m_n;

        uint8_t m_type;

        /**
         * @brief Frame synchronization correlator.
         */
        void correlateSync();
        /**
         * @brief 
         */
        void resetSlot();

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

#endif // DUPLEX
#endif // __DMR_SLOT_RX_H__
