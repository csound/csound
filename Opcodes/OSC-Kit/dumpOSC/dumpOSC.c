/*
Copyright (c) 1992,1993,1994,1995,1996,1997,2000.  
The Regents of the University of California (Regents).
All Rights Reserved.

Permission to use, copy, modify, and distribute this software and its
documentation for educational, research, and not-for-profit purposes, without
fee and without a signed licensing agreement, is hereby granted, provided that
the above copyright notice, this paragraph and the following two paragraphs
appear in all copies, modifications, and distributions.  Contact The Office of
Technology Licensing, UC Berkeley, 2150 Shattuck Avenue, Suite 510, Berkeley,
CA 94720-1620, (510) 643-7201, for commercial licensing opportunities.

Written by Matt Wright and Adrian Freed, The Center for New Music and Audio
Technologies, University of California, Berkeley.

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

  /* 

     dumpOSC.c
	server that displays OpenSoundControl messages sent to it
	for debugging client udp and UNIX protocol

     by Matt Wright, 6/3/97
       modified from dumpSC.c, by Matt Wright and Adrian Freed

     version 0.2: Added "-silent" option a.k.a. "-quiet"

     version 0.3: Incorporated patches from Nicola Bernardini to make
       things Linux-friendly.  Also added ntohl() in the right places
       to support little-endian architectures.
 
     1/13/03 Make it compile on OSX.

	compile:
		cc -o dumpOSC dumpOSC.c

	to-do:

	    More robustness in saying exactly what's wrong with ill-formed
	    messages.  (If they don't make sense, show exactly what was
	    received.)

	    Time-based features: print time-received for each packet

	    Clean up to separate OSC parsing code from socket/select stuff

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

#ifdef NEED_SCHEDCTL_AND_LOCK
#include <sys/schedctl.h>
#include <sys/lock.h>
#endif

#if defined(__APPLE__) && defined(__GNUC__)
#define OSX
#endif

char *htm_error_string;
typedef int Boolean;
typedef void *OBJ;

typedef struct ClientAddressStruct {
        struct sockaddr_in  cl_addr;
        int clilen;
        int sockfd;
} *ClientAddr;

Boolean ShowBytes = FALSE;
Boolean Silent = FALSE;

/* Declarations */
static int unixinitudp(int chan);
static int initudp(int chan);
static void closeudp(int sockfd);
Boolean ClientReply(int packetsize, void *packet, int socketfd, 
	void *clientaddresspointer, int clientaddressbufferlength);
void sgi_CleanExit(void);
Boolean sgi_HaveToQuit(void);
int RegisterPollingDevice(int fd, void (*callbackfunction)(int , void *), void *dummy);
static void catch_sigint();
static int Synthmessage(char *m, int n, void *clientdesc, int clientdesclength, int fd) ;
void ParseOSCPacket(char *buf, int n, ClientAddr returnAddr);
static void Smessage(char *address, void *v, int n, ClientAddr returnAddr);
static void PrintTypeTaggedArgs(void *v, int n);
static void PrintHeuristicallyTypeGuessedArgs(void *v, int n, int skipComma);
char *DataAfterAlignedString(char *string, char *boundary) ;
Boolean IsNiceString(char *string, char *boundary) ;
void complain(char *s, ...);

extern int errno;

#define UNIXDG_PATH "/tmp/htm"

