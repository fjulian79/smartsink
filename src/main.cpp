/*
 * smartsink, this is the firmware for the smartsink project. It builds a smart
 * tty operated current sink to discharge LiPo batteries and collect data like 
 * cell voltages, current and other things. 
 *
 * Copyright (C) 2019 Julian Friedrich
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>. 
 *
 * You can file issues at https://github.com/fjulian79/smartsink
 */

#include "bsp/bsp.h"
#include "bsp/bsp_gpio.h"
#include "bsp/bsp_tty.h"
#include "bsp/bsp_flash.h"
#include "bsp/bsp_adc.h"

#include "cli/cli.h"
#include "generic/generic.h"

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#define VERSIONSTRING       "rel_1_1_0"

Cli cli;

/**
 * @brief Prints the version inforamtion
 * 
 * @param args      The arguemnt list
 * @return          0
 */
int8_t cmd_ver(char *argv[], uint8_t argc)
{
	unused(argv);
    unused(argc);

	printf("smartsink %s Copyright (C) 2019 Julian Friedrich\n", VERSIONSTRING);
    printf("build:   %s, %s\n", __DATE__, __TIME__);
    printf("\n");
    printf("This program comes with ABSOLUTELY NO WARRANTY. This is free software, and you \n");
    printf("are welcome to redistribute it under certain conditions.\n");
    printf("See GPL v3 licence at https://www.gnu.org/licenses/ for details.\n");

    return 0;
}

/**
 * @brief To print the help text
 * 
 * @param args      The argument list
 * @return          0
 */
int8_t cmd_help(char *argv[], uint8_t argc)
{
	unused(argv);
    unused(argc);

    printf("Supported commands:\n");
    printf("  ver               Used to print version and licence infos.\n");
    printf("  test              Uesd to test the ADC during developement.\n");
    printf("  help              Prints this text.\n");

    return 0;
}

/**
 * @brief Used for ADC Tests
 * 
 * @param args      The argument list
 * @return          0
 */
int8_t cmd_test(char *argv[], uint8_t argc)
{
    uint32_t data[BSP_ADC_LIPOCELLS];
    uint32_t samples = 0;
    uint8_t cells = 0;
	unused(argv);
    unused(argc);

    memset(data, 0, sizeof(data));
    samples = bspAdcGetResult(&data[0]);

    for (cells = BSP_ADC_LIPOCELLS; cells > 0; cells--)
    {
        if(data[cells-1] > 50)
            break;
    }

    printf("Cells: %d, Samples: %lu\n", cells, samples);

    if (cells == 0)
        return 0;

    for (uint8_t cell = BSP_ADC_LIPOCELLS-1 ; cell > 0; cell--)
    {
        if (cell >= cells)
        {
            data[cell] = 0;
        }
        else
        {
            data[cell] -= data[cell-1];
        }
    }

    printf("%lu, %lu, %lu, %lu, %lu, %lu \n", 
        data[0], data[1], data[2], data[3], data[4], data[5]);

    return 0;
}

cliCmd_t cmd_table[] =
{
   {"ver", cmd_ver},
   {"test", cmd_test},
   {"help", cmd_help},
   {0,      0}
};

int main(void)
{
    LL_GPIO_InitTypeDef init;    
    uint32_t sysTick = 0;
    uint32_t ledTick = 0;

    bspChipInit();

    init.Mode = LL_GPIO_MODE_OUTPUT;
    init.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    init.Pull = LL_GPIO_PULL_DOWN;
    init.Speed = LL_GPIO_SPEED_FREQ_MEDIUM;
    bspGpioPinInit(BSP_DEBUGPIN_0, &init);
    bspGpioPinInit(BSP_DEBUGPIN_1, &init);
    bspGpioClear(BSP_DEBUGPIN_0);
    bspGpioClear(BSP_DEBUGPIN_1);

    printf("\n");
    printf("\n");
    cmd_ver(0, 0);
    printf("\n");

    cli.init(cmd_table, arraysize(cmd_table));

    while (1)
    {
        sysTick = bspGetSysTick();

        if (sysTick - ledTick >= 250)
        {
            ledTick = sysTick;
            bspGpioToggle(BSP_GPIO_LED);
        }

        if (bspTTYDataAvailable())
        {
        	cli.procByte((uint8_t) bspTTYGetChar());
        }
    }
}
