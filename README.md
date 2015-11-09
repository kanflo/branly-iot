# Branly IoT

"Branly" is the name given to my personal adventure into the world of IoT. [Édouard Branly](https://en.wikipedia.org/wiki/Édouard_Branly) was one of the early pioneers in wireless telegraphy. This project will span everything between software design/architecture and hardware design.

This git holds the following:

* [hardware](https://github.com/kanflo/branly-iot/hardware) is my first steps at designing hardware. In this case an RFM69 based modem for the RaspberryPi. There is one SMT version and one "through hole" version that most should be able to solder. The zip files contains the gerbers.
* [BranlyPi modem](https://github.com/kanflo/branly-iot/branly_pi_modem) is Arduino based software for the above hardware.
* [Branly node](https://github.com/kanflo/branly-iot/branly_node) is an Arduino node that talks to the BranlPi modem.
* [Branly Gateway](https://github.com/kanflo/branly-iot/branly-gateway) is a gateway that receives data from the BranlyPi modem and posts it to EmonCMS.
