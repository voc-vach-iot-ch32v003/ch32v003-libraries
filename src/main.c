#include "ch32fun.h"
#include "ch32v003_delay.h"

static void setup()
{
}

static void loop()
{
}

int main()
{
    SystemInit(); // Khởi tạo hệ thống, thiết lập xung nhịp, SysTick, v.v.
    delayMs(1000); // Luôn luôn chờ để nạp
    setup();
    // ReSharper disable once CppDFAEndlessLoop
    while (1)
    {
        loop();
    }
}
