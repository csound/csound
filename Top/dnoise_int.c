/*  
    dnoise_int.c:

    Copyright (C) 2000 John ffitch, matt ingalls

    This file is part of Csound.

    The Csound Library is free software; you can redistribute it
    and/or modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    Csound is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with Csound; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
*/

/* ******************************************************************** */
/* ******** Macintosh Dialogs ***************************************** */
/* ******************************************************************** */
#include        <Menus.h>
#include        <Dialogs.h>
#include        <QuickDraw.h>
#include        <Windows.h>
#include        <SegLoad.h>
#include        <Resources.h>
#include        <Memory.h>
#include        <Events.h>
#include        "resource.h"
#include        "cs.h"

void UpdateFileDlg(SO_FILE);
void formCmdLine(void);
#define MAX_ARG         32
char *gargv[MAX_ARG];
int gargc = 0;
long endp = 0;
DialogPtr gFileDlg = NULL;
char ssdir_path[PATH_LEN];
char sfdir_path[PATH_LEN];
char sadir_path[PATH_LEN];
char incdir_path[PATH_LEN];


/*
 * Current command line.
 */

char cmd_line[MAX_COMMAND] = "";

Boolean shortDisplay = true;
SO_FILE so_file = {
        "", 0, 0,
        "", 0, 0,
        "test.aiff",
        "", 0, 0,
        0
};

/* The variables which are given values in dialogs */
long opt_flags = 0;
MYFLT threshold = FL(30.0);
MYFLT m_g0 = -FL(40.0);
int n_aver = 5;
MYFLT b_beg = -FL(1.0);
int B_beg = -1;
int D_dec = -1;
int E_end = -1;
int L_synthlen = -1;
int M_anallen = -1;
int N_nfilts = -1;
int S_sharp = -1;
int w_ovlp = -1;
MYFLT e_end = -FL(1.0);
// output format options.
OUT_FMT out_fmt;

static int ok_enabled = FALSE;          /* TRUE if allow \r to mean OK button */

/*
 * Text that appears in "About CSound..." dialog.
 */
char *info[] = {
(char *)"\p      A Power Macintosh Front End Application to \"Dnoise\"",
(char *)"\p       2001 Development by John ffitch after matt ingalls",
(char *)"\p",
(char *)"\p               Copyright 2000 by John ffitch.",
(char *)"\p                    No rights reserved.",
(char *)"\p",
};
#define N_INFO (sizeof(info) / sizeof(char *))

/*============================================================================
 * Process "About CSound..." dialog.
 ============================================================================*/
void doAboutBox(void)
{
    DialogPtr dp;
    short itemHit;
    short itemType;
    Rect itemBox;
    Handle item;
    FontInfo fi;
    int height;
    int i;

    dp = GetNewDialog(ABOUT_ID, 0, (WindowPtr)-1);
    SetPort(dp);
    TextFont(kFontIDMonaco);
    TextSize(9);
    GetFontInfo(&fi);
    height = fi.ascent + fi.descent + fi.leading;
    GetDialogItem(dp,2,&itemType,&item,&itemBox);
    for (i = 0; i < N_INFO; i++) {
      MoveTo(itemBox.left,itemBox.top + (i + 1) * height);
      DrawString((StringPtr)info[i]);
    }
    TextFont(systemFont);
    TextSize(12);
    ModalDialog(0, &itemHit);
    DisposeDialog(dp);
}


/*============================================================================
 * Set the contents of a dialog text item via sprintf.
 ============================================================================*/
void SetDlgStrItem(DialogPtr dp, short itemHit, char *fmt)
{
    short itemType;
    Rect itemBox;
    Handle item;

    GetDialogItem(dp,itemHit,&itemType,&item,&itemBox);
    SetDialogItemText(item,CtoPstr(fmt));
}

/*============================================================================
 * Scan the contents of a dialog text item via sscanf.
 ============================================================================*/
