
#include <stdint.h>
#include <string.h>

#include <Arduino.h>
#include <EEPROM.h>
#include <generic/crc8.hpp>

template <typename T> class Param
{
    public:

        Param(uint8_t _addr = 0) :
          addr(_addr)
        {
            clear();
        }

        void clear(void)
        {
            memset(&data, 0, sizeof(T));
        }

        bool read(void)
        {
            crc8 crc;

            EEPROM.get(addr, data);
            
            if (crc.calc(&data, sizeof(data)) ==
                EEPROM.read(addr + sizeof(data)))
            {
                return true;
            }
            else
            {
                return false;
            }
        }

        void save(void)
        {
            crc8 crc;

            EEPROM.put(addr, data);
            EEPROM.write(addr + sizeof(data), crc.calc(&data, sizeof(data)));
        }

        void discard(void)
        {
            EEPROM.write(addr + sizeof(data),
                    ~EEPROM.read(addr + sizeof(data)));
        }

        size_t size(void)
        {
            return sizeof(T) + sizeof(uint8_t);
        }

        T data;

    private:

        uint8_t addr;
};
