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
*   Copyright (C) 2016 by Jonathan Naylor G4KLX
*   Copyright (C) 2018 by Andy Uribe CA6JAU
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
#include "CalRSSI.h"
#include "Utils.h"

// ---------------------------------------------------------------------------
//  Public Class Members
// ---------------------------------------------------------------------------
/// <summary>
/// Initializes a new instance of the CalRSSI class.
/// </summary>
CalRSSI::CalRSSI() :
    m_count(0U),
    m_navg(0U),
    m_accum(0U),
    m_min(0xFFFFU),
    m_max(0x0000U)
{
    /* stub */
}

/// <summary>
/// Sample RSSI values from the air interface.
/// </summary>
void CalRSSI::process()
{
    m_count++;

    if (m_count >= 32000U) {
        uint16_t rssi = io.readRSSI();
        m_count = 0U;
        m_navg++;

        m_accum += rssi;

        if (rssi > m_max)
            m_max = rssi;
        if (rssi < m_min)
            m_min = rssi;

        if (m_navg >= 6U) {
            uint16_t ave = m_accum / 6U;

            uint8_t buffer[6U];
            buffer[0U] = (m_max >> 8) & 0xFFU;
            buffer[1U] = (m_max >> 0) & 0xFFU;
            buffer[2U] = (m_min >> 8) & 0xFFU;
            buffer[3U] = (m_min >> 0) & 0xFFU;
            buffer[4U] = (ave >> 8) & 0xFFU;
            buffer[5U] = (ave >> 0) & 0xFFU;

            serial.writeRSSIData(buffer, 6U);

            m_navg = 0U;
            m_accum = 0U;
            m_min = 0xFFFFU;
            m_max = 0x0000U;
        }
    }
}
