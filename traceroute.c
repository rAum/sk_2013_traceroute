/// Piotr Gródek  241632
/// sieci komputerowe, 2013
///-----------------------------
/// Simple traceroute.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern void print(int ttl, int avg, int recived);
extern void ping(int s_icmp, int ttl);
extern void pong(int s_icmp, int ttl, int* avg, int* received, int*reached_target);
extern int  create_raw_icmp_socket();
extern void parse_arg(int argc, char** arg);

extern char observed_ip[3][20];

//------------------------------

void print(int ttl, int avg, int recived);

//--------------------------------

int main (int argc, char** argv) {
    parse_arg(argc, argv);

	int s_icmp = create_raw_icmp_socket();
	int reached = 0, received, avg;

	for (int ttl = 1; ttl <= 30 && !reached; ttl++) {
	    ping(s_icmp, ttl);
        pong(s_icmp, ttl, &avg, &received, &reached);
        print(ttl, avg, received);
	}

	return 0;
}


void print(int ttl, int avg, int received) {
    char res[129];
    char* p = res;

    if (received == 0) sprintf(res, "%15s", "*");
    else {
        p += sprintf(p, "%15s ", observed_ip[0]);
        if (strcmp(observed_ip[0], observed_ip[1]) != 0)
            p += sprintf(p, "%15s ", observed_ip[1]);
        if ((strcmp(observed_ip[0], observed_ip[2]) != 0) && (strcmp(observed_ip[1], observed_ip[2]) != 0) )
            p += sprintf(p, "%15s ", observed_ip[2]);

        if (received != 3) strcpy(p, " ???");
        else              sprintf(p, "%4.1d ms", avg / received);
    }

    printf("%2.0d. %s\n", ttl, res);

}
