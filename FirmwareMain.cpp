/**
* Digital Voice Modem - DSP Firmware (Hotspot)
* GPLv2 Open Source. Use is subject to license terms.
* DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
*
* @package DVM / DSP Firmware (Hotspot)
*
*/
//
// Based on code from the MMDVM_HS project. (https://github.com/juribeparada/MMDVM_HS)
// Licensed under the GPLv2 License (https://opensource.org/licenses/GPL-2.0)
//
/*
*   Copyright (C) 2015,2016,2020 by Jonathan Naylor G4KLX
*   Copyright (C) 2016 by Mathis Schmieder DB9MAT
*   Copyright (C) 2016 by Colin Durbridge G4EML
*   Copyright (C) 2016,2017,2018,2019 by Andy Uribe CA6JAU
*   Copyright (C) 2019 by Florian Wolters DF2ET
*
*   This program is free software; you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation; either version 2 of the License, or
*   (at your option) any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with this program; if not, write to the Free Software
*   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/
#include "Globals.h"

// ---------------------------------------------------------------------------
//  Globals
// ---------------------------------------------------------------------------

DVM_STATE m_modemState = STATE_IDLE;

bool m_cwIdState = false;
uint8_t m_cwIdTXLevel = 30;

#ifdef ENABLE_DMR
bool m_dmrEnable = true;
#else
bool m_dmrEnable = false;
#endif
#ifdef ENABLE_P25
bool m_p25Enable = true;
#else
bool m_p25Enable = false;
#endif
#ifdef ENABLE_NXDN
bool m_nxdnEnable = true;
#else
bool m_nxdnEnable = false;
#endif

bool m_duplex = false;

bool m_tx = false;
bool m_dcd = false;

uint8_t m_control;

/** DMR BS */
#if defined(ENABLE_DMR)
#if defined(DUPLEX)
dmr::DMRIdleRX dmrIdleRX;
dmr::DMRRX dmrRX;
#endif
dmr::DMRTX dmrTX;

/** DMR MS-DMO */
dmr::DMRDMORX dmrDMORX;
dmr::DMRDMOTX dmrDMOTX;
#endif

/** P25 */
#if defined(ENABLE_P25)
p25::P25RX p25RX;
p25::P25TX p25TX;
#endif

/** NXDN */
#if defined(ENABLE_NXDN)
nxdn::NXDNRX nxdnRX;
nxdn::NXDNTX nxdnTX;
#endif

/** Calibration */
#if defined(ENABLE_DMR)
dmr::CalDMR calDMR;
#endif
#if defined(ENABLE_P25)
p25::CalP25 calP25;
#endif
#if defined(ENABLE_NXDN)
nxdn::CalNXDN calNXDN;
#endif
CalRSSI calRSSI;

/** CW */
CWIdTX cwIdTX;

/** RS232 and Air Interface I/O */
SerialPort serial;
IO io;

// ---------------------------------------------------------------------------
//  Global Functions
// ---------------------------------------------------------------------------

void setup()
{
    serial.start();
}

void loop()
{
    serial.process();
    
    io.process();

    // The following is for transmitting
#if defined(ENABLE_DMR)
    if (m_dmrEnable && m_modemState == STATE_DMR) {
#if defined(DUPLEX)
        if (m_duplex)
            dmrTX.process();
        else
            dmrDMOTX.process();
#else
        dmrDMOTX.process();
#endif // defined(DUPLEX)
    }
#endif // defined(ENABLE_DMR)

#if defined(ENABLE_P25)
    if (m_p25Enable && m_modemState == STATE_P25)
        p25TX.process();
#endif // defined(ENABLE_P25)

#if defined(ENABLE_NXDN)
    if (m_nxdnEnable && m_modemState == STATE_NXDN)
        nxdnTX.process();
#endif // defined(ENABLE_NXDN)

#if defined(ENABLE_DMR)
    if (m_modemState == STATE_DMR_DMO_CAL_1K || m_modemState == STATE_DMR_CAL_1K ||
        m_modemState == STATE_DMR_LF_CAL || m_modemState == STATE_DMR_CAL ||
        m_modemState == STATE_INT_CAL)
        calDMR.process();
#endif // defined(ENABLE_DMR)

#if defined(ENABLE_P25)
    if (m_modemState == STATE_P25_CAL_1K || m_modemState == STATE_P25_CAL)
        calP25.process();
#endif // defined(ENABLE_P25)

#if defined(ENABLE_NXDN)
    if (m_modemState == STATE_NXDN_CAL)
        calNXDN.process();
#endif // defined(ENABLE_NXDN)

    if (m_modemState == STATE_RSSI_CAL)
        calRSSI.process();

    if (m_modemState == STATE_CW || m_modemState == STATE_IDLE)
        cwIdTX.process();
}

// ---------------------------------------------------------------------------
//  Firmware Entry Point
// ---------------------------------------------------------------------------

int main()
{
    setup();

    for (;;)
        loop();
}
