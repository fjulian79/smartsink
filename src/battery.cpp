

#include "battery.h"

#include <string.h>
#include <Arduino.h>
#include <stm32yyxx_ll_adc.h>

#define CELL_VMIN           250

#define VREFINT             1200

#define DEFAULT_GATETIME    250

Battery::Battery() : 
      GateTime(DEFAULT_GATETIME)
    , LastTick(0)
    , SampleCnt(0)
    , Samples(0)
    , VRefAdc(0)
{
    memset(Raw, 0, sizeof(Raw));
    memset(VCell, 0, sizeof(VCell));
}

Battery::~Battery()
{
    
}

void Battery::setGateTime(uint16_t millis)
{
    GateTime = millis;
}

void Battery::update(uint32_t millis)
{
    SampleCnt++;

    for (size_t i = 0; i < BATTERY_NUMCELLS; i++)
    {
        Raw[i] += analogRead(i);
    }

    if (millis - LastTick >= GateTime)
    {
        LastTick = millis;
        updateCells();
    }
}

uint32_t Battery::getCell(uint8_t cell, bool abs)
{
    uint32_t volt = 0;

    if (cell == 0 || abs)
    {
        volt = VCell[cell];
    }
    else if (cell >= BATTERY_NUMCELLS)
    {
        volt = 0;
    }
    else if (VCell[cell] >= VCell[cell - 1])
    {
        volt = VCell[cell] - VCell[cell - 1];
    }

    return volt;
}

uint16_t Battery::getSamples(void)
{
    return Samples;
}

int8_t Battery::getNumCells(void)
{
    int8_t cells = 0;
    bool done = false;

    for (uint8_t i = 0; i < BATTERY_NUMCELLS; i++)
    {
        if(getCell(i) >= CELL_VMIN)
        {
            if(!done)
            {
                cells++;
            }
            else
            {
                cells = -1;
                break;
            }
        }
        else
        {
            done = true;
        }
    }
    
    return cells;
}

uint32_t Battery::getMinCell(void)
{
    uint32_t volt = 0;
    int8_t cells = getNumCells();

    if(cells > 0)
    {
        volt = UINT32_MAX;
        for (uint8_t i = 0; i < cells; i++)
        {
            uint32_t cell = getCell(i);
            if(cell >= CELL_VMIN)
            {
                volt = min(volt, cell);
            }
        }
    }

    return volt;
}

uint32_t Battery::calibrate(uint8_t cell, uint32_t voltage)
{
    updateVref();

    Raw[cell] = 0;
    SampleCnt = 0;    
    LastTick = millis();

    while (millis() - LastTick <= GateTime)
    {
        Raw[cell] += analogRead(cell);
        SampleCnt++;
    }
    
    voltage <<= CellScaleDen;
    Raw[cell] /= SampleCnt;

    printf("raw: %lu, samples: %lu\n", Raw[cell], SampleCnt);
    Raw[cell] = __LL_ADC_CALC_DATA_TO_VOLTAGE(
                VRefAdc, Raw[cell], LL_ADC_RESOLUTION_12B);
    printf("Scale old: %lu\n", CellScaleNum[cell]); 
    CellScaleNum[cell] = voltage/Raw[cell];
    printf("Scale new: %lu\n", CellScaleNum[cell]); 

    memset(Raw, 0, sizeof(Raw));
    SampleCnt = 0;
    LastTick = millis();

    return 0;
}

void Battery::updateVref(void)
{
    VRefAdc = VREFINT * 4096 / analogRead(AVREF);
}


void Battery::updateCells(void)
{
    updateVref();

    for (size_t i = 0; i < BATTERY_NUMCELLS; i++)
    {
        Raw[i] /= SampleCnt;
        VCell[i] = __LL_ADC_CALC_DATA_TO_VOLTAGE(
                VRefAdc, Raw[i], LL_ADC_RESOLUTION_12B);
        VCell[i] = (VCell[i] * CellScaleNum[i]) >> CellScaleDen; 
        Raw[i] = 0;
    }

    Samples = SampleCnt;
    SampleCnt = 0;
}