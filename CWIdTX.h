// SPDX-License-Identifier: GPL-2.0-only
/*
 * Digital Voice Modem - Hotspot Firmware
 * GPLv2 Open Source. Use is subject to license terms.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 *  Copyright (C) 2009-2015 Jonathan Naylor, G4KLX
 *  Copyright (C) 2016 Colin Durbridge, G4EML
 *  Copyright (C) 2017 Andy Uribe, CA6JAU
 *
 */
/**
 * @file CWIdTX.h
 * @ingroup hotspot_fw
 * @file CWIdTX.cpp
 * @ingroup hotspot_fw
 */
#if !defined(__CWID_TX_H__)
#define __CWID_TX_H__

#include "Defines.h"

// ---------------------------------------------------------------------------
//  Class Declaration
// ---------------------------------------------------------------------------

/**
 * @brief Implements logic to transmit a CW ID.
 * @ingroup hotspot_fw
 */
class DSP_FW_API CWIdTX {
public:
    /**
     * @brief Initializes a new instance of the CWIdTX class.
     */
    CWIdTX();

    /**
     * @brief Process local buffer and transmit on the air interface.
     */
    void process();

    /**
     * @brief Write CW ID data to the local buffer.
     * @param[in] data Buffer.
     * @param length Length of buffer.
     * @returns uint8_t Reason code.
     */
    uint8_t write(const uint8_t* data, uint8_t length);

    /**
     * @brief Helper to reset data values to defaults.
     */
    void reset();

private:
    uint8_t  m_poBuffer[300U];
    uint16_t m_poLen;
    uint16_t m_poPtr;

    uint8_t  m_n;
};

#endif // __CWID_TX_H__