void GetDlgStrItem(DialogPtr dp, short itemHit, char *fmt, char *p)
{
    short itemType;
    Rect itemBox;
    Handle item;
    char buf[256];

    GetDialogItem(dp,itemHit,&itemType,&item,&itemBox);
    GetDialogItemText(item,(StringPtr)buf);
    PtoCstr((StringPtr)buf);
    sscanf(buf,fmt,p);
}

/*============================================================================
 * Get the contents of a dialog string item. Note that
 * you cannot use GetDlgStrItem() if the text contains spaces.
 ============================================================================*/
void GetDlgString(DialogPtr dp, short itemHit, char *p)
{
    short itemType;
    Rect itemBox;
    Handle item;

    GetDialogItem(dp,itemHit,&itemType,&item,&itemBox);
    GetDialogItemText(item,(StringPtr)p);
    PtoCstr((StringPtr)p);
}


/*============================================================================
 * Event filter for dlgFileOpen() modal dialog.
 ============================================================================*/
pascal Boolean myFilter(DialogPtr dp, EventRecord *ev, short *itemHit)
{
#define ENTER 0x03

    if (ev->what == keyDown)
      switch(ev->message & charCodeMask) {
      case '\r':
      case ENTER:
        *itemHit = 1;
        return ok_enabled;

      default:
        return FALSE;
      }
    else return FALSE;
}


// Redraw the file open dialog box

void UpdateFileDlg(SO_FILE tmp)
{
    short itemType;
    Rect itemBox;
    Handle item;

    /* display current parameters and*/
    /* frame rectangles around static text items */
    GetDialogItem(gFileDlg,OPEN_IN_TEX,&itemType,&item,&itemBox);
    SetDlgStrItem(gFileDlg,OPEN_IN_TEX,tmp.in);

    GetDialogItem(gFileDlg,OPEN_NOI_TEX,&itemType,&item,&itemBox);
    SetDlgStrItem(gFileDlg,OPEN_NOI_TEX,tmp.noise);

    GetDialogItem(gFileDlg,OPEN_OUT_TEX,&itemType,&item,&itemBox);
    SetDlgStrItem(gFileDlg,OPEN_OUT_TEX,tmp.out);

    GetDialogItem(gFileDlg,OPEN_LST_TEX,&itemType,&item,&itemBox);
    if (so_file.flags & LST_VALID) {
      SetDlgStrItem(gFileDlg,OPEN_LST_TEX,tmp.lst);
      GetDialogItem(gFileDlg,OPEN_CHK_LST,&itemType,&item,&itemBox);
      SetControlValue((ControlHandle)item,1);
      GetDialogItem(gFileDlg,OPEN_SEL_LST,&itemType,&item,&itemBox);
      HiliteControl((ControlHandle)item, ENABLE_CTL);
    }
    else {
      SetDlgStrItem(gFileDlg,OPEN_LST_TEX,tmp.lst);
      GetDialogItem(gFileDlg,OPEN_CHK_LST,&itemType,&item,&itemBox);
      SetControlValue((ControlHandle)item,0);
      GetDialogItem(gFileDlg,OPEN_SEL_LST,&itemType,&item,&itemBox);
      HiliteControl((ControlHandle)item, DISABLE_CTL);
    }

    GetDialogItem(gFileDlg,OPEN_FORMAT,&itemType,&item,&itemBox);
    HiliteControl((ControlHandle)item, DISABLE_CTL);
    GetDialogItem(gFileDlg,OPEN_WIDTH,&itemType,&item,&itemBox);
    HiliteControl((ControlHandle)item, DISABLE_CTL);

    /* set popup menus          -matt */
    GetDialogItem(gFileDlg,OPEN_FORMAT,&itemType,&item,&itemBox);
    SetControlValue((ControlHandle)item,out_fmt.header);
    GetDialogItem(gFileDlg,OPEN_WIDTH,&itemType,&item,&itemBox);
    SetControlValue((ControlHandle)item,out_fmt.type);

    /*
     * Enable OK button only if sco/orc files valid, else disable.
     */
    GetDialogItem(gFileDlg,1,&itemType,&item,&itemBox);

    if ((tmp.flags & IN_VALID) && (tmp.flags & NOI_VALID)) {
      HiliteControl((ControlHandle)item, ENABLE_CTL);
      //        SetDialogDefaultItem(gFileDlg, 1) ;
      ok_enabled = TRUE;
    }
    else {
      HiliteControl((ControlHandle)item, DISABLE_CTL);
      ok_enabled = FALSE;
    }
}

