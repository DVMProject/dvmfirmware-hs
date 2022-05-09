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
#include "Globals.h"
#include "dmr/DMRRX.h"

using namespace dmr;

#if defined(DUPLEX)

// ---------------------------------------------------------------------------
//  Public Class Members
// ---------------------------------------------------------------------------

/// <summary>
/// Initializes a new instance of the DMRRX class.
/// </summary>
DMRRX::DMRRX() :
    m_slot1RX(false),
    m_slot2RX(true)
{
    /* stub */
}


/// <summary>
/// Helper to reset data values to defaults.
/// </summary>
void DMRRX::reset()
{
    m_slot1RX.reset();
    m_slot2RX.reset();
}

/// <summary>
/// Sample DMR bits from the air interface.
/// </summary>
/// <param name="bit"></param>
void DMRRX::databit(bool bit, const uint8_t control)
{
    bool dcd1 = false;
    bool dcd2 = false;
   
    switch (control) {
    case CONTROL_SLOT1:
        m_slot1RX.start();
        break;
    case CONTROL_SLOT2:
        m_slot2RX.start();
        break;
    default:
        break;
    }

    dcd1 = m_slot1RX.databit(bit);
    dcd2 = m_slot2RX.databit(bit);

    io.setDecode(dcd1 || dcd2);
}

/// <summary>
/// Sets the DMR color code.
/// </summary>
/// <param name="colorCode">Color code.</param>
void DMRRX::setColorCode(uint8_t colorCode)
{
    m_slot1RX.setColorCode(colorCode);
    m_slot2RX.setColorCode(colorCode);
}

/// <summary>
/// Sets the number of samples to delay before processing.
/// </summary>
/// <param name="delay">Number of samples to delay.</param>
void DMRRX::setRxDelay(uint8_t delay)
{
    m_slot1RX.setRxDelay(delay);
    m_slot2RX.setRxDelay(delay);
}

#endif // DUPLEX
