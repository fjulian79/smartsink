
#include <stdint.h>

#define BATTERY_NUMCELLS     6

class Battery
{
   public:

      Battery();

      ~Battery();

      void setGateTime(uint16_t millis);

      void update(uint32_t millis);

      uint32_t getCell(uint8_t cell, bool abs = false);

      uint16_t getSamples(void);

      int8_t getNumCells(void);

      uint32_t getMinCell(void);

      uint32_t calibrate(uint8_t cell, uint32_t voltage);
      
      uint32_t CellScaleNum[BATTERY_NUMCELLS] = {3106, 5598, 8704, 11332, 13653, 16839};

      const uint8_t CellScaleDen = 11;

   private:

      void updateVref(void);

      void updateCells(void);

      uint16_t GateTime;

      uint32_t LastTick;

      uint32_t SampleCnt;

      uint32_t Samples;

      uint32_t VRefAdc;

      uint32_t Raw[BATTERY_NUMCELLS];
      
      uint32_t VCell[BATTERY_NUMCELLS];
};
