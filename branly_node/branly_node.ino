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

#include <SPI.h>
#include <EEPROM.h>
#include <RFM69.h>
#include "BranlyNode.h"
#include "BranlyContact.h"

#define HW_VERSION 0x10
#define SW_VERSION 0x25
#define NODE_ID   (101)

#define CRYPT_KEY "sampleEncryptKey"
#define NWK_ID    (2)
#define NODE_FREQ RF69_868MHZ


// Contact value getters
long temperatureValue(bool setValue, long value);
long batteryValue(bool setValue, long value);
long rgbValue(bool setValue, long value);


RFM69 radio;
BranlyNode node(&radio, HW_VERSION, SW_VERSION);
BranlyContact battery(1, kTypeVoltage, batteryValue, k1Hour+2*k15Minutes);
BranlyContact temperature(2, kTypeTemperature, temperatureValue, k1Hour);
BranlyContact button(3, kTypeButton); // Buttons get reported from IRQ mode
BranlyContact rgbLight(4, kTypeRGBLight, rgbValue);

long batteryValue(bool setValue, long value)
{
  (void) setValue; (void) value; // Contact is not writeable
  bprintf("Reading battery\n");
  return 2940;
}

long temperatureValue(bool setValue, long value)
{
  (void) setValue; (void) value; // Contact is not writeable
  bprintf("Reading temperature\n");
  return 245; // Fixedpoint, 24.5deg
}

long rgbValue(bool setValue, long value)
{
  if (setValue) {
    bprintf("Setting RGB value to %lx\n", value);
  }
  return 0; // Node does not send report for this contact
}

void setup()
{
  bprintf("Node setup\n");
  radio.initialize(NODE_FREQ, NWK_ID, NODE_ID); // key, node, network
  radio.sleep();
  radio.encrypt(CRYPT_KEY);
  battery.setLowerThreshold(2400);
  rgbLight.makeWriteable();
}

void loop()
{
  node.run();
//  button.enqueueReport(10); // Called from ISR
}
