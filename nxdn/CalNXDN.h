// SPDX-License-Identifier: GPL-2.0-only
/**
* Digital Voice Modem - Hotspot Firmware
* GPLv2 Open Source. Use is subject to license terms.
* DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
*
* @package DVM / Hotspot Firmware
* @derivedfrom MMDVM (https://github.com/g4klx/MMDVM)
* @license GPLv2 License (https://opensource.org/licenses/GPL-2.0)
*
*  Copyright (C) 2018 Andy Uribe, CA6JAU
*  Copyright (C) 2020 Jonathan Naylor, G4KLX
*
*/
/**
 * @file CalNXDN.h
 * @ingroup nxdn_hfw
 * @file CalNXDN.cpp
 * @ingroup nxdn_hfw
 */
#if !defined(__CAL_NXDN_H__)
#define __CAL_NXDN_H__

#include "Defines.h"
#include "nxdn/NXDNDefines.h"

namespace nxdn
{
    // ---------------------------------------------------------------------------
    //  Constants
    // ---------------------------------------------------------------------------

    /**
     * @brief Calibration States
     * @ingroup nxdn_hfw
     */
    enum NXDNCAL1K {
        NXDNCAL1K_IDLE,     //! Idle
        NXDNCAL1K_TX        //! Transmit
    };

    // ---------------------------------------------------------------------------
    //  Class Declaration
    // ---------------------------------------------------------------------------

    /**
     * @brief Implements logic for NXDN calibration mode.
     * @ingroup nxdn_hfw
     */
    class DSP_FW_API CalNXDN {
    public:
        /**
         * @brief Initializes a new instance of the CalNXDN class.
         */
        CalNXDN();

        /**
         * @brief Process local state and transmit on the air interface.
         */
        void process();

        /**
         * @brief Write NXDN calibration state.
         * @param[in] data Buffer.
         * @param length Length of buffer.
         * @returns uint8_t Reason code.
         */
        uint8_t write(const uint8_t* data, uint16_t length);

    private:
        bool m_transmit;
        NXDNCAL1K m_state;
        
        uint8_t m_audioSeq;
    };
} // namespace nxdn

#endif // __CAL_NXDN_H__
