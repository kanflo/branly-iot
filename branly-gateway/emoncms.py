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

import requests
import json
import time
import logging
import httplib

# Radio packets types
kPacketHello = 0
kPacketPing = 1
kPacketContactList = 2
kPacketContactReport = 3
kPacketContactValue = 4

log = logging.getLogger(__name__)

class BranlyNode:
    """
    A class describing a Branly node
    """

    """
    Class members
    """
    id = False       # Node id (int)
    name = False     # Node name (string)
    contacts = False # Node contacts (BranlyContact[])

    def __init__(self, id, name):
        self.id = id
        self.name = name
        self.contacts = []

    def __str__(self):
        """
        Return string describing this object
        """
        str = "BranlyNode: id:%s name:%s" % (self.id, self.name)
        return str


    def addContact(self, contact):
        """
        Add contact to node
        """
        self.contacts.append(contact)

    def findContact(self, id):
        """
        Find BranlyContact of given id or return None if not found
        """
        for contact in self.contacts:
            if contact.id == id:
                return contact
        return None

    def setContactValue(self, id, value, flags = None):
        """
        Set value and optional flags of contact
        """
        contact = self.findContact(id)
        if not contact:
            log.error("Contact %d not found for node %d" % (self.id, id))
        else:
            contact.setValue(value)
            if flags != None:
                contact.setFlags(flags)



class BranlyContact:
    """
    A class describing a contact of a Branly node
    """

    """
    Class members
    """
    id = False    # Contact id (int)
    type = False  # Contact type (int)
    name = False  # Contact name (string)
    flags = False # Contact flags (int)
    value = False # Contact value (float)

    def __init__(self, id, type, name, flags):
        self.id = id
        self.type = type
        self.name = name
        self.flags = flags

    def __str__(self):
        """
        Return string describing this object
        """
        str = "BranlyContact: id:%d type:%d name:%s value:%s flags:%s" % (self.id, self.type, self.name, self.value, self.flags)
        return str

    def setValue(self, value):
        """
        Set contact value
        """
        self.value = value

    def setFlags(self, flags):
        """
        Set contact flags
        """
        self.flags = flags


