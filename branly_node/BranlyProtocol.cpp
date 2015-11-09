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
#include <iostream>
#endif // _MACOSX
#include "BranlyProtocol.h"
#include "BranlyNode.h"
#include "BranlyContact.h"


static unsigned char buf[64];

unsigned char* BranlyProtocol::mPacketBuffer = (unsigned char*)buf;
unsigned char BranlyProtocol::mPacketLength = 0;
unsigned char BranlyProtocol::mPacketCounter = 0;


BranlyProtocol::BranlyProtocol()
{
#ifdef _MACOSX
  std::cout << "BranlyNode created\n";
#endif // _MACOSX
}

unsigned char* BranlyProtocol::packetData()
{
  return BranlyProtocol::mPacketBuffer;
}

unsigned int BranlyProtocol::packetLength()
{
  return BranlyProtocol::mPacketLength;
}

/*
 *
 * Packet assembly stuff
 *
 */
unsigned char BranlyProtocol::buildDefaultPacket(BranlyPacket_t type)
{
  BranlyProtocol::mPacketBuffer[0] = type;
  BranlyProtocol::mPacketBuffer[1] = BranlyProtocol::mPacketCounter++;
  bprintf("cnt:%d\n", BranlyProtocol::mPacketBuffer[1]);
  BranlyProtocol::mPacketLength = 2;
  return BranlyProtocol::mPacketLength;
}


#warning "TODO: Add node type"
void BranlyProtocol::buildHelloPacket(unsigned char hwVersion, unsigned char swVersion)
{
  unsigned char i = BranlyProtocol::buildDefaultPacket(kPacketHello);
  BranlyProtocol::mPacketBuffer[i++] = hwVersion;
  BranlyProtocol::mPacketBuffer[i++] = swVersion;
  BranlyProtocol::mPacketBuffer[i++] = 0; // AVR mcusr register
  BranlyProtocol::mPacketLength = i;
}

void BranlyProtocol::buildPingPacket()
{
  BranlyProtocol::buildDefaultPacket(kPacketPing);
}

void BranlyProtocol::buildContactListPacket(BranlyNode *node)
{
  unsigned char j, i = BranlyProtocol::buildDefaultPacket(kPacketContactList);
  for(j=0; j<node->numContacts(); j++) {
    BranlyContact *contact = node->getContact(j);
    BranlyProtocol::mPacketBuffer[i++] = CONTACTLIST_SET_WR_ID_TYPE(contact->isWriteable(), contact->id(), contact->type());
    bprintf("  id %d of type %d\n", contact->id(), contact->type());
  }
  BranlyProtocol::mPacketLength = i;
}

void BranlyProtocol::buildContactReportPacket(BranlyNode *node)
{
  unsigned char j = 1, i = BranlyProtocol::buildDefaultPacket(kPacketContactReport);
  for(j=0; j<node->numContacts(); j++) {
    BranlyContact *contact = node->getContact(j);
    unsigned char flags = 0;
    ContactSize_t size;
    contact->refreshValue();
    size = contact->valueSize();
    if (contact->isVioloted()) {
      flags |= kFlagViolated;
    }

    BranlyProtocol::mPacketBuffer[i++] = CONTACT_SET_FLAGS_SIZE_ID(flags, size, contact->id());
    if (size == kSize32) {
      unsigned long value = contact->value();
      bprintf("  id:%d value:%d flags:%d size:%d\n", contact->id(), contact->value(), (contact->isVioloted() ? kFlagViolated : 0), size);
      BranlyProtocol::mPacketBuffer[i++] = value & 0xff;
      value >>= 8;
      BranlyProtocol::mPacketBuffer[i++] = value & 0xff;
      value >>= 8;
      BranlyProtocol::mPacketBuffer[i++] = value & 0xff;
      value >>= 8;
      BranlyProtocol::mPacketBuffer[i++] = value;
    } else {
      bprintf("  id:%d value:%d flags:%d size:%d\n", contact->id(), contact->value(), (contact->isVioloted() ? kFlagViolated : 0), size);
      BranlyProtocol::mPacketBuffer[i++] = (char) contact->value();
    }
  }
  BranlyProtocol::mPacketLength = i;
}

void BranlyProtocol::buildContactValuePacket(BranlyContact *contact)
{
  unsigned char i = BranlyProtocol::buildDefaultPacket(kPacketContactValue);
  unsigned char flags = 0;
  ContactSize_t size;
  contact->refreshValue();
  size = contact->valueSize();
  if (contact->isVioloted()) {
    flags |= kFlagViolated;
  }

  BranlyProtocol::mPacketBuffer[i++] = CONTACT_SET_FLAGS_SIZE_ID(flags, size, contact->id());;
  if (size == kSize32) {
    unsigned long value = contact->value();
    BranlyProtocol::mPacketBuffer[i++] = value & 0xff;
    value >>= 8;
    BranlyProtocol::mPacketBuffer[i++] = value & 0xff;
    value >>= 8;
    BranlyProtocol::mPacketBuffer[i++] = value & 0xff;
    value >>= 8;
    BranlyProtocol::mPacketBuffer[i++] = value;
  } else {
    BranlyProtocol::mPacketBuffer[i++] = (char) contact->value();
  }
  BranlyProtocol::mPacketLength = i;
}

