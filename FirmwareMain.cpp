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
*   Copyright (C) 2015,2016,2020 Jonathan Naylor, G4KLX
*   Copyright (C) 2016 Mathis Schmieder, DB9MAT
*   Copyright (C) 2016 Colin Durbridge, G4EML
*   Copyright (C) 2016,2017,2018,2019 Andy Uribe, CA6JAU
*   Copyright (C) 2019 Florian Wolters, DF2ET
*   Copyright (C) 2021 Bryan Biedenkapp, N2PLL
*
*/
#include "Globals.h"

// ---------------------------------------------------------------------------
//  Globals
// ---------------------------------------------------------------------------

DVM_STATE m_modemState = STATE_IDLE;

bool m_cwIdState = false;
uint8_t m_cwIdTXLevel = 30;

bool m_dmrEnable = true;
bool m_p25Enable = true;
bool m_nxdnEnable = true;

bool m_duplex = false;
bool m_forceDMO = false;

bool m_tx = false;
bool m_dcd = false;

uint8_t m_control;

/** DMR BS */
#if defined(DUPLEX)
dmr::DMRIdleRX dmrIdleRX;
dmr::DMRRX dmrRX;
#endif
dmr::DMRTX dmrTX;

/** DMR MS-DMO */
dmr::DMRDMORX dmrDMORX;
dmr::DMRDMOTX dmrDMOTX;

/** P25 */
p25::P25RX p25RX;
p25::P25TX p25TX;

/** NXDN */
nxdn::NXDNRX nxdnRX;
nxdn::NXDNTX nxdnTX;

/** Calibration */
dmr::CalDMR calDMR;
p25::CalP25 calP25;
nxdn::CalNXDN calNXDN;
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
    if (m_dmrEnable && m_modemState == STATE_DMR) {
#if defined(DUPLEX)
        if (m_duplex)
            dmrTX.process();
        else
            dmrDMOTX.process();
#else
        dmrDMOTX.process();
#endif
    }

    if (m_p25Enable && m_modemState == STATE_P25)
        p25TX.process();

    if (m_nxdnEnable && m_modemState == STATE_NXDN)
        nxdnTX.process();

    if (m_modemState == STATE_DMR_DMO_CAL_1K || m_modemState == STATE_DMR_CAL_1K ||
        m_modemState == STATE_DMR_LF_CAL || m_modemState == STATE_DMR_CAL ||
        m_modemState == STATE_INT_CAL)
        calDMR.process();

    if (m_modemState == STATE_P25_CAL_1K || m_modemState == STATE_P25_CAL)
        calP25.process();

    if (m_modemState == STATE_NXDN_CAL)
        calNXDN.process();

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
