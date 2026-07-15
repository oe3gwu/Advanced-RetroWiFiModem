#ifndef _DFU_H
   #define _DFU_H

   #include "globals.h"
   #include "Advanced-RetroWiFiModem.h"

   enum DfuState { DFU_IDLE, DFU_DOWNLOADING, DFU_VERIFYING, DFU_FLASHING, DFU_XMODEM_WAIT, DFU_ERROR };
   extern DfuState dfuState;
   extern char dfuLastError[64];

   bool dfuInProgress(void);
   bool dfuCanStart(void);
   void dfuReportProgress(const char *stage, unsigned pct);
   bool dfuHttpUpdate(const char *url);
   bool dfuStartXmodem(void);
   void dfuXmodemLoop(void);
   char *doDfu(char *atCmd);

   #if defined(ESP8266)
      #include <ESP8266WiFi.h>
      #include <ESP8266httpUpdate.h>
      #include <Updater.h>
   #elif defined(ESP32)
      #include <WiFi.h>
      #include <HTTPUpdate.h>
      #include <Update.h>
   #endif

   DfuState dfuState = DFU_IDLE;
   char dfuLastError[64] = "";

   bool dfuInProgress(void) {
      return dfuState != DFU_IDLE && dfuState != DFU_ERROR;
   }

   bool dfuCanStart(void) {
      if( state == ONLINE || state == PASSWORD || state == ONLINE_PPP ) {
         return false;
      }
      if( tcpClient.connected() ) {
         return false;
      }
      if( WiFi.status() != WL_CONNECTED && dfuState != DFU_XMODEM_WAIT ) {
         return false;
      }
      return true;
   }

   void dfuReportProgress(const char *stage, unsigned pct) {
      if( pct <= 100 ) {
         Serial.printf("%s %u%%\r\n", stage, pct);
      } else {
         Serial.println(stage);
      }
   }

   static void dfuProgressCb(int current, int total) {
      if( total > 0 ) {
         unsigned pct = (unsigned)((current * 100L) / total);
         static unsigned lastPct = 999;
         if( pct != lastPct && (pct % 5 == 0 || pct == 100) ) {
            lastPct = pct;
            dfuReportProgress("DOWNLOADING", pct);
         }
      }
   }

   bool dfuHttpUpdate(const char *url) {
      if( !dfuCanStart() ) {
         strncpy(dfuLastError, "busy or offline", sizeof dfuLastError);
         dfuState = DFU_ERROR;
         return false;
      }

      dfuState = DFU_DOWNLOADING;
      dfuLastError[0] = NUL;
      digitalWrite(DSR, !ACTIVE);
      Serial.println(F("WARNING: EXPERIMENTAL DFU - wrong image may brick device"));

      WiFiClient client;
      dfuReportProgress("DOWNLOADING", 0);

      #if defined(ESP8266)
         ESPhttpUpdate.rebootOnUpdate(true);
         ESPhttpUpdate.onProgress(dfuProgressCb);
         ESPhttpUpdate.onEnd([]() {
            dfuState = DFU_FLASHING;
            dfuReportProgress("REBOOTING", 100);
         });
         t_httpUpdate_return ret = ESPhttpUpdate.update(client, url);
         ESPhttpUpdate.onProgress(nullptr);
         switch( ret ) {
            case HTTP_UPDATE_OK:
               return true;
            case HTTP_UPDATE_NO_UPDATES:
               strncpy(dfuLastError, "no updates", sizeof dfuLastError);
               break;
            case HTTP_UPDATE_FAILED:
               snprintf(dfuLastError, sizeof dfuLastError, "failed %d",
                  ESPhttpUpdate.getLastError());
               break;
            default:
               strncpy(dfuLastError, "unknown error", sizeof dfuLastError);
               break;
         }
      #elif defined(ESP32)
         httpUpdate.rebootOnUpdate(true);
         httpUpdate.onProgress(dfuProgressCb);
         httpUpdate.onEnd([]() {
            dfuState = DFU_FLASHING;
            dfuReportProgress("REBOOTING", 100);
         });
         t_httpUpdate_return ret = httpUpdate.update(client, url);
         httpUpdate.onProgress(nullptr);
         switch( ret ) {
            case HTTP_UPDATE_OK:
               return true;
            case HTTP_UPDATE_NO_UPDATES:
               strncpy(dfuLastError, "no updates", sizeof dfuLastError);
               break;
            case HTTP_UPDATE_FAILED:
               snprintf(dfuLastError, sizeof dfuLastError, "failed %d",
                  httpUpdate.getLastError());
               break;
            default:
               strncpy(dfuLastError, "unknown error", sizeof dfuLastError);
               break;
         }
      #endif

      dfuState = DFU_ERROR;
      digitalWrite(DSR, ACTIVE);
      return false;
   }

   bool dfuStartXmodem(void) {
      if( !dfuCanStart() ) {
         return false;
      }
      dfuState = DFU_XMODEM_WAIT;
      dfuLastError[0] = NUL;
      Serial.println(F("WARNING: EXPERIMENTAL DFU - wrong image may brick device"));
      Serial.println(F("XMODEM READY"));
      return true;
   }

   char *doDfu(char *atCmd) {
      switch( atCmd[0] ) {
         case '?':
            ++atCmd;
            switch( dfuState ) {
               case DFU_IDLE:
                  Serial.println(F("idle"));
                  break;
               case DFU_DOWNLOADING:
                  Serial.println(F("downloading"));
                  break;
               case DFU_VERIFYING:
                  Serial.println(F("verifying"));
                  break;
               case DFU_FLASHING:
                  Serial.println(F("flashing"));
                  break;
               case DFU_XMODEM_WAIT:
                  Serial.println(F("xmodem"));
                  break;
               case DFU_ERROR:
                  Serial.print(F("error "));
                  Serial.println(dfuLastError);
                  break;
            }
            if( !atCmd[0] ) {
               sendResult(R_OK);
            }
            break;

         case '=':
            ++atCmd;
            if( !strncasecmp(atCmd, "xmodem", 6) ) {
               atCmd += 6;
               if( dfuStartXmodem() ) {
                  if( !atCmd[0] ) {
                     sendResult(R_OK);
                  }
               } else {
                  sendResult(R_ERROR);
               }
            } else if( !strncasecmp(atCmd, "http://", 7) ) {
               if( dfuHttpUpdate(atCmd) ) {
                  atCmd[0] = NUL;
               } else {
                  sendResult(R_ERROR);
               }
            } else {
               sendResult(R_ERROR);
            }
            break;

         default:
            sendResult(R_ERROR);
            break;
      }
      return atCmd;
   }

#endif
