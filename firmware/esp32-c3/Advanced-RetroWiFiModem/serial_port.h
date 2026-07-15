#ifndef _SERIAL_PORT_H
   #define _SERIAL_PORT_H

   // LOLIN C3 Mini: RS-232 is UART0 on GPIO20/21 (shield Rx/Tx holes).
   // With USB CDC on boot (default), Arduino maps Serial to USB — not the modem port.
   #include <HardwareSerial.h>

   #define MODEM_RX_PIN 20
   #define MODEM_TX_PIN 21

   #if ARDUINO_USB_CDC_ON_BOOT
      #define Serial Serial0
   #endif

   inline void modemSerialBegin(unsigned long baud, uint32_t config) {
      Serial.begin(baud, config, MODEM_RX_PIN, MODEM_TX_PIN);
   }

#endif
