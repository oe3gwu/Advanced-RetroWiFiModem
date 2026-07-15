#ifndef MODEM_EEPROM_STORAGE_H
#define MODEM_EEPROM_STORAGE_H

// ESP_EEPROM creates its global EEPROM object before esp8266 flashinit()
// runs. On core 3.x with FLASH_MAP_SUPPORT, EEPROM_start depends on the
// detected flash size — the default library instance can therefore use the
// wrong sector (commit OK, data gone after reboot). Initialise on first use
// from setup() instead.

#define NO_GLOBAL_EEPROM
#include <ESP_EEPROM.h>

extern "C" {
#include "spi_flash.h"
   extern uint32_t _EEPROM_start;
}

#if defined(ESP8266)
#include <flash_hal.h>
#endif

inline uint32_t modemEepromBaseAddress() {
#if defined(EEPROM_start)
   return EEPROM_start;
#else
   return (uint32_t)&_EEPROM_start;
#endif
}

inline uint32_t modemEepromSector() {
   return (modemEepromBaseAddress() - 0x40200000) / SPI_FLASH_SEC_SIZE;
}

inline EEPROMClass &modemEeprom() {
   static EEPROMClass eeprom(modemEepromSector());
   return eeprom;
}

#define EEPROM modemEeprom()

#endif
