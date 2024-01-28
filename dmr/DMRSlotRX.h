// SPDX-License-Identifier: GPL-2.0-only
/**
* Digital Voice Modem - Hotspot Firmware
* GPLv2 Open Source. Use is subject to license terms.
* DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
*
* @package DVM / Hotspot Firmware
* @derivedfrom MMDVM_HS (https://github.com/g4klx/MMDVM_HS)
* @license GPLv2 License (https://opensource.org/licenses/GPL-2.0)
*
*   Copyright (C) 2015,2016,2017 Jonathan Naylor, G4KLX
*   Copyright (C) 2017,2018 Andy Uribe, CA6JAU
*   Copyright (C) 2021 Bryan Biedenkapp, N2PLL
*
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

    enum DMRRX_STATE {
        DMRRXS_NONE,
        DMRRXS_VOICE,
        DMRRXS_DATA
    };

    // ---------------------------------------------------------------------------
    //  Class Declaration
    //      Implements receiver logic for DMR slots.
    // ---------------------------------------------------------------------------

    class DSP_FW_API DMRSlotRX {
    public:
        /// <summary>Initializes a new instance of the DMRSlotRX class.</summary>
        DMRSlotRX(bool slot);

        /// <summary>Helper to set data values for start of Rx.</summary>
        void start();
        /// <summary>Helper to reset data values to defaults.</summary>
        void reset();

        /// <summary>Sample DMR bits from the air interface.</summary>
        bool databit(bool bit);

        /// <summary>Sets the DMR color code.</summary>
        void setColorCode(uint8_t colorCode);
        /// <summary>Sets the number of samples to delay before processing.</summary>
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

        /// <summary>Frame synchronization correlator.</summary>
        void correlateSync();
        /// <summary></summary>
        void resetSlot();

        /// <summary></summary>
        void bitsToBytes(uint16_t start, uint8_t count, uint8_t* buffer);
        /// <summary></summary>
        void writeRSSIData(uint8_t* frame);
    };
} // namespace dmr

#endif // DUPLEX

#endif // __DMR_SLOT_RX_H__
