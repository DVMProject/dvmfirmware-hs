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
*   Copyright (C) 2016 Jonathan Naylor, G4KLX
*   Copyright (C) 2018 Andy Uribe, CA6JAU
*
*/
#if !defined(__CAL_RSSI_H__)
#define __CAL_RSSI_H__

#include "Defines.h"

// ---------------------------------------------------------------------------
//  Class Declaration
//      Implements logic for RSSI calibration mode.
// ---------------------------------------------------------------------------

class DSP_FW_API CalRSSI {
public:
    /// <summary>Initializes a new instance of the CalRSSI class.</summary>
    CalRSSI();

    /// <summary>Sample RSSI values from the air interface.</summary>
    void process();

private:
    uint32_t m_count;
    uint8_t  m_navg;
    uint32_t m_accum;
    uint16_t m_min;
    uint16_t m_max;
};

#endif // __CAL_RSSI_H__
