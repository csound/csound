#if defined(__sgi) || defined(LINUX)
#include <netinet/in.h>
#endif

struct NetworkReturnAddressStruct {
    struct sockaddr_in  cl_addr;
    int clilen;
    int sockfd;
};
