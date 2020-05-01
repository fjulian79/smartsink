#include <Arduino.h>
#include <stm32yyxx_ll_adc.h>

#include <cli/cli.h>
#include <generic/generic.hpp>

#include "battery.h"
#include "param.h"

/**
 * @brief The version string of the application.
 * 
 */
#define VERSIONSTRING      "rel_2_0_0"

#define CALX_TEMP 25
#define V25       1430
#define AVG_SLOPE 4300
#define VREFINT   1200

typedef struct
{
    BatteryParams_t Battery;

}Parameter_t;

Param<Parameter_t> param;
Battery batt(&param.data.Battery);
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
        Serial.printf("No argument.\n");
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

int8_t cmd_param(char *argv[], uint8_t argc)
{
    if(argc == 0)
    {
        return -1;
    }

    if(strcmp(argv[0],"clear") == 0)
    {
        param.clear();
        Serial.printf("Parameter clear in RAM.\n");
    }

    if(strcmp(argv[0],"save") == 0)
    {
        param.save();
        Serial.printf("Parameter saved.\n");
    }

    if(strcmp(argv[0],"discard") == 0)
    {
        param.discard();
        Serial.printf("Parameter discarded.\n");
    }

    return 0;
}

int8_t cmd_temp(char *argv[], uint8_t argc)
{
    uint32_t temp = __LL_ADC_CALC_TEMPERATURE_TYP_PARAMS(
            AVG_SLOPE, V25, CALX_TEMP, batt.getVref(), analogRead(ATEMP), 
            LL_ADC_RESOLUTION_12B);

    Serial.printf("CPU: %lu°C\n", temp);

    return 0; 
}

int8_t cmd_echo(char *argv[], uint8_t argc)
{
    bool state = true;

    if(argc != 1)
    {
        return -1;
    }

    if(strcmp(argv[0],"on") == 0)
    {
        state = true;
    }
    else if(strcmp(argv[0],"off") == 0)
    {
        state = false;
    }
    else
    {
        return -2;
    }

    cli.setEcho(state);
    Serial.printf("Echo is now %s\n", argv[0]);
    return 0;
}

int8_t cmd_reset(char *argv[], uint8_t argc)
{
    Serial.printf("Resetting the CPU...\n");
    delay(100);

    NVIC_SystemReset();

    return 0;
}

int8_t cmd_help(char *argv[], uint8_t argc)
{
    Serial.printf("Supported commands:\n");
    Serial.printf("  ver           Prints version infos.\n");
    Serial.printf("  cal cell mV   Calibrate the given cell.\n");
    Serial.printf("  read [abs] n  Read the cell voltage n times.\n");
    Serial.printf("                Provides relative cell voltages per default.\n"); 
    Serial.printf("                When using [abs] it will provide absolute voltages.\n");
    Serial.printf("  param         TBD...\n");
    Serial.printf("  temp          Reads the temperature.\n");
    Serial.printf("  echo on|off   Turns echo on or off.\n");
    Serial.printf("  reset         Resets the CPU.\n");
    Serial.printf("  help          Prints this text.\n");

    return 0;
}

cliCmd_t cmdTable[] =
{
    {"ver", cmd_ver},
    {"read", cmd_read},
    {"cal", cmd_cal},
    {"param", cmd_param},
    {"temp", cmd_temp},
    {"echo", cmd_echo},
    {"reset", cmd_reset},
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

    if(param.read() != true)
    {
        param.data.Battery = {3128, 5609, 8771, 11460, 13718, 16984};
        param.save();
        Serial.printf("Parameter reset.\n");
    }
    else
    {
        Serial.printf("Parameter loaded.\n");
    }

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
  
        if(sampleCnt == 0)
        {
            cli.clearLine();
            cli.setEcho(false);
        }

        Serial.printf("%lu; ", batt.getSamples());
  
        for (uint8_t i = 0; i < BATTERY_NUMCELLS; i++)
        {
            Serial.printf("%lu; ", batt.getCell(i, absmode));
        }      
  
        Serial.printf("%d; %lu\n", batt.getNumCells(), batt.getMinCell());
        sampleCnt++;

        if(sampleCnt == samples)
        {
            Serial.println();
            cli.setEcho(true);
            cli.refreshPrompt();
        }
    }

    cli.read();
}