#include "ch32fun.h"
#include "ch32v003_delay.h"
#include "ch32v003_timer.h"
#include "ch32v003_debug.h"
#include "ch32v003_gpio.h"
#include "ch32v003_intmath.h"
#include "ch32v003_i2c.h"
#include "ch32v003_spi.h"
#include "ch32v003_max7219.h"
#include "ch32v003_led_matrix.h"
#include "ch32v003_bmi160.h"
#include "ch32v003_ir.h"
#include "ch32v003_power.h"
#include "ch32v003_ws2812.h"

int main(void)
{
    SystemInit();
    delayMs(1000);
    while (1)
    {
    }
}
