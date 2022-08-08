/**
* Digital Voice Modem - DSP Firmware (Hotspot)
* GPLv2 Open Source. Use is subject to license terms.
* DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
*
* @package DVM / DSP Firmware (Hotspot)
*
*/
//
// Based on code from the MMDVM project. (https://github.com/g4klx/MMDVM)
// Licensed under the GPLv2 License (https://opensource.org/licenses/GPL-2.0)
//
/*
 *   Copyright (C) 2015,2016,2017,2018,2020 by Jonathan Naylor G4KLX
 *   Copyright (C) 2022 by Bryan Biedenkapp N2PLL
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

    enum NXDNTXSTATE {
        NXDNTXSTATE_NORMAL,
        NXDNTXSTATE_CAL
    };

    // ---------------------------------------------------------------------------
    //  Class Declaration
    //      Implements transmitter logic for NXDN mode operation.
    // ---------------------------------------------------------------------------

    class DSP_FW_API NXDNTX {
    public:
        /// <summary>Initializes a new instance of the NXDNTX class.</summary>
        NXDNTX();

        /// <summary>Process local buffer and transmit on the air interface.</summary>
        void process();

        /// <summary>Write data to the local buffer.</summary>
        uint8_t writeData(const uint8_t* data, uint16_t length);

        /// <summary>Clears the local buffer.</summary>
        void clear();

        /// <summary>Sets the FDMA preamble count.</summary>
        void setPreambleCount(uint8_t preambleCnt);
        /// <summary>Sets the transmit hang time.</summary>
        void setTxHang(uint8_t txHang);
        /// <summary>Helper to set the calibration state for Tx.</summary>
        void setCal(bool start);

        /// <summary>Helper to get how much space the ring buffer has for samples.</summary>
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

        /// <summary></summary>
        void createData();

        /// <summary></summary>
        void writeByte(uint8_t c);
        /// <summary></summary>
        void writeSilence();
    };
} // namespace nxdn

#endif // __NXDN_TX_H__
