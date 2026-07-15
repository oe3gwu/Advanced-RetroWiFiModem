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
      #include "lwip/tcpip.h"
      #include "esp_netif.h"

      static struct netif pppNetif;
      static ppp_pcb *pppPcb = nullptr;
      static bool pppLinkUp = false;
      static bool pppHadLinkUp = false;

      static u32_t pppSerialOutput(ppp_pcb *pcb, const void *data, u32_t len, void *ctx) {
         (void)pcb;
         (void)ctx;
         return (u32_t)Serial.write((const uint8_t *)data, len);
      }

      static bool pppNatEnabled = false;

      static void pppEnableNat(ppp_pcb *pcb) {
         struct netif *nif = ppp_netif(pcb);
         if( !nif ) {
            return;
         }
         // Do not gate on netif_is_up() — at PPPERR_NONE the flag may still be clear.
         if( !ip_napt_enable_netif(nif, 1) ) {
            ip4_addr_t ourAddr;
            IP4_ADDR(&ourAddr, PPP_OUR_IP1, PPP_OUR_IP2, PPP_OUR_IP3, PPP_OUR_IP4);
            ip_napt_enable(ip4_addr_get_u32(&ourAddr), 1);
         }
         esp_netif_t *sta = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
         if( sta ) {
            esp_netif_set_default_netif(sta);
         }
         pppNatEnabled = true;
      }

      static void pppDisableNat(ppp_pcb *pcb) {
         struct netif *nif = ppp_netif(pcb);
         if( nif ) {
            ip_napt_enable_netif(nif, 0);
         }
         pppNatEnabled = false;
      }

      static void pppSetNatFromCallback(ppp_pcb *pcb, bool enable) {
         // pppStatusHandler runs on the tcpip thread — never lock here.
         if( enable ) {
            pppEnableNat(pcb);
         } else {
            pppDisableNat(pcb);
         }
      }

      static void pppSetNatFromMain(ppp_pcb *pcb, bool enable) {
         if( !pcb ) {
            return;
         }
         LOCK_TCPIP_CORE();
         if( enable ) {
            pppEnableNat(pcb);
         } else {
            pppDisableNat(pcb);
         }
         UNLOCK_TCPIP_CORE();
      }

      static void pppSetPeerDns(ppp_pcb *pcb) {
         #if LWIP_DNS
            esp_netif_t *sta = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
            if( !sta ) {
               return;
            }
            esp_netif_dns_info_t dns;
            if( esp_netif_get_dns_info(sta, ESP_NETIF_DNS_MAIN, &dns) != ESP_OK ) {
               return;
            }
            ip4_addr_t dnsAddr;
            dnsAddr.addr = dns.ip.u_addr.ip4.addr;
            if( dnsAddr.addr ) {
               ppp_set_ipcp_dnsaddr(pcb, 0, &dnsAddr);
            }
         #endif
      }

      static void pppStatusHandler(ppp_pcb *pcb, int err_code, void *ctx) {
         (void)ctx;
         switch( err_code ) {
            case PPPERR_NONE:
               pppLinkUp = true;
               pppHadLinkUp = true;
               // NAPT on the PPP (client-facing) netif, not WiFi STA — see ESP-IDF lwIP guide.
               pppSetNatFromCallback(pcb, true);
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

      bool pppIsActive(void) {
         return pppPcb != nullptr && pppLinkUp;
      }

      bool pppLinkDropped(void) {
         return pppPcb != nullptr && pppHadLinkUp && !pppLinkUp;
      }

      bool pppWasLinkUp(void) {
         return pppHadLinkUp;
      }

      bool pppStart(void) {
         if( WiFi.status() != WL_CONNECTED ) {
            return false;
         }
         if( pppPcb ) {
            return true;
         }

         pppHadLinkUp = false;
         pppLinkUp = false;
         pppNatEnabled = false;

         pppPcb = pppapi_pppos_create(&pppNetif, pppSerialOutput, pppStatusHandler, nullptr);
         if( !pppPcb ) {
            return false;
         }

         LOCK_TCPIP_CORE();
         #if CHAP_SUPPORT
            ppp_set_auth(pppPcb, PPPAUTHTYPE_PAP | PPPAUTHTYPE_CHAP, "", "");
         #else
            ppp_set_auth(pppPcb, PPPAUTHTYPE_PAP, "", "");
         #endif

         ip4_addr_t ourAddr;
         ip4_addr_t peerAddr;
         IP4_ADDR(&ourAddr, PPP_OUR_IP1, PPP_OUR_IP2, PPP_OUR_IP3, PPP_OUR_IP4);
         IP4_ADDR(&peerAddr, PPP_OUR_IP1, PPP_OUR_IP2, PPP_OUR_IP3, PPP_PEER_IP4);
         ppp_set_ipcp_ouraddr(pppPcb, &ourAddr);
         ppp_set_ipcp_hisaddr(pppPcb, &peerAddr);
         pppSetPeerDns(pppPcb);
         ppp_set_passive(pppPcb, 1);
         UNLOCK_TCPIP_CORE();

         // Arduino ESP32 prebuilt lwIP has no PPP_SERVER (no pppapi_listen); passive
         // connect waits for the peer (pppd) to start LCP like a dial-up modem.
         err_t pppErr;
         #if PPP_SERVER
            pppErr = pppapi_listen(pppPcb);
         #else
            pppErr = pppapi_connect(pppPcb, 0);
         #endif
         if( pppErr != ERR_OK ) {
            pppapi_free(pppPcb);
            pppPcb = nullptr;
            return false;
         }
         return true;
      }

      void pppStop(void) {
         if( pppPcb ) {
            pppSetNatFromMain(pppPcb, false);
            pppapi_close(pppPcb, 0);
            pppapi_free(pppPcb);
            pppPcb = nullptr;
         }
         pppLinkUp = false;
         pppHadLinkUp = false;
      }

      void pppLoop(void) {
         if( !pppPcb || state != ONLINE_PPP ) {
            return;
         }
         if( pppLinkUp && !pppNatEnabled ) {
            LOCK_TCPIP_CORE();
            if( !pppNatEnabled ) {
               pppEnableNat(pppPcb);
            }
            UNLOCK_TCPIP_CORE();
         }
         uint8_t buf[128];
         size_t n = 0;
         while( Serial.available() && n < sizeof(buf) ) {
            buf[n++] = (uint8_t)Serial.read();
         }
         if( n > 0 ) {
            pppos_input_tcpip(pppPcb, buf, n);
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
         state = ONLINE_PPP;
         connectTime = millis();
         dtrWentInactive = false;
         sendResult(R_CONNECT);
         digitalWrite(DCD, ACTIVE);
         amClient = true;
         if( !pppStart() ) {
            pppStop();
            state = CMD_NOT_IN_CALL;
            digitalWrite(DCD, !ACTIVE);
            sendResult(R_NO_CARRIER);
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

   #elif defined(ESP8266) && __has_include("netif/ppp/pppos.h")

      #include "netif/ppp/pppos.h"
      #include "netif/ppp/pppapi.h"
      #include "netif/ppp/ppp.h"
      #include "lwip/netif.h"

      static struct netif pppNetif;
      static ppp_pcb *pppPcb = nullptr;
      static bool pppLinkUp = false;
      static bool pppHadLinkUp = false;
      static uint32_t pppLastTimerMs = 0;

      static u32_t pppSerialOutput(ppp_pcb *pcb, u8_t *data, u32_t len, void *ctx) {
         (void)pcb;
         (void)ctx;
         return (u32_t)Serial.write(data, len);
      }

      static void pppStatusHandler(ppp_pcb *pcb, int err_code, void *ctx) {
         (void)pcb;
         (void)ctx;
         if( err_code == PPPERR_NONE ) {
            pppLinkUp = true;
            pppHadLinkUp = true;
         } else if( err_code == PPPERR_USER || err_code == PPPERR_CONNECT || err_code == PPPERR_AUTHFAIL ) {
            pppLinkUp = false;
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

      bool pppLinkDropped(void) {
         return pppPcb != nullptr && pppHadLinkUp && !pppLinkUp;
      }

      bool pppStart(void) {
         if( WiFi.status() != WL_CONNECTED ) {
            return false;
         }
         if( settings.serialSpeed > 57600 ) {
            return false;
         }
         if( pppPcb ) {
            return true;
         }

         pppHadLinkUp = false;
         pppLinkUp = false;

         pppPcb = pppapi_pppos_create(&pppNetif, pppSerialOutput, pppStatusHandler, nullptr);
         if( !pppPcb ) {
            return false;
         }

         ppp_set_auth(pppPcb, PPPAUTHTYPE_PAP, "", "");

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
         pppHadLinkUp = false;
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
      bool pppWasLinkUp(void) { return false; }
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
