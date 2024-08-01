// SPDX-License-Identifier: GPL-2.0-only
/*
 * Digital Voice Modem - Hotspot Firmware
 * GPLv2 Open Source. Use is subject to license terms.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 *  Copyright (C) 2020 Jonathan Naylor, G4KLX
 *  Copyright (C) 2016 Jim McLaughlin, KI6ZUM
 *  Copyright (C) 2016,2017,2018,2019,2020 Andy Uribe, CA6JAU
 *  Copyright (C) 2017 Danilo, DB4PLE
 *  Copyright (C) 2021,2024 Bryan Biedenkapp, N2PLL
 *
 */
#include "Globals.h"
#include "IO.h"

// ---------------------------------------------------------------------------
//  Constants
// ---------------------------------------------------------------------------

#if defined(STM32F10X_MD)
/**
 * The STM32 factory-programmed UUID memory.
 * Three values of 32 bits each starting at this address
 * Use like this: STM32_UUID[0], STM32_UUID[1], STM32_UUID[2]
 */
#define STM32_UUID ((uint32_t *)0x1FFFF7E8)

#if defined(PI_HAT_7021_REV_02)

#define PIN_SCLK             GPIO_Pin_4
#define PORT_SCLK            GPIOB

#define PIN_SREAD            GPIO_Pin_5
#define PORT_SREAD           GPIOB

#define PIN_SDATA            GPIO_Pin_6
#define PORT_SDATA           GPIOB

#define PIN_SLE              GPIO_Pin_7
#define PORT_SLE             GPIOB

#define PIN_CE               GPIO_Pin_14
#define PORT_CE              GPIOC

#define PIN_RXD              GPIO_Pin_3
#define PORT_RXD             GPIOB

// TXD used in SPI Data mode of ADF7021
// TXD is TxRxCLK of ADF7021, standard TX/RX data interface
#define PIN_TXD              GPIO_Pin_15
#define PORT_TXD             GPIOA
#define PIN_TXD_INT          GPIO_PinSource15
#define PORT_TXD_INT         GPIO_PortSourceGPIOA

// CLKOUT used in SPI Data mode of ADF7021
#define PIN_CLKOUT           GPIO_Pin_14
#define PORT_CLKOUT          GPIOA
#define PIN_CLKOUT_INT       GPIO_PinSource14
#define PORT_CLKOUT_INT      GPIO_PortSourceGPIOA

#define PIN_LED              GPIO_Pin_13
#define PORT_LED             GPIOC

#define PIN_DEB              GPIO_Pin_11
#define PORT_DEB             GPIOA

#define PIN_NXDN_LED         GPIO_Pin_8
#define PORT_NXDN_LED        GPIOA

#define PIN_DMR_LED          GPIO_Pin_15
#define PORT_DMR_LED         GPIOB

#define PIN_P25_LED          GPIO_Pin_12
#define PORT_P25_LED         GPIOA

#define PIN_PTT_LED          GPIO_Pin_12
#define PORT_PTT_LED         GPIOB

#define PIN_COS_LED          GPIO_Pin_13
#define PORT_COS_LED         GPIOB

#elif defined(ZUMSPOT_ADF7021) || defined(SKYBRIDGE_HS)

#define PIN_SCLK             GPIO_Pin_5
#define PORT_SCLK            GPIOB

#define PIN_SREAD            GPIO_Pin_6
#define PORT_SREAD           GPIOB

#define PIN_SDATA            GPIO_Pin_7
#define PORT_SDATA           GPIOB

#define PIN_SLE              GPIO_Pin_8
#define PORT_SLE             GPIOB

#define PIN_SLE2             GPIO_Pin_6
#define PORT_SLE2            GPIOA

#define PIN_CE               GPIO_Pin_14
#define PORT_CE              GPIOC

#define PIN_RXD              GPIO_Pin_4
#define PORT_RXD             GPIOB

#define PIN_SGL_DBL          GPIO_Pin_12
#define PORT_SGL_DBL         GPIOA

#define PIN_DL_DPX           GPIO_Pin_15
#define PORT_DL_DPX          GPIOC

#define PIN_SET_BAND         GPIO_Pin_15
#define PORT_SET_BAND        GPIOA

// TXD used in SPI Data mode of ADF7021
// TXD is TxRxCLK of ADF7021, standard TX/RX data interface
#define PIN_TXD              GPIO_Pin_3
#define PORT_TXD             GPIOB
#define PIN_TXD_INT          GPIO_PinSource3
#define PORT_TXD_INT         GPIO_PortSourceGPIOB