class BranlyPacket:
    """
    A class describing an RF packet received from the Branly Pi RF modem.
    """

    """
    Class members
    """
    valid = False    # Is this packet valid (boolean)
    fromAddr = False # Node id of transmitter (int)
    toAddr = False   # Node id of receiver (int)
    rssi = False     # Receiver RSSI (int)
    type = False     # Packet type (int)
    seqNo = False    # Packet sequence number (int)
    payload = False  # Packet payload (int[])

    def __init__(self, parts):
        self.valid = len(parts) == 4
        if self.valid:
            log.debug(parts)
            self.fromAddr = int(parts[0], 16)
            self.toAddr = int(parts[1], 16)
            self.rssi = int(parts[2])
            payload = parts[3].split(" ")
            self.valid = len(payload) >= 2
        if self.valid:
            self.type = int(payload[0], 16)
            self.seqNo = int(payload[1], 16)
            payload = payload[2:] # Chop packet type and seqNo from payload
            payload = bytearray.fromhex(" ".join(payload)) # Convert payload to array of integers
            self.payload = payload
        if self.valid:
            self.__parsePacket()

    def __str__(self):
        """
        Return string describing this object
        """
        if self.type == kPacketHello:
            str = "Hello Packet : src:%d dst:%d cnt:%d rssi:%d HW:0x%02x SW:0x%02x mcusr:0x%02x" % (self.fromAddr, self.toAddr, self.seqNo, self.rssi, self.hwVersion, self.swVersion, self.mcusrRegister)
        elif self.type == kPacketPing:
            str = "Ping Packet : src:%d dst:%d cnt:%d rssi:%d " % (self.fromAddr, self.toAddr, self.seqNo, self.rssi)
        elif self.type == kPacketContactList:
            str = "ContactList Packet : src:%d dst:%d cnt:%d rssi:%d " % (self.fromAddr, self.toAddr, self.seqNo, self.rssi)
            for contact in self.contactList:
                writeable = ""
                if contact["writeable"]:
                    writeable = "writeable"
                str = str + "\n  id:%s type:%d %s" % (contact["id"], contact["type"], writeable)
        elif self.type == kPacketContactReport:
            str = "Contact Report Packet : src:%d dst:%d cnt:%d rssi:%d " % (self.fromAddr, self.toAddr, self.seqNo, self.rssi)
            for contact in self.contactValues:
                str = str + "\n  id:%s value:%.1f flags:%d" % (contact["id"], contact["value"], contact["flags"])
        elif self.type == kPacketContactValue:
            str = "Contact Value Packet : src:%d dst:%d cnt:%d rssi:%d " % (self.fromAddr, self.toAddr, self.seqNo, self.rssi)
            for contact in self.contactValues:
                str = str + "\n  id:%s value:%.1f flags:%d" % (contact["id"], contact["value"], contact["flags"])
        else:
            str = "Unknown packet"
        return str


    def __parsePacket(self):
        """
        Parse self.payload
        """
        if self.type == kPacketHello:
            self.__parseHelloPacket()
        elif self.type == kPacketPing:
            self.__parsePingPacket()
        elif self.type == kPacketContactList:
            self.__parseContactListPacket()
        elif self.type == kPacketContactReport:
            self.__parseContactReportPacket()
        elif self.type == kPacketContactValue:
            self.__parseContactValuePacket()


    def __parseHelloPacket(self):
        self.valid = len(self.payload) == 3
        if self.valid:
            self.hwVersion = self.payload[0]
            self.swVersion = self.payload[1]
            self.mcusrRegister = self.payload[2]
          

    def __parsePingPacket(self):
        self.valid = len(self.payload) == 0


    def __parseContactListPacket(self):
        index = 0
        self.contactList = []
        while index < len(self.payload):
            writeable = (self.payload[index] >> 7) & 1
            contactId = (self.payload[index] >> 4) & 7
            contactType = self.payload[index] & 0x0f
            index = index + 1
            self.contactList.append({"id":contactId, "type":contactType, "writeable":writeable})


    def __parseContactReportPacket(self):
        index = 0
        self.contactValues = []
        while index < len(self.payload):
            contactFlags = (self.payload[index] >> 6) & 3
            contactSize = (self.payload[index] >> 4) & 3
            contactId = self.payload[index] & 0x0f
            if contactSize == 3:
                contactValue = int(self.payload[index+1]) | (int(self.payload[index+2]) << 8) | (int(self.payload[index+3]) << 16) | (int(self.payload[index+4]) << 24)
                index = index + 5
            else:
                contactValue = int(self.payload[index+1])
                index = index + 2
            self.contactValues.append({"id":contactId, "value":contactValue, "flags":contactFlags})
            

    def __parseContactValuePacket(self):
        contactFlags = (self.payload[0] >> 6) & 3
        contactSize = (self.payload[0] >> 4) & 3
        contactId = self.payload[0] & 0x0f
        if contactSize == 3:
            contactValue = int(self.payload[1]) | (int(self.payload[2]) << 8) | (int(self.payload[3]) << 16) | (int(self.payload[4]) << 24)
        else:
            contactValue = int(self.payload[1])
        self.contactValues = [{"id":contactId, "value":contactValue, "flags":contactFlags}]



class Emoncms:
    """
    A class describing the emoncms, specified by its base URL and your API write
    key.
    The main function is handlePacket(self, packet) that acts on a BranlyPacket.
    BranlyPacket data will be synced to emoncms and stored according to its concept
    of node inputs and feeds.
    """
    serverAddress = False   # Emoncms base url
    apiWriteKey = False     # API write key
    nodes = False           # Known nodes (BranlyNode[])
    lastHttpCode = False    # HTTP response code of last API call
    lastPacketSeqNo = False # Sequence of last received packet
