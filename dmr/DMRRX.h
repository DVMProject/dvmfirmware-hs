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
*   Copyright (C) 2015,2016 by Jonathan Naylor G4KLX
*   Copyright (C) 2017 by Andy Uribe CA6JAU
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
#if !defined(__DMR_RX_H__)
#define __DMR_RX_H__

#include "Defines.h"
#include "dmr/DMRSlotRX.h"

#if defined(DUPLEX)

namespace dmr
{
    // ---------------------------------------------------------------------------
    //  Class Declaration
    //      Implements receiver logic for duplex DMR mode operation.
    // ---------------------------------------------------------------------------

    class DSP_FW_API DMRRX {
    public:
        /// <summary>Initializes a new instance of the DMRRX class.</summary>
        DMRRX();

        /// <summary>Helper to reset data values to defaults.</summary>
        void reset();

        /// <summary>Sample DMR bits from the air interface.</summary>
        void databit(bool bit, const uint8_t control);

        /// <summary>Sets the DMR color code.</summary>
        void setColorCode(uint8_t colorCode);
        /// <summary>Sets the number of samples to delay before processing.</summary>
        void setRxDelay(uint8_t delay);

    private:
        DMRSlotRX m_slot1RX;
        DMRSlotRX m_slot2RX;
    };
} // namespace dmr

#endif // DUPLEX

#endif // __DMR_RX_H__
