#ifndef _XMODEM_H
   #define _XMODEM_H

   #include "dfu.h"

   #if defined(ESP8266)
      #include <Updater.h>
   #elif defined(ESP32)
      #include <Update.h>
   #endif

   #define XMODEM_SOH  0x01
   #define XMODEM_EOT  0x04
   #define XMODEM_ACK  0x06
   #define XMODEM_NAK  0x15
   #define XMODEM_CAN  0x18

   #if defined(ESP8266)
      static size_t dfuUpdateMaxSize(void) {
         return (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
      }

      static bool dfuUpdateBegin(void) {
         return Update.begin(dfuUpdateMaxSize());
      }

      static void dfuUpdateCancel(void) {
         Update.end(false);
      }
   #elif defined(ESP32)
      static bool dfuUpdateBegin(void) {
         return Update.begin(UPDATE_SIZE_UNKNOWN);
      }

      static void dfuUpdateCancel(void) {
         Update.abort();
      }
   #endif

   static uint16_t xmodemCrc16(const uint8_t *data, size_t len) {
      uint16_t crc = 0;
      for( size_t i = 0; i < len; ++i ) {
         crc ^= (uint16_t)data[i] << 8;
         for( int b = 0; b < 8; ++b ) {
            if( crc & 0x8000 ) {
               crc = (crc << 1) ^ 0x1021;
            } else {
               crc <<= 1;
            }
         }
      }
      return crc;
   }

   static int xmodemReadByte(uint32_t timeoutMs) {
      uint32_t start = millis();
      while( millis() - start < timeoutMs ) {
         if( Serial.available() ) {
            return Serial.read();
         }
         yield();
      }
      return -1;
   }

   static bool xmodemReceiveBlock(uint8_t expectedBlk, uint8_t *buf, bool useCrc) {
      int hdr = xmodemReadByte(3000);
      if( hdr == XMODEM_EOT ) {
         return false;
      }
      if( hdr != XMODEM_SOH ) {
         return false;
      }
      int blk = xmodemReadByte(1000);
      int inv = xmodemReadByte(1000);
      if( blk < 0 || inv < 0 || (uint8_t)(blk + inv) != 0xFF ) {
         return false;
      }
      for( int i = 0; i < 128; ++i ) {
         int c = xmodemReadByte(1000);
         if( c < 0 ) {
            return false;
         }
         buf[i] = (uint8_t)c;
      }
      if( useCrc ) {
         int crch = xmodemReadByte(1000);
         int crcl = xmodemReadByte(1000);
         if( crch < 0 || crcl < 0 ) {
            return false;
         }
         uint16_t recv = ((uint16_t)crch << 8) | (uint16_t)crcl;
         if( recv != xmodemCrc16(buf, 128) ) {
            return false;
         }
      } else {
         int cks = xmodemReadByte(1000);
         if( cks < 0 ) {
            return false;
         }
         uint8_t sum = 0;
         for( int i = 0; i < 128; ++i ) {
            sum += buf[i];
         }
         if( (uint8_t)cks != sum ) {
            return false;
         }
      }
      return (uint8_t)blk == expectedBlk;
   }

   void dfuXmodemLoop(void) {
      static bool started = false;
      static uint8_t blockNum = 1;
      static uint8_t buf[128];
      static bool useCrc = true;
      static size_t totalWritten = 0;

      if( dfuState != DFU_XMODEM_WAIT ) {
         started = false;
         return;
      }

      if( !started ) {
         started = true;
         blockNum = 1;
         totalWritten = 0;
         useCrc = true;
         if( !dfuUpdateBegin() ) {
            dfuState = DFU_ERROR;
            strncpy(dfuLastError, "update begin failed", sizeof dfuLastError);
            Serial.write(XMODEM_CAN);
            sendResult(R_ERROR);
            started = false;
            return;
         }
         Serial.write('C');
      }

      int hdr = Serial.peek();
      if( hdr < 0 ) {
         return;
      }

      if( hdr == XMODEM_EOT ) {
         Serial.read();
         Serial.write(XMODEM_ACK);
         dfuState = DFU_VERIFYING;
         dfuReportProgress("VERIFYING", 100);
         if( Update.end(true) ) {
            dfuState = DFU_FLASHING;
            dfuReportProgress("REBOOTING", 100);
            Serial.flush();
            digitalWrite(TXEN, HIGH);
            ESP.restart();
         } else {
            dfuState = DFU_ERROR;
            strncpy(dfuLastError, "update end failed", sizeof dfuLastError);
            sendResult(R_ERROR);
         }
         started = false;
         return;
      }

      if( xmodemReceiveBlock(blockNum, buf, useCrc) ) {
         if( Update.write(buf, 128) != 128 ) {
            Serial.write(XMODEM_CAN);
            dfuUpdateCancel();
            dfuState = DFU_ERROR;
            strncpy(dfuLastError, "flash write failed", sizeof dfuLastError);
            sendResult(R_ERROR);
            started = false;
            return;
         }
         totalWritten += 128;
         if( totalWritten % 1280 == 0 ) {
            unsigned pct = (unsigned)((totalWritten % (1024UL * 1024UL)) * 100UL / (1024UL * 1024UL));
            if( pct > 100 ) {
               pct = 99;
            }
            dfuReportProgress("DOWNLOADING", pct);
         }
         Serial.write(XMODEM_ACK);
         ++blockNum;
      } else if( hdr == XMODEM_SOH ) {
         Serial.write(XMODEM_NAK);
      } else if( !useCrc ) {
         Serial.write(XMODEM_NAK);
      } else {
         useCrc = false;
         Serial.write(XMODEM_NAK);
      }
   }

#endif
