/* rtNetBSD.h */

#ifndef _RTNETBSD_INCLUDED
#define _RTNETBSD_INCLUDED

#define NETBSD_RECORD 0
#define NETBSD_PLAY   1
#define NETBSD_DUPLEX 2

#undef USE_SETSCHEDULER 

#define NETBSD_MIXER "/dev/mixer0"
#define NETBSD_SAMPLER "/dev/sound0"

#endif /* _RTNETBSD_INCLUDED */
