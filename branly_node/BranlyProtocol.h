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

#ifndef __MacBranly__BranlyProtocol__
#define __MacBranly__BranlyProtocol__

#ifdef _MACOSX
#include <stdio.h>
#endif // _MACOSX

#ifndef NULL
#define NULL (0) // Not defined in Arduino
#endif

class BranlyContact;
class BranlyNode;

typedef enum {
  kPacketHello = 0,
  kPacketPing,
  kPacketContactList,
  kPacketContactReport,
  kPacketContactValue
} BranlyPacket_t;


// Macros to set/get flags and id for kPacketContactReport and
// kPacketContactValue packets.
#define CONTACT_SET_FLAGS_SIZE_ID(flags, size, id) (((flags&3) << 6) | ((size&3) << 4) | ((id)&0xf))
#define CONTACT_GET_FLAGS(b) (((b)>>6)&3)
#define CONTACT_GET_SIZE(b) (((b)>>4)&3)
#define CONTACT_GET_ID(b) ((b)&0xf)

// Macros to set id and type for kPacketContactList packets.
#define CONTACTLIST_SET_WR_ID_TYPE(writeable, id, type) ((((writeable)&1) << 7) | (((id)&7) << 4) | ((type)&0xf))

// 'flag' field in kPacketContactReport and kPacketContactValue packets.

typedef enum {
  kFlagRFU = 0x2,
  kFlagViolated = 0x1
} ContactFlags_t;

typedef enum {
  kSize32 = 0x3,
  kSize24 = 0x2,
  kSize16 = 0x1,
  kSize8  = 0x0
} ContactSize_t;

class BranlyProtocol
{
public:
  BranlyProtocol();

  static unsigned char *packetData();
  static unsigned int packetLength();
  
  static unsigned char buildDefaultPacket(BranlyPacket_t type);
  static void buildHelloPacket(unsigned char hwVersion, unsigned char swVersion);
  static void buildPingPacket();
  static void buildContactValuePacket(BranlyContact *contact);
  static void buildContactListPacket(BranlyNode *node);
  static void buildContactReportPacket(BranlyNode *node);

//private:
  static unsigned char *mPacketBuffer;
  static unsigned char mPacketLength;
  static unsigned char mPacketCounter;
};

#endif /* defined(__MacBranly__BranlyProtocol__) */
