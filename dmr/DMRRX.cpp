// SPDX-License-Identifier: GPL-2.0-only
/*
 * Digital Voice Modem - Hotspot Firmware
 * GPLv2 Open Source. Use is subject to license terms.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 *  Copyright (C) 2015,2016 Jonathan Naylor, G4KLX
 *  Copyright (C) 2017 Andy Uribe, CA6JAU
 *  Copyright (C) 2021 Bryan Biedenkapp, N2PLL
 *
 */
#include "Globals.h"
#include "dmr/DMRRX.h"

using namespace dmr;

#if defined(DUPLEX)

// ---------------------------------------------------------------------------
//  Public Class Members
// ---------------------------------------------------------------------------

/* Initializes a new instance of the DMRRX class. */

DMRRX::DMRRX() :
    m_slot1RX(false),
    m_slot2RX(true)
{
    /* stub */
}


/* Helper to reset data values to defaults. */

void DMRRX::reset()
{
    m_slot1RX.reset();
    m_slot2RX.reset();
}

/* Sample DMR bits from the air interface. */

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

/* Sets the DMR color code. */

void DMRRX::setColorCode(uint8_t colorCode)
{
    m_slot1RX.setColorCode(colorCode);
    m_slot2RX.setColorCode(colorCode);
}

/* Sets the number of samples to delay before processing. */

void DMRRX::setRxDelay(uint8_t delay)
{
    m_slot1RX.setRxDelay(delay);
    m_slot2RX.setRxDelay(delay);
}

#endif // DUPLEX
