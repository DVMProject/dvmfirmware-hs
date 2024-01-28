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
*   Copyright (C) 2016,2017 Jonathan Naylor, G4KLX
*   Copyright (C) 2016,2017 Andy Uribe, CA6JAU
*   Copyright (C) 2021-2022 Bryan Biedenkapp, N2PLL
*
*/
#if !defined(__P25_TX_H__)
#define __P25_TX_H__

#include "Defines.h"
#include "SerialBuffer.h"

namespace p25
{
    // ---------------------------------------------------------------------------
    //  Constants
    // ---------------------------------------------------------------------------

    #define P25_FIXED_DELAY 90      // 90 = 20ms
    #define P25_FIXED_TX_HANG 750   // 750 = 625ms

    enum P25TXSTATE {
        P25TXSTATE_NORMAL,
        P25TXSTATE_CAL
    };

    // ---------------------------------------------------------------------------
    //  Class Declaration
    //      Implements transmitter logic for P25 mode operation.
    // ---------------------------------------------------------------------------

    class DSP_FW_API P25TX {
    public:
        /// <summary>Initializes a new instance of the P25TX class.</summary>
        P25TX();

        /// <summary>Process local buffer and transmit on the air interface.</summary>
        void process();

        /// <summary>Write data to the local buffer.</summary>
        uint8_t writeData(const uint8_t* data, uint8_t length);

        /// <summary>Clears the local buffer.</summary>
        void clear();

        /// <summary>Sets the FDMA preamble count.</summary>
        void setPreambleCount(uint8_t preambleCnt);
        /// <summary>Sets the transmit hang time.</summary>
        void setTxHang(uint8_t txHang);
        /// <summary>Helper to set the calibration state for Tx.</summary>
        void setCal(bool start);

        /// <summary>Helper to resize the FIFO buffer.</summary>
        void resizeBuffer(uint16_t size);

        /// <summary>Helper to get how much space the ring buffer has for samples.</summary>
        uint8_t getSpace() const;

    private:
        SerialBuffer m_fifo;

        P25TXSTATE m_state;

        uint8_t m_poBuffer[1200U];
        uint16_t m_poLen;
        uint16_t m_poPtr;

        uint16_t m_preambleCnt;
        uint32_t m_txHang;
        uint32_t m_tailCnt;

        /// <summary></summary>
        void createData();
        /// <summary></summary>
        void createCal();

        /// <summary></summary>
        void writeByte(uint8_t c);
        /// <summary></summary>
        void writeSilence();
    };
} // namespace p25

#endif // __P25_TX_H__
