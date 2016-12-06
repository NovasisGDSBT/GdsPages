#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>

/* I2C Defines */
#define I2CDEV                  "/dev/i2c-2"
#define DEV_FAIL_ORETVAL        -1
#define DEV_FAIL_IRETVAL        -2

#define MAX11607_I2CADDR        0x68
#define MAX11607_FAIL_WRETVAL   -33
#define MAX11607_FAIL_RRETVAL   -34

#define MAX11607_REG_SETUP      0x80
#define MAX11607_SETUP_SEL2     0x40
#define MAX11607_SETUP_SEL1     0x20
#define MAX11607_SETUP_SEL0     0x10
#define MAX11607_SETUP_CLK      0x08
#define MAX11607_SETUP_BIP_UNI  0x04
#define MAX11607_SETUP_RST      0x02
#define MAX11607_SETUP_X        0x01

#define MAX11607_REG_CONFIG     0x00
#define MAX11607_CONFIG_SCAN1   0x40
#define MAX11607_CONFIG_SCAN0   0x20
#define MAX11607_CONFIG_CS3     0x10
#define MAX11607_CONFIG_CS2     0x08
#define MAX11607_CONFIG_CS1     0x04
#define MAX11607_CONFIG_CS0     0x02
#define MAX11607_CONFIG_SGL     0x01

#define TSL2550_I2CADDR         0x72
#define TSL2550_FAIL_WRETVAL    -21
#define TSL2550_FAIL_RRETVAL    -22


#define LM77_I2CADDR            0x90
#define LM77_FAIL_WRETVAL       -11
#define LM77_FAIL_RRETVAL       -12

/* GPIO */
#define GPIO_PORT1_BASE         0*32
#define GPIO_PORT2_BASE         1*32
#define GPIO_PORT3_BASE         2*32
#define GPIO_PORT4_BASE         3*32
#define GPIO_PORT5_BASE         4*32
#define GPIO_PORT6_BASE         5*32
/* Outputs */
#define SW_FAULT                GPIO_PORT5_BASE + 31    /* 159 */
#define URL_COM                 GPIO_PORT6_BASE + 2     /* 162 */
#define OVERTEMP                GPIO_PORT6_BASE + 3     /* 163 */
#define PANEL_LIGHT_FAIL        GPIO_PORT6_BASE + 5     /* 165 */
#define ETH_SWITCH              GPIO_PORT1_BASE + 24    /* 56 */
/* Inputs */
#define CFG_BIT0                GPIO_PORT5_BASE + 20
#define CFG_BIT1                GPIO_PORT1_BASE + 2
#define CFG_BIT2                GPIO_PORT3_BASE + 28
#define CFG_BIT3                GPIO_PORT3_BASE + 30
#define IRQ_TSET                GPIO_PORT6_BASE + 4
#define IRQ_TCRIT               GPIO_PORT5_BASE + 19
#define BACKLIGHT_FAULT         GPIO_PORT5_BASE + 30
#define RTC_IRQ                 GPIO_PORT5_BASE + 18

/* External definitions */

extern  unsigned char advalues[8];
extern  unsigned char buf[32];
extern  int           read_buf[10],light_value;
extern  unsigned char lvds_ptr[256];

extern  void set_max11607_device(int file);
extern  unsigned char max11607_setup(int file);
extern  void max11607_get_values(int file);
extern  int max11607_get_light(void);
extern  int bkg_algo(void);
extern  int do_light(void);
extern  int max11607_get_voltage24(void);
extern  int max11607_get_voltage12(void);

extern  int set_tls2550_device(int file);
extern  unsigned char set_tls2550_powerup(int file);
extern  unsigned char set_tls2550_powerdown(int file);
extern  int tls2550_get_light(int file);

extern  void set_lm77_device(int file);
extern  short read_lm77_read_temperature(int file);

extern  void gpio_init(void);

//#define DEBUG