/*
 * Concatenates name, ':', and path. Returns in path.
 */
void concatPath(char *path, char *name)
{
    char buf[256];
    sprintf(buf,"%s%s%s",name,*path ? ":" : "",path);
    strcpy(path,buf);
}


/*============================================================================
 * Replaces ".suffix" with x.
 ============================================================================*/
void addFileExt(char *s, char *x)
{
    char *t;
    t = s;
    while (*t && *t != '.')
      t++;
    strcpy(t, x);
}

/*============================================================================
 * Called after the score file has been set up to form an output file name.
 * If dp provided, set up the edit text item for the output file.
 ============================================================================*/
void formOutputFile(DialogPtr dp, SO_FILE *p)
{
    switch (out_fmt.header) {
    case POPUP_SDII:
      addFileExt(p->out,".snd");
      break;
    case POPUP_WAV:
      addFileExt(p->out,".wav");
      break;
    case POPUP_AIFF:
      if (out_fmt.type == POPUP_32F)    // we only do aifc for floating point
        addFileExt(p->out,".aic");
      else
        addFileExt(p->out,".aif");
      break;
    default:
      addFileExt(p->out,".raw");
      break;
    }
}


//#pragma mark -

void do_mac_dialogs(void)
{
    static Boolean dispFlag = FALSE;    // to make it possible to type in editable text
    static Boolean forTesting = true;

    char        buff[256];
    short       itemType;
    Rect        itemBox;
    Handle      item;
    short       val;
    OSErr       myErr;
    char        errStr[255];
    short       myItem;
    StandardFileReply   reply;
    SFTypeList  typeList;
    short       numTypes = 1;// -for some reason causing crash...1; -sdb changed nil to 1
    FSSpec      sfSpec;
    static int  prevFormat =    POPUP_16;        //set to default
    ModalFilterUPP filterupp = NewModalFilterProc(myFilter);

    typeList[0] = 'TEXT';// -for some reason causing crash...'TEXT'; -sdb changed nil to 'TEXT'

    gFileDlg = GetNewDialog(OPEN_DLG_ID, NULL, (WindowPtr)-1);
//    SetWTitle(gFileDlg,(const unsigned char *)so_file.in);
//    SizeWindow(gFileDlg, 477, 132, true);
//    shortDisplay = true;
//    SetPort(gFileDlg);

    while (1) {                 /* Dialog interface; get events */
      ModalDialog(filterupp, &myItem);

      switch (myItem) {
      case 1:           /* the OK button */
        formCmdLine();
        goto ending;
      case OPEN_SEL_NOI:
//        StandardGetFile(nil, numTypes, typeList, &reply);
                StandardGetFile(nil, -1, nil, &reply);
        if (reply.sfGood) {     // if 'Open' was selected
          so_file.noise_vrn = reply.sfFile.vRefNum ;
          so_file.noise_pid = reply.sfFile.parID ;
          strcpy(so_file.noise,PtoCstr(reply.sfFile.name));
          so_file.flags |= NOI_VALID;
        }
        break;
      case OPEN_SEL_IN:
                StandardGetFile(nil, -1, nil, &reply);
//        StandardGetFile(nil, numTypes, typeList, &reply);
        if (reply.sfGood) {     // if 'Open' was selected
          so_file.in_vrn = reply.sfFile.vRefNum ;
          so_file.in_pid = reply.sfFile.parID ;
          strcpy(so_file.in,PtoCstr(reply.sfFile.name));
          so_file.flags |= IN_VALID;
        }
        break;
      case OPEN_SEL_OUT:                /* really Dirs... */
        StandardPutFile("\pOutput File", nil, &reply);
        if (reply.sfGood) {     // if 'Open' was selected
          strcpy(so_file.out,PtoCstr(reply.sfFile.name));
        }
        break;
      case OPEN_OUT_TEX:
        break;
      case OPEN_SEL_LST:
        StandardPutFile("\pListing file:", CtoPstr(so_file.lst), &reply);
        if (reply.sfGood) {     // if 'Open' was selected
          so_file.lst_vrn = reply.sfFile.vRefNum ;
          so_file.lst_pid = reply.sfFile.parID ;
          strcpy(so_file.lst,PtoCstr(reply.sfFile.name));
        }
        else PtoCstr((unsigned char *)so_file.lst);
        break;
      case OPEN_FORMAT:
        GetDialogItem(gFileDlg,OPEN_FORMAT,&itemType,&item,&itemBox);
        out_fmt.header = val = GetControlValue((ControlHandle)item);
        switch (out_fmt.header) {
        case POPUP_SDII:
          // SDII supports chars, shorts, and longs only
          if (
#ifdef ULAW
              out_fmt.type == POPUP_ULAW || 
#endif
              out_fmt.type == POPUP_8_US)
            out_fmt.type = POPUP_8;
          else if (out_fmt.type == POPUP_32F)
            out_fmt.type = POPUP_16;
          break;
        case POPUP_WAV:
          // WAV supports u-chars and shorts only
          if (
#ifdef ULAW
              out_fmt.type == POPUP_ULAW ||
#endif
              out_fmt.type == POPUP_8)
            out_fmt.type = POPUP_8_US;
          if (out_fmt.type == POPUP_32 || out_fmt.type == POPUP_32F)
            out_fmt.type = POPUP_16;
          break;
        }
        break;
      case OPEN_WIDTH:
        GetDialogItem(gFileDlg,OPEN_WIDTH,&itemType,&item,&itemBox);
        out_fmt.type = GetControlValue((ControlHandle)item);
        switch (out_fmt.type) {
        case POPUP_8:
          // WAV chars are unsigned
          if (out_fmt.header == POPUP_WAV)
            out_fmt.header = POPUP_AIFF;
          break;
        case POPUP_8_US:
          // WAV chars are unsigned
          if (out_fmt.header == POPUP_SDII || out_fmt.header == POPUP_AIFF)
            out_fmt.header = POPUP_WAV;
          break;
#ifdef ULAW
        case POPUP_ULAW:
          // SDII or WAV dont support ulaw
          if (out_fmt.header == POPUP_SDII || out_fmt.header == POPUP_WAV)
            out_fmt.header = POPUP_AIFF;
          break;
#endif
        case POPUP_32:
          // WAV dont support longs
          if (out_fmt.header == POPUP_WAV)
            out_fmt.header = POPUP_AIFF;
          break;
        case POPUP_32F:
          // SDII or WAV dont support floats
          if (out_fmt.header == POPUP_SDII || out_fmt.header == POPUP_WAV)
            out_fmt.header = POPUP_AIFF;
          break;
        }
        break;
      case OPEN_KILL:
        exit(1);
      case OPEN_MORE:  // little button to toggle display (should really be a disclosure arrow)
        GetDialogItem(gFileDlg,42,&itemType,&item,&itemBox);
        if (shortDisplay) {
          SizeWindow(gFileDlg, 477, 400, true);
          SetControlTitle((ControlHandle)item,"\p-");
        }
        else {
          SizeWindow(gFileDlg, 477, 132, true);
          SetControlTitle((ControlHandle)item,"\p+");
        }
        shortDisplay = !shortDisplay;
        break;
      case OPEN_VERBOSE:
        opt_flags ^= OF_VERBOSE;
        break;
      case OPEN_REWRITE:
        opt_flags ^= OF_REWRITE;
        break;
      case OPEN_T:
        GetDlgString(gFileDlg, myItem, buff);
        sscanf(buff, "%f", &threshold);
        break;
      default:
        printf("Item is %d`\n", myItem);
        break;
      }
      UpdateFileDlg(so_file);
    }
 ending:
    //    GetDialogItem(gFileDlg,OPEN_OUT_TEX,&itemType,&item,&itemBox);
    //    GetDialogItemText(item, (StringPtr)so_file.out) ;
    //    PtoCstr((StringPtr)so_file.out);
    DisposeDialog(gFileDlg);
    gFileDlg = 0;
}