#if defined(DUPLEX)
#define PIN_RXD2             GPIO_Pin_11
#define PORT_RXD2            GPIOA

// TXD2 is TxRxCLK of the second ADF7021, standard TX/RX data interface
#define PIN_TXD2             GPIO_Pin_8
#define PORT_TXD2            GPIOA
#define PIN_TXD2_INT         GPIO_PinSource8
#define PORT_TXD2_INT        GPIO_PortSourceGPIOA
#endif

// CLKOUT used in SPI Data mode of ADF7021
#define PIN_CLKOUT           GPIO_Pin_15
#define PORT_CLKOUT          GPIOA
#define PIN_CLKOUT_INT       GPIO_PinSource15
#define PORT_CLKOUT_INT      GPIO_PortSourceGPIOA

#define PIN_LED              GPIO_Pin_13
#define PORT_LED             GPIOC

#define PIN_DEB              GPIO_Pin_9
#define PORT_DEB             GPIOB

#define PIN_NXDN_LED         GPIO_Pin_7
#define PORT_NXDN_LED        GPIOA

#define PIN_DMR_LED          GPIO_Pin_13
#define PORT_DMR_LED         GPIOB

#define PIN_P25_LED          GPIO_Pin_0
#define PORT_P25_LED         GPIOB

#define PIN_PTT_LED          GPIO_Pin_14
#define PORT_PTT_LED         GPIOB

#define PIN_COS_LED          GPIO_Pin_15
#define PORT_COS_LED         GPIOB

#elif defined(MMDVM_HS_HAT_REV12) || defined(MMDVM_HS_DUAL_HAT_REV10) || defined(NANO_HOTSPOT) || defined(NANO_DV_REV11)

#define PIN_SCLK             GPIO_Pin_5
#define PORT_SCLK            GPIOB

#define PIN_SREAD            GPIO_Pin_7
#define PORT_SREAD           GPIOB

#define PIN_SDATA            GPIO_Pin_6
#define PORT_SDATA           GPIOB

#define PIN_SLE              GPIO_Pin_8
#define PORT_SLE             GPIOB

#define PIN_SLE2             GPIO_Pin_6
#define PORT_SLE2            GPIOA

#define PIN_CE               GPIO_Pin_14
#define PORT_CE              GPIOC

#define PIN_RXD              GPIO_Pin_4
#define PORT_RXD             GPIOB

#define PIN_RXD2             GPIO_Pin_4
#define PORT_RXD2            GPIOA

// TXD used in SPI Data mode of ADF7021
// TXD is TxRxCLK of ADF7021, standard TX/RX data interface
#define PIN_TXD              GPIO_Pin_3
#define PORT_TXD             GPIOB
#define PIN_TXD_INT          GPIO_PinSource3
#define PORT_TXD_INT         GPIO_PortSourceGPIOB

// TXD2 is TxRxCLK of the second ADF7021, standard TX/RX data interface
#define PIN_TXD2             GPIO_Pin_5
#define PORT_TXD2            GPIOA
#define PIN_TXD2_INT         GPIO_PinSource5
#define PORT_TXD2_INT        GPIO_PortSourceGPIOA

// CLKOUT used in SPI Data mode of ADF7021
#define PIN_CLKOUT           GPIO_Pin_15
#define PORT_CLKOUT          GPIOA
#define PIN_CLKOUT_INT       GPIO_PinSource15
#define PORT_CLKOUT_INT      GPIO_PortSourceGPIOA

#define PIN_LED              GPIO_Pin_13
#define PORT_LED             GPIOC

#define PIN_DEB              GPIO_Pin_9
#define PORT_DEB             GPIOB

#define PIN_NXDN_LED         GPIO_Pin_8
#define PORT_NXDN_LED        GPIOA

#define PIN_DMR_LED          GPIO_Pin_13
#define PORT_DMR_LED         GPIOB

#define PIN_P25_LED          GPIO_Pin_0
#define PORT_P25_LED         GPIOB

#define PIN_PTT_LED          GPIO_Pin_14
#define PORT_PTT_LED         GPIOB

#define PIN_COS_LED          GPIO_Pin_15
#define PORT_COS_LED         GPIOB

