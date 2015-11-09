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

#ifndef __MacBranly__BranlyContact__
#define __MacBranly__BranlyContact__

#ifdef _MACOSX
#include <stdio.h>
#endif // _MACOSX
#include "BranlyProtocol.h"

typedef enum {
  kTypeTemperature = 1,
  kTypeVoltage,
  kTypeWattUsage,
  kTypeHumidity,
  kTypeLDR, // Photocell (Light Dependent Resistor)
  kTypeMotion,
  kTypeButton,
  kTypePowerSwitch,
  kTypeLight,
  kTypeRGBLight,
  kUserDefined = 100
} ContactType_t;

typedef enum {
  k1Minute = 60, // In minutes
  k15Minutes = 900,
  k1Hour = 3600,
  k1Day = 86400,
} ContactReportInterval_t;

#define kReportingIntervalNone (0)

typedef long (*NodeValue)(bool setValue, long value);

class BranlyNode;

class BranlyContact
{
public:
  BranlyContact(unsigned char contactId, ContactType_t contactType, NodeValue valueFunc = NULL, unsigned long reportingInterval = kReportingIntervalNone);

  bool isVioloted(); // Virtual method, may be overridden for more complex threshold limits
  
  void setLowerThreshold(long threshold);

  void setValue(long value);
  void refreshValue();

  ContactSize_t valueSize();

  bool sendReport();
  // Enqueueing. Set from interrupt mode will cause report to be sent in the next run loop
  void enqueueReport(long value);
  bool isEnqueued() { return mIsEnqueued; };

  unsigned long nextTickInterval() { return mNextReport; };
  void intervalTick(unsigned long time);
  void intervalReset() { mNextReport = mReportingInterval; };

  unsigned char id() { return mId; };
  ContactType_t type() { return mType; };
  long value() { return mValue; };
  void makeWriteable() { mIsWriteable = true; }; // Light switches and so on are writeable
  bool isWriteable();
  
private:
  unsigned char mId;
  ContactType_t mType;
  long mValue;
  bool mIsWriteable;
  bool mHasLowerThreshold;
  long mLowerThreshold;
  unsigned long mReportingInterval;
  unsigned long mNextReport;
  NodeValue mValueFunc;
  bool mIsEnqueued;
};

#endif /* defined(__MacBranly__BranlyContact__) */
