// **********************************************************************************
// Driver definition for Branly
// **********************************************************************************
// Creative Commons Attrib Share-Alike License
// You are free to use/extend this library but please abide with the CC-BY-SA license:
// http://creativecommons.org/licenses/by-sa/3.0/
// 2014-03-09 (C) johan@kanflo.com
// **********************************************************************************
#include <Branly.h>
#include <EEPROM.h>

Branly::Branly()
{
  _pin = 0;
}

void Branly::setup(RFM69 &radio)
{
  if (get_node_state() == EEPROM_STATE_FACTORY) {
    while (get_node_state() == EEPROM_STATE_FACTORY) {
        enter_zorkmon(radio); // Always enter zorkmon when we're in factory mode as we have no node id etc.
    }
  } else {
    int uart_timer = 25;
    while(uart_timer--) {
      if (Serial.available() > 0) {
        char input = Serial.read();
        if (input == ':') {
          enter_zorkmon(radio);
          break;
        }
      }
      delay(100);
//      Sleepy::loseSomeTime(100);
    }
  }  
}

void Branly::receive_factory_data()
{
  unsigned char network_id, node_id;
  unsigned char node_state = EEPROM_STATE_PROD;
  unsigned char key[17] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
  char buf[80];
  int index = 0;
  while(index < 79) {
    buf[index] = Serial.read();
    buf[index+1] = 0;
    if (buf[index] == ';') break;
    index++;
  }
  sscanf(buf, "%d:%d:%s", &network_id, &node_id, (char*) key);
  Serial.print("# Network:");
  Serial.print(network_id);
  Serial.print(" Node id:");
  Serial.print(node_id);
  Serial.print(" Key:");
  key[16] = 0;
  Serial.println((char*) key);

  factory_write(node_state, network_id, node_id, key);
}


// Factory programming:
/*
f100:nodeid:sampleEncryptKey
*/
void Branly::enter_zorkmon(RFM69 &radio)
{
	char done = 0;
	Serial.println("ZORKMON");
	while(!done) {
		if (Serial.available() > 0) {
			char input = Serial.read();
				switch(input) {
				case ':':
				break;
			case 'r':
				radio.readAllRegs();
				break;
			case 'd':
				factory_dump();
				break;
			case 'f':
				receive_factory_data();
				break;
			case 'q':
				Serial.println("EXIT");
				delay(100);
				return;
			}
		}
		delay(100);
	}
}

int Branly::get_node_state()
{
	if (EEPROM.read(0) == EEPROM_MAGIC0 && EEPROM.read(1) == EEPROM_MAGIC1) {
		return EEPROM.read(EEPROM_STATE_ADDR);
	} else {
		return EEPROM_STATE_FACTORY;
	}
}

void Branly::factory_dump()
{
	Serial.print("EEPROM:");
	for(int i = 0; i < EEPROM_KEY_ADDR + EEPROM_KEY_SIZE; i++) {
		Serial.print(EEPROM.read(i), HEX);
		Serial.print(" ");
	}
	Serial.println("");
}

void Branly::factory_write(unsigned char node_state, unsigned char network_id, unsigned char node_id, unsigned char *key16)
{
	EEPROM.write(EEPROM_STATE_ADDR, node_state);
	EEPROM.write(EEPROM_NETWORK_ADDR, network_id);
	EEPROM.write(EEPROM_NODE_ID_ADDR, node_id);
	for(int i = 0; i < EEPROM_KEY_SIZE; i++) {
		EEPROM.write(EEPROM_KEY_ADDR+i, key16[i]);
	}
	EEPROM.write(0, EEPROM_MAGIC0);
	EEPROM.write(1, EEPROM_MAGIC1);
}

void Branly::factory_read(unsigned char *node_state, unsigned char *network_id, unsigned char *node_id, unsigned char *key16)
{
	*node_state = EEPROM.read(EEPROM_STATE_ADDR);
	*network_id = EEPROM.read(EEPROM_NETWORK_ADDR);
	*node_id = EEPROM.read(EEPROM_NODE_ID_ADDR);
	for(int i = 0; i < EEPROM_KEY_SIZE; i++) {
		key16[i] = EEPROM.read(EEPROM_KEY_ADDR+i);
	}
}