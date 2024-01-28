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
*   Copyright (C) 2015,2016,2017 Jonathan Naylor, G4KLX
*   Copyright (C) 2016 Colin Durbridge, G4EML
*   Copyright (C) 2017 Andy Uribe, CA6JAU
*   Copyright (C) 2021-2022 Bryan Biedenkapp, N2PLL
*
*/
#if !defined(__DMR_TX_H__)
#define __DMR_TX_H__

#include "Defines.h"
#include "dmr/DMRDefines.h"
#include "SerialBuffer.h"

namespace dmr
{
    // ---------------------------------------------------------------------------
    //  Constants
    // ---------------------------------------------------------------------------

    enum DMRTXSTATE {
        DMRTXSTATE_IDLE,
        DMRTXSTATE_SLOT1,
        DMRTXSTATE_CACH1,
        DMRTXSTATE_SLOT2,
        DMRTXSTATE_CACH2,
        DMRTXSTATE_CAL
    };

    // ---------------------------------------------------------------------------
    //  Class Declaration
    //      Implements receiver logic for duplex DMR mode operation.
    // ---------------------------------------------------------------------------

    class DSP_FW_API DMRTX {
    public:
        /// <summary>Initializes a new instance of the DMRTX class.</summary>
        DMRTX();

        /// <summary>Process local buffer and transmit on the air interface.</summary>
        void process();

        /// <summary>Write slot 1 data to the local buffer.</summary>
        uint8_t writeData1(const uint8_t* data, uint8_t length);
        /// <summary>Write slot 2 data to the local buffer.</summary>
        uint8_t writeData2(const uint8_t* data, uint8_t length);

        /// <summary>Write short LC data to the local buffer.</summary>
        uint8_t writeShortLC(const uint8_t* data, uint8_t length);
        /// <summary>Write abort data to the local buffer.</summary>
        uint8_t writeAbort(const uint8_t* data, uint8_t length);

        /// <summary>Helper to set the start state for Tx.</summary>
        void setStart(bool start);
        /// <summary>Helper to set the calibration state for Tx.</summary>
        void setCal(bool start);

        /// <summary>Helper to get how much space the slot 1 ring buffer has for samples.</summary>
        uint8_t getSpace1() const;
        /// <summary>Helper to get how much space the slot 2 ring buffer has for samples.</summary>
        uint8_t getSpace2() const;

        /// <summary>Sets the ignore flags for setting the CACH Access Type bit.</summary>
        void setIgnoreCACH_AT(uint8_t slot);
        /// <summary>Sets the DMR color code.</summary>
        void setColorCode(uint8_t colorCode);

        /// <summary>Helper to reset data values to defaults for slot 1 FIFO.</summary>
        void resetFifo1();
        /// <summary>Helper to reset data values to defaults for slot 2 FIFO.</summary>
        void resetFifo2();

        /// <summary>Helper to resize the FIFO buffer.</summary>
        void resizeBuffer(uint16_t size);

        /// <summary></summary>
        uint32_t getFrameCount();

    private:
        SerialBuffer m_fifo[2U];

        DMRTXSTATE m_state;

        uint8_t m_idle[DMR_FRAME_LENGTH_BYTES];
        uint8_t m_cachPtr;

        uint8_t m_shortLC[12U];
        uint8_t m_newShortLC[12U];

        uint8_t m_markBuffer[40U];

        uint8_t m_poBuffer[40U];
        uint16_t m_poLen;
        uint16_t m_poPtr;

        uint32_t m_frameCount;

        uint32_t m_abortCount[2U];
        bool m_abort[2U];

        uint8_t m_cachATControl;

        uint8_t m_controlPrev;

        /// <summary></summary>
        void createData(uint8_t slotIndex);
        /// <summary></summary>
        void createCACH(uint8_t txSlotIndex, uint8_t rxSlotIndex);
        /// <summary></summary>
        void createCal();

        /// <summary></summary>
        void writeByte(uint8_t c, uint8_t control);
    };
} // namespace dmr

#endif // __DMR_TX_H__