static int unixinitudp(int chan)
{
	struct sockaddr_un serv_addr;
	int  sockfd;
	int filenameLength;
	
	if((sockfd = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0)
			return sockfd;
	
	bzero((char *)&serv_addr, sizeof(serv_addr));
	serv_addr.sun_family = AF_UNIX;
	strcpy(serv_addr.sun_path, UNIXDG_PATH);

	sprintf(serv_addr.sun_path+strlen(serv_addr.sun_path), "%d", chan);

	unlink(serv_addr.sun_path);

	filenameLength = sizeof(serv_addr.sun_family)+strlen(serv_addr.sun_path);

#ifdef OSX
	/* Somehow the bind() call on OSX creates a file with the last character missing */
	++filenameLength;
#endif

	if(bind(sockfd, (struct sockaddr *) &serv_addr, filenameLength) < 0)
	{
		perror("unable to  bind\n");
		return -1;
	}

	fcntl(sockfd, F_SETFL, FNDELAY); 
	return sockfd;
}

static int initudp(int chan)
{
	struct sockaddr_in serv_addr;
	int  sockfd;
	
	if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
			return sockfd;
	bzero((char *)&serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(chan);
	
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

static Boolean catchupflag=FALSE;
Boolean ClientReply(int packetsize, void *packet, int socketfd, 
	void *clientaddresspointer, int clientaddressbufferlength)
{
	if(!clientaddresspointer) return FALSE;
	catchupflag= TRUE;
	return packetsize==sendto(socketfd, packet, packetsize, 0, clientaddresspointer, clientaddressbufferlength);
}

static Boolean exitflag= FALSE;
void sgi_CleanExit(void) {
	exitflag = TRUE;
}

Boolean sgi_HaveToQuit(void) {
	return exitflag;
}


/* file descriptor poll table */
static int npolldevs =0;
typedef struct polldev
{
	int fd;
	void (*callbackfunction)(int , void *);
	void *dummy;
} polldev;
#define TABMAX 8
static polldev polldevs[TABMAX];


/*      Register a device (referred to by a file descriptor that the caller
	should have already successfully obtained from a system call) to be
	polled as real-time constraints allowed.
        
        When a select(2) call indicates activity on the file descriptor, the
	callback function is called with the file descripter as first
	argument and the given dummy argument (presumably a pointer to the
	instance variables associated with the device).
*/
int RegisterPollingDevice(int fd, void (*callbackfunction)(int , void *), void *dummy)
{
	if(npolldevs<TABMAX)
	{
		polldevs[npolldevs].fd = fd;
		polldevs[npolldevs].callbackfunction = callbackfunction;
		polldevs[npolldevs].dummy = dummy;
	}
	else return -1;
	return npolldevs++;
}

static int caught_sigint;

static void catch_sigint()  {
   caught_sigint = 1;
}

static int sockfd, usockfd;



static int Synthmessage(char *m, int n, void *clientdesc, int clientdesclength, int fd)  {
    struct ClientAddressStruct ras;
    ClientAddr ra = &ras;

    catchupflag= FALSE;

    ras.cl_addr = *((struct sockaddr_in *) clientdesc);
    ras.clilen = clientdesclength;
    ras.sockfd = fd;

    if (ShowBytes) {
	int i;
	printf("%d byte message:\n", n);
	for (i = 0; i < n; ++i) {
	    printf(" %x (%c)\t", m[i], m[i]);
	    if (i%4 == 3) printf("\n");
	}
	printf("\n");
    }

    ParseOSCPacket(m, n, ra);
    return catchupflag;
}

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


void ParseOSCPacket(char *buf, int n, ClientAddr returnAddr) {
    int size, messageLen, i;
    char *messageName;
    char *args;

#ifdef PRINTADDRS
    PrintClientAddr(returnAddr);
#endif


    if ((n%4) != 0) {
	complain("SynthControl packet size (%d) not a multiple of 4 bytes: dropping",
		 n);
	return;
    }

    if ((n >= 8) && (strncmp(buf, "#bundle", 8) == 0)) {
	/* This is a bundle message. */

	if (n < 16) {
	    complain("Bundle message too small (%d bytes) for time tag", n);
	    return;
	}

	/* Print the time tag */
	printf("[ %lx%08lx\n", ntohl(*((unsigned long *)(buf+8))),
	       ntohl(*((unsigned long *)(buf+12))));
	/* Note: if we wanted to actually use the time tag as a little-endian
	   64-bit int, we'd have to word-swap the two 32-bit halves of it */

	i = 16; /* Skip "#group\0" and time tag */
	while(i<n) {
	    size = ntohl(*((int *) (buf + i)));
	    if ((size % 4) != 0) {
		complain("Bad size count %d in bundle (not a multiple of 4)", size);
		return;
	    }
	    if ((size + i + 4) > n) {
		complain("Bad size count %d in bundle (only %d bytes left in entire bundle)",
			 size, n-i-4);
		return;	
	    }
	    
	    /* Recursively handle element of bundle */
	    ParseOSCPacket(buf+i+4, size, returnAddr);
	    i += 4 + size;
	}
	if (i != n) {
	    complain("This can't happen");
	}
	printf("]\n");
    } else {
	/* This is not a bundle message */

	messageName = buf;
	args = DataAfterAlignedString(messageName, buf+n);
	if (args == 0) {
	    complain("Bad message name string: %s\nDropping entire message.\n",
		     htm_error_string);
	    return;
	}
	messageLen = args-messageName;	    
	Smessage(messageName, (void *)args, n-messageLen, returnAddr);
    }
}

#define SMALLEST_POSITIVE_FLOAT 0.000001f

static void Smessage(char *address, void *v, int n, ClientAddr returnAddr) {
    char *chars = v;

    printf("%s ", address);

    if (n != 0) {
	if (chars[0] == ',') {
	    if (chars[1] != ',') {
		/* This message begins with a type-tag string */
		PrintTypeTaggedArgs(v, n);
	    } else {
		/* Double comma means an escaped real comma, not a type string */
		PrintHeuristicallyTypeGuessedArgs(v, n, 1);
	    }
	} else {
	    PrintHeuristicallyTypeGuessedArgs(v, n, 0);
	}
    }

    printf("\n");
    fflush(stdout);	/* Added for Sami 5/21/98 */
}

static void PrintTypeTaggedArgs(void *v, int n) { 
    char *typeTags, *thisType;
    char *p;

    typeTags = v;

    if (!IsNiceString(typeTags, typeTags+n)) {
	/* No null-termination, so maybe it wasn't a type tag
	   string after all */
	PrintHeuristicallyTypeGuessedArgs(v, n, 0);
	return;
    }

    p = DataAfterAlignedString(typeTags, typeTags+n);


    for (thisType = typeTags + 1; *thisType != 0; ++thisType) {
	switch (*thisType) {
	    case 'i': case 'r': case 'm': case 'c':
	    printf("%d ", ntohl(*((int *) p)));
	    p += 4;
	    break;

	    case 'f': {
		int i = ntohl(*((int *) p));
		float *floatp = ((float *) (&i));
		printf("%f ", *floatp);
		p += 4;
	    }
	    break;

	    case 'h': case 't':
	    printf("[A 64-bit int] ");
	    p += 8;
	    break;

	    case 'd':
	    printf("[A 64-bit float] ");
	    p += 8;
	    break;

	    case 's': case 'S':
	    if (!IsNiceString(p, typeTags+n)) {
		printf("Type tag said this arg is a string but it's not!\n");
		return;
	    } else {
		printf("\"%s\" ", p);
		p = DataAfterAlignedString(p, typeTags+n);
	    }
	    break;

	    case 'T': printf("[True] "); break;
	    case 'F': printf("[False] "); break;
	    case 'N': printf("[Nil]"); break;
	    case 'I': printf("[Infinitum]"); break;

	    default:
	    printf("[Unrecognized type tag %c]", *thisType);
	    return;
	}
    }
}

static void PrintHeuristicallyTypeGuessedArgs(void *v, int n, int skipComma) {
    int i, thisi;
    float thisf;
    int *ints;
    char *chars;
    char *string, *nextString;
	

    /* Go through the arguments 32 bits at a time */
    ints = v;
    chars = v;

    for (i = 0; i<n/4; ) {
	string = &chars[i*4];
	thisi = ntohl(ints[i]);
	/* Reinterpret the (potentially byte-reversed) thisi as a float */
	thisf = *(((float *) (&thisi)));

	if  (thisi >= -1000 && thisi <= 1000000) {
	    printf("%d ", thisi);
	    i++;
	} else if (thisf >= -1000.f && thisf <= 1000000.f &&
		   (thisf <=0.0f || thisf >= SMALLEST_POSITIVE_FLOAT)) {
	    printf("%f ",  thisf);
	    i++;
	} else if (IsNiceString(string, chars+n)) {
	    nextString = DataAfterAlignedString(string, chars+n);
	    printf("\"%s\" ", (i == 0 && skipComma) ? string +1 : string);
	    i += (nextString-string) / 4;
	} else {
	    printf("0x%x ", ints[i]);
	    i++;
	}
    }
}


#define STRING_ALIGN_PAD 4

char *DataAfterAlignedString(char *string, char *boundary) 
{
    /* The argument is a block of data beginning with a string.  The
       string has (presumably) been padded with extra null characters
       so that the overall length is a multiple of STRING_ALIGN_PAD
       bytes.  Return a pointer to the next byte after the null
       byte(s).  The boundary argument points to the character after
       the last valid character in the buffer---if the string hasn't
       ended by there, something's wrong.

       If the data looks wrong, return 0, and set htm_error_string */

    int i;

    if ((boundary - string) %4 != 0) {
	fprintf(stderr, "Internal error: DataAfterAlignedString: bad boundary\n");
	return 0;
    }

    for (i = 0; string[i] != '\0'; i++) {
	if (string + i >= boundary) {
	    htm_error_string = "DataAfterAlignedString: Unreasonably long string";
	    return 0;
	}
    }

    /* Now string[i] is the first null character */
    i++;

    for (; (i % STRING_ALIGN_PAD) != 0; i++) {
	if (string + i >= boundary) {
	    htm_error_string = "DataAfterAlignedString: Unreasonably long string";
	    return 0;
	}
	if (string[i] != '\0') {
	    htm_error_string = "DataAfterAlignedString: Incorrectly padded string.";
	    return 0;
	}
    }

    return string+i;
}

Boolean IsNiceString(char *string, char *boundary) 
{
    /* Arguments same as DataAfterAlignedString().  Is the given "string"
       really a string?  I.e., is it a sequence of isprint() characters
       terminated with 1-4 null characters to align on a 4-byte boundary? */

    int i;

    if ((boundary - string) %4 != 0) {
	fprintf(stderr, "Internal error: IsNiceString: bad boundary\n");
	return 0;
    }

    for (i = 0; string[i] != '\0'; i++) {
	if (!isprint(string[i])) return FALSE;
	if (string + i >= boundary) return FALSE;
    }

    /* If we made it this far, it's a null-terminated sequence of printing characters 
       in the given boundary.  Now we just make sure it's null padded... */

    /* Now string[i] is the first null character */
    i++;
    for (; (i % STRING_ALIGN_PAD) != 0; i++) {
	if (string[i] != '\0') return FALSE;
    }

    return TRUE;
}







#define MAXMESG 32768
static char mbuf[MAXMESG];

int main(int argc, char **argv) {
    int udp_port;		/* port to receive parameter updates from */
		
#ifdef OSX
    struct sockaddr cl_addr;
    struct sockaddr ucl_addr;
#else
    struct sockaddr_in  cl_addr;
    struct sockaddr_un  ucl_addr;
#endif

    int clilen,maxclilen=sizeof(cl_addr);

    int uclilen,umaxclilen=sizeof(ucl_addr);
    int i,n;
	
    clilen = maxclilen;
    uclilen = umaxclilen;

    udp_port = -1;
    for (i=1; i < argc; ++i) {
	if (strcmp(argv[i], "-showbytes") == 0) {
	    ShowBytes = TRUE;
	} else if (strcmp(argv[i], "-silent") == 0 ||
		   strcmp(argv[i], "-quiet") == 0) {
	    Silent = TRUE;
	} else if (udp_port != -1) {
	    goto usageError;
	} else {
	    udp_port = atoi(argv[i]);
	    if (udp_port == 0) {
		goto usageError;
	    }
	}
    }

    if (udp_port == -1) {
	usageError:
	    fprintf(stderr, "Usage\n\tdumpOSC portno [-showbytes] [-quiet]\n\t(responds to udp and UNIX packets on that port no)\n");
	    exit(1);
    }


	n = recvfrom(0, mbuf, MAXMESG, 0, &cl_addr, &clilen);
	if(n>0)
	{
		sockfd = 0;
		udp_port = -1;
		Synthmessage(mbuf, n, &cl_addr, clilen,sockfd) ;
	}
	else
	{	sockfd=initudp(udp_port);
		usockfd=unixinitudp(udp_port);
	}

    if (!Silent) {
	printf("dumpOSC version 0.2 (6/18/97 Matt Wright). Unix/UDP Port %d \n", udp_port);	
	printf("Copyright (c) 1992,96,97,98,99,2000,01,02,03 Regents of the University of California.\n");
    }

    if (sockfd < 0) {
      perror("initudp");
    } else if (usockfd < 0) {
      perror("unixinitudp");
    } else {
		fd_set read_fds, write_fds;
		int nfds;
#define max(a,b) (((a) > (b)) ? (a) : (b))
		nfds = max(sockfd, usockfd)+ 1;
		{
			int j;
			for(j=0;j<npolldevs;++j)
				if(polldevs[j].fd>=nfds)
				{
						nfds = polldevs[j].fd+1;
/*
printf("polldev %d\n", polldevs[j].fd);
*/
				}
		}
/*
		printf("nfds %d\n", nfds);
*/
		caught_sigint = 0;

		/* Set signal handler */
#ifdef OSX
		signal(SIGINT, catch_sigint);
#else
   		sigset(SIGINT, catch_sigint);
#endif
	
		while(!caught_sigint)
		{
			
			int r;
 	
		back:	

			FD_ZERO(&read_fds);                /* clear read_fds        */
			FD_ZERO(&write_fds);               /* clear write_fds        */
			FD_SET(sockfd, &read_fds); 
			FD_SET(usockfd, &read_fds); 
		       	{
		        	int j;
		        	
		        	for(j=0;j<npolldevs;++j)
		        		FD_SET(polldevs[j].fd, &read_fds);
		        }
 	      
		        r = select(nfds, &read_fds, &write_fds, (fd_set *)0, 
		                        (struct timeval *)0);
		        if (r < 0)  /* select reported an error */
			  return;
		        {
			    int j;
			    
			    for(j=0;j<npolldevs;++j)
				    if(FD_ISSET(polldevs[j].fd, &read_fds))
				    (*(polldevs[j].callbackfunction))(polldevs[j].fd,polldevs[j].dummy );
		        }
		        if(FD_ISSET(sockfd, &read_fds))
			{
				clilen = maxclilen;
				while( (n = recvfrom(sockfd, mbuf, MAXMESG, 0, &cl_addr, &clilen)) >0) 
				{
				  int r;
				  /* printf("received UDP packet of length %d\n",  n); */
				  r = Synthmessage(mbuf, n, &cl_addr, clilen, sockfd) ;

				  if( sgi_HaveToQuit()) return;
				  if(r>0) goto back;
				  clilen = maxclilen;
				}
			}
		        if(FD_ISSET(usockfd, &read_fds))
			{
				uclilen = umaxclilen;
				while( (n = recvfrom(usockfd, mbuf, MAXMESG, 0, &ucl_addr, &uclilen)) >0) 
				{
				  int r;
				  /* printf("received UNIX packet of length %d\n",  n); */

				  r=Synthmessage(mbuf, n, &ucl_addr, uclilen,usockfd) ;

				  if( sgi_HaveToQuit()) return;
				  if(r>0) goto back;
				  uclilen = umaxclilen;
				}
			}
		} /* End of while(!caught_sigint) */
    }
	
    return 0;
}


#include <stdarg.h>
void complain(char *s, ...) {
    va_list ap;
    va_start(ap, s);
    fprintf(stderr, "*** ERROR: ");
    vfprintf(stderr, s, ap);
    fprintf(stderr, "\n");
    va_end(ap);
}
