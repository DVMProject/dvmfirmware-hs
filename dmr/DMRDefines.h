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
*   Copyright (C) 2009-2016 Jonathan Naylor, G4KLX
*
*/
#if !defined(__DMR_DEFINES_H__)
#define __DMR_DEFINES_H__

#include "Defines.h"

namespace dmr
{
    // ---------------------------------------------------------------------------
    //  Constants
    // ---------------------------------------------------------------------------

    const uint32_t  DMR_RADIO_SYMBOL_LENGTH = 5U;      // At 24 kHz sample rate

    const uint32_t  DMR_FRAME_LENGTH_BYTES = 33U;
    const uint32_t  DMR_FRAME_LENGTH_BITS = DMR_FRAME_LENGTH_BYTES * 8U;
    const uint32_t  DMR_FRAME_LENGTH_SYMBOLS = DMR_FRAME_LENGTH_BYTES * 4U;
    const uint32_t  DMR_FRAME_LENGTH_SAMPLES = DMR_FRAME_LENGTH_SYMBOLS * DMR_RADIO_SYMBOL_LENGTH;

    const uint32_t  DMR_SYNC_LENGTH_BYTES = 6U;
    const uint32_t  DMR_SYNC_LENGTH_BITS = DMR_SYNC_LENGTH_BYTES * 8U;
    const uint32_t  DMR_SYNC_LENGTH_SYMBOLS = DMR_SYNC_LENGTH_BYTES * 4U;
    const uint32_t  DMR_SYNC_LENGTH_SAMPLES = DMR_SYNC_LENGTH_SYMBOLS * DMR_RADIO_SYMBOL_LENGTH;

    const uint32_t  DMR_EMB_LENGTH_BITS = 16U;
    const uint32_t  DMR_EMB_LENGTH_SYMBOLS = 8U;
    const uint32_t  DMR_EMB_LENGTH_SAMPLES = DMR_EMB_LENGTH_SYMBOLS * DMR_RADIO_SYMBOL_LENGTH;

    const uint32_t  DMR_EMBSIG_LENGTH_BITS = 32U;
    const uint32_t  DMR_EMBSIG_LENGTH_SYMBOLS = 16U;
    const uint32_t  DMR_EMBSIG_LENGTH_SAMPLES = DMR_EMBSIG_LENGTH_SYMBOLS * DMR_RADIO_SYMBOL_LENGTH;

    const uint32_t  DMR_SLOT_TYPE_LENGTH_BITS = 20U;
    const uint32_t  DMR_SLOT_TYPE_LENGTH_SYMBOLS = 10U;
    const uint32_t  DMR_SLOT_TYPE_LENGTH_SAMPLES = DMR_SLOT_TYPE_LENGTH_SYMBOLS * DMR_RADIO_SYMBOL_LENGTH;

    const uint32_t  DMR_INFO_LENGTH_BITS = 196U;
    const uint32_t  DMR_INFO_LENGTH_SYMBOLS = 98U;
    const uint32_t  DMR_INFO_LENGTH_SAMPLES = DMR_INFO_LENGTH_SYMBOLS * DMR_RADIO_SYMBOL_LENGTH;

    const uint32_t  DMR_AUDIO_LENGTH_BITS = 216U;
    const uint32_t  DMR_AUDIO_LENGTH_SYMBOLS = 108U;
    const uint32_t  DMR_AUDIO_LENGTH_SAMPLES = DMR_AUDIO_LENGTH_SYMBOLS * DMR_RADIO_SYMBOL_LENGTH;

    const uint32_t  DMR_CACH_LENGTH_BYTES = 3U;
    const uint32_t  DMR_CACH_LENGTH_BITS = DMR_CACH_LENGTH_BYTES * 8U;
    const uint32_t  DMR_CACH_LENGTH_SYMBOLS = DMR_CACH_LENGTH_BYTES * 4U;
    const uint32_t  DMR_CACH_LENGTH_SAMPLES = DMR_CACH_LENGTH_SYMBOLS * DMR_RADIO_SYMBOL_LENGTH;

    const uint8_t   DMR_SYNC_BYTES_LENGTH = 7U;
    const uint8_t   DMR_MS_DATA_SYNC_BYTES[] = { 0x0DU, 0x5DU, 0x7FU, 0x77U, 0xFDU, 0x75U, 0x70U };
    const uint8_t   DMR_MS_VOICE_SYNC_BYTES[] = { 0x07U, 0xF7U, 0xD5U, 0xDDU, 0x57U, 0xDFU, 0xD0U };
    const uint8_t   DMR_BS_DATA_SYNC_BYTES[] = { 0x0DU, 0xFFU, 0x57U, 0xD7U, 0x5DU, 0xF5U, 0xD0U };
    const uint8_t   DMR_BS_VOICE_SYNC_BYTES[] = { 0x07U, 0x55U, 0xFDU, 0x7DU, 0xF7U, 0x5FU, 0x70U };

