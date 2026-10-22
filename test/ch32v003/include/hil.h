/**
 * @file hil.h
 * @author Vọc Vạch IoT
 * @brief Tiện ích báo kết quả cho firmware kiểm thử HIL, trao đổi với test/runner/hil.py qua SWIO (printf).
 *
 * Giao thức (mỗi dòng in ra là một sự kiện):
 *   HIL:BEGIN <suite> <chip>       bắt đầu bộ test
 *   HIL:WAIT <bước> [tham số...]   dừng chờ: runner thực hiện bước (điều khiển/đo bằng ESP32) rồi gửi 1 dòng trả lời
 *   HIL:CHECK <tên> PASS|FAIL [chi tiết]
 *   HIL:END pass=<n> fail=<m>
 */

#ifndef HIL_H
#define HIL_H

#include "ch32fun.h"
#include "ch32v003_debug.h"

#if defined(CH32V003_J4M6)
#define HIL_CHIP "J4M6"
#elif defined(CH32V003_A4M6)
#define HIL_CHIP "A4M6"
#else
#define HIL_CHIP "F4P6"
#endif

static uint16_t hilPassCount = 0;
static uint16_t hilFailCount = 0;

// Bộ đệm nhận 1 dòng trả lời từ runner (đi qua minichlink -T -> DMDATA)
static volatile uint8_t hilReplyLength = 0;
static volatile uint8_t hilReplyDone = 0;
static char hilReply[32];

/**
 * @brief Callback của ch32fun khi máy tính gửi dữ liệu xuống qua SWIO (ghi đè hàm weak).
 */
void handle_debug_input(int numbytes, uint8_t* data)
{
    for (int i = 0; i < numbytes; i++)
    {
        const char c = (char)data[i];
        if (c == '\n' || c == '\r')
        {
            hilReplyDone = 1;
        }
        else if (hilReplyLength < sizeof(hilReply) - 1)
        {
            hilReply[hilReplyLength++] = c;
        }
    }
}

/**
 * @brief In dòng bắt đầu bộ test.
 */
static __attribute__((unused)) inline void hilBegin(const char* suite)
{
    printf("HIL:BEGIN %s " HIL_CHIP "\n", suite);
}

/**
 * @brief Dừng chờ runner thực hiện một bước và trả lời.
 * @param step Dòng mô tả bước (VD: "expect PA1 1"), runner dựa vào đây để biết phải làm gì.
 * @return Chuỗi trả lời của runner (đã kết thúc bằng '\0', có thể rỗng).
 */
static __attribute__((unused)) const char* hilWait(const char* step)
{
    hilReplyLength = 0;
    hilReplyDone = 0;
    printf("HIL:WAIT %s\n", step);
    while (!hilReplyDone)
    {
        poll_input();
    }
    hilReply[hilReplyLength] = '\0';
    return hilReply;
}

/**
 * @brief Ghi nhận một phép kiểm tra tự đánh giá trên chip.
 */
static __attribute__((unused)) void hilCheck(const char* name, const int passed, const char* detail)
{
    if (passed) hilPassCount++;
    else hilFailCount++;
    printf("HIL:CHECK %s %s %s\n", name, passed ? "PASS" : "FAIL", detail ? detail : "");
}

/**
 * @brief Kết thúc bộ test, in tổng kết rồi đứng yên.
 */
static __attribute__((unused)) void hilEnd(void)
{
    printf("HIL:END pass=%u fail=%u\n", hilPassCount, hilFailCount);
    while (1)
    {
    }
}

#endif // HIL_H