#else
#error "Either PI_HAT_7021_REV_02, ZUMSPOT_ADF7021, MMDVM_HS_HAT_REV12, MMDVM_HS_DUAL_HAT_REV10, NANO_HOTSPOT, NANO_DV_REV11, or SKYBRIDGE_HS need to be defined"
#endif

// ---------------------------------------------------------------------------
//  Global Functions
// ---------------------------------------------------------------------------

extern "C" {
#if defined(PI_HAT_7021_REV_02)

#if defined(BIDIR_DATA_PIN)
    void EXTI15_10_IRQHandler(void)
    {
        if (EXTI_GetITStatus(EXTI_Line15) != RESET) {
            io.interrupt1();
            EXTI_ClearITPendingBit(EXTI_Line15);
        }
    }
#else
    void EXTI15_10_IRQHandler(void)
    {
        if (EXTI_GetITStatus(EXTI_Line14) != RESET) {
            io.interrupt1();
            EXTI_ClearITPendingBit(EXTI_Line14);
        }
    }
#endif // BIDIR_DATA_PIN

#elif defined(ZUMSPOT_ADF7021) || defined(LIBRE_KIT_ADF7021) || defined(MMDVM_HS_HAT_REV12) || defined(MMDVM_HS_DUAL_HAT_REV10) || defined(NANO_HOTSPOT) || defined(NANO_DV_REV11) || defined(D2RG_MMDVM_HS) || defined(SKYBRIDGE_HS)

#if defined(BIDIR_DATA_PIN)
    void EXTI3_IRQHandler(void) {
        if (EXTI_GetITStatus(EXTI_Line3) != RESET) {
            io.interrupt1();
            EXTI_ClearITPendingBit(EXTI_Line3);
        }
    }
#else
    void EXTI15_10_IRQHandler(void) {
        if (EXTI_GetITStatus(EXTI_Line15) != RESET) {
            io.interrupt1();
            EXTI_ClearITPendingBit(EXTI_Line15);
        }
    }
#endif // BIDIR_DATA_PIN

#if defined(DUPLEX)
    void EXTI9_5_IRQHandler(void) {
#if defined(ZUMSPOT_ADF7021) || defined(SKYBRIDGE_HS)
        if (EXTI_GetITStatus(EXTI_Line8) != RESET) {
            io.interrupt2();
            EXTI_ClearITPendingBit(EXTI_Line8);
        }
#else
        if (EXTI_GetITStatus(EXTI_Line5) != RESET) {
            io.interrupt2();
            EXTI_ClearITPendingBit(EXTI_Line5);
        }
#endif // ZUMSPOT_ADF7021 || SKYBRIDGE_HS
    }
#endif

#endif
}

/* Function delay_us() from stm32duino project */

static inline void delay_us(uint32_t us) 
{
    us *= 12;

    /* fudge for function call overhead  */
    us--;
    asm volatile(
        "   mov r0, %[us]          \n\t"
        "1: subs r0, #1            \n\t"
        "   bhi 1b                 \n\t"
        :
    : [us] "r" (us)
        : "r0");
}

/* */

static inline void delay_ns()
{

    asm volatile(
        "nop          \n\t"
        "nop          \n\t"
        "nop          \n\t"
    );
}

// ---------------------------------------------------------------------------
//  Public Class Members
// ---------------------------------------------------------------------------

/* Gets the CPU type the firmware is running on. */

uint8_t IO::getCPU() const
{
    return CPU_TYPE_STM32;
}

/* Gets the unique identifier for the air interface. */

void IO::getUDID(uint8_t* buffer)
{
    ::memcpy(buffer, (void*)STM32_UUID, 12U);
}

/* */

void IO::resetMCU()
{
    DEBUG1("reset - bye-bye");

    delayUS(250 * 1000);

    setLEDInt(false);
    setCOSInt(false);
    setDMRInt(false);
    setP25Int(false);
    setNXDNInt(false);

    delayUS(250 * 1000);

    NVIC_SystemReset();
}

/* */

void IO::delayBit()
{
    delay_ns();
}

#if defined(ZUMSPOT_ADF7021) || defined(SKYBRIDGE_HS)
/* */

bool IO::isDualBand()
{
    return GPIO_ReadInputDataBit(PORT_DL_DPX, PIN_DL_DPX) == Bit_SET;
}
#endif

/* */

