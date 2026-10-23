/**
 * @file sniff.h
 * @author Vọc Vạch IoT
 * @brief Tiện ích cho firmware test được ESP32 nghe lén SPI (SNIFF), đi cặp với test/runner/spi_sniff.py.
 *
 * Mỗi thao tác được đo theo vòng: "sniff <tag> <case>" -> thao tác -> "verify <tag> <case>"; runner trả "retry"
 * nếu lần bắt không đáng tin, khi đó thao tác được làm lại (thao tác phải cho cùng khung SPI khi lặp lại).
 * SPI phần mềm ở 48MHz có xung SCK ~200ns, ngắn hơn 1 lượt đọc GPIO của ESP32: đặt sniffSlowClock = 1 để hạ
 * clock lõi xuống /8 trong lúc thao tác (tốc độ bit-bang đã có suite spi kiểm tra).
 */

#ifndef SNIFF_H
#define SNIFF_H

#include "hil.h"

static uint8_t sniffSlowClock = 0;
static char sniffText[48];

static __attribute__((unused)) void sniffBegin(const char* tag, const char* testCase)
{
    mini_snprintf(sniffText, sizeof(sniffText), "sniff %s %s", tag, testCase);
    hilWait(sniffText);
    if (sniffSlowClock) RCC->CFGR0 = (RCC->CFGR0 & ~RCC_HPRE) | RCC_HPRE_DIV8;
}

// Trả clock về 48MHz trước khi in qua SWIO, hỏi runner có cần làm lại không (1 = xong)
static __attribute__((unused)) uint8_t sniffVerify(const char* tag, const char* testCase)
{
    RCC->CFGR0 = (RCC->CFGR0 & ~RCC_HPRE) | RCC_HPRE_DIV1;
    mini_snprintf(sniffText, sizeof(sniffText), "verify %s %s", tag, testCase);
    return hilWait(sniffText)[0] == 'o';
}

/** @brief Nghe lén một thao tác, làm lại tới khi runner bắt được trọn vẹn. */
#define SNIFFED(tag, testCase, statement) \
    do                                    \
    {                                     \
        sniffBegin(tag, testCase);        \
        statement;                        \
    } while (!sniffVerify(tag, testCase))

#endif // SNIFF_H
