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
*   Copyright (C) 2009-2017 by Jonathan Naylor G4KLX
*   Copyright (C) 2016 by Colin Durbridge G4EML
*   Copyright (C) 2017 by Andy Uribe CA6JAU
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
#include "CWIdTX.h"

// ---------------------------------------------------------------------------
//  Constants
// ---------------------------------------------------------------------------

// 4FSK symbol sequence (800 Hz "tone" at 4800 baud): +1 +3 +1 -1 -3 -1
// Bit sequence: 00 01 00 10 11 10
uint8_t TONE[] = { 0, 0, 0, 1, 0, 0, 1, 0, 1, 1, 1, 0 };

uint8_t SILENCE[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

const uint8_t CYCLE_LENGTH = 12U;

const uint8_t DOT_LENGTH = 40U;

const struct {
    uint8_t  c;
    uint32_t pattern;
    uint8_t  length;
} SYMBOL_LIST[] = {
    { 'A', 0xB8000000U, 8U },
    { 'B', 0xEA800000U, 12U },
    { 'C', 0xEBA00000U, 14U },
    { 'D', 0xEA000000U, 10U },
    { 'E', 0x80000000U, 4U },
    { 'F', 0xAE800000U, 12U },
    { 'G', 0xEE800000U, 12U },
    { 'H', 0xAA000000U, 10U },
    { 'I', 0xA0000000U, 6U },
    { 'J', 0xBBB80000U, 16U },
    { 'K', 0xEB800000U, 12U },
    { 'L', 0xBA800000U, 12U },
    { 'M', 0xEE000000U, 10U },
    { 'N', 0xE8000000U, 8U },
    { 'O', 0xEEE00000U, 14U },
    { 'P', 0xBBA00000U, 14U },
    { 'Q', 0xEEB80000U, 16U },
    { 'R', 0xBA000000U, 10U },
    { 'S', 0xA8000000U, 8U },
    { 'T', 0xE0000000U, 6U },
    { 'U', 0xAE000000U, 10U },
    { 'V', 0xAB800000U, 12U },
    { 'W', 0xBB800000U, 12U },
    { 'X', 0xEAE00000U, 14U },
    { 'Y', 0xEBB80000U, 16U },
    { 'Z', 0xEEA00000U, 14U },
    { '1', 0xBBBB8000U, 20U },
    { '2', 0xAEEE0000U, 18U },
    { '3', 0xABB80000U, 16U },
    { '4', 0xAAE00000U, 14U },
    { '5', 0xAA800000U, 12U },
    { '6', 0xEAA00000U, 14U },
    { '7', 0xEEA80000U, 16U },
    { '8', 0xEEEA0000U, 18U },
    { '9', 0xEEEE8000U, 20U },
    { '0', 0xEEEEE000U, 22U },
    { '/', 0xEAE80000U, 16U },
    { '?', 0xAEEA0000U, 18U },
    { ',', 0xEEAEE000U, 22U },
    { '-', 0xEAAE0000U, 18U },
    { '=', 0xEAB80000U, 16U },
    { '.', 0xBAEB8000U, 20U },
    { ' ', 0x00000000U, 4U },
    { 0U,  0x00000000U, 0U }
};

// ---------------------------------------------------------------------------
//  Public Class Members
// ---------------------------------------------------------------------------

/// <summary>
/// Initializes a new instance of the CWIdTX class.
/// </summary>
CWIdTX::CWIdTX() :
    m_poBuffer(),
    m_poLen(0U),
    m_poPtr(0U),
    m_n(0U)
{
    /* stub */
}

/// <summary>
/// Process local buffer and transmit on the air interface.
/// </summary>
void CWIdTX::process()
{
    if (m_poLen == 0U)
        return;

    uint16_t space = io.getSpace();

    while (space > CYCLE_LENGTH) {
        bool b = _READ_BIT(m_poBuffer, m_poPtr);
        if (b)
            io.write(TONE, CYCLE_LENGTH);
        else
            io.write(SILENCE, CYCLE_LENGTH);

        space -= CYCLE_LENGTH;

        m_n++;
        if (m_n >= DOT_LENGTH) {
            m_poPtr++;
            m_n = 0U;
        }

        if (m_poPtr >= m_poLen) {
            m_poPtr = 0U;
            m_poLen = 0U;
            return;
        }
    }
}

/// <summary>
/// Write CW ID data to the local buffer.
/// </summary>
/// <param name="data"></param>
/// <param name="length"></param>
/// <returns></returns>
uint8_t CWIdTX::write(const uint8_t* data, uint8_t length)
{
    ::memset(m_poBuffer, 0x00U, 300U * sizeof(uint8_t));

    m_poLen = 8U;
    m_poPtr = 0U;
    m_n = 0U;

    for (uint8_t i = 0U; i < length; i++) {
        for (uint8_t j = 0U; SYMBOL_LIST[j].c != 0U; j++) {
            if (SYMBOL_LIST[j].c == data[i]) {
                uint32_t MASK = 0x80000000U;
                for (uint8_t k = 0U; k < SYMBOL_LIST[j].length; k++, m_poLen++, MASK >>= 1) {
                    bool b = (SYMBOL_LIST[j].pattern & MASK) == MASK;
                    _WRITE_BIT(m_poBuffer, m_poLen, b);

                    if (m_poLen >= 295U) {
                        m_poLen = 0U;
                        return 4U;
                    }
                }

                break;
            }
        }
    }

    // An empty message
    if (m_poLen == 8U) {
        m_poLen = 0U;
        return RSN_ILLEGAL_LENGTH;
    }

    m_poLen += 5U;

    DEBUG2("CWIdTx::write() message length", m_poLen);

    return RSN_OK;
}

/// <summary>
/// Helper to reset data values to defaults.
/// </summary>
void CWIdTX::reset()
{
    m_poLen = 0U;
    m_poPtr = 0U;
    m_n = 0U;
}
