/*
Copyright (c) 1998.  The Regents of the University of California (Regents).
All Rights Reserved.

Permission to use, copy, modify, and distribute this software and its
documentation for educational, research, and not-for-profit purposes, without
fee and without a signed licensing agreement, is hereby granted, provided that
the above copyright notice, this paragraph and the following two paragraphs
appear in all copies, modifications, and distributions.  Contact The Office of
Technology Licensing, UC Berkeley, 2150 Shattuck Avenue, Suite 510, Berkeley,
CA 94720-1620, (510) 643-7201, for commercial licensing opportunities.

Written by Matt Wright, The Center for New Music and Audio Technologies,
University of California, Berkeley.

     IN NO EVENT SHALL REGENTS BE LIABLE TO ANY PARTY FOR DIRECT, INDIRECT,
     SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, INCLUDING LOST PROFITS,
     ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF
     REGENTS HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

     REGENTS SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING, BUT NOT
     LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
     FOR A PARTICULAR PURPOSE. THE SOFTWARE AND ACCOMPANYING
     DOCUMENTATION, IF ANY, PROVIDED HEREUNDER IS PROVIDED "AS IS".
     REGENTS HAS NO OBLIGATION TO PROVIDE MAINTENANCE, SUPPORT, UPDATES,
     ENHANCEMENTS, OR MODIFICATIONS.

dumpUDP.c: smallest UDP receiving application
by Matt Wright, 9/9/98

*/


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <rpc/rpc.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/times.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pwd.h>
#include <signal.h>
#include <grp.h>
#include <sys/file.h>
#include <bstring.h>
#include <sys/prctl.h>
#include <sys/schedctl.h>
#include <sys/lock.h>


typedef struct ClientAddressStruct {
        struct sockaddr_in  cl_addr;
        int clilen;
        int sockfd;
} *ClientAddr;

void PrintClientAddr(ClientAddr CA) {
    unsigned long addr = CA->cl_addr.sin_addr.s_addr;
    printf("Client address %p:\n", CA);
    printf("  clilen %d, sockfd %d\n", CA->clilen, CA->sockfd);
    printf("  sin_family %d, sin_port %d\n", CA->cl_addr.sin_family,
	   CA->cl_addr.sin_port);
    printf("  address: (%x) %s\n", addr,  inet_ntoa(CA->cl_addr.sin_addr));

    printf("  sin_zero = \"%c%c%c%c%c%c%c%c\"\n", 
	   CA->cl_addr.sin_zero[0],
	   CA->cl_addr.sin_zero[1],
	   CA->cl_addr.sin_zero[2],
	   CA->cl_addr.sin_zero[3],
	   CA->cl_addr.sin_zero[4],
	   CA->cl_addr.sin_zero[5],
	   CA->cl_addr.sin_zero[6],
	   CA->cl_addr.sin_zero[7]);

    printf("\n");
}


static int initudp(int port) {
	struct sockaddr_in serv_addr;
	int n, sockfd;
	
	if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
			return sockfd;
	bzero((char *)&serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(port);
	
	if(bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
	{
		perror("unable to  bind\n");
		return -1;
	}

	fcntl(sockfd, F_SETFL, FNDELAY); 
	return sockfd;
}


static void closeudp(int sockfd) {
    close(sockfd);
}


static int time_to_quit;

static void catch_sigint()  {
   time_to_quit = 1;
}

void GotAPacket(char *buf, int n, ClientAddr returnAddr) {
    printf("received UDP packet of length %d\n",  n);
    PrintClientAddr(returnAddr);
}

#define MAXMESG 32768
static char mbuf[MAXMESG];

void ReceivePacket(int sockfd) {
	struct ClientAddressStruct returnAddress;
	int maxclilen=sizeof(returnAddress.cl_addr);
	int n;

	returnAddress.clilen = maxclilen;
	while( (n = recvfrom(sockfd, mbuf, MAXMESG, 0, &(returnAddress.cl_addr),
			     &(returnAddress.clilen))) >0)  {
	    GotAPacket(mbuf, n, &returnAddress);

	    if (time_to_quit) return;
	    returnAddress.clilen = maxclilen;
	}
}

void main(int argc,  char **argv) {
	int udp_port;  /* port to receive parameter updates from */
	int sockfd;
	int i;

	fd_set read_fds, write_fds;
	int nfds;
		
	udp_port = 7000;

	sockfd=initudp(udp_port);

	if(sockfd<0) {
	    perror("initudp");
	    return;
	}

	nfds = sockfd + 1;

	time_to_quit = 0;
	sigset(SIGINT, catch_sigint);       /* set sig handler       */

	while(!time_to_quit)
	{
		
		int c,r;

	back:	

		FD_ZERO(&read_fds);                /* clear read_fds        */
		FD_ZERO(&write_fds);               /* clear write_fds        */
		FD_SET(sockfd, &read_fds); 

      
		r = select(nfds, &read_fds, &write_fds, (fd_set *)0, 
				(struct timeval *)0);
		if (r < 0)  /* select reported an error */
		    goto out;

		if(FD_ISSET(sockfd, &read_fds)) {
		    ReceivePacket(sockfd);
		}

	} /* End of while(!time_to_quit) */
out: ;
}
