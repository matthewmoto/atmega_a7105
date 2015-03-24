/****************************************/
     /*  declare PortB */
__io union{
             uchar   PINB;
             struct{
                     uchar  inputBbit0:1,
                            inputBbit1:1,
                            inputBbit2:1,
                            inputBbit3:1,
                            inputBbit4:1,
                            inputBbit5:1,
                            inputBbit6:1,
                            inputBbit7:1;
                    };
           } @ 0x03;
__io union{
             uchar   DDRB;
             struct{
                     uchar  ddrBbit0:1,
                            ddrBbit1:1,
                            ddrBbit2:1,
                            ddrBbit3:1,
                            ddrBbit4:1,
                            ddrBbit5:1,
                            ddrBbit6:1,
                            ddrBbit7:1;
                    };
           } @ 0x04;
__io union{
             uchar   PORTB;
             struct{
                     uchar  portBbit0:1,
                            portBbit1:1,
                            portBbit2:1,
                            portBbit3:1,
                            portBbit4:1,
                            portBbit5:1,
                            portBbit6:1,
                            portBbit7:1;
                    };
           } @ 0x05;
/****************************************/
     /*  declare PortC */
__io union{
             uchar   PINC;
             struct{
                     uchar  inputCbit0:1,
                            inputCbit1:1,
                            inputCbit2:1,
                            inputCbit3:1,
                            inputCbit4:1,
                            inputCbit5:1,
                            inputCbit6:1,
                            inputCbit7:1;
                    };
           } @ 0x06;
__io union{
             uchar   DDRC;
             struct{
                     uchar  ddrCbit0:1,
                            ddrCbit1:1,
                            ddrCbit2:1,
                            ddrCbit3:1,
                            ddrCbit4:1,
                            ddrCbit5:1,
                            ddrCbit6:1,
                            ddrCbit7:1;
                    };
           } @ 0x07;
__io union{
             uchar   PORTC;
             struct{
                     uchar  portCbit0:1,
                            portCbit1:1,
                            portCbit2:1,
                            portCbit3:1,
                            portCbit4:1,
                            portCbit5:1,
                            portCbit6:1,
                            portCbit7:1;
                    };
           } @ 0x08;
/****************************************/
     /*  declare PortD */
__io union{
             uchar   PIND;
             struct{
                     uchar  inputDbit0:1,
                            inputDbit1:1,
                            inputDbit2:1,
                            inputDbit3:1,
                            inputDbit4:1,
                            inputDbit5:1,
                            inputDbit6:1,
                            inputDbit7:1;
                    };
           } @ 0x09;
__io union{
             uchar   DDRD;
             struct{
                     uchar  ddrDbit0:1,
                            ddrDbit1:1,
                            ddrDbit2:1,
                            ddrDbit3:1,
                            ddrDbit4:1,
                            ddrDbit5:1,
                            ddrDbit6:1,
                            ddrDbit7:1;
                    };
           } @ 0x0a;
__io union{
             uchar   PORTD;
             struct{
                     uchar  portDbit0:1,
                            portDbit1:1,
                            portDbit2:1,
                            portDbit3:1,
                            portDbit4:1,
                            portDbit5:1,
                            portDbit6:1,
                            portDbit7:1;
                    };
           } @ 0x0b;
