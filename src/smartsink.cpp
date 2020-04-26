#include <Arduino.h>
#include <stm32yyxx_ll_adc.h>

#include <cli/cli.h>
#include <generic/generic.hpp>

#include "battery.h"

/**
 * @brief The version string of the application.
 * 
 */
#define VERSIONSTRING      "rel_2_0_0"

#define CALX_TEMP 25
#define V25       1430
#define AVG_SLOPE 4300
#define VREFINT   1200

Battery batt;
Cli cli;

uint32_t ledTick = 0;
uint32_t adcTick = 0;
uint32_t sampleCnt = 0;
uint32_t samples = 0;
bool absmode = false;

/**
 * @brief Used to print the version informaton.
 * 
 * This function has to match the p_cmd_func definition in libcli, see cliCmd_t.
 * 
 * @param argv      not used.
 * @param argc      not used.
 * @return int8_t   Zero.
 */
int8_t cmd_ver(char *argv[], uint8_t argc)
{
    Serial.printf("\nsmartsink %s Copyright (C) 2020 Julian Friedrich\n", VERSIONSTRING);
    Serial.printf("build: %s, %s\n", __DATE__, __TIME__);
    Serial.printf("\n");
    Serial.printf("This program comes with ABSOLUTELY NO WARRANTY. This is free software, and you\n");
    Serial.printf("are welcome to redistribute it under certain conditions.\n");
    Serial.printf("See GPL v3 licence at https://www.gnu.org/licenses/ for details.\n\n");
        
    return 0;
}

int8_t cmd_read(char *argv[], uint8_t argc)
{
    uint32_t val = 0;

    if(argc == 0)
    {
        printf("No argument.\n");
        return 0;    
    }
    else if (argc == 1) 
    {
        absmode = false;
        val = strtol(argv[0], 0 ,0);
    }
    else if (argc == 2)
    {
        if(strcmp(argv[0], "abs") == 0)
        {
            absmode = true;
        }

        val = strtol(argv[1], 0 ,0);
    }
    
    sampleCnt = 0;
    samples = val;

    return 0;
}

int8_t cmd_cal(char *argv[], uint8_t argc)
{
    uint8_t cell = 0;
    uint32_t voltage = 0;

    if(argc != 2)
    {
        return -1;    
    }
    
    cell = strtol(argv[0], 0 ,0);
    voltage = strtol(argv[1], 0 ,0);
    batt.calibrate(cell, voltage);

    return 0;
}

int8_t cmd_help(char *argv[], uint8_t argc)
{
    Serial.printf("Supported commands:\n");
    Serial.printf("  ver           Used to print version infos.\n");
    Serial.printf("  cal cell mV   Used to calibrate the given cell.\n");
    Serial.printf("  read [abs] n  Used to read the cell voltage n times.\n");
    Serial.printf("                Provides relative cell voltages per default.\n"); 
    Serial.printf("                When using [abs] it will provide absolute voltages.\n");
    Serial.printf("  help          Prints this text.\n");

    return 0;
}

int32_t readTempSensor(int32_t VRef)
{
    return __LL_ADC_CALC_TEMPERATURE_TYP_PARAMS(
            AVG_SLOPE, V25, CALX_TEMP, VRef, analogRead(ATEMP), 
            LL_ADC_RESOLUTION_12B);
}

cliCmd_t cmdTable[] =
{
    {"ver", cmd_ver},
    {"read", cmd_read},
    {"cal", cmd_cal},
    {"help", cmd_help}
};

void setup()
{
    pinMode(LED_BUILTIN, OUTPUT);
    analogReadResolution(12);
 
    batt.setGateTime(500);
 
    Serial.begin(115200);
    while (!Serial);

    Serial.println();   
    cmd_ver(0, 0);
    cli.begin(cmdTable);
}

void loop()
{
    uint32_t tick = millis();

    batt.update(tick);

    if (tick - ledTick >= 250)
    {
        ledTick = tick;
        digitalToggle(LED_BUILTIN);
    }

    if (tick - adcTick >= 250 && sampleCnt < samples)
    {
        adcTick = tick;
  
        //Serial.printf("Temp: %lu\n", readTempSensor(vref));
        Serial.printf("%lu; ", batt.getSamples());
  
        for (uint8_t i = 0; i < 6; i++)
        {
            Serial.printf("%lu; ", batt.getCell(i, absmode));
        }      
  
        Serial.printf("%d; %lu\n", batt.getNumCells(), batt.getMinCell());
        sampleCnt++;
    }

    cli.read();
}