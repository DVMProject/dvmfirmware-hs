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
*   Copyright (C) 2016 by Jim McLaughlin KI6ZUM
*   Copyright (C) 2016,2017,2018,2019 by Andy Uribe CA6JAU
*   Copyright (C) 2021-2022 Bryan Biedenkapp N2PLL
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
#include "SerialPort.h"
#include "STM_UART.h"

#if defined(STM32F10X_MD)
#include <stm32f10x_flash.h>

#if defined(STM32_USB_HOST)
#include <usb_serial.h>
#endif

#if defined(STM32_USART1_HOST) && defined(STM32_USB_HOST)
#error "You have to select STM32_USART1_HOST or STM32_USB_HOST, but not both"
#endif

// ---------------------------------------------------------------------------
//  Constants
// ---------------------------------------------------------------------------

#define STM32_CNF_PAGE_ADDR (uint32_t)0x0800FC00
#define STM32_CNF_PAGE      ((uint32_t *)0x0800FC00)

/*
    Pin definitions:

    - Host communication:
    1) USART1 - TXD PA9  - RXD PA10
    2) USB VCOM

    - Serial repeater
    USART2 - TXD PA2  - RXD PA3 
*/

#if defined(STM32_USART1_HOST) || defined(SERIAL_REPEATER_USART1)
// ---------------------------------------------------------------------------
//  Global Functions and Variables
// ---------------------------------------------------------------------------

extern "C" {
    void USART1_IRQHandler();
    void USART2_IRQHandler();
}

// ---------------------------------------------------------------------------
//  UART1
// ---------------------------------------------------------------------------

static STM_UART m_USART1;

/// <summary>
///
/// </summary>
void USART1_IRQHandler()
{
    m_USART1.handleIRQ();
}

/// <summary>
///
/// </summary>
/// <param name="speed"></param>
void InitUSART1(int speed)
{
    // USART1 - TXD PA9  - RXD PA10
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);

    // USART IRQ init
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 15;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 15;
    NVIC_Init(&NVIC_InitStructure);

    // Configure USART as alternate function
    GPIO_StructInit(&GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;       //  Tx
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;       //  Rx
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // Configure USART baud rate
    USART_StructInit(&USART_InitStructure);
    USART_InitStructure.USART_BaudRate = speed;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART1, &USART_InitStructure);

    USART_Cmd(USART1, ENABLE);

    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);

    m_USART1.init(USART1);
}

#endif

// ---------------------------------------------------------------------------
//  UART2
// ---------------------------------------------------------------------------

static STM_UART m_USART2;

/// <summary>
///
/// </summary>
void USART2_IRQHandler()
{
    m_USART2.handleIRQ();
}

/// <summary>
/// 
/// </summary>
/// <param name="speed"></param>
void InitUSART2(int speed)
{
    // USART2 - TXD PA2  - RXD PA3
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);

    // USART IRQ init
    NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 15;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 15;
    NVIC_Init(&NVIC_InitStructure);

    // Configure USART as alternate function
    GPIO_StructInit(&GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;       //  Tx
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;       //  Rx
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // Configure USART baud rate
    USART_StructInit(&USART_InitStructure);
    USART_InitStructure.USART_BaudRate = speed;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART2, &USART_InitStructure);

    USART_Cmd(USART2, ENABLE);

    USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);

    m_USART2.init(USART2);
}

// ---------------------------------------------------------------------------
//  Private Class Members
// ---------------------------------------------------------------------------
/// <summary>
///
/// </summary>
void SerialPort::flashRead()
{
    uint8_t reply[249U];

    reply[0U] = DVM_FRAME_START;
    reply[1U] = 249U;
    reply[2U] = CMD_FLSH_READ;

    ::memcpy(reply + 3U, (void*)STM32_CNF_PAGE, 246U);

    DEBUG1("SerialPort: flashRead(): read bytes from flash");

    writeInt(1U, reply, 249U);
}