void IO::SCLK(bool on)
{
    GPIO_WriteBit(PORT_SCLK, PIN_SCLK, on ? Bit_SET : Bit_RESET);
}

/* */

void IO::SDATA(bool on)
{
    GPIO_WriteBit(PORT_SDATA, PIN_SDATA, on ? Bit_SET : Bit_RESET);
}

/* */

bool IO::SREAD()
{
    return GPIO_ReadInputDataBit(PORT_SREAD, PIN_SREAD) == Bit_SET;
}

/* */

void IO::SLE1(bool on)
{
    GPIO_WriteBit(PORT_SLE, PIN_SLE, on ? Bit_SET : Bit_RESET);
}

#if defined(DUPLEX)
/* */

void IO::SLE2(bool on)
{
    GPIO_WriteBit(PORT_SLE2, PIN_SLE2, on ? Bit_SET : Bit_RESET);
}

/* */

bool IO::RXD2()
{
    return GPIO_ReadInputDataBit(PORT_RXD2, PIN_RXD2) == Bit_SET;
}
#endif

/* */

void IO::CE(bool on)
{
    GPIO_WriteBit(PORT_CE, PIN_CE, on ? Bit_SET : Bit_RESET);
}

/* */

bool IO::RXD1()
{
    return GPIO_ReadInputDataBit(PORT_RXD, PIN_RXD) == Bit_SET;
}

/* */

bool IO::CLK()
{
#if defined(BIDIR_DATA_PIN)
    return GPIO_ReadInputDataBit(PORT_TXD, PIN_TXD) == Bit_SET;
#else
    return GPIO_ReadInputDataBit(PORT_CLKOUT, PIN_CLKOUT) == Bit_SET;
#endif
}

// ---------------------------------------------------------------------------
//  Private Class Members
// ---------------------------------------------------------------------------

#if defined(ZUMSPOT_ADF7021) || defined(SKYBRIDGE_HS)
/* */

void IO::setBandVHF(bool enable)
{
    GPIO_WriteBit(PORT_SET_BAND, PIN_SET_BAND, enable ? Bit_SET : Bit_RESET);
}

/* */

bool IO::hasSingleADF7021()
{
    return GPIO_ReadInputDataBit(PORT_SGL_DBL, PIN_SGL_DBL) == Bit_SET;
}
#endif

/* */

void IO::delayIfCal() 
{
    delayUS(10000);
}

/* */

void IO::delayReset() 
{
    delayUS(300);
}

/* */

void IO::delayUS(uint32_t us) 
{
    ::delay_us(us);
}

/* Initializes hardware interrupts. */

void IO::initInt()
{
    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_StructInit(&GPIO_InitStruct);

    EXTI_InitTypeDef EXTI_InitStructure;
#if defined(DUPLEX)
    EXTI_InitTypeDef EXTI_InitStructure2;
#endif

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC | RCC_APB2Periph_AFIO, ENABLE);

#if defined(PI_HAT_7021_REV_02)
    GPIO_PinRemapConfig(GPIO_Remap_SWJ_Disable, ENABLE);
#elif defined(ZUMSPOT_ADF7021) || defined(LIBRE_KIT_ADF7021) || defined(MMDVM_HS_HAT_REV12) || defined(MMDVM_HS_DUAL_HAT_REV10) || defined(NANO_HOTSPOT) || defined(NANO_DV_REV11) || defined(D2RG_MMDVM_HS) || defined(SKYBRIDGE_HS)
    GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);
#endif

#if defined(ZUMSPOT_ADF7021) || defined(SKYBRIDGE_HS)
    // Pin defines if the board has a single ADF7021 or double
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStruct.GPIO_Pin = PIN_SGL_DBL;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(PORT_SGL_DBL, &GPIO_InitStruct);

    // Pin defines if the board is dual band or duplex
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStruct.GPIO_Pin = PIN_DL_DPX;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(PORT_DL_DPX, &GPIO_InitStruct);

    // Pin will set UHF or VHF
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStruct.GPIO_Pin = PIN_SET_BAND;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(PORT_SET_BAND, &GPIO_InitStruct);
    // TODO: Remove this line
    // GPIO_WriteBit(PORT_SET_BAND, PIN_SET_BAND, Bit_RESET);

