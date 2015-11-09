#!/usr/bin/python
# -*- coding: utf-8 -*-
#
# Copyright (c) 2015 Johan Kanflo (github.com/kanflo)
#
# Permission is hereby granted, free of charge, to any person obtaining
# a copy of this software and associated documentation files (the
# "Software"), to deal in the Software without restriction, including
# without limitation the rights to use, copy, modify, merge, publish,
# distribute, sublicense, and/or sell copies of the Software, and to
# permit persons to whom the Software is furnished to do so, subject to
# the following conditions:
#
# The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
# LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
# OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
# WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


"""
Branly Pi Gateway daemon

Recevies Branly style packets on the Raspberry Pi UART and posts data to
emoncms.

 Usage

1. Set your emoncms credentials under "User Configuration" below
2. Make sure tornado, pyserial and requests are installed
3. Check https://github.com/lurch/rpi-serial-console to make sure this script
   is the only process using the RPi UART
"""


import serial
import re
import requests
import datetime
import tornado.ioloop
import tornado.web
import threading
import sys, traceback
import json
import socket
import time, calendar
import os
import Queue
import logging
import logging.handlers
import remotelogger
from emoncms import *


"""
Emoncms configuration
"""
confEmonCmsServer = "http://localhost/emoncms"
confEmonCmsKey = "ff1b9d810ce39fe7009477c5ac83b113"


"""
# "Constants", if there was such a thing in Python
"""
kGatewayVersion = "001" # 0.0.1


"""
# Globals
"""
gQuit = False # Quit flag
gSerialRXQueue = False
gCMS = False


log = logging.getLogger(__name__)

def uartThread():
	"""
	This thread handles RX from the UART. Lines received are posted one by one
	to the gSerialRXQueue queue. The reason for keeping a separate thread
	handeling the UART is that the emoncms thread might hang for some time due to
	network latency and we do not want to miss any data received on the UART.
	"""
	log.info("UART thread running")
	global gSerialRXQueue
	global qQuit
	if sys.platform == "darwin":
		# Special case for development on a Mac
		serialPort = serial.Serial("/dev/cu.usbserial", 115200, timeout=0.5)
	else: # Assume Raspberry Pi
		serialPort = serial.Serial("/dev/ttyAMA0", 115200, timeout=0.5)

	while not gQuit:
		message = serialPort.readlines(None)
		if (len(message)):
			log.debug( "UART thread got '%s'" % message)
			for line in message:
				line = line.rstrip()
				if len(line) > 0:
					gSerialRXQueue.put(line)
	serialPort.close()


"""
Message handling
"""

def validMessage(message):
	"""
	Return True if message is valid. Messages are formatted as ":<data>;"
	"""
	return message.count(';') == 1 and message.startswith(':') and message.endswith(';')

def handlePacketMessage(message):
	"""
	Radio packet message received
	"""
	message = message[1:] # Remove message start and end markers
	message = message[:-1]
	parts = message.split(":")
	packet = BranlyPacket(parts[1:])
	if packet.valid:
		# TODO: Send packets to emoncms sinks
		if not gCMS.handlePacket(packet):
			log.error("Error: CMS packet handeling failed")
	else:
		log.error("Illegal packet")

		
def handleDebugMessage(message):
	"""
	Debug message received from modem
	"""
	log.debug("MODEM:%s" % (message))


def gatewayThread():
	"""
	# This thread receives lines from the gSerialRXQueue queue and posts data to
	# emoncms. 
	"""
	global gQuit
	global gSerialRXQueue

	log.info("Gateway thread running")
	while not gQuit:
		try:
			message = gSerialRXQueue.get()
			if message[0] == "#":
				# Debug messages from the BranlyPi modem
				handleDebugMessage(message)
			else:
				if validMessage(message): # TODO: Move to BranlyPacket, or not
					msgType = message[1]
					if msgType == "P":
						handlePacketMessage(message)
					else:
						log.error("Unknown type '%s' in packet '%s'" % (msgType, message))
				else:
					log.error("Invalid message '%s'" % (message))
				
		except NameError as e:
			log.critical("Gateway got NameError exception", exc_info=True)
