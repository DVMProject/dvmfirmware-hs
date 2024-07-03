// SPDX-License-Identifier: GPL-2.0-only
/*
 * Digital Voice Modem - Hotspot Firmware
 * GPLv2 Open Source. Use is subject to license terms.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 *  Copyright (C) 2015,2016,2020 Jonathan Naylor, G4KLX
 *  Copyright (C) 2016,2017,2018,2019 Andy Uribe, CA6JAU
 *  Copyright (C) 2019 Florian Wolters, DF2ET
 *  Copyright (C) 2021 Bryan Biedenkapp, N2PLL
 *
 */
/**
 * @defgroup hotspot_fw Hotspot Firmware
 * @brief Digital Voice Modem - Hotspot Firmware
 * @details Firmware that is used for all-in-one hotspots.
 * @ingroup hotspot_fw
 * 
 * @file Globals.h
 * @ingroup hotspot_fw
 */
#if !defined(__GLOBALS_H__)
#define __GLOBALS_H__

#if defined(STM32F10X_MD)
#include <stm32f10x.h>
#include "string.h"
#elif defined(STM32F4XX)
#include "stm32f4xx.h"
#include "string.h"
#elif defined(STM32F7XX)
#include "stm32f7xx.h"
#include "string.h"
#endif

#include "Defines.h"
#include "SerialPort.h"
#if defined(DUPLEX)
#include "dmr/DMRIdleRX.h"
#include "dmr/DMRRX.h"
#endif
#include "dmr/DMRTX.h"
#include "dmr/DMRDMORX.h"
#include "dmr/DMRDMOTX.h"
#include "dmr/CalDMR.h"
#include "p25/P25RX.h"
#include "p25/P25TX.h"
#include "p25/CalP25.h"
#include "nxdn/NXDNRX.h"
#include "nxdn/NXDNTX.h"
#include "nxdn/CalNXDN.h"
#include "CalRSSI.h"
#include "CWIdTX.h"
#include "IO.h"

// ---------------------------------------------------------------------------
//  Constants
// ---------------------------------------------------------------------------

const uint8_t   MARK_SLOT1 = 0x08U;
const uint8_t   MARK_SLOT2 = 0x04U;
const uint8_t   MARK_NONE = 0x00U;

const uint8_t   CONTROL_SLOT1 = 0x01U;
const uint8_t   CONTROL_SLOT2 = 0x00U;

// ---------------------------------------------------------------------------
//  Macros
// ---------------------------------------------------------------------------

#define  DEBUG1(a)          serial.writeDebug((a))
#define  DEBUG2(a,b)        serial.writeDebug((a),(b))
#define  DEBUG3(a,b,c)      serial.writeDebug((a),(b),(c))
#define  DEBUG4(a,b,c,d)    serial.writeDebug((a),(b),(c),(d))
#define  DEBUG5(a,b,c,d,e)  serial.writeDebug((a),(b),(c),(d),(e))
#define  DEBUG_DUMP(a,b)    serial.writeDump((a),(b))

// ---------------------------------------------------------------------------
//  Global Externs
// ---------------------------------------------------------------------------

extern DVM_STATE m_modemState;

extern bool m_cwIdState;
extern uint8_t m_cwIdTXLevel;

extern bool m_dmrEnable;
extern bool m_p25Enable;
extern bool m_nxdnEnable;

extern bool m_duplex;
extern bool m_forceDMO;

extern bool m_tx;
extern bool m_dcd;

extern uint8_t m_control;

extern SerialPort serial;
extern IO io;

/* DMR BS */
#if defined(DUPLEX)
extern dmr::DMRIdleRX dmrIdleRX;
extern dmr::DMRRX dmrRX;
#endif
extern dmr::DMRTX dmrTX;

/* DMR MS-DMO */
extern dmr::DMRDMORX dmrDMORX;
extern dmr::DMRDMOTX dmrDMOTX;

/* P25 BS */
extern p25::P25RX p25RX;
extern p25::P25TX p25TX;

/* NXDN BS */
extern nxdn::NXDNRX nxdnRX;
extern nxdn::NXDNTX nxdnTX;

/* Calibration */
extern dmr::CalDMR calDMR;
extern p25::CalP25 calP25;
extern nxdn::CalNXDN calNXDN;
extern CalRSSI calRSSI;

/* CW */
extern CWIdTX cwIdTX;

#endif // __GLOBALS_H__
