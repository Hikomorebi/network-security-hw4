#ifndef HELPFUNC_H
#define HELPFUNC_H
#include "common.h"


unsigned short in_cksum(unsigned short *ptr, int nbytes);
bool IsPortOK(unsigned bPort,unsigned ePort);
unsigned int GetLocalHostIP(void);
bool Ping(string HostIP,unsigned LocalHostIP);

#endif