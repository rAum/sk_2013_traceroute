#ifndef __ICMP_H
#define __ICMP_H

#define ICMP_HEADER_LEN 8

u_short in_cksum(const u_short *addr, register int len, u_short csum);

#endif

