#define MAX_NAME_LEN    32      /* for client and port name */

typedef struct RtJackBuffer_ {
#ifdef LINUX
    pthread_mutex_t csndLock;               /* signaled by process callback */
    pthread_mutex_t jackLock;               /* signaled by audio thread     */
#else
    void    *csndLock;                      /* signaled by process callback */
    void    *jackLock;                      /* signaled by audio thread     */
#endif
    jack_default_audio_sample_t **inBufs;   /* 'nChannels' capture buffers  */
    jack_default_audio_sample_t **outBufs;  /* 'nChannels' playback buffers */
} RtJackBuffer;

typedef struct RtJackGlobals_ {
    CSOUND  *csound;                    /* Csound instance pointer          */
    int     jackState;                  /* 0: OK, 1: sr changed, 2: quit    */
    char    clientName[MAX_NAME_LEN + 1];       /* client name              */
    char    inputPortName[MAX_NAME_LEN + 1];    /* input port name prefix   */
    char    outputPortName[MAX_NAME_LEN + 1];   /* output port name prefix  */
    int     sleepTime;                  /* sleep time in us (deprecated)    */
    char    *inDevName;                 /* device name for -i adc           */
    char    *outDevName;                /* device name for -o dac           */
    int     sampleRate;                 /* sample rate in Hz                */
    int     nChannels;                  /* number of channels               */
    int     nChannels_i;                /* number of in channels            */
    int     bufSize;                    /* buffer size in sample frames     */
    int     nBuffers;                   /* number of buffers (>= 2)         */
    int     inputEnabled;               /* non-zero if capture (adc) is on  */
    int     outputEnabled;              /* non-zero if playback (dac) is on */
    int     csndBufCnt;                 /* current buffer in Csound thread  */
    int     csndBufPos;                 /* buffer position in Csound thread */
    int     jackBufCnt;                 /* current buffer in JACK callback  */
    int     jackBufPos;                 /* buffer position in JACK callback */
    jack_client_t   *client;            /* JACK client pointer              */
    jack_port_t     **inPorts;          /* 'nChannels' ports for capture    */
    jack_default_audio_sample_t **inPortBufs;
    jack_port_t     **outPorts;         /* 'nChannels' ports for playback   */
    jack_default_audio_sample_t **outPortBufs;
    RtJackBuffer    **bufs;             /* 'nBuffers' I/O buffers           */
    int     xrunFlag;                   /* non-zero if an xrun has occured  */
    jack_client_t   *listclient;
    int outDevNum, inDevNum;            /* select devs by number */
} RtJackGlobals;
