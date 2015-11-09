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

#ifndef __BRANLYNODE__
#define __BRANLYNODE__

#ifndef CONFIG_MAX_CONTACTS
#define CONFIG_MAX_CONTACTS (16)
#endif

#include "BranlyProtocol.h"

void bprintf(const char *fmt, ... );

typedef enum {
  kSendHello = 1,
  kSendContactList,
  kSendContactReport,
  kRunning
} NodeState_t;

class RFM69;
class BranlyContact;
class BranlyProtocol;

class BranlyNode
{
public:
  BranlyNode(RFM69 *radio, unsigned char hwVersion, unsigned char swVersion);

  static BranlyNode *node();
  
  bool sendPing();
  bool sendHello();
  bool sendContactList();
  bool sendContactReport();

  void run();

  void enterFactoryMon();
  bool isFactoryMode() { return mIsInfactoryMode; };

  RFM69 *getRadio() { return mRadio; };
  void addContact(BranlyContact *contact);
  BranlyContact* getContact(unsigned int index) { return mContacts[index]; };
  unsigned int numContacts() { return mNumContacts; };

private:
  RFM69 *mRadio;
  bool mIsInfactoryMode;
  unsigned int mNumContacts;
  BranlyContact *mContacts[CONFIG_MAX_CONTACTS];
  unsigned char mVersionHW, mVersionSW;
  NodeState_t mCurState;
};

#endif // __BRANLYNODE__