#			print traceback.format_exc()
			gQuit = True
		except Exception as e:
			log.critical("Gateway got exception", exc_info=True)
#			print traceback.format_exc()


def addTestData():
	"""
	Add test packets
	Packet format :":P:<from>:<to>:<rssi>:<data 0> <data 1> <data 2> ... <data n>;"
	"""

	# hello
	gSerialRXQueue.put(":P:10:1:-42:00 00 10 25 00;")

	# contact list
	gSerialRXQueue.put(":P:10:1:-42:02 01 12 21 37 ca;")

	# contact report
	gSerialRXQueue.put(":P:10:1:-42:03 02 31 7c 0b 00 00 32 f5 00 00 00 03 00;")

	# reports
	gSerialRXQueue.put(":P:10:1:-42:04 03 32 f5 00 00 00;")
	gSerialRXQueue.put(":P:10:1:-42:04 04 03 0a;")
	gSerialRXQueue.put(":P:10:1:-42:04 05 31 7c 0b 00 00;")
	gSerialRXQueue.put(":P:10:1:-42:04 06 03 0a;")
	gSerialRXQueue.put(":P:10:1:-42:04 07 32 f5 00 00 00;")

	# Illegal message
	gSerialRXQueue.put(":P:10:1:-42:00 00 10 25 00")

	# Debug message
	gSerialRXQueue.put("# Hello world")
	gSerialRXQueue.put("\n# FRQ:868 RTEMP:45\n")
	gSerialRXQueue.put("\n")



def loggingInit(level = logging.INFO):
	"""
	Initialize logging
	"""
	logger = logging.getLogger()
	logger.setLevel(level)

	appName = "branlygw"
	if sys.platform == "darwin":
		subSystem = "dev"
	else:
		# Assume raspberry Pi
		subSystem = "prod"

	# Log to remote
	remotelogger.init(logger = logger, appName = appName, subSystem = subSystem, level = level, host = "midi.local")

	if 0:
		# Log to branlygw.log
		fileFormatter = logging.Formatter("%(asctime)s - %(name)s - %(levelname)s - %(message)s")
		logfile = logging.handlers.RotatingFileHandler("branlygw.log")
		logfile.setLevel(logging.INFO)
		logfile.setFormatter(fileFormatter)
		logger.addHandler(logfile)

	# Log to stdout
	ch = logging.StreamHandler(sys.stdout)
	formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s')
	ch.setFormatter(formatter)
	logger.addHandler(ch)

	
def main():
	"""
	Trusty 'ol main
	"""
	global gQuit
	global gSerialRXQueue
	global gCMS

	loggingInit(logging.DEBUG)

	logging.getLogger("requests").setLevel(logging.WARNING) # Kill request logging
	gCMS = Emoncms(confEmonCmsServer, confEmonCmsKey)
#	gCMS.enableDebug()
	log.info("Branly Pi Gateway %s running on %s" % (kGatewayVersion, sys.platform))
	log.info(gCMS)

	if 1:
		nodes = gCMS.readBranlyNodes()
		for node in nodes:
			log.info(" %s" % node)
			for contact in node.contacts:
				log.info("  %s" % contact)

	gSerialRXQueue = Queue.Queue()

	gwThr = threading.Thread(target=gatewayThread)
	gwThr.daemon=True
	gwThr.start()
	if sys.platform == "darwin":
		addTestData()
	else:
		uartThr = threading.Thread(target=uartThread)
		uartThr.daemon=True
		uartThr.start()
	try:
		while not gQuit:
			time.sleep(1) # TODO: Increase in production
	except KeyboardInterrupt:
		log.info("Shutdown requested...exiting")
		gQuit = True
		sys.exit(1)
	except Exception:
		log.critical("GW got exception ", exc_info=True)
		gQuit = True
		sys.exit(1)


if __name__ == "__main__":
	os.environ['TZ'] = 'UTC'
	time.tzset()

	while not gQuit:
		main() # Don't exit on crashes please

