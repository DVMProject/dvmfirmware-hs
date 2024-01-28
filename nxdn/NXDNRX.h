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
*   Copyright (C) 2015,2016,2017,2018,2020 Jonathan Naylor, G4KLX
*   Copyright (C) 2022 Bryan Biedenkapp, N2PLL
*
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
        uint8_t m_outBuffer[NXDN_FRAME_LENGTH_BYTES + 3U];
        uint8_t* m_buffer;

        uint16_t m_dataPtr;

        uint16_t m_lostCount;

        NXDNRX_STATE m_state;

        /// <summary>Helper to process NXDN data bits.</summary>
        void processData(bool bit);

        /// <summary>Frame synchronization correlator.</summary>
        bool correlateSync(bool first = false);
    };
} // namespace nxdn

#endif // __NXDN_RX_H__
