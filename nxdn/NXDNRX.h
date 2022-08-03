/**
* Digital Voice Modem - DSP Firmware
* GPLv2 Open Source. Use is subject to license terms.
* DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
*
* @package DVM / DSP Firmware
*
*/
//
// Based on code from the MMDVM project. (https://github.com/g4klx/MMDVM)
// Licensed under the GPLv2 License (https://opensource.org/licenses/GPL-2.0)
//
/*
 *   Copyright (C) 2015,2016,2017,2018,2020 by Jonathan Naylor G4KLX
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
#if !defined(__NXDN_RX_H__)
#define __NXDN_RX_H__

#include "Defines.h"
#include "nxdn/NXDNDefines.h"

namespace nxdn
{
    // ---------------------------------------------------------------------------
    //  Constants
    // ---------------------------------------------------------------------------

    enum NXDNRX_STATE {
        NXDNRXS_NONE,
        NXDNRXS_DATA
    };

    // ---------------------------------------------------------------------------
    //  Class Declaration
    //      Implements receiver logic for DMR slots.
    // ---------------------------------------------------------------------------

    class DSP_FW_API NXDNRX {
    public:
        /// <summary>Initializes a new instance of the NXDNRX class.</summary>
        NXDNRX();

        /// <summary>Helper to reset data values to defaults.</summary>
        void reset();

        /// <summary>Sample NXDN bits from the air interface.</summary>
        void databit(bool bit);

    private:
        uint64_t m_bitBuffer;
        uint8_t m_buffer[NXDN_FRAME_LENGTH_BYTES + 3U];

        uint16_t m_dataPtr;

        uint16_t m_endPtr;

        uint16_t m_lostCount;

        NXDNRX_STATE m_state;

        /// <summary>Helper to process NXDN data bits.</summary>
        void processData(bool bit);

        /// <summary>Frame synchronization correlator.</summary>
        bool correlateSync();
    };
} // namespace nxdn

#endif // __NXDN_RX_H__