#endif

    volatile unsigned int delay;
    for (delay = 0; delay < 512; delay++);

    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_11 | GPIO_Pin_12;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStruct);

    RCC_USBCLKConfig(RCC_USBCLKSource_PLLCLK_1Div5);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USB, ENABLE);
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);

    // Pin SCLK
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStruct.GPIO_Pin = PIN_SCLK;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(PORT_SCLK, &GPIO_InitStruct);

    // Pin SDATA
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStruct.GPIO_Pin = PIN_SDATA;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(PORT_SDATA, &GPIO_InitStruct);

    // Pin SREAD
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStruct.GPIO_Pin = PIN_SREAD;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(PORT_SREAD, &GPIO_InitStruct);

    // Pin SLE
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStruct.GPIO_Pin = PIN_SLE;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(PORT_SLE, &GPIO_InitStruct);

#if defined(DUPLEX)
    // Pin SLE2
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStruct.GPIO_Pin = PIN_SLE2;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(PORT_SLE2, &GPIO_InitStruct);

    // Pin RXD2
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStruct.GPIO_Pin = PIN_RXD2;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(PORT_RXD2, &GPIO_InitStruct);
#endif

    // Pin CE
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStruct.GPIO_Pin = PIN_CE;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(PORT_CE, &GPIO_InitStruct);

    // Pin RXD
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStruct.GPIO_Pin = PIN_RXD;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(PORT_RXD, &GPIO_InitStruct);

    // Pin TXD
    // TXD is TxRxCLK of ADF7021, standard TX/RX data interface
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStruct.GPIO_Pin = PIN_TXD;
#if defined(BIDIR_DATA_PIN)
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN_FLOATING;
#else
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
#endif
    GPIO_Init(PORT_TXD, &GPIO_InitStruct);
#if defined(DUPLEX)
    GPIO_InitStruct.GPIO_Pin = PIN_TXD2;
    GPIO_Init(PORT_TXD2, &GPIO_InitStruct);
#endif

    // Pin TXRX_CLK
#if !defined(BIDIR_DATA_PIN)
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStruct.GPIO_Pin = PIN_CLKOUT;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(PORT_CLKOUT, &GPIO_InitStruct);
#endif

    // Pin LED
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStruct.GPIO_Pin = PIN_LED;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(PORT_LED, &GPIO_InitStruct);

    // Pin Debug
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStruct.GPIO_Pin = PIN_DEB;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(PORT_DEB, &GPIO_InitStruct);

    // NXDN LED
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStruct.GPIO_Pin = PIN_NXDN_LED;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(PORT_NXDN_LED, &GPIO_InitStruct);

    // DMR LED
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStruct.GPIO_Pin = PIN_DMR_LED;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(PORT_DMR_LED, &GPIO_InitStruct);

    // P25 LED
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStruct.GPIO_Pin = PIN_P25_LED;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(PORT_P25_LED, &GPIO_InitStruct);

    // PTT LED
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStruct.GPIO_Pin = PIN_PTT_LED;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(PORT_PTT_LED, &GPIO_InitStruct);

    // COS LED
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStruct.GPIO_Pin = PIN_COS_LED;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(PORT_COS_LED, &GPIO_InitStruct);

#if defined(PI_HAT_7021_REV_02)

#if defined(BIDIR_DATA_PIN)
    // Connect EXTI15 Line
    GPIO_EXTILineConfig(PORT_TXD_INT, PIN_TXD_INT);
    // Configure EXTI15 line
    EXTI_InitStructure.EXTI_Line = EXTI_Line15;
#else
    // Connect EXTI14 Line
    GPIO_EXTILineConfig(PORT_CLKOUT_INT, PIN_CLKOUT_INT);
    // Configure EXTI14 line
    EXTI_InitStructure.EXTI_Line = EXTI_Line14;
#endif

#elif defined(ZUMSPOT_ADF7021) || defined(LIBRE_KIT_ADF7021) || defined(MMDVM_HS_HAT_REV12) || defined(MMDVM_HS_DUAL_HAT_REV10) || defined(NANO_HOTSPOT) || defined(NANO_DV_REV11) || defined(D2RG_MMDVM_HS) || defined(SKYBRIDGE_HS)

#if defined(BIDIR_DATA_PIN)
    // Connect EXTI3 Line
    GPIO_EXTILineConfig(PORT_TXD_INT, PIN_TXD_INT);
    // Configure EXTI3 line
    EXTI_InitStructure.EXTI_Line = EXTI_Line3;
