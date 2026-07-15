#ifndef _RAW_MODE_H
   #define _RAW_MODE_H

   #include "globals.h"
   #include "Advanced-RetroWiFiModem.h"

   void getHostAndPort(char *number, char* &host, char* &port, int &portNum);
   void inAtCommandMode();
   void inPasswordMode();
   void checkForIncomingCall(bool silentResults);
   void printBootBanner();

   static bool rawMaintActive = false;
   static uint32_t rawDtrInactiveSince = 0;
   static uint32_t rawMaintExpires = 0;

   inline bool isRawMode() {
      return settings.operationMode == MODE_RAW;
   }

   bool rawMaintenanceActive() {
      return rawMaintActive;
   }

   void validateOperationMode() {
      if( settings.operationMode > MODE_RAW ) {
         settings.operationMode = MODE_AT;
      }
   }

   uint8_t nvramOperationMode(void) {
      uint8_t mode;
      EEPROM.get(offsetof(struct Settings, operationMode), mode);
      if( mode > MODE_RAW ) {
         mode = MODE_AT;
      }
      return mode;
   }

   bool persistOperationMode(uint8_t mode) {
      EEPROM.put(offsetof(struct Settings, operationMode), mode);
      return EEPROM.commit();
   }

   inline bool dtrIsActive() {
      return digitalRead(DTR) == ACTIVE;
   }

   void rawDisconnect() {
      tcpClient.stop();
      state = CMD_NOT_IN_CALL;
      connectTime = 0;
      digitalWrite(DCD, !ACTIVE);
   }

   bool rawConnectFromSlot0() {
      char tempNumber[MAX_SPEED_DIAL_LEN + 1];
      char *host, *port;
      int portNum;

      if( !settings.speedDial[0][0] ) {
         return false;
      }
      strncpy(tempNumber, settings.speedDial[0], MAX_SPEED_DIAL_LEN);
      tempNumber[MAX_SPEED_DIAL_LEN] = NUL;
      getHostAndPort(tempNumber, host, port, portNum);
      if( host[0] == '-' || host[0] == '=' || host[0] == '+' ) {
         ++host;
      }
      if( !tcpClient.connect(host, portNum) ) {
         return false;
      }
      tcpClient.setNoDelay(true);
      sessionTelnetType = NO_TELNET;
      connectTime = millis();
      dtrWentInactive = false;
      amClient = true;
      state = ONLINE;
      digitalWrite(DCD, ACTIVE);
      return true;
   }

   void rawEnterMaintenance() {
      rawMaintActive = true;
      rawMaintExpires = millis() + RAW_MAINT_TIMEOUT_MS;
      rawDtrInactiveSince = 0;
      atCmd[0] = NUL;
      atCmdLen = 0;
      Serial.println(F("RAW MAINT (experimental): Hayes commands accepted for 120 s."));
      Serial.println(F("Switch to AT mode: AT$MODE=AT  (saved automatically)"));
      Serial.println(F("Stay in RAW:       wait for timeout, then ATZ recommended after AT$MODE=RAW"));
   }

   void rawLeaveMaintenance() {
      rawMaintActive = false;
      rawMaintExpires = 0;
      rawDtrInactiveSince = 0;
      atCmd[0] = NUL;
      atCmdLen = 0;
   }

   void rawPassThrough() {
      size_t len = Serial.available();
      if( len ) {
         if( len > TX_BUF_SIZE ) {
            len = TX_BUF_SIZE;
         }
         Serial.readBytes(txBuf, len);
         bytesOut += tcpClient.write(txBuf, len);
      }
      while( tcpClient.available() && !Serial.available() ) {
         int c = tcpClient.read();
         if( c >= 0 ) {
            ++bytesIn;
            Serial.write((char)c);
         }
      }
   }

   char *doOperationMode(char *atCmd) {
      switch( atCmd[0] ) {
         case '?':
            ++atCmd;
            if( nvramOperationMode() == MODE_RAW ) {
               Serial.println(F("RAW (experimental)"));
            } else {
               Serial.println(F("AT"));
            }
            if( !atCmd[0] ) {
               sendResult(RC_OK);
            }
            break;
         case '=':
            ++atCmd;
            if( !strncasecmp(atCmd, "AT", 2) ) {
               atCmd += 2;
               settings.operationMode = MODE_AT;
               if( persistOperationMode(MODE_AT) ) {
                  rawLeaveMaintenance();
                  sendResult(RC_OK);
                  Serial.println(F("Operating mode AT — ATZ recommended"));
               } else {
                  sendResult(RC_ERROR);
               }
            } else if( !strncasecmp(atCmd, "RAW", 3) ) {
               atCmd += 3;
               if( persistOperationMode(MODE_RAW) ) {
                  sendResult(RC_OK);
                  Serial.println(F("WARNING: RAW mode is experimental — use at your own risk"));
                  Serial.println(F("ATZ required to enter RAW mode"));
               } else {
                  sendResult(RC_ERROR);
               }
            } else {
               sendResult(RC_ERROR);
            }
            break;
         default:
            sendResult(RC_ERROR);
            break;
      }
      return atCmd;
   }

   void rawModeLoop() {
      if( rawMaintActive ) {
         if( millis() > rawMaintExpires ) {
            rawLeaveMaintenance();
            return;
         }
         if( state == PASSWORD ) {
            inPasswordMode();
         } else {
            inAtCommandMode();
         }
         if( settings.operationMode == MODE_AT ) {
            rawLeaveMaintenance();
         }
         return;
      }

      checkForIncomingCall(true);

      if( state == PASSWORD ) {
         inPasswordMode();
         return;
      }

      if( state == ONLINE ) {
         if( !dtrIsActive() ) {
            rawDisconnect();
            rawDtrInactiveSince = millis();
            return;
         }
         if( !tcpClient.connected() ) {
            rawDisconnect();
            rawDtrInactiveSince = dtrIsActive() ? 0 : millis();
            return;
         }
         rawPassThrough();
         return;
      }

      if( dtrIsActive() ) {
         rawDtrInactiveSince = 0;
         if( settings.speedDial[0][0] && state == CMD_NOT_IN_CALL && !ringing ) {
            rawConnectFromSlot0();
         }
         return;
      }

      if( ringing ) {
         rawDtrInactiveSince = 0;
         return;
      }

      if( state != CMD_NOT_IN_CALL ) {
         return;
      }

      if( !rawDtrInactiveSince ) {
         rawDtrInactiveSince = millis();
      } else if( millis() - rawDtrInactiveSince >= RAW_DTR_MAINT_MS ) {
         rawEnterMaintenance();
      }
   }

#endif