/*
 * Append the (fmt, args) to the current command line using sprintf.
 */
void appendCmd(char *fmt)
{
    char *b = &cmd_line[endp];
    sprintf(b,fmt);
    gargv[gargc++] = b;
    endp += strlen(b)+1;
}

void appendCmds(char *fmt, char *a)
{
    char *b = &cmd_line[endp];
    sprintf(b,fmt,a);
    gargv[gargc++] = b;
    endp += strlen(b)+1;
}

void appendCmdd(char *fmt, int a)
{
    char *b = &cmd_line[endp];
    sprintf(b,fmt,a);
    gargv[gargc++] = b;
    endp += strlen(b)+1;
}

void appendCmdf(char *fmt, MYFLT a)
{
    char *b = &cmd_line[endp];
    sprintf(b,fmt,a);
    gargv[gargc++] = b;
    endp += strlen(b)+1;
}


/*
 * getHFSPath() function builds full pathname from fname and vrefnum.
 * Path is limited to 255 characters. If fname is NULL, then builds
 * directory pathname of vrefnum. Returns noErr is success, else error.
 * Will crash if run on non-HFS system.
 */
#define ROOT_DIR_ID  (2)
OSErr getHFSPath(char *path,char *fname,short vrefnum, long parid)
{
    //extern int app_ev;
    CInfoPBRec pb;
    WDPBRec wd;
    int stat;
    long curDir;
    char buf[256];
    *path = 0;
    memset(&pb,'\0',sizeof(HFileInfo));
    if (fname) {
      strcpy(buf,fname);
      CtoPstr(buf);

      pb.hFileInfo.ioNamePtr = (StringPtr) buf;
      pb.hFileInfo.ioVRefNum = vrefnum;
      pb.hFileInfo.ioFDirIndex = 0;
      //                if (app_ev) pb.hFileInfo.ioDirID = parid;
      pb.hFileInfo.ioDirID = parid;
    }
    else {
      memset(&wd,'\0',sizeof(WDParam));
      wd.ioVRefNum = vrefnum;
      if ((stat = PBGetWDInfo(&wd,FALSE)) != noErr) return stat;
      buf[0] = 0;
      pb.hFileInfo.ioNamePtr = (StringPtr) buf;
      pb.hFileInfo.ioDirID = wd.ioWDDirID;
      pb.hFileInfo.ioFDirIndex = -1;
    }

    do {
      if ((stat = PBGetCatInfo(&pb,FALSE)) != noErr) return stat;
      PtoCstr((StringPtr)pb.hFileInfo.ioNamePtr);
      concatPath(path,(char*)pb.hFileInfo.ioNamePtr);
      CtoPstr((char *)pb.hFileInfo.ioNamePtr);
      curDir = pb.hFileInfo.ioDirID;
      pb.hFileInfo.ioFDirIndex = -1;
      pb.hFileInfo.ioDirID = pb.hFileInfo.ioFlParID;
    } while (curDir != ROOT_DIR_ID);

    return noErr;
}

