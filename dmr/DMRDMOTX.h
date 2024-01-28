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
*   Copyright (C) 2015,2016 Jonathan Naylor, G4KLX
*   Copyright (C) 2016 Colin Durbridge, G4EML
*   Copyright (C) 2016,2017,2018 Andy Uribe, CA6JAU
*   Copyright (C) 2021 Bryan Biedenkapp, N2PLL
*
*/
#if !defined(__DMR_DMO_TX_H__)
#define __DMR_DMO_TX_H__

#include "Defines.h"
#include "dmr/DMRDefines.h"
#include "SerialBuffer.h"

namespace dmr
{
    // ---------------------------------------------------------------------------
    //  Constants
    // ---------------------------------------------------------------------------

    #define DMRDMO_FIXED_DELAY 300  // 300 = 62.49ms
                                    // Delay Value * 0.2083 = Preamble Length (ms)

    // ---------------------------------------------------------------------------
    //  Class Declaration
    //      Implements transmitter logic for DMR DMO mode operation.
    // ---------------------------------------------------------------------------

    class DSP_FW_API DMRDMOTX {
    public:
        /// <summary>Initializes a new instance of the DMRDMOTX class.</summary>
        DMRDMOTX();

        /// <summary>Process local buffer and transmit on the air interface.</summary>
        void process();

        /// <summary>Write data to the local buffer.</summary>
        uint8_t writeData(const uint8_t* data, uint8_t length);

        /// <summary>Sets the FDMA preamble count.</summary>
        void setPreambleCount(uint8_t preambleCnt);

        /// <summary>Helper to resize the FIFO buffer.</summary>
        void resizeBuffer(uint16_t size);

        /// <summary>Helper to get how much space the ring buffer has for samples.</summary>
        uint16_t getSpace() const;

    private:
        SerialBuffer m_fifo;
        
        uint8_t m_poBuffer[80U];
        uint16_t m_poLen;
        uint16_t m_poPtr;

        uint32_t m_preambleCnt;

        /// <summary></summary>
        void writeByte(uint8_t c);
    };
} // namespace dmr

#endif // __DMR_DMO_TX_H__
