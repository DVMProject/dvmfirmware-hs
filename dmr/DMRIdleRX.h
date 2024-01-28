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
*   Copyright (C) 2015 Jonathan Naylor, G4KLX
*   Copyright (C) 2017,2018 Andy Uribe, CA6JAU
*
*/
#if !defined(__DMR_IDLE_RX_H__)
#define __DMR_IDLE_RX_H__

#include "Defines.h"
#include "dmr/DMRDefines.h"

#if defined(DUPLEX)

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

#endif // DUPLEX

#endif // __DMR_IDLE_RX_H__
