/* 
 * The MIT License (MIT)
 * 
 * Copyright (c) 2015 Johan Kanflo (github.com/kanflo)
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "Branly.h"
#include <RFM69.h>
#include <SPI.h>
#include <EEPROM.h>

#define GATEWAY_SW_VERSION 1

#define NODEID        1    //unique for each node on same network
#define NETWORKID     2  //the same on all nodes that talk to each other
//Match frequency to the hardware version of the RFM69 radio
//#define FREQUENCY     RF69_433MHZ
#define FREQUENCY     RF69_868MHZ
//#define FREQUENCY     RF69_915MHZ
#define ENCRYPTKEY    "sampleEncryptKey" //exactly the same 16 characters/bytes on all nodes!
//#define IS_RFM69HW    //uncomment only for RFM69HW! Leave out if you have RFM69W!
#define ACK_TIME      30 // max # of ms to wait for an ack
#define LED           9  // Pin 9 has an LED connected on Branly Pi.
#define SERIAL_BAUD   115200

RFM69 radio;
bool promiscuousMode = false; //set to 'true' to sniff all packets on the same network


#ifndef ABS
#define ABS(x) ((x)<0?(-x):(x))
#endif // ABS

#define PRINTF_BUF_LEN  (128)
void bprintf(const char *fmt, ... )
{
  char buf[PRINTF_BUF_LEN];
  static bool isInited = false;
  if (!isInited) {
    Serial.begin(SERIAL_BAUD);
    isInited = true;
  }
  va_list args;
  va_start (args, fmt );
  vsnprintf(buf, PRINTF_BUF_LEN, fmt, args);
  va_end (args);
  Serial.print(buf);
}

void print_reset_reason()
{
  volatile char mcusr = MCUSR;
  if (mcusr) Serial.print("# Reset reason:");
  if (mcusr & 0x01) Serial.print(" power-on");
  if (mcusr & 0x02) Serial.print(" external");
  if (mcusr & 0x04) Serial.print(" brown-out ");
  if (mcusr & 0x08) Serial.print(" watchdog");
  if (mcusr) Serial.println("");
}

void setup() {
  Blink(LED, 250);
  delay(10);
  print_reset_reason();
  radio.initialize(FREQUENCY, NODEID, NETWORKID);

#ifdef IS_RFM69HW
  radio.setHighPower(); //uncomment only for RFM69HW!
#endif
  radio.encrypt(ENCRYPTKEY);
  radio.promiscuous(promiscuousMode);
  bprintf("# FRQ:%d", FREQUENCY==RF69_433MHZ ? 433 : FREQUENCY==RF69_868MHZ ? 868 : 915);
  bprintf(" RTEMP:%d\n", radio.readTemperature(-1));
}


// Dump packet formatted as :":P:<from>:<to>:<rssi>:<data 0> <data 1> <data 2> ... <data n>;"
void dump_report(int src_node, int dst_node, int rssi, char* payload, int payload_size)
{
  int i;
  bprintf(":P:%d:%d:%d:", src_node, dst_node, rssi);
  for(i=0; i<payload_size; i++) {
    if (i) Serial.print(" ");
    bprintf("%02x", ((unsigned char*)payload)[i]);
  }
  bprintf(";\n");
}


void loop()
{
  if (radio.receiveDone()) {
    dump_report(radio.SENDERID, radio.TARGETID, radio.RSSI, (char*) radio.DATA, radio.DATALEN);
    if (radio.ACK_REQUESTED) {
      radio.sendACK();
    }
    Blink(LED, 50);
  }
}

void Blink(byte PIN, int DELAY_MS)
{
  pinMode(PIN, OUTPUT);
  digitalWrite(PIN,HIGH);
  delay(DELAY_MS);
  digitalWrite(PIN,LOW);
}
