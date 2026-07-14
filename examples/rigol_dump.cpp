#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern "C" {
#include "lib/vxi11/library/vxi11_user.h"
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        printf("usage: %s <ip>\n", argv[0]);
        return 0;
    }

    VXI11_CLINK *dev = NULL;
    char buf[4096];
    char *ip = argv[1];

    if (vxi11_open_device(&dev, ip, NULL) != 0) {
        printf("VXI11 open failed\n");
        return -1;
    }

    printf("connected OK\n");

    vxi11_send(dev, (char *)"*IDN?", 5);
    int r = vxi11_receive(dev, buf, sizeof(buf));
    buf[r] = 0;
    printf("IDN: %s\n", buf);

    vxi11_send(dev, (char *)":WAV:SOUR CHAN1", 15);
    vxi11_send(dev, (char *)":WAV:MODE NORM", 14);
    vxi11_send(dev, (char *)":WAV:FORM BYTE", 14);
    vxi11_send(dev, (char *)":WAV:POIN 1000", 14);

    vxi11_send(dev, (char *)":WAV:PRE?", 9);
    r = vxi11_receive(dev, buf, sizeof(buf));
    buf[r] = 0;
    printf("PRE: %s\n", buf);

    r = vxi11_send_and_receive(dev,
        (char *)":WAV:DATA?",
        buf,
        sizeof(buf),
        10 * VXI11_READ_TIMEOUT);

    printf("RAW length = %d\n", r);

    if (buf[0] != '#') {
        printf("ERROR: no IEEE header\n");
        return 0;
    }

    int nd = buf[1] - '0';
    int len = 0;
    for (int i = 0; i < nd; i++)
        len = 10 * len + (buf[2 + i] - '0');

    int off = 2 + nd;

    printf("IEEE block: nd=%d len=%d off=%d\n", nd, len, off);

    for (int i = 0; i < 32; i++)
        printf("%02x ", (unsigned char)buf[off + i]);
    printf("\n");

    for (int i = 0; i < 16; i++)
        printf("%4d ", (int)((unsigned char)buf[off + i]));
    printf("\n");

    vxi11_close_device(dev, ip);
    return 0;
}
