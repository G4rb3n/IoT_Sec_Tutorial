#include <stdio.h>
#include <stdlib.h>

#define MIB_IP_ADDR 170
#define MIB_HW_VER 0x250
#define MIB_CAPTCHA 0x2C1

int apmib_init(void)
{
    return 1;
}

int fork(void)
{
    return 0;
}

void apmib_get(int code, int* value)
{
    switch(code)
    {
        case MIB_HW_VER:
            *value = 0xF1;
            break;
        case MIB_IP_ADDR:
            *value = 0x7F000001;
            break;
        case MIB_CAPTCHA:
            *value = 1;
            break;
    }
}

