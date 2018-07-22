#UPLOAD_PORT = /dev/ttyUSB1
#BOARD = esp210
#SKETCH = $(ESP_ROOT)/libraries/Ticker/examples/TickerBasic/TickerBasic.ino
#include $(HOME)/makeEspArduino/makeEspArduino.mk

BOARD = d1_mini
ESP_ROOT=$(HOME)/git/public/esp8266

#CUSTOM_LIBS=libs/
HTTP_ADDR = garage-back.lazog.undo.it
HTTP_URI = /firmware
HTTP_USR = toby
#HTTP_PWD = xxxx

include $(HOME)/git/public/makeEspArduino/makeEspArduino.mk
