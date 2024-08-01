// SPDX-License-Identifier: GPL-2.0-only
/*
 * Digital Voice Modem - Hotspot Firmware
 * GPLv2 Open Source. Use is subject to license terms.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 *  Copyright (C) 2015,2016,2020 Jonathan Naylor, G4KLX
 *  Copyright (C) 2016 Mathis Schmieder, DB9MAT
 *  Copyright (C) 2016 Colin Durbridge, G4EML
 *  Copyright (C) 2016,2017,2018,2019 Andy Uribe, CA6JAU
 *  Copyright (C) 2019 Florian Wolters, DF2ET
 *  Copyright (C) 2021 Bryan Biedenkapp, N2PLL
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

/* DMR BS */
#if defined(DUPLEX)
dmr::DMRIdleRX dmrIdleRX;
dmr::DMRRX dmrRX;
#endif
dmr::DMRTX dmrTX;

/* DMR MS-DMO */
dmr::DMRDMORX dmrDMORX;
dmr::DMRDMOTX dmrDMOTX;

/* P25 */
p25::P25RX p25RX;
p25::P25TX p25TX;

/* NXDN */
nxdn::NXDNRX nxdnRX;
nxdn::NXDNTX nxdnTX;

/* Calibration */
dmr::CalDMR calDMR;
p25::CalP25 calP25;
nxdn::CalNXDN calNXDN;
CalRSSI calRSSI;

/* CW */
CWIdTX cwIdTX;

/* RS232 and Air Interface I/O */
SerialPort serial;
IO io;

// ---------------------------------------------------------------------------
//  Global Functions
// ---------------------------------------------------------------------------

void setup()
{
    io.init();

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
#include <stm32f10x_flash.h>

#define STM32_CNF_PAGE_ADDR (uint32_t)0x0800FC00
#define STM32_CNF_PAGE      ((uint32_t *)0x0800FC00)
#define STM32_CNF_PAGE_24   24U

void jumpToBootLoader()
{
    // Disable RCC, set it to default (after reset) settings Internal clock, no PLL, etc.
    RCC_DeInit();
    USART_DeInit(USART1);
    USART_DeInit(UART5);

    // Disable Systick timer
    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL = 0;

    // Clear Interrupt Enable Register & Interrupt Pending Register
    for (uint8_t i = 0; i < sizeof(NVIC->ICER) / sizeof(NVIC->ICER[0]); i++) {
        NVIC->ICER[i] = 0xFFFFFFFF;
        NVIC->ICPR[i] = 0xFFFFFFFF;
    }

#if defined(STM32F10X_MD)
    volatile uint32_t addr = 0x1FFFF000;
#endif

    // Update the NVIC's vector
    SCB->VTOR = addr;

    void (*SysMemBootJump)(void);
    SysMemBootJump = (void (*)(void))(*((uint32_t *)(addr + 4)));
    __ASM volatile ("MSR msp, %0" : : "r" (*(uint32_t *)addr) : "sp"); // __set_MSP
    SysMemBootJump();
}

int main()
{
    // does the configuration page contain the request bootloader flag?
    if ((STM32_CNF_PAGE[STM32_CNF_PAGE_24] != 0xFFFFFFFFU) && (STM32_CNF_PAGE[STM32_CNF_PAGE_24] != 0x00U)) {
        uint8_t bootloadMode = (STM32_CNF_PAGE[STM32_CNF_PAGE_24] >> 8) & 0xFFU;
        if ((bootloadMode & 0x20U) == 0x20U) {
            // we unfortunately need to discard the configuration area entirely for bootloader mode...
            FLASH_Unlock();
            FLASH_ClearFlag(FLASH_FLAG_BSY | FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);

#if defined(STM32F4XX) || defined(STM32F7XX)
            if (FLASH_EraseSector(STM32_CNF_SECTOR, VoltageRange_3) != FLASH_COMPLETE) {
                FLASH_Lock();
                return RSN_FAILED_ERASE_FLASH;
            }
#elif defined(STM32F10X_MD)
            if (FLASH_ErasePage(STM32_CNF_PAGE_ADDR) != FLASH_COMPLETE) {
                FLASH_Lock();
                return RSN_FAILED_ERASE_FLASH;
            }
#endif
            jumpToBootLoader();
        }
    }

    setup();

    for (;;)
        loop();
}
