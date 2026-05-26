#include "ch32fun.h"

void setup()
{
}

void loop()
{
}

int main()
{
    SystemInit(); // Khởi tạo hệ thống, thiết lập xung nhịp, SysTick, v.v.
    setup();
    // ReSharper disable once CppDFAEndlessLoop
    while (1)
    {
        loop();
    }
}
