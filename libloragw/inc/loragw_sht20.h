#ifndef _LORAGW_SHT20_H
#define _LORAGW_SHT20_H

#include <stdint.h>     /* C99 types */

int sht2x_init(void);
int sht2x_softreset(int fd);
int sht2x_get_serialnumber(int fd, uint8_t *serialnumber, int size);
int sht2x_get_temperature(int fd, float *temp);

#endif
