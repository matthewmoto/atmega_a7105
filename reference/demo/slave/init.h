#include         "global.h"
/****************************************/
    /*  declare const */
#define    rs_1k2bps         0x01
#define    rs_2k4bps         0x02
#define    rs_4k8bps         0x03
#define    rs_9k6bps         0x04
#define    rs_19k2bps        0x05
#define    rs_38k4bps        0x06                /* 串口速率 */

#define    mode_config       0x01
#define    mode_commun       0x00

#define    config_read       0xc8
#define    config_read_ack   0xf8
#define    config_write      0xf8
#define    config_write_ack  0xaa                /* 与上层软件命令接口 */

#define    rs232_over_1k2bps 0x0a
#define    rs232_over_2k4bps 0x06
#define    rs232_over_4k8bps 0x04
#define    rs232_over_9k6bps 0x03
#define    rs232_over_19k2bps 0x02
#define    rs232_over_38k4bps 0x02
/****************************************/
    /*  declare variable */
    
/****************************************/
    /* declare founction */
void       PowerOn_Initialisation(void);
