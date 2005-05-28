/*
Copyright (c) 1992,1996,1998.
The Regents of the University of California (Regents).
All Rights Reserved.

Permission to use, copy, modify, and distribute this software and its
documentation for educational, research, and not-for-profit purposes, without
fee and without a signed licensing agreement, is hereby granted, provided that
the above copyright notice, this paragraph and the following two paragraphs
appear in all copies, modifications, and distributions.  Contact The Office of
Technology Licensing, UC Berkeley, 2150 Shattuck Avenue, Suite 510, Berkeley,
CA 94720-1620, (510) 643-7201, for commercial licensing opportunities.

Written by Adrian Freed, The Center for New Music and Audio Technologies,
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
*/

 /* htmsocket.c

        Adrian Freed
        send parameters to htm servers by udp or UNIX protocol

    Modified 6/6/96 by Matt Wright to understand symbolic host names
    in addition to X.X.X.X addresses.
 */

#include <stdio.h>
#include <unistd.h>
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
#include <sys/fcntl.h>
#include <sys/file.h>
#include <sys/time.h>
#include <sys/types.h>

#include <stdlib.h>

#define UNIXDG_PATH "/tmp/htm"
#define UNIXDG_TMP "/tmp/htm.XXXXXX"
#include "htmsocket.h"
typedef struct
{
        float srate;

        struct sockaddr_in serv_addr; /* udp socket */
        struct sockaddr_un     userv_addr; /* UNIX socket */
        int sockfd;             /* socket file descriptor */
        int index, len,uservlen;
        void *addr;
        int id;
} desc;

/* open a socket for HTM communication to given  host on given portnumber */
/* if host is 0 then UNIX protocol is used (i.e. local communication */
void *OpenHTMSocket(char *host, int portnumber)
{
        int sockfd;
        struct sockaddr_in  cl_addr;
        struct sockaddr_un  ucl_addr;
        desc *o;
        o = malloc(sizeof(*o));
        if(!o)
                return 0;
        if(!host)
        {
                char *mktemp(char *);
                int clilen;
                  o->len = sizeof(ucl_addr);
                /*
                         * Fill in the structure "userv_addr" with the address of the
                         * server that we want to send to.
                */

                bzero((char *) &o->userv_addr, sizeof(o->userv_addr));
                       o->userv_addr.sun_family = AF_UNIX;
                strcpy(o->userv_addr.sun_path, UNIXDG_PATH);
                        sprintf(o->userv_addr.sun_path+strlen(o->userv_addr.sun_path), "%d", portnumber);
                o->uservlen = sizeof(o->userv_addr.sun_family) + strlen(o->userv_addr.sun_path);
                o->addr = &(o->userv_addr);
                /*
                * Open a socket (a UNIX domain datagram socket).
                */

                if ( (sockfd = socket(AF_UNIX, SOCK_DGRAM, 0)) >= 0)
                {
                        /*
                         * Bind a local address for us.
                         * In the UNIX domain we have to choose our own name (that
                         * should be unique).  We'll use mktemp() to create a unique
                         * pathname, based on our process id.
                         */

                        bzero((char *) &ucl_addr, sizeof(ucl_addr));    /* zero out */
                        ucl_addr.sun_family = AF_UNIX;
                        strcpy(ucl_addr.sun_path, UNIXDG_TMP);

                        mktemp(ucl_addr.sun_path);
                        clilen = sizeof(ucl_addr.sun_family) + strlen(ucl_addr.sun_path);

                        if (bind(sockfd, (struct sockaddr *) &ucl_addr, clilen) < 0)
                        {
                                perror("client: can't bind local address");
                                close(sockfd);
                                sockfd = -1;
                        }
                }
                else
                        perror("unable to make socket\n");

        }else
        {
                /*
                         * Fill in the structure "serv_addr" with the address of the
                         * server that we want to send to.
                */
                o->len = sizeof(cl_addr);
                bzero((char *)&o->serv_addr, sizeof(o->serv_addr));
                o->serv_addr.sin_family = AF_INET;

            /* MW 6/6/96: Call gethostbyname() instead of inet_addr(),
               so that host can be either an Internet host name (e.g.,
               "les") or an Internet address in standard dot notation
               (e.g., "128.32.122.13") */
            {
                struct hostent *hostsEntry;
                unsigned long address;

                hostsEntry = gethostbyname(host);
                if (hostsEntry == NULL) {
                    fprintf(stderr, "Couldn't decipher host name \"%s\"\n",
                            host);
                    herror(NULL);
                    return 0;
                }

                address = *((unsigned long *) hostsEntry->h_addr_list[0]);
                o->serv_addr.sin_addr.s_addr = address;
            }

            /* was: o->serv_addr.sin_addr.s_addr = inet_addr(host); */

            /* End MW changes */

                        o->serv_addr.sin_port = htons(portnumber);
                o->addr = &(o->serv_addr);
                /*
                * Open a socket (a UDP domain datagram socket).
                */
                if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) >= 0)
                {
                        bzero((char *)&cl_addr, sizeof(cl_addr));
                        cl_addr.sin_family = AF_INET;
                        cl_addr.sin_addr.s_addr = htonl(INADDR_ANY);
                        cl_addr.sin_port = htons(0);

                        if(bind(sockfd, (struct sockaddr *) &cl_addr, sizeof(cl_addr)) < 0)
                        {
                                perror("could not bind\n");
                                close(sockfd);
                                sockfd = -1;
                        }
                }
                else
                {
                        perror("unable to make socket\n");
                }

        }
        if(sockfd<0)
        {
                free(o); o = 0;
        }
        else
                o->sockfd = sockfd;
        return o;
}
#include <errno.h>

static  bool sendudp(const struct sockaddr *sp, int sockfd,int length, int count, void  *b)
{
        int rcount;
        if((rcount=sendto(sockfd, b, count, 0, sp, length)) != count)
        {
/*      printf("sockfd %d count %d rcount %dlength %d errno %d\n", sockfd,count,rcount,length,
                        errno); */
                        return FALSE;
        }
        return TRUE;
}
bool SendHTMSocket(void *htmsendhandle, int length_in_bytes, void *buffer)
{
        desc *o = (desc *)htmsendhandle;
        return sendudp(o->addr, o->sockfd, o->len, length_in_bytes, buffer);
}
void CloseHTMSocket(void *htmsendhandle)
{
        desc *o = (desc *)htmsendhandle;
        close(o->sockfd);
        free(o);
}
