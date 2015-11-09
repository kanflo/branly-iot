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
#include <RFM69.h>
#include "BranlyContact.h"
#include "BranlyNode.h"
#include "BranlyProtocol.h"

BranlyContact::BranlyContact(unsigned char contactId, ContactType_t contactType, NodeValue valueFunc, unsigned long reportingInterval)
{
#ifdef _MACOSX
  std::cout << "BranlyContact id " << (int) contactId << " of type " << contactType << " created\n";
#endif // _MACOSX

  mId = contactId;
  mType = contactType;
  mHasLowerThreshold = false;
  mValue = 0;
  mLowerThreshold = 0;
  mIsEnqueued = false;
  mReportingInterval = reportingInterval;
  mValueFunc = valueFunc;
  mIsWriteable = false;
  this->intervalReset();
  
  BranlyNode *node = BranlyNode::node();
  if (node) {
    node->addContact(this);
  }
}

bool BranlyContact::isVioloted()
{
  if (mHasLowerThreshold) {
    return (mValue <= mLowerThreshold);
  } else {
    return false;
  }
}

ContactSize_t BranlyContact::valueSize()
{
  if (mValue < 128 && mValue > -128) return kSize8;
  else return kSize32;
}

void BranlyContact::enqueueReport(long value)
{
  mValue = value;
  mIsEnqueued = true;
}

bool BranlyContact::sendReport()
{
//  this->refreshValue(); // Done in BranlyProtocol::buildContactValuePacket
  bprintf("Contact %d sending report\n", mId);
  BranlyNode *node = BranlyNode::node();
  if (node) {
    BranlyProtocol::buildContactValuePacket(this);
    bool success = node->getRadio()->sendWithRetry(1, (const void*) BranlyProtocol::packetData(), BranlyProtocol::packetLength());
    node->getRadio()->sleep();
    return success;
  } else {
    return false;
  }
}

void BranlyContact::setLowerThreshold(long threshold)
{
  bprintf("BranlyContact lower threshold:%d\n", threshold);
  mHasLowerThreshold = true;
  mLowerThreshold = (unsigned long) threshold;
}

void BranlyContact::setValue(long value)
{
  bprintf("BranlyContact set value:%d\n", value);
  mValue = value;
  if (isWriteable() && mValueFunc) {
    (void) mValueFunc(true, value);
  }
}

void BranlyContact::refreshValue()
{
  if (mValueFunc) {
    mValue = mValueFunc(false, 0);
  }
}

void BranlyContact::intervalTick(unsigned long time)
{

  if (mReportingInterval > 0) {
    if (time >= mNextReport) {
      mNextReport = 0;
    } else {
      mNextReport -= time;
    }
    
    if (mNextReport == 0) {
      this->sendReport();
      this->intervalReset();
    }
  } else if (mIsEnqueued) {
    mIsEnqueued = false;
    this->sendReport();
  }
}

bool BranlyContact::isWriteable()
{
  return mIsWriteable;
};
