/*
  WiFiTelnetToSerial - Example Transparent UART to Telnet Server for esp8266

  Copyright (c) 2015 Hristo Gochkov. All rights reserved.
  This file is part of the ESP8266WiFi library for Arduino environment.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/
/* KBL
  I've tried this on a WeMos D1 Mini
  I thought I could swap uart pins (to GPIO15 and GPIO13) so that the boot up
  characters wouldn't get sent to the device I wish to telnet into.
  This seemed to work.
  But when I start up telnet on the local machine into the D1 mini - there are
  some garbage stuff received on the remote.
  This can cause the remote to get out of sync...
  For instance if the remote is a console into a raspberry pi and it is waiting
  for a login - this garbage will mess it up.
  I think the garbage might be part of the telnet protocol that this code isn't
  handling properly.

  Currently running on an ESP-01. Only swap the TX pin. I'm swapping it to GPIO2.
  Pinn connnections:
    ESP-01    RPi
    ------    ---
      1 GND    6 GND
      3 TX    10 RX
      7 RX     8 TX
      8 3V3    1 3V3

  From 'granpa':
  $ granpa:~$ telnet 192.168.1.83
  Trying 192.168.1.83...
  Connected to 192.168.1.83.
  Escape character is '^]'.

  Raspbian GNU/Linux 10 pluto ttyS0

  pluto login: yyyyyyyyy
  yyyyyyyyyy
  Password: xxxxxxx

  Linux pluto 4.19.75+ #1270 Tue Sep 24 18:38:54 BST 2019 armv6l

  The programs included with the Debian GNU/Linux system are free software;
  the exact distribution terms for each program are described in the
  individual files in /usr/share/doc//copyright.

  Debian GNU/Linux comes with ABSOLUTELY NO WARRANTY, to the extent
  permitted by applicable law.
  yyyyyyyy@pluto:~$ ^]
  telnet> mode character
  yyyyyyyy@pluto:~$ 
      o
      o
      o
  yyyyyyyy@pluto:~$ ^]
  telnet> quit
  Connection closed.
  $ granpa:~$ 
    
 */
#include <ESP8266WiFi.h>

#include <algorithm> // std::min

#ifndef STASSID
#define STASSID "xxxxxxxx"
#define STAPSK  "xxxxxxxxxxxxxxxxxxxxx"
#endif


/*
    SWAP_PINS:
   0: configure Hardware Serial port on RX:GPIO3 TX:GPIO1
//   1: configure Hardware Serial port on RX:GPIO13 TX:GPIO15
   1: configure Hardware Serial port on RX:GPIO3 TX:GPIO2
*/

#define SWAP_PINS 1


#define BAUD_SERIAL 115200
#define RXBUFFERSIZE 1024

////////////////////////////////////////////////////////////


#define STACK_PROTECTOR  512 // bytes

const char *ssid = STASSID;
const char *password = STAPSK;

const int port = 23;

WiFiServer server(port);
WiFiClient client;

char tmpch;

void setup()
{

  Serial.begin(BAUD_SERIAL);
  Serial.setRxBufferSize(RXBUFFERSIZE);

#if SWAP_PINS
//  Serial.swap();  // Hardware serial is now on RX:GPIO13 TX:GPIO15
  Serial.set_tx(2);   // Hardware serial is now on RX:GPIO3  TX:GPIO2
  delay(500);
#endif

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(50);
  }

  //start server
  server.begin();
///  server.setNoDelay(true);  // if set to true - disables Nagle alg.

}

int iac = 0;
char tbuf[16];

void loop() 
{
  //check if there are any new clients
  if (server.hasClient()) {
    client = server.available();
  }

  //check TCP clients for data
#if 1
  // Incredibly, this code is faster than the buffered one below - #4620 is needed
  while (client.available() && Serial.availableForWrite() > 0) {
    // working char by char is not very efficient
    tmpch = client.read();
#if 0
    sprintf(tbuf, "%02X_", tmpch);
    Serial.write(tbuf);
#else
    switch (iac) {
    case 0:
    	if (tmpch == (char)0xFF)
    		iac = 1;
    	else
        Serial.write(tmpch);
    	break;
    case 1:
    	if (tmpch == (char)0xFD || tmpch == 0xFB || tmpch == 0xFE)
    	  iac = 2;
    	else
    	  iac = 0;
    	break;
    case 2:
    default:
    	iac = 0;
    	break;
    }

#endif
  }
#else
  while (client.available() && Serial.availableForWrite() > 0) {
    size_t maxToSerial = std::min(client.available(), Serial.availableForWrite());
    maxToSerial = std::min(maxToSerial, (size_t)STACK_PROTECTOR);
    uint8_t buf[maxToSerial];
    size_t tcp_got = client.read(buf, maxToSerial);
    size_t serial_sent = Serial.write(buf, tcp_got);
  }
#endif

  // determine maximum output size "fair TCP use"
  // client.availableForWrite() returns 0 when !client.connected()
  size_t maxToTcp = 0;
  if (client) {
    size_t afw = client.availableForWrite();
    if (afw) {
      if (!maxToTcp) {
        maxToTcp = afw;
      } else {
        maxToTcp = std::min(maxToTcp, afw);
      }
    }
  }

  //check UART for data
  size_t len = std::min((size_t)Serial.available(), maxToTcp);
  len = std::min(len, (size_t)STACK_PROTECTOR);
  if (len) {
    uint8_t sbuf[len];
    size_t serial_got = Serial.readBytes(sbuf, len);
    // if client.availableForWrite() was 0 (congested)
    // and increased since then,
    // ensure write space is sufficient:
    if (client.availableForWrite() >= serial_got) {
      client.write(sbuf, serial_got);
    }
  }
}
