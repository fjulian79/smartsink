
#include <stdint.h>

#define BATTERY_NUMCELLS            6

#define BATTERY_DEFAULT_PARAM       {3106, 5598, 8704, 11332, 13653, 16839}

typedef struct
{
    uint32_t CellScale[BATTERY_NUMCELLS];

} BatteryParams_t;

class Battery
{
    public:

        Battery(BatteryParams_t *_pParam);

        ~Battery();

        void setGateTime(uint16_t millis);

        bool update(uint32_t millis);

        uint32_t getCell(uint8_t cell, bool abs = false);

        int8_t getNumCells(void);

        uint32_t getMinCell(void);

        uint16_t getSamples(void);

        uint32_t getVref(void);

        uint32_t calibrate(uint8_t cell, uint32_t voltage);

    private:

        void updateVref(void);

        void updateCells(void);

        BatteryParams_t *const pParam;

        const uint8_t CellScaleDen;

        uint16_t GateTime;

        uint32_t LastTick;

        uint32_t SampleCnt;

        uint32_t Samples;

        uint32_t VRefAdc;

        uint32_t Raw[BATTERY_NUMCELLS];

        uint32_t VCell[BATTERY_NUMCELLS];
};