/* like above, but without an existing file */
OSErr getDirPath(char *path,short vrefnum, long parid)
{
    CInfoPBRec pb;
    int stat;
    long curDir;
    char buf[256];
    *path = 0;
    memset(&pb,'\0',sizeof(HFileInfo));
    strcpy(buf,(const char *)"\p");

    pb.hFileInfo.ioNamePtr = (StringPtr) buf;
    pb.hFileInfo.ioVRefNum = vrefnum;
    pb.hFileInfo.ioFDirIndex = 0;
    pb.hFileInfo.ioDirID = parid;

    do {
      if ((stat = PBGetCatInfo(&pb,FALSE)) != noErr) return stat;
      PtoCstr((StringPtr)pb.hFileInfo.ioNamePtr);
      concatPath(path,(char *)pb.hFileInfo.ioNamePtr);
      CtoPstr((char *)pb.hFileInfo.ioNamePtr);
      curDir = pb.hFileInfo.ioDirID;
      pb.hFileInfo.ioFDirIndex = -1;
      pb.hFileInfo.ioDirID = pb.hFileInfo.ioFlParID;
    } while (curDir != ROOT_DIR_ID);

    return noErr;
}

#pragma mark -
/*
 * Using the current options and files, create a UNIX-style command line
 * to invoke CSound. If pathnames contain spaces, double-quote them.
 */
