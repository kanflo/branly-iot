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

#ifdef _MACOSX
#include <stdio.h>
#include <iostream>
#else
#include <stdarg.h>
#endif // _MACOSX
#include <RFM69.h>
#include "BranlyNode.h"
#include "BranlyContact.h"

static BranlyNode *gBranlyNode = NULL;

#define PRINTF_BUF_LEN  (80)
void bprintf(const char *fmt, ... )
{
  char buf[PRINTF_BUF_LEN];
#ifndef __x86_64__
  static bool isInited = false;
  if (!isInited) {
    Serial.begin(115200);
    Serial.println("Serial init done");
    isInited = true;
  }
#endif // _MACOSX
  va_list args;
  va_start (args, fmt );
  vsnprintf(buf, PRINTF_BUF_LEN, fmt, args);
  va_end (args);
#ifdef __x86_64__
  printf(buf);
#else
  Serial.print(buf);
#endif // __x86_64__
}

BranlyNode::BranlyNode(RFM69 *radio, unsigned char hwVersion, unsigned char swVersion)
{
  gBranlyNode = this;
  mRadio = radio;
  mIsInfactoryMode = false;
  mNumContacts = 0;
  mVersionHW = hwVersion;
  mVersionSW = swVersion;
  mCurState = kSendHello;
}

void BranlyNode::addContact(BranlyContact *contact)
{
  if (mNumContacts <= CONFIG_MAX_CONTACTS) {
    mContacts[mNumContacts++] = contact;
  }
}

BranlyNode *BranlyNode::node()
{
  return gBranlyNode;
}

void BranlyNode::run()
{
  switch (mCurState) {
    case kSendHello:
      if (this->sendHello()) {
        mCurState = kSendContactList;
      } else {
        // TODO: wait until next attempt, do not run out of batteries or jam the radio
      }
      break;
    case kSendContactList:
      if (this->sendContactList()) {
        mCurState = kSendContactReport;
      } else {
        // TODO: wait until next attempt, do not run out of batteries or jam the radio
      }
      break;
    case kSendContactReport:
      if (this->sendContactReport()) {
        mCurState = kRunning;
      } else {
        // TODO: wait until next attempt, do not run out of batteries or jam the radio
      }
      break;
    case kRunning:
      unsigned long sleepInterval = 0xffffffff;
      for (uint8_t i=0; i<mNumContacts; i++) {
        if (mContacts[i]->isEnqueued()) {
          mContacts[i]->sendReport();
        } else {
          unsigned long interval = mContacts[i]->nextTickInterval();
          if (interval > 0 && interval < sleepInterval) {
            sleepInterval = interval;
          }
        }
      }
      bprintf("Sleeping for %d minutes\n", sleepInterval);
      for (uint8_t i=0; i<mNumContacts; i++) {
        mContacts[i]->intervalTick(sleepInterval);
        // TODO: Combine contact reports if several contacts timed out concurrently
      }
      break;
  }
}

bool BranlyNode::sendPing()
{
  bprintf("BranlyNode sending ping\n");
  BranlyProtocol::buildPingPacket();
  bool success = this->getRadio()->sendWithRetry(1, (const void*) BranlyProtocol::packetData(), BranlyProtocol::packetLength());
  this->getRadio()->sleep();
  return success;
}

bool BranlyNode::sendHello()
{
  bprintf("BranlyNode sending hello\n");
  BranlyProtocol::buildHelloPacket(mVersionHW, mVersionSW);
  bool success = this->getRadio()->sendWithRetry(1, (const void*) BranlyProtocol::packetData(), BranlyProtocol::packetLength());
  this->getRadio()->sleep();
  return success;
}

bool BranlyNode::sendContactList()
{
  bprintf("BranlyNode sending contact list (%d contacts)\n", mNumContacts);
  BranlyProtocol::buildContactListPacket(this);
  bool success = this->getRadio()->sendWithRetry(1, (const void*)BranlyProtocol::packetData(), BranlyProtocol::packetLength());
  this->getRadio()->sleep();
  return success;
}

bool BranlyNode::sendContactReport()
{
  bprintf("BranlyNode sending contact report (%d contacts)\n", mNumContacts);
  BranlyProtocol::buildContactReportPacket(this);
  bool success = this->getRadio()->sendWithRetry(1, (const void*)BranlyProtocol::packetData(), BranlyProtocol::packetLength());
  this->getRadio()->sleep();
  return success;
}

void BranlyNode::enterFactoryMon()
{
  bprintf("BranlyNode entering factory monitor\n");
  // TODO
}
