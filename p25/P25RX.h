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
*   Copyright (C) 2016,2017,2018 Andy Uribe, CA6JAU
*   Copyright (C) 2021 Bryan Biedenkapp, N2PLL
*
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

    enum P25RX_STATE {
        P25RXS_NONE,
        P25RXS_SYNC,
        P25RXS_VOICE,
        P25RXS_DATA
    };

    // ---------------------------------------------------------------------------
    //  Class Declaration
    //      Implements receiver logic for P25 mode operation.
    // ---------------------------------------------------------------------------

    class DSP_FW_API P25RX {
    public:
        /// <summary>Initializes a new instance of the P25RX class.</summary>
        P25RX();

        /// <summary>Helper to reset data values to defaults.</summary>
        void reset();

        /// <summary>Sample P25 bits from the air interface.</summary>
        void databit(bool bit);

        /// <summary>Sets the P25 NAC.</summary>
        void setNAC(uint16_t nac);

    private:
        uint64_t m_bitBuffer;
        uint8_t m_buffer[P25_LDU_FRAME_LENGTH_BYTES + 3U];

        uint16_t m_dataPtr;

        uint16_t m_endPtr;

        uint16_t m_lostCount;

        uint16_t m_nac;

        P25RX_STATE m_state;

        uint8_t m_duid;

        /// <summary>Helper to process P25 bits.</summary>
        void processBit(bool bit);
        /// <summary>Helper to process LDU P25 bits.</summary>
        void processVoice(bool bit);
        /// <summary>Helper to process PDU P25 bits.</summary>
        void processData(bool bit);

        /// <summary>Frame synchronization correlator.</summary>
        bool correlateSync();

        /// <summary>Helper to decode the P25 NID.</summary>
        bool decodeNid();
    };
} // namespace p25

#endif // __P25_RX_H__