void formCmdLine(void)
{
    int msg;
    int fmt_char,i;
    char path[256];
    long        foobuff;

    for (i=0; i<MAX_ARG; i++) gargv[i] = NULL;

    i = 0;
    /*
     * program name
     */
    appendCmd("dnoise");
    i = 1;
    /*
     * options
     */
    if (opt_flags & OF_VERBOSE) appendCmd("-V");
    if (out_fmt.header == POPUP_AIFF) appendCmd("-A");
    else if (out_fmt.header == POPUP_WAV) appendCmd("-W");
    else if (out_fmt.header == POPUP_NO) appendCmd("-h");
    else if (out_fmt.header == POPUP_IRCAM) appendCmd("-J");
    /*
     * output format
     */
    switch (out_fmt.type) {
    case POPUP_8:
      fmt_char = 'c';
      break;
    case POPUP_8_US:
      fmt_char = '8';
      break;
#ifdef never
    case POPUP_ALAW:
      fmt_char = 'a';
      break;
#endif
#ifdef ULAW
    case POPUP_ULAW:
      fmt_char = 'u';
      break;
#endif
    default:
    case POPUP_16:
      fmt_char = 's';
      break;
    case POPUP_32:
      fmt_char = 'l';
      break;
    case POPUP_32F:
      fmt_char = 'f';
      break;
    }

    appendCmds("-%c",(char *)fmt_char);

/******** put in listfile name *******  -matt */
    if (so_file.flags & LST_VALID)  {
      //getHFSPath(path,NULL,so_file.lst_vrn,so_file.lst_pid); //form directory path
      getDirPath(path,so_file.lst_vrn,so_file.lst_pid); //form directory path
      strcat(path,":"); /* now append listingfile name */
      strcat(path,so_file.lst);
      appendCmds("--%s", path);
    }
    if (!so_file.out[0]) formOutputFile(NULL,&so_file);
    strcpy(path,so_file.out) ;
    appendCmds("-o%s",path);
    if (so_file.flags & NOI_VALID) {
      getHFSPath(path,so_file.noise,so_file.noise_vrn,so_file.noise_pid);
      appendCmds("-i%s",path);
    }
    if (threshold != -1.0f) appendCmdf("-t%f", threshold);
    if (m_g0 != -FL(40.0)) appendCmdf("-m%f", m_g0);
    if (n_aver != 5) appendCmdd("-n%d", n_aver);
    if (b_beg > FL(0.0)) appendCmdf("-b%f", b_beg);
    if (e_end > FL(0.0)) appendCmdf("-e%f", e_end);
    if (B_beg > 0) appendCmdd("-B%d", B_beg);
    if (D_dec > 0) appendCmdd("-D%d", D_dec);
    if (E_end > 0) appendCmdd("-E%d", E_end);
    if (L_synthlen > 0) appendCmdd("-L%d", L_synthlen);
    if (M_anallen > 0) appendCmdd("-M%d", M_anallen);
    if (N_nfilts > 0) appendCmdd("-N%d", N_nfilts);
    if (S_sharp > 0) appendCmdd("-S%d", S_sharp);
    if (w_ovlp > 0) appendCmdd("-w%d", w_ovlp);
    if (so_file.flags & IN_VALID) {
      getHFSPath(path,so_file.in,so_file.in_vrn,so_file.in_pid);
      appendCmd(path);
    }
}

