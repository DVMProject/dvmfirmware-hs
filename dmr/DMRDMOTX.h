// SPDX-License-Identifier: GPL-2.0-only
/*
 * Digital Voice Modem - Hotspot Firmware
 * GPLv2 Open Source. Use is subject to license terms.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 *  Copyright (C) 2015,2016 Jonathan Naylor, G4KLX
 *  Copyright (C) 2016 Colin Durbridge, G4EML
 *  Copyright (C) 2016,2017,2018 Andy Uribe, CA6JAU
 *  Copyright (C) 2021 Bryan Biedenkapp, N2PLL
 *
 */
/**
 * @file DMRDMOTX.h
 * @ingroup dmr_hfw
 * @file DMRDMOTX.h
 * @ingroup dmr_hfw
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
    // ---------------------------------------------------------------------------

    /**
     * @brief Implements transmitter logic for DMR DMO mode operation.
     * @ingroup dmr_hfw
     */
    class DSP_FW_API DMRDMOTX {
    public:
        /**
         * @brief Initializes a new instance of the DMRDMOTX class.
         */
        DMRDMOTX();

        /**
         * @brief Process local buffer and transmit on the air interface.
         */
        void process();

        /**
         * @brief Write data to the local buffer.
         * @param[in] data Buffer.
         * @param length Length of buffer.
         * @returns uint8_t Reason code.
         */
        uint8_t writeData(const uint8_t* data, uint8_t length);

        /**
         * @brief Sets the FDMA preamble count.
         * @param preambleCnt FDMA preamble count.
         */
        void setPreambleCount(uint8_t preambleCnt);

        /**
         * @brief Helper to resize the FIFO buffer.
         * @param size 
         */
        void resizeBuffer(uint16_t size);

        /**
         * @brief Helper to get how much space the ring buffer has for samples.
         * @returns uint8_t Amount of space in ring buffer for samples. 
         */
        uint16_t getSpace() const;

    private:
        SerialBuffer m_fifo;
        
        uint8_t m_poBuffer[80U];
        uint16_t m_poLen;
        uint16_t m_poPtr;

        uint32_t m_preambleCnt;

        /**
         * @brief Helper to write a raw byte to the DAC.
         * @param c Byte.
         */
        void writeByte(uint8_t c);
    };
} // namespace dmr

#endif // __DMR_DMO_TX_H__
