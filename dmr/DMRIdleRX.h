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
*   Copyright (C) 2015 by Jonathan Naylor G4KLX
*   Copyright (C) 2017,2018 by Andy Uribe CA6JAU
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
#if !defined(__DMR_IDLE_RX_H__)
#define __DMR_IDLE_RX_H__

#include "Defines.h"
#include "dmr/DMRDefines.h"

#if defined(ENABLE_DMR) && defined(DUPLEX)

namespace dmr
{
    // ---------------------------------------------------------------------------
    //  Constants
    // ---------------------------------------------------------------------------

    const uint16_t DMR_IDLE_LENGTH_BITS = 320U;

    // ---------------------------------------------------------------------------
    //  Class Declaration
    //      Implements receiver logic for idle DMR mode operation.
    // ---------------------------------------------------------------------------

    class DSP_FW_API DMRIdleRX {
    public:
        /// <summary>Initializes a new instance of the DMRIdleRX class.</summary>
        DMRIdleRX();

        /// <summary>Helper to reset data values to defaults.</summary>
        void reset();

        /// <summary>Sample DMR bits from the air interface.</summary>
        void databit(bool bit);

        /// <summary>Sets the DMR color code.</summary>
        void setColorCode(uint8_t colorCode);

    private:
        uint64_t m_bitBuffer;
        uint8_t m_buffer[DMR_IDLE_LENGTH_BITS / 8U];
        
        uint16_t m_dataPtr;
        uint16_t m_endPtr;
        
        uint8_t m_colorCode;

        /// <summary></summary>
        void bitsToBytes(uint16_t start, uint8_t count, uint8_t* buffer);
    };
} // namespace dmr

#endif // defined(ENABLE_DMR) && defined(DUPLEX)

#endif // __DMR_IDLE_RX_H__