/// <summary>
///
/// </summary>
/// <param name="data"></param>
/// <param name="length"></param>
uint8_t SerialPort::flashWrite(const uint8_t* data, uint8_t length)
{
    if (length > 249U) {
        return RSN_FLASH_WRITE_TOO_BIG;
    }

    DEBUG1("SerialPort: flashWrite(): unlocking flash");

    FLASH_Unlock();
    FLASH_ClearFlag(FLASH_FLAG_BSY | FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);

#if defined(STM32F4XX)
    DEBUG1("SerialPort: flashWrite(): erasing flash sector");
    if (FLASH_EraseSector(STM32_CNF_SECTOR, VoltageRange_3) != FLASH_COMPLETE) {
        FLASH_Lock();
        return RSN_FAILED_ERASE_FLASH;
    }
#elif defined(STM32F10X_MD)
    DEBUG1("SerialPort: flashWrite(): erasing flash page");
    if (FLASH_ErasePage(STM32_CNF_PAGE_ADDR) != FLASH_COMPLETE) {
        FLASH_Lock();
        return RSN_FAILED_ERASE_FLASH;
    }
#endif

    // write data to the user flash area
    uint32_t address = STM32_CNF_PAGE_ADDR;
    uint8_t i = 0U;
    while (i < length) {
        uint32_t word =
            (data[i + 3] << 24) +
            (data[i + 2] << 16) +
            (data[i + 1] << 8) +
            (data[i + 0] << 0);

        DEBUG3("SerialPort: flashWrite(): writing byte data", address, i);
        if (FLASH_ProgramWord(address, word) == FLASH_COMPLETE) {
            address += 4;
            i += 4;
        }
        else {
            FLASH_Lock();
            return RSN_FAILED_WRITE_FLASH;
        }
    }

    DEBUG1("SerialPort: flashWrite(): finished writing, locking flash");
    FLASH_Lock();
    return RSN_OK;
}

/// <summary>
///
/// </summary>
/// <param name="n"></param>
/// <param name="speed"></param>
void SerialPort::beginInt(uint8_t n, int speed)
{
    switch (n) {
    case 1U:
#if defined(STM32_USART1_HOST)
        InitUSART1(speed);
#elif defined(STM32_USB_HOST)
        usbserial.begin();
#endif
        break;
    case 3U:
        InitUSART2(speed);
        break;
    default:
        break;
    }
}

/// <summary>
///
/// </summary>
/// <param name="n"></param>
/// <returns></returns>
int SerialPort::availableInt(uint8_t n)
{
    switch (n) {
    case 1U:
#if defined(STM32_USART1_HOST)
        return m_USART1.available();
#elif defined(STM32_USB_HOST)
        return usbserial.available();
#endif
    case 3U:
        m_USART2.available();
    default:
        return 0;
    }
}

/// <summary>
///
/// </summary>
/// <param name="n"></param>
/// <returns></returns>
int SerialPort::availableForWriteInt(uint8_t n)
{
    switch (n) {
    case 1U:
#if defined(STM32_USART1_HOST)
        return m_USART1.availableForWrite();
#elif defined(STM32_USB_HOST)
        //return usbserial.availableForWrite();
        return 1U; // we don't have this -- so fake it
#endif
    case 3U:
        return m_USART2.availableForWrite();
    default:
        return 0;
    }
}

/// <summary>
///
/// </summary>
/// <param name="n"></param>
/// <returns></returns>
uint8_t SerialPort::readInt(uint8_t n)
{
    switch (n) {
    case 1U:
#if defined(STM32_USART1_HOST)
        return m_USART1.read();
#elif defined(STM32_USB_HOST)
        return usbserial.read();
#endif
    case 3U:
        return m_USART2.read();
    default:
        return 0U;
    }
}


/// <summary>
///
/// </summary>
/// <param name="n"></param>
/// <param name="data"></param>
/// <param name="length"></param>
/// <param name="flush"></param>
void SerialPort::writeInt(uint8_t n, const uint8_t* data, uint16_t length, bool flush)
{
    switch (n) {
    case 1U:
#if defined(STM32_USART1_HOST)
        m_USART1.write(data, length);
        if (flush)
            m_USART1.flush();
#elif defined(STM32_USB_HOST)
        usbserial.write(data, length);
        if (flush)
            usbserial.flush();
#endif
        break;
    case 3U:
        m_USART2.write(data, length);
        if (flush)
            m_USART2.flush();
        break;
    default:
        break;
    }
}

#endif
