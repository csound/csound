
/*
 * Menu IDs.
 */
#define APPLE_ID                1               /* Apple */

#define FILE_ID
        #define F_QUIT          1

#define EDIT_ID                 3               /* Edit */
        //#define       EDIT_UNDO       1
        //--------line------2
        #define EDIT_CUT        1
        #define EDIT_COPY       2
        #define EDIT_PASTE      3
        #define EDIT_CLEAR      4

/*
 * Dialog IDs and item numbers
 */
#define ABOUT_ID                130

#define OPEN_DLG_ID     128
#define OPEN_KILL       2
#define OPEN_SEL_IN     3
#define OPEN_SEL_NOI    4
#define OPEN_SEL_OUT    5
#define OPEN_IN_TEX     6
#define OPEN_NOI_TEX    7
#define OPEN_OUT_TEX    8
#define OPEN_MORE       11
#define OPEN_NN         12
#define OPEN_W          13
#define OPEN_MM         14
#define OPEN_L          15
#define OPEN_FORMAT     16
        #define POPUP_NO                1
        #define POPUP_SDII              2
        #define POPUP_AIFF              3
        #define POPUP_WAV               4
        #define POPUP_IRCAM             5  //###TODO###
#define OPEN_WIDTH      17
        #define POPUP_8                 1
        #define POPUP_8_US              2
#ifdef never
        #define POPUP_ALAW              -2 // "not yet implemented"
#endif
#ifdef ULAW
        #define POPUP_ULAW              3
#endif
        #define POPUP_16                4
        #define POPUP_32                5
        #define POPUP_32F               6
#define OPEN_HEART      18
#define OPEN_REWRITE    19
#define OPEN_VERBOSE    20
#define OPEN_D          21
#define OPEN_B          22
#define OPEN_BB         23
#define OPEN_E          24
#define OPEN_EE         25
#define OPEN_T          26
#define OPEN_S          27
#define OPEN_N          28
#define OPEN_M          29
#define OPEN_SEL_LST    30
#define OPEN_LST_TEX    31
#define OPEN_CHK_LST    32

#define FMT_DLG_ID              131
#define FMT_8                   4
#ifdef never
#define FMT_ALAW                5
#endif
#ifdef ULAW
#define FMT_ULAW                6
#endif
#define FMT_16                  7
#define FMT_32                  8
#define FMT_32F                 9
#define FMT_NOHDR               10
#define FMT_BLKSIZE             12
#define FMT_AIFF                14

#define ENABLE_CTL              0
#define DISABLE_CTL             255

#define IN_VALID                0x01    /* set if score file valid */
#define NOI_VALID               0x02    /* set if orchestra file valid */
#define LST_VALID               0x08    /* set if there's a listing file */

// Alerts
#define ALERT_ID                228


// Max Length of a path name
#define PATH_LEN        128
// Max Length of csound command
#define MAX_COMMAND 1024

/*
 * Currently selected files. The in and noise files are necessary,
 * and each may come from a different directory.
 * The listing file is optional and may be in any directory.
 */
typedef struct {
        char in[PATH_LEN];      /* score file name */
        short in_vrn;           /* score volume reference number */
        long  in_pid;           /* the parID of sco used for appleevents -ead */
        char noise[PATH_LEN];   /* orchestra file name */
        short noise_vrn;                /* orchestra volume reference number */
        long  noise_pid;                /* the parID of sco used for appleevents -ead */
        char out[PATH_LEN];     /* output sound file name */
        char lst[PATH_LEN];     /* listing (stdout) file name */
        short lst_vrn;          /* listing file volume reference number */
        long lst_pid;           // listing file parid -matt
        int  flags;
} SO_FILE;

#define SCO_VALID               0x01    /* set if score file valid */
#define ORC_VALID               0x02    /* set if orchestra file valid */
#define LST_VALID               0x08    /* set if there's a listing file */
#define MIDI_VALID              0x10    /* set if there's a MIDI file */
#define SND_VREF_VALID  0x20    /* set if snd vrefnum (sfdir) valid */
#define BAT_VALID               0x40    /* set if batch is turned on */

/*
 * Output format, and sound editor signature.
 */
typedef struct {
        int type;                       /* output format type - same as item number */
        int header;                     /* header type */
} OUT_FMT;

#define OF_VERBOSE  (0x1)
#define OF_REWRITE  (0x2)