#    __cmsPrecision = 2      # Precision of values posted to emoncms

    def __init__(self, serverAddress, apiWriteKey):
        self.serverAddress = serverAddress
        self.apiWriteKey = apiWriteKey


    def __str__(self):
        """
        Return string describing this object
        """
        str = "Emoncms instance at %s" % (self.serverAddress)
        return str


    def enableDebug(self):
        """
        Enable HTTP debugging
        """
        httplib.HTTPConnection.debuglevel = 1
        logging.basicConfig() # you need to initialize logging, otherwise you will not see anything from requests
        logging.getLogger().setLevel(logging.DEBUG)
        requests_log = logging.getLogger("requests.packages.urllib3")
        requests_log.setLevel(logging.DEBUG)
        requests_log.propagate = True        


    def readBranlyNodes(self):
        """
        Read Branly nodes from the cms and return a list of BranlyNodes
        """
        nodes = []
        curNode = None
        feeds = self.__apiGet("feed/list.json", {})
        for f in feeds:
            if f["tag"] != None:
                tag = f["tag"].split(":")
                if len(tag) == 5 and tag[0] == "bc": # Branly Contact
                    # Tag has the format "bc:<node id>:<contact id>:<type>:<flags>"
                    # Eg. "bc:2:1:2:r"
                    #  <node id>    : integer
                    #  <contact id> : integer
                    #  <type>       : integer
                    #  <flags>      : string
                    c = {}
                    nodeId = int(tag[1])
                    if curNode == None:
                        curNode = BranlyNode(nodeId, "Node names currently not supported")
                    if curNode == None or curNode.id != nodeId:
                        nodes.append(curNode)
                        curNode = BranlyNode(nodeId, "Node names currently not supported")

                    name = f["name"].encode('utf8','ignore')
                    id = int(tag[2])
                    type = int(tag[3])
                    flags = tag[4].encode('utf8','ignore')
                    contact = BranlyContact(id, type, name, flags)
    
                    if f["value"] != None:
                        contact.setValue(f["value"])
                    curNode.addContact(contact)
        
        if curNode != None:
            nodes.append(curNode)
        
        self.nodes = nodes
        return nodes


    def handlePacket(self, packet):
        """
        Handle Branly style packet.
        Newly found nodes/contacts will be created in the cms.
        Returns True if all went well.
        """
        success = True
        log.debug(packet)
        if self.lastPacketSeqNo != False:
            if packet.seqNo == self.lastPacketSeqNo:
                log.info("Skipping duplicate packet #%d" % packet.seqNo)
                return True # Per definition, all went well
        self.lastPacketSeqNo = packet.seqNo

        # Report RSSI and current time stamp to cms
        if not self.__reportCmsInput(packet.fromAddr, "_time", int(time.time())):
            return False
        if not self.__reportCmsInput(packet.fromAddr, "_rssi", packet.rssi, True):
            return False

        if packet.type == kPacketHello:
            log.debug("CMS got %s" % packet)
        elif packet.type == kPacketPing:
            log.debug("CMS got %s " % packet)
        elif packet.type == kPacketContactList:
            node = self.__findNode(packet.fromAddr)
            if node == None:
                node = BranlyNode(packet.fromAddr, "New node")
                log.debug("New node %s" % node)
                if self.__createCmsNode(node):
                    self.nodes.append(node)
                else:
                    success = False

            # TODO: We currently do not handle contacts changing type
            for contact in packet.contactList:
                newContact = node.findContact(contact["id"])
                if newContact == None:
                    if contact["writeable"]:
                        flags = "w"
                    else:
                        flags = "r"
                    newContact = BranlyContact(contact["id"], contact["type"], "New contact", flags)
                    log.debug("New contact: %s" % newContact)
                    if self.__createCmsNodeContact(node, newContact):
                        node.addContact(newContact)
                    else:
                        success = False

        elif packet.type == kPacketContactReport or packet.type == kPacketContactValue:
            log.debug("CMS got %s" % packet)
            node = self.__findNode(packet.fromAddr)
            if not node:
                log.error("Got contact values from unknown node %d" % (packet.fromAddr))
            else:
                for contactValue in packet.contactValues:
                    contact = node.findContact(contactValue["id"])
                    if contact == None:
                        log.error("Got contact value from unknown contact id %d" % (contact.id))
                    else:
                        log.debug("Contact:%s" % contact)
                        if self.__reportCmsContact(node.id, contactValue["id"], contactValue["value"], contactValue["flags"]):
                            node.setContactValue(contactValue["id"], contactValue["value"], contactValue["flags"])
                        else:
                            success = False

        else:
            log.error("Unknown packet type %d" % (packet.type))

        return success


    """ Private methods below """

    def __findNode(self, branlyNodeId):
        """
        Find given Branly node id or return None if not known to us
        """
        for node in self.nodes:
            if node.id == branlyNodeId:
                return node
        return None


    def __createCmsNode(self, node):
        """
        Create node in the cms.
        TODO: There currently is no node concept in the cms
        Returns True if all went well
        """
        success = True
        return success


    def __createCmsNodeContact(self, node, contact):
        """
        Create contact for given node in the cms.
        Note that we at this point do not know the value of the contact
        Returns True if all went well
        """
        success = False
        j = self.__postInput(node.id, contact.id, 0)
        log.debug("postInput %s" % j)
        if j and "success" in j and "created" in j and "idlist" in j and len(j["idlist"]) > 0 and j["success"]:
            # Create feed for contact
            emonCmsInputId = j["idlist"][0]
            j = self.__createFeed(emonCmsInputId)
            log.debug(j)
            if j and "success" in j and "feedid" in j:
                emonCmsFeedId = j["feedid"]
                j = self.__createProcess(emonCmsInputId, emonCmsFeedId)
                log.debug(j)
                if j and "success" in j and j["success"]:
                    j = self.__setInputProperties(emonCmsInputId, "c%d"%contact.id, "New contact")
                    log.debug(j)
                    if j and "success" in j and j["success"]:
                        # bc:NODE ID:CONTACT ID:TYPE:FLAGS
                        j = self.__setFeedProperties(emonCmsFeedId, "New contact", "bc:%d:%d:%d:%s" % (node.id, contact.id, contact.type, contact.flags))
                        log.debug(j)
                        if j and "success" in j:
                            success = j["success"]
        else:
            log.error("postInput failed : %s" % j)
        
        return success


    def __reportCmsInput(self, nodeId, inputName, inputValue, createFeed = False):
        """
        Report Emoncms input value for given node.
        Optionally creates feed for input
        Returns True if all went well
        """
        success = False
        j = self.__postInputNamed(nodeId, inputName, inputValue)
        if j and "success" in j and "created" in j and j["success"]:
            if createFeed and j["created"]:
                emonCmsInputId = j["idlist"][0]
                j = self.__createFeed(emonCmsInputId)
                if j and "success" in j and "feedid" in j and j["success"]:
                    emonCmsFeedId = j["feedid"]
                    j = self.__createProcess(emonCmsInputId, emonCmsFeedId)
                    if j and "success" in j and j["success"] and j["success"]:
                        j = self.__setFeedProperties(emonCmsFeedId, inputName, "")
                        if j and "success" in j and j["success"]:
                            j = self.__postInputNamed(nodeId, inputName, inputValue)
                            if j and "success" in j:
                                success = j["success"]
            else: # Don't create feed
                success = j["success"]
        return success


    def __reportCmsContact(self, nodeId, contactId, contactValue, contactFlags):
        """
        Report contact value for given node in the cms.
        TODO: Handle contactFlags
        Returns True if all went well
        """
        success = False
        j = self.__postInput(nodeId, contactId, contactValue)
        if j and "success" in j:
            success = j["success"]
        return success


    def __apiCall(self, api, doPost, parameterDict):
        """
        Perform HTTP GET or POST to the API with given parameters
        Returns JSON decoded response data or False in case of errors
        The last HTTP response code can be found in the lastHttpCode member
        """
        ret = False
        parameterDict["apikey"] = self.apiWriteKey
        url = "%s/%s" % (self.serverAddress, api)
        if doPost:
            r = requests.get(url, data=parameterDict)
        else:
            r = requests.get(url, params=parameterDict)
        self.lastHttpCode = r.status_code
        if self.lastHttpCode == 200:
            try:
                ret = r.json()
            except ValueError:
                log.error("No JSON in %s, failed url:%s" % (r.text, r.url))
        else:
            log.error("%s returned %d for url %s" % (api, self.lastHttpCode, r.url))

        if ret and "success" in ret and "message" in ret and not ret["success"]:
            log.error("API call to %s failed with '%s' for %s" % (api, ret["message"], r.url))
        return ret

    def __apiGet(self, api, parameterDict):
        """
        Perform HTTP GET to the API with given parameters
        Returns JSON decoded response data or False in case of errors
        """
        return self.__apiCall(api, False, parameterDict)


    def __apiPost(self, api, parameterDict):
        """
        Perform HTTP POST to the API with given parameters.
        Returns JSON decoded response data or False in case of errors
        """
        return self.__apiCall(api, True, parameterDict)


    def __postInput(self, nodeId, inputId, value):
        """
        Post data to input
        Returns JSON decoded response data or False in case of errors
        """
        # http://localhost/emoncms/input/post.json?node=999&json={contact1:200}
        # {"success":true,"result":true,"created":true,"id":72}
        params = {"node" : nodeId, "json" : "{c%d:%.2f}" % (inputId, value)}
        return self.__apiGet("input/post.json", params)


    def __postInputNamed(self, nodeId, inputName, value):
        """
        Post data to named input
        Returns JSON decoded response data or False in case of errors
        """
        # http://localhost/emoncms/input/post.json?node=999&json={contact1:200}
        # {"success":true,"result":true,"created":true,"id":72}
        params = {"node" : nodeId, "json" : "%s:%.2f}" % (inputName, value)}
        return self.__apiGet("input/post.json", params)


    def __createFeed(self, emonCmsInputId):
        """
        Create feed for given input id
        Returns JSON decoded response data or False in case of errors
        """
        # http://localhost/emoncms/feed/create.json?name=c1_feed&datatype=1&engine=4&options={"interval":"10"}
        # {"success":true,"feedid":61,"result":true}
        params = {"name" : "New contact", "datatype" : 1, "engine" : 4, "options" : "{\"interval\":\"10\"}"}
        return self.__apiGet("feed/create.json", params)


    def __createProcess(self, emonCmsInputId, emonCmsFeedId):
        """
        Create process for given feed id
        Returns JSON decoded response data or False in case of errors
        """
        # Create process, connecting the input 72 to feed 61
        # http://localhost/emoncms/input/process/add.json?inputid=72&processid=1&arg=61
        # {"success":true,"message":"Process added"}
        params = {"inputid" : emonCmsInputId, "processid" : 1, "arg" : emonCmsFeedId}
        return self.__apiGet("input/process/add.json", params)


    def __setInputProperties(self, emonCmsInputId, name, description):
        """
        Set name and description of specified emoncms input id
        Returns JSON decoded response data or False in case of errors
        """
        # http://localhost/emoncms/input/set.json?inputid=95&fields={"name":"contact1","description":"test1"}
        # {"success":true,"message":"Field updated"}
        params = {"inputid" : emonCmsInputId, "fields" : "{\"name\" : \"%s\", \"description\" : \"%s\"}" % (name, description)}
        # TODO: Make emoncms handle nested json input:
#        params = {"inputid" : emonCmsInputId, "fields" : {"name" : name, "description" : description}}
        return self.__apiGet("input/set.json", params)


    def __setFeedProperties(self, emonCmsFeedId, name, tag):
        """
        Set name and tag of specified emoncms input id
        Returns JSON decoded response data or False in case of errors
        """
        # http://localhost/emoncms/feed/set.json?id=61&fields={"name":"feed4","tag":"tag4"}
        # {"success":true,"message":"Field updated"}
        params = {"id" : emonCmsFeedId, "fields" : "{\"name\" : \"%s\", \"tag\" : \"%s\"}" % (name, tag)}
        # TODO: Make emoncms handle nested json input:
#        params = {"id" : emonCmsFeedId, "fields" : {"name" : name, "tag" : tag}}
        return self.__apiGet("feed/set.json", params)


