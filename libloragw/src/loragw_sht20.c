#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <sys/stat.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <stdint.h>
/* https://stackoverflow.com/questions/3875197/gcc-with-std-c99-complains-about-not-knowing-struct-timespec */
#define __USE_POSIX199309
#include <time.h>
#include <errno.h>
#include <string.h>
#include <loragw_sht20.h>
#define SOFTRESET                   0xFE
#define TRIGGER_TEMPERATURE_NO_HOLD 0xF3
#define TRIGGER_HUMIDITY_NO_HOLD    0xF5
#define I2C_API_RDWR /* Use I2C userspace driver read/write API */

static inline void msleep(unsigned long ms)
{
    struct timespec cSleep;
    unsigned long ulTmp;
    cSleep.tv_sec = ms / 1000;

    if (cSleep.tv_sec == 0) {
        ulTmp = ms * 10000;
        cSleep.tv_nsec = ulTmp * 100;
    }
    else {
        cSleep.tv_nsec = 0;
    }
    nanosleep(&cSleep, 0);
}

static inline void dump_buf(const char *prompt, uint8_t *buf, int size)
{
    int i;

    if (!buf)
        return;

    if (prompt)
        printf("%s ", prompt);

    for (i = 0; i < size; i++)
        printf("%02x ", buf[i]);

    printf("\n");

    return;
}

int sht2x_init(void)
{
    int fd;

    if ((fd = open("/dev/i2c-2", O_RDWR)) < 0) {
        printf("i2c device open failed: %s\n", strerror(errno));
        return -1;
    }
    /* set I2C mode and SHT2x slave address */
    ioctl(fd, I2C_TENBIT, 0);   /* Not 10-bit but 7-bit mode */
    ioctl(fd, I2C_SLAVE, 0x40); /* sht20 device address ix 0x40 */

    return fd;
}

int sht2x_softreset(int fd)
{
    uint8_t buf[4];

    if (fd < 0) {
        printf("%s line [%d] %s() get invalid input arguments\n", __FILE__, __LINE__, __func__ );
        return -1;
    }
    /* software reset SHT2x */
    memset(buf, 0, sizeof(buf));
    buf[0] = SOFTRESET;
    write(fd, buf, 1);
    msleep(50);

    return 0;
}

int sht2x_get_serialnumber(int fd, uint8_t *serialnumber, int size)
{
    uint8_t buf[4];

    if (fd < 0 || !serialnumber || size != 8) {
        printf("%s line [%d] %s() get invalid input arguments\n", __FILE__, __LINE__, __func__ );
        return -1;
    }
    memset(buf, 0, sizeof(buf));
    buf[0] = 0xfa;
    buf[1] = 0x0f;
    write(fd, buf, 2);

    memset(buf, 0, sizeof(buf));
    read(fd, buf, 4);

    serialnumber[5]=buf[0]; /*  Read SNB_3 */
    serialnumber[4]=buf[1]; /*  Read SNB_2 */
    serialnumber[3]=buf[2]; /*  Read SNB_1 */
    serialnumber[2]=buf[3]; /*  Read SNB_0 */

    memset(buf, 0, sizeof(buf) );
    buf[0]=0xfc; /*  command for readout on-chip memory */
    buf[1]=0xc9; /*  on-chip memory address */
    write(fd, buf, 2);

    memset(buf, 0, sizeof(buf) );
    read(fd, buf, 4);
    serialnumber[1]=buf[0]; /*  Read SNC_1 */
    serialnumber[0]=buf[1]; /*  Read SNC_0 */
    serialnumber[7]=buf[2]; /*  Read SNA_1 */
    serialnumber[6]=buf[3]; /*  Read SNA_0 */
    dump_buf("SHT2x Serial number: ", serialnumber, 8);

    return 0;
}

int sht2x_get_temperature(int fd, float *temp)
{
    uint8_t buf[4];

    if (fd < 0 || !temp) {
        printf("%s line [%d] %s() get invalid input arguments\n", __FILE__, __LINE__, __func__ );
        return -1;
    }
    /* send trigger temperature measure command and read the data */
    memset(buf, 0, sizeof(buf));
    buf[0]=TRIGGER_TEMPERATURE_NO_HOLD;
    write(fd, buf, 1); // transmit a temperature monitor command

    msleep(85); /* datasheet: typ=66, max=85 */
    memset(buf, 0, sizeof(buf));
    read(fd, buf, 3); // read transmit in byte
    *temp = 175.72 * (((((int) buf[0]) << 8) + buf[1]) / 65536.0) - 46.85;

    return 0;
}
