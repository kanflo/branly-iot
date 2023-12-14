// **********************************************************************************
// Driver definition for Branly
// **********************************************************************************
// Creative Commons Attrib Share-Alike License
// You are free to use/extend this library but please abide with the CC-BY-SA license:
// http://creativecommons.org/licenses/by-sa/3.0/
// 2014-03-09 (C) johan@kanflo.com
// **********************************************************************************
#ifndef BRANLY_h
#define BRANLY_h
#include <Arduino.h>            //assumes Arduino IDE v1.0 or greater
#include <RFM69.h>

#define REPORT_TYPE_TEMP   (1)
#define REPORT_TYPE_BATT   (2)
#define REPORT_TYPE_ENERGY (3)
#define REPORT_TYPE_BOOT   (4)
#define REPORT_TYPE_PLANT  (5)

#define EEPROM_MAGIC0 (0xde)
#define EEPROM_MAGIC1 (0xad)

#define EEPROM_STATE_FACTORY (0)
#define EEPROM_STATE_INSTALLER (1)
#define EEPROM_STATE_PROD (2)

#define EEPROM_STATE_ADDR (2)
#define EEPROM_NETWORK_ADDR (3)
#define EEPROM_NODE_ID_ADDR (4)
#define EEPROM_KEY_ADDR (5)
#define EEPROM_KEY_SIZE (16)


typedef struct {
  unsigned char type;
  unsigned char seq_no;
} branly_report_t;


#define TEMP_INVALID (-100)

typedef struct {
  branly_report_t br;
  int radio_temp;
  int temp2;
  int temp1;
} temp_report_t;

typedef struct {
  branly_report_t br;
  unsigned char vcc;
} battery_report_t;

typedef struct {
  branly_report_t br;
  unsigned int duration;
  unsigned int counter;
} energy_report_t;

typedef struct {
  branly_report_t br;
  unsigned char mcusr;
  unsigned char version;
  unsigned char ex_fuse;
} boot_report_t;

typedef struct {
  branly_report_t br;
  int temperature;
  unsigned int moisture;
  unsigned char alarm;
} plant_report_t;



class Branly
{
  public:
    Branly();
  	void setup(RFM69 &radio);
	void enter_zorkmon(RFM69 &radio);
    int get_node_state();
	void factory_dump();
	void factory_write(unsigned char node_state, unsigned char network_id, unsigned char node_id, unsigned char *key16);
	void factory_read(unsigned char *node_state, unsigned char *network_id, unsigned char *node_id, unsigned char *key16);
  private:
	void receive_factory_data();
    int _pin;
};

#endif // BRANLY_h