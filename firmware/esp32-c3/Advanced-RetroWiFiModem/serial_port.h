#ifndef _SERIAL_PORT_H
   #define _SERIAL_PORT_H

   // LOLIN C3 Mini: RS-232 is UART0 on GPIO20/21 (shield Tx/Rx pins).
   // With USB CDC on boot (default), Arduino maps Serial to USB — not the modem port.
   #include <HardwareSerial.h>

   #if ARDUINO_USB_CDC_ON_BOOT
      #define Serial Serial0
   #endif

#endif
