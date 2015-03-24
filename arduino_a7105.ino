/*
  Issue: device not talking back
    * tried shiftin/shiftout spi (get data from GIO2S in its default state)
    * swapping devices (same results on each)
    * checked pins on breakout from chip pins on datasheet, my connections all match...

  To try:
    * copy software SPI from the demo code (probably do this next)
    * try the SPI code in python via the ftdi breakout

 */

#include <SPI.h>

//Pin used to activate recoding on my logic analyzer...
#define RECORD 4

#define RADIO_SELECT_PIN 9
#define RADIO_MOSI 8
#define RADIO_MISO 7
#define RADIO_SCK 6

#define GIO1S 0x0B
#define GIO2S 0x0C
#define ENABLE_4WIRE 0x19
#define MODE_CONTROL 0x01
#define FIFO_1 0x03
#define RADIO_CLOCK 0x0d
#define DATA_RATE 0x0e
#define TX_II 0x2b
#define RX 0x62
#define RX_GAIN_I 0x80
#define RX_GAIN_IV 0x0a

// the setup function runs once when you press reset or power the board
void setup() {
  Serial.begin(9600);

  Serial.println("Starting...");

  pinMode(RADIO_SELECT_PIN, OUTPUT);
  //pinMode(RADIO_MISO,INPUT);
  //pinMode(RADIO_MOSI,OUTPUT);
  //pinMode(RADIO_SCK, OUTPUT);
  pinMode(MOSI,OUTPUT);
  pinMode(MISO,INPUT);
  pinMode(SCK,OUTPUT);
  pinMode(RECORD,OUTPUT);

  digitalWrite(RECORD,LOW);
  digitalWrite(RADIO_SELECT_PIN,HIGH);
  delay(5000);

  // initialize digital pin 13 as an output.
  //pinMode(13, OUTPUT);

  //Set up the SPI bus
  SPI.begin(); 

  //soft reset the radio
  write_reg(0x00,0x00);
  
  
  Serial.println("Reset chip, waiting");
  delay(500);
  Serial.println("done");

  //enable 4-wire SPI
  write_reg(GIO1S,ENABLE_4WIRE,1);

  //write_reg(MODE_CONTROL,0x063);

  //write_reg(FIFO_1,0x0f);

  write_reg(RADIO_CLOCK,0x05);

  byte sanity_check = read_reg(RADIO_CLOCK);
  Serial.print("Sanity check: ");
  Serial.println(sanity_check,HEX);

}
void write_reg(byte reg,byte val)
{
  write_reg(reg,val,0);
}
void write_reg(byte reg, byte val, int record)
{
  //digitalWrite(RADIO_SCK,LOW);
  if (record)
    digitalWrite(RECORD,HIGH);

                                   
  digitalWrite(RADIO_SELECT_PIN,LOW);
  SPI.beginTransaction(SPISettings(500000, MSBFIRST, SPI_MODE0));  // gain control of SPI bus

  //shiftOut(RADIO_MOSI,RADIO_SCK, MSBFIRST, reg);
  //shiftOut(RADIO_MOSI,RADIO_SCK, MSBFIRST, val);

  SPI.transfer(reg);
  SPI.transfer(val);

  SPI.endTransaction();          // release the SPI bus
  digitalWrite(RADIO_SELECT_PIN,HIGH);
  digitalWrite(RECORD,LOW);

}

byte read_reg(byte reg)
{
  //digitalWrite(RADIO_SCK,LOW);
  byte command = reg | 0x40;
  digitalWrite(RADIO_SELECT_PIN,LOW);
  SPI.beginTransaction(SPISettings(500000, MSBFIRST, SPI_MODE0));  // gain control of SPI bus

  //shiftOut(RADIO_MOSI,RADIO_SCK, MSBFIRST, command);
  //byte read_byte = shiftIn(RADIO_MISO,RADIO_SCK,MSBFIRST);
  SPI.transfer(command);
  byte read_byte = SPI.transfer(0x00);

  SPI.endTransaction();          // release the SPI bus
  digitalWrite(RADIO_SELECT_PIN,HIGH);
  return read_byte;
}

// the loop function runs over and over again forever
void loop() {
  delay(100000000);
  //Serial.print("RADIO_CLOCK register: ");
  //Serial.print(read_reg(RADIO_CLOCK),HEX);
  //Serial.println("");
  //digitalWrite(13, HIGH);   // turn the LED on (HIGH is the voltage level)
  //delay(100000000);              // wait for a second
  //digitalWrite(13, LOW);    // turn the LED off by making the voltage LOW
  //delay(100000000);              // wait for a second
}
