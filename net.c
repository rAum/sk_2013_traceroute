/// Piotr Gródek  241632
/// sieci komputerowe, 2013
///-----------------------------
/// Traceroute network functions
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <netinet/ip_icmp.h>
#include "sockwrap.h"
#include "icmp.h"
#include <unistd.h>
#include <sys/time.h>
#include <sys/select.h>

pid_t pid;
struct sockaddr_in remote_address;
char remote_ip[20];
char observed_ip[3][20];
unsigned char  buffer[IP_MAXPACKET+1];
unsigned char* buffer_ptr;

//--------------------------------------

int create_raw_icmp_socket() {
    return Socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
}


void parse_arg(int argc, char** arg) {
    if (argc != 2){
	    puts ("Usage: ./traceroute <ip>\n");
	    exit(1);
    }

    pid = getpid(); // save pid for later usage

    bzero(&remote_address, sizeof(struct sockaddr_in));
    remote_address.sin_family = AF_INET;
    strncpy(remote_ip, arg[1], sizeof(remote_ip));
    if (!inet_pton(AF_INET, arg[1], &remote_address.sin_addr)) {
        puts("Invalid IP address.");
        exit(1);
    }
}


void send_icmp_packet(int s_icmp, int seq_num, int ttl) {
    struct icmp p_icmp;
    p_icmp.icmp_type = ICMP_ECHO;
    p_icmp.icmp_code = 0;
    p_icmp.icmp_id   = pid;
    p_icmp.icmp_seq  = seq_num;
    p_icmp.icmp_cksum = 0;
    p_icmp.icmp_cksum = in_cksum((u_short*)&p_icmp, 8, 0);

    Setsockopt(s_icmp, IPPROTO_IP, IP_TTL, &ttl, sizeof(int));
    Sendto(s_icmp, &p_icmp, ICMP_HEADER_LEN, 0, &remote_address, sizeof(struct sockaddr_in));
}


void ping(int s_icmp, int ttl) {
    int seq = 3 * ttl;
    send_icmp_packet(s_icmp, seq,   ttl);
    send_icmp_packet(s_icmp, seq+1, ttl);
    send_icmp_packet(s_icmp, seq+2, ttl);
}


int valid_seq_num(int ttl, int seq) {
    register int s = seq - 3 * ttl;
    return s >= 0 && s < 3;
}



int read_icmp(int s_icmp, int ttl) {
    struct sockaddr_in sender;
    socklen_t sender_len = sizeof(sender);

    buffer_ptr = buffer; // reset pointer
    Recvfrom(s_icmp, buffer_ptr, IP_MAXPACKET, 0, &sender, &sender_len);

    struct ip* packet = (struct ip*) buffer_ptr;
    buffer_ptr += packet->ip_hl * 4; // skip IP   header
    struct icmp* p_icmp = (struct icmp*) buffer_ptr;
    buffer_ptr += ICMP_HEADER_LEN;   // skip ICMP header

    if (p_icmp->icmp_type == ICMP_TIME_EXCEEDED && p_icmp->icmp_code == ICMP_EXC_TTL) {
        struct ip* p_orig = (struct ip*) buffer_ptr; // orignal header
        if (p_orig->ip_p == IPPROTO_ICMP) {
            buffer_ptr += p_orig->ip_hl * 4; // skip orignal IP header
            p_icmp = (struct icmp*) buffer_ptr;
        } else {
            return -1; // nothing interesing
        }
    } else if (p_icmp->icmp_type != ICMP_ECHOREPLY) {
        return -1;
    }

    // check if packet was send by this host
    if (p_icmp->icmp_id == pid && valid_seq_num(ttl, p_icmp->icmp_seq)) {
        int seq = p_icmp->icmp_seq - 3 * ttl; // can be 0, 1, 2
        inet_ntop(AF_INET, &(sender.sin_addr), observed_ip[seq], sizeof(observed_ip[0]));
        if (strcmp(observed_ip[seq], remote_ip) == 0) seq += 8;
        return seq;
    }

    return -1;
}


void pong(int s_icmp, int ttl, int* avg, int* received, int*reached_target) {
    int res;
    struct timeval start, end;

    *received = 0;
    *avg      = 0;

    for (int i = 0; i < 3; ++i)
        memset(observed_ip[i], 0, sizeof(observed_ip[0]));

    gettimeofday(&start, NULL);
    for (;;) {
        struct timeval timeout;
        timeout.tv_sec  = 1;
        timeout.tv_usec = 0;

        fd_set desc;
        FD_ZERO(&desc);
        FD_SET(s_icmp, &desc);

        int ready = Select(s_icmp+1, &desc, NULL, NULL, &timeout); // sadly Linux does not decrease timeout :(

        if (ready == 0) // timeout!
            break;

        if (ready > 0) {
            res = read_icmp(s_icmp, ttl);
            if (res >= 0) {
                gettimeofday(&end, NULL);
                int diff = (int) ((end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_usec - start.tv_usec)/1000.0 + 0.5);
                *avg += diff;
                (*received)++;
                if (res & 0x8)
                    (*reached_target) = 1;
            }
        }
    }
}
