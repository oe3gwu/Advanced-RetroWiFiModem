#ifndef _PPP_H
   #define _PPP_H

   #include "globals.h"
   #include "Advanced-RetroWiFiModem.h"

   #if defined(ESP8266)
      #include <ESP8266WiFi.h>
   #elif defined(ESP32)
      #include <WiFi.h>
   #endif

   bool dfuInProgress(void);

   #define PPP_OUR_IP1   192
   #define PPP_OUR_IP2   168
   #define PPP_OUR_IP3   240
   #define PPP_OUR_IP4   1
   #define PPP_PEER_IP4  2

   #if defined(ESP32) && __has_include("netif/ppp/pppos.h")

      #include "netif/ppp/pppos.h"
      #include "netif/ppp/pppapi.h"
      #include "netif/ppp/ppp.h"
      #include "lwip/netif.h"
      #include "lwip/lwip_napt.h"
      #include "esp_netif.h"

      static struct netif pppNetif;
      static ppp_pcb *pppPcb = nullptr;
      static bool pppLinkUp = false;
      static uint32_t pppLastTimerMs = 0;

      static u32_t pppSerialOutput(ppp_pcb *pcb, u8_t *data, u32_t len, void *ctx) {
         (void)pcb;
         (void)ctx;
         return (u32_t)Serial.write(data, len);
      }

      static void pppStatusHandler(ppp_pcb *pcb, int err_code, void *ctx) {
         (void)pcb;
         (void)ctx;
         switch( err_code ) {
            case PPPERR_NONE:
               pppLinkUp = true;
               {
                  esp_netif_t *sta = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
                  if( sta ) {
                     esp_netif_ip_info_t ip;
                     if( esp_netif_get_ip_info(sta, &ip) == ESP_OK ) {
                        ip_napt_enable(ip.ip.addr, 1);
                     }
                  }
               }
               break;
            case PPPERR_USER:
            case PPPERR_CONNECT:
            case PPPERR_AUTHFAIL:
            case PPPERR_PROTOCOL:
               pppLinkUp = false;
               break;
            default:
               break;
         }
      }

      static void pppServiceTimers(void) {
         if( millis() - pppLastTimerMs >= 50 ) {
            pppLastTimerMs = millis();
            sys_check_timeouts();
         }
      }

      bool pppIsActive(void) {
         return pppPcb != nullptr && pppLinkUp;
      }

      bool pppStart(void) {
         if( WiFi.status() != WL_CONNECTED ) {
            return false;
         }
         if( pppPcb ) {
            return true;
         }

         pppPcb = pppapi_pppos_create(&pppNetif, pppSerialOutput, pppStatusHandler, nullptr);
         if( !pppPcb ) {
            return false;
         }

         ppp_set_auth(pppPcb, PPPAUTHTYPE_PAP | PPPAUTHTYPE_CHAP, "", "");

         ip4_addr_t ourAddr;
         ip4_addr_t peerAddr;
         IP4_ADDR(&ourAddr, PPP_OUR_IP1, PPP_OUR_IP2, PPP_OUR_IP3, PPP_OUR_IP4);
         IP4_ADDR(&peerAddr, PPP_OUR_IP1, PPP_OUR_IP2, PPP_OUR_IP3, PPP_PEER_IP4);
         ppp_set_ipcp_ouraddr(pppPcb, &ourAddr);
         ppp_set_ipcp_hisaddr(pppPcb, &peerAddr);

         if( pppapi_listen(pppPcb) != ERR_OK ) {
            pppapi_free(pppPcb);
            pppPcb = nullptr;
            return false;
         }
         pppLinkUp = false;
         return true;
      }

      void pppStop(void) {
         if( pppPcb ) {
            pppapi_close(pppPcb, 0);
            pppapi_free(pppPcb);
            pppPcb = nullptr;
         }
         pppLinkUp = false;
         esp_netif_t *sta = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
         if( sta ) {
            esp_netif_ip_info_t ip;
            if( esp_netif_get_ip_info(sta, &ip) == ESP_OK ) {
               ip_napt_enable(ip.ip.addr, 0);
            }
         }
      }

      void pppLoop(void) {
         if( !pppPcb || state != ONLINE_PPP ) {
            return;
         }
         pppServiceTimers();
         while( Serial.available() ) {
            uint8_t c = (uint8_t)Serial.read();
            pppos_input(pppPcb, &c, 1);
         }
      }

      char *dialPpp(char *atCmd) {
         if( dfuInProgress() ) {
            sendResult(R_ERROR);
            return atCmd;
         }
         if( strncasecmp(atCmd, "*99", 3) ) {
            sendResult(R_ERROR);
            return atCmd;
         }
         if( settings.serialSpeed > 57600 ) {
            if( !settings.quiet && settings.extendedCodes ) {
               Serial.println(F("PPP: recommend AT$SB=57600 or lower"));
            }
         }
         delay(500);
         if( pppStart() ) {
            state = ONLINE_PPP;
            connectTime = millis();
            dtrWentInactive = false;
            sendResult(R_CONNECT);
            digitalWrite(DCD, ACTIVE);
            amClient = true;
         } else {
            sendResult(R_NO_CARRIER);
            digitalWrite(DCD, !ACTIVE);
         }
         atCmd[0] = NUL;
         return atCmd;
      }

      char *doPppMode(char *atCmd) {
         switch( atCmd[0] ) {
            case '?':
               ++atCmd;
               Serial.println(pppIsActive() ? 1 : 0);
               if( !atCmd[0] ) {
                  sendResult(R_OK);
               }
               break;
            case '=':
               ++atCmd;
               if( atCmd[0] == '1' ) {
                  ++atCmd;
                  dialPpp((char *)"*99#");
               } else if( atCmd[0] == '0' ) {
                  ++atCmd;
                  if( state == ONLINE_PPP ) {
                     pppStop();
                     state = CMD_NOT_IN_CALL;
                     sendResult(R_OK);
                     digitalWrite(DCD, !ACTIVE);
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

   #else

      bool pppIsActive(void) { return false; }
      bool pppLinkDropped(void) { return false; }
      bool pppStart(void) { return false; }
      void pppStop(void) {}
      void pppLoop(void) {}

      char *dialPpp(char *atCmd) {
         (void)atCmd;
         sendResult(R_NO_CARRIER);
         return atCmd;
      }

      char *doPppMode(char *atCmd) {
         (void)atCmd;
         sendResult(R_ERROR);
         return atCmd;
      }

   #endif

#endif