    const uint8_t   DMR_S1_DATA_SYNC_BYTES[] = { 0x0FU, 0x7FU, 0xDDU, 0x5DU, 0xDFU, 0xD5U, 0x50U };
    const uint8_t   DMR_S1_VOICE_SYNC_BYTES[] = { 0x05U, 0xD5U, 0x77U, 0xF7U, 0x75U, 0x7FU, 0xF0U };
    const uint8_t   DMR_S2_DATA_SYNC_BYTES[] = { 0x0DU, 0x75U, 0x57U, 0xF5U, 0xFFU, 0x7FU, 0x50U };
    const uint8_t   DMR_S2_VOICE_SYNC_BYTES[] = { 0x07U, 0xDFU, 0xFDU, 0x5FU, 0x55U, 0xD5U, 0xF0U };

    const uint8_t   DMR_SYNC_BYTES_MASK[] = { 0x0FU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xF0U };

    const uint8_t   DMR_START_SYNC = 0x5FU;

    const uint64_t  DMR_MS_DATA_SYNC_BITS = 0x0000D5D7F77FD757U;
    const uint64_t  DMR_MS_VOICE_SYNC_BITS = 0x00007F7D5DD57DFDU;
    const uint64_t  DMR_BS_DATA_SYNC_BITS = 0x0000DFF57D75DF5DU;
    const uint64_t  DMR_BS_VOICE_SYNC_BITS = 0x0000755FD7DF75F7U;

    const uint64_t  DMR_S1_DATA_SYNC_BITS  = 0x0000F7FDD5DDFD55U;
    const uint64_t  DMR_S1_VOICE_SYNC_BITS = 0x00005D577F7757FFU;
    const uint64_t  DMR_S2_DATA_SYNC_BITS  = 0x0000D7557F5FF7F5U;
    const uint64_t  DMR_S2_VOICE_SYNC_BITS = 0x00007DFFD5F55D5FU;

    const uint64_t  DMR_SYNC_BITS_MASK = 0x0000FFFFFFFFFFFFU;

    const uint32_t  DMR_MS_DATA_SYNC_SYMBOLS = 0x0076286EU;
    const uint32_t  DMR_MS_VOICE_SYNC_SYMBOLS = 0x0089D791U;
    const uint32_t  DMR_BS_DATA_SYNC_SYMBOLS = 0x00439B4DU;
    const uint32_t  DMR_BS_VOICE_SYNC_SYMBOLS = 0x00BC64B2U;

    const uint32_t  DMR_S1_DATA_SYNC_SYMBOLS  = 0x0021751FU;
    const uint32_t  DMR_S1_VOICE_SYNC_SYMBOLS = 0x00DE8AE0U;
    const uint32_t  DMR_S2_DATA_SYNC_SYMBOLS  = 0x006F8C23U;
    const uint32_t  DMR_S2_VOICE_SYNC_SYMBOLS = 0x009073DCU;

    const uint32_t  DMR_SYNC_SYMBOLS_MASK = 0x00FFFFFFU;

    // D     5      D     7      F     7      7     F      D     7      5     7
    // 11 01 01 01  11 01 01 11  11 11 01 11  01 11 11 11  11 01 01 11  01 01 01 11
    // -3 +3 +3 +3  -3 +3 +3 -3  -3 -3 +3 -3  +3 -3 -3 -3  -3 +3 +3 -3  +3 +3 +3 -3

    const int8_t    DMR_MS_DATA_SYNC_SYMBOLS_VALUES[] = { -3, +3, +3, +3, -3, +3, +3, -3, -3, -3, +3, -3, +3, -3, -3, -3, -3, +3, +3, -3, +3, +3, +3, -3 };

    // 7     F      7     D      5     D      D     5      7     D      F     D
    // 01 11 11 11  01 11 11 01  01 01 11 01  11 01 01 01  01 11 11 01  11 11 11 01
    // +3 -3 -3 -3  +3 -3 -3 +3  +3 +3 -3 +3  -3 +3 +3 +3  +3 -3 -3 +3  -3 -3 -3 +3

    const int8_t    DMR_MS_VOICE_SYNC_SYMBOLS_VALUES[] = { +3, -3, -3, -3, +3, -3, -3, +3, +3, +3, -3, +3, -3, +3, +3, +3, +3, -3, -3, +3, -3, -3, -3, +3 };

    // 505 = DMR_FRAME_LENGTH_BYTES * 15 + 10 (BUFFER_LEN = DMR_FRAME_LENGTH_BYTES * NO_OF_FRAMES + 10)
    const uint32_t  DMR_TX_BUFFER_LEN = 505U; // 15 frames + pad

    // Data Type(s)
    const uint8_t   DT_VOICE_PI_HEADER = 0U;
    const uint8_t   DT_VOICE_LC_HEADER = 1U;
    const uint8_t   DT_TERMINATOR_WITH_LC = 2U;
    const uint8_t   DT_CSBK = 3U;
    const uint8_t   DT_DATA_HEADER = 6U;
    const uint8_t   DT_RATE_12_DATA = 7U;
    const uint8_t   DT_RATE_34_DATA = 8U;
    const uint8_t   DT_IDLE = 9U;
    const uint8_t   DT_RATE_1_DATA = 10U;
} // namespace dmr

#endif // __DMR_DEFINES_H__
