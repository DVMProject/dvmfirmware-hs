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
*   Copyright (C) 2015,2016,2017 by Jonathan Naylor G4KLX
*   Copyright (C) 2016,2017,2018 by Andy Uribe CA6JAU
*   Copyright (C) 2021 Bryan Biedenkapp N2PLL
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
        /// <summary>Sets the P25 sync correlation countdown.</summary>
        void setCorrCount(uint8_t count);

    private:
        uint64_t m_bitBuffer;
        uint8_t m_buffer[P25_LDU_FRAME_LENGTH_BITS / 8U + 3U];

        uint16_t m_dataPtr;

        uint16_t m_minSyncPtr;
        uint16_t m_maxSyncPtr;

        uint16_t m_startPtr;
        uint16_t m_endPtr;
        uint16_t m_syncPtr;

        uint16_t m_lostCount;
        uint8_t m_countdown;

        uint16_t m_nac;

        uint8_t m_corrCountdown;

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
        bool decodeNid(uint16_t start);

        /// <summary></summary>
        void bitsToBytes(uint16_t start, uint8_t count, uint8_t* buffer);
    };
} // namespace p25

#endif // __P25_RX_H__