#else
    // Connect EXTI15 Line
    GPIO_EXTILineConfig(PORT_CLKOUT_INT, PIN_CLKOUT_INT);
    // Configure EXTI15 line
    EXTI_InitStructure.EXTI_Line = EXTI_Line15;
#endif

#if defined(DUPLEX)
    // Connect EXTI5 Line
    GPIO_EXTILineConfig(PORT_TXD2_INT, PIN_TXD2_INT);
    // Configure EXT5 line
#if defined(ZUMSPOT_ADF7021) || defined(SKYBRIDGE_HS)
    EXTI_InitStructure2.EXTI_Line = EXTI_Line8;
#else
    EXTI_InitStructure2.EXTI_Line = EXTI_Line5;
#endif
#endif

#endif

    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising_Falling;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);

#if defined(DUPLEX)
    EXTI_InitStructure2.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure2.EXTI_Trigger = EXTI_Trigger_Rising;
    EXTI_InitStructure2.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure2);
#endif
}

/* Starts hardware interrupts. */

void IO::startInt()
{
    NVIC_InitTypeDef NVIC_InitStructure;

#if defined(DUPLEX)
    NVIC_InitTypeDef NVIC_InitStructure2;
#endif

#if defined(PI_HAT_7021_REV_02)

    NVIC_InitStructure.NVIC_IRQChannel = EXTI15_10_IRQn;

#elif defined(ZUMSPOT_ADF7021) || defined(LIBRE_KIT_ADF7021) || defined(MMDVM_HS_HAT_REV12) || defined(MMDVM_HS_DUAL_HAT_REV10) || defined(NANO_HOTSPOT) || defined(NANO_DV_REV11) || defined(D2RG_MMDVM_HS) || defined(SKYBRIDGE_HS)

#if defined(BIDIR_DATA_PIN)
    // Enable and set EXTI3 Interrupt
    NVIC_InitStructure.NVIC_IRQChannel = EXTI3_IRQn;
#else
    // Enable and set EXTI15 Interrupt
    NVIC_InitStructure.NVIC_IRQChannel = EXTI15_10_IRQn;
#endif

#if defined(DUPLEX)
    NVIC_InitStructure2.NVIC_IRQChannel = EXTI9_5_IRQn;
#endif

#endif

    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 15;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

#if defined(DUPLEX)
    NVIC_InitStructure2.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure2.NVIC_IRQChannelSubPriority = 15;
    NVIC_InitStructure2.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure2);
#endif
}

#if defined(BIDIR_DATA_PIN)

/* */

void IO::setDataDirOut(bool dir)
{
    GPIO_InitTypeDef GPIO_InitStruct;

    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStruct.GPIO_Pin = PIN_RXD;

    if (dir)
        GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
    else
        GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN_FLOATING;

    GPIO_Init(PORT_RXD, &GPIO_InitStruct);
}
#endif

#if defined(BIDIR_DATA_PIN)
/* */

void IO::setRXDInt(bool on)
{
    GPIO_WriteBit(PORT_RXD, PIN_RXD, on ? Bit_SET : Bit_RESET);
}
#endif

/* */

void IO::setTXDInt(bool on)
{
    GPIO_WriteBit(PORT_TXD, PIN_TXD, on ? Bit_SET : Bit_RESET);
}

/* */

void IO::setLEDInt(bool on)
{
    GPIO_WriteBit(PORT_LED, PIN_LED, on ? Bit_SET : Bit_RESET);
}

/* */

void IO::setPTTInt(bool on)
{
    GPIO_WriteBit(PORT_PTT_LED, PIN_PTT_LED, on ? Bit_SET : Bit_RESET);
}

/* */

void IO::setCOSInt(bool on)
{
    GPIO_WriteBit(PORT_COS_LED, PIN_COS_LED, on ? Bit_SET : Bit_RESET);
}

/* */

void IO::setDMRInt(bool on)
{
    GPIO_WriteBit(PORT_DMR_LED, PIN_DMR_LED, on ? Bit_SET : Bit_RESET);
}

/* */

void IO::setP25Int(bool on)
{
    GPIO_WriteBit(PORT_P25_LED, PIN_P25_LED, on ? Bit_SET : Bit_RESET);
}

/* */

void IO::setNXDNInt(bool on)
{
    GPIO_WriteBit(PORT_NXDN_LED, PIN_NXDN_LED, on ? Bit_SET : Bit_RESET);
}

#endif // STM32F10X_MD
