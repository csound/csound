//
// Fl_Native_File_Chooser_MAC.cxx -- FLTK native OS file chooser widget
//
// Copyright 2004 by Greg Ercolano.
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
// USA.
//
// Please keep code 80 column compliant.
//
//      10        20        30        40        50        60        70
//       |         |         |         |         |         |         |
// 4567890123456789012345678901234567890123456789012345678901234567890123456789
//
// TODO:
//      o When doing 'open file', only dir is preset, not filename.
//        Possibly 'preset_file' could be used to select the filename.
//
#include <FL/Fl.H>
#include "Fl_Native_File_Chooser.H"
#include "common.cxx"           // strnew/strfree/strapp/chrcat

// TRY TO CONVERT AN AEDesc TO AN FSSpec
//     As per Apple Technical Q&A QA1274
//     eg: http://developer.apple.com/qa/qa2001/qa1274.html
//     Returns 'noErr' if OK,
//             or an 'OSX result code' on error.
//
static int AEDescToFSSpec(const AEDesc* desc, FSSpec* fsspec) {
    OSStatus err = noErr;
    AEDesc coerceDesc;
    // If AEDesc isn't already an FSSpec, convert it to one
    if ( desc->descriptorType != typeFSS ) {
        if ( ( err = AECoerceDesc(desc, typeFSS, &coerceDesc) ) == noErr ) {
            // Get FSSpec out of AEDesc
            err = AEGetDescData(&coerceDesc, fsspec, sizeof(FSSpec));
            AEDisposeDesc(&coerceDesc);
        }
    } else {
        err = AEGetDescData(desc, fsspec, sizeof(FSSpec));
    }
    return( err );
}

// CONVERT AN FSSpec TO A PATHNAME
static void FSSpecToPath(const FSSpec &spec, char *buff, int bufflen) {
    FSRef fsRef;
    FSpMakeFSRef(&spec, &fsRef);
    FSRefMakePath(&fsRef, (UInt8*)buff, bufflen);
}

// CONVERT REGULAR PATH -> FSSpec
//     If file does not exist, expect fnfErr.
//     Returns 'noErr' if OK,
//             or an 'OSX result code' on error.
//
static OSStatus PathToFSSpec(const char *path, FSSpec &spec) {
    OSStatus err;
    FSRef ref;
    if ((err = FSPathMakeRef((const UInt8*)path, &ref, NULL)) != noErr) {
        return(err);
    }
    // FSRef -> FSSpec
    if ((err = FSGetCatalogInfo(&ref, kFSCatInfoNone, NULL, NULL, &spec,
                                                             NULL)) != noErr) {
        return(err);
    }
    return(noErr);
}

// NAVREPLY: CTOR
Fl_Native_File_Chooser::NavReply::NavReply() {
    _valid_reply = 0;
}

// NAVREPLY: DTOR
Fl_Native_File_Chooser::NavReply::~NavReply() {
    if ( _valid_reply ) {
        NavDisposeReply(&_reply);
    }
}

// GET REPLY FROM THE NAV* DIALOG
int Fl_Native_File_Chooser::NavReply::get_reply(NavDialogRef& ref) {
    if ( _valid_reply ) {
        NavDisposeReply(&_reply);       // dispose of previous
        _valid_reply = 0;
    }
    if ( ref == NULL || NavDialogGetReply(ref, &_reply) != noErr ) {
        return(-1);
    }
    _valid_reply = 1;
    return(0);
}

// RETURN THE BASENAME USER WANTS TO 'Save As'
int Fl_Native_File_Chooser::NavReply::get_saveas_basename(char *s, int slen) {
    if (CFStringGetCString(_reply.saveFileName, s, slen-1,
                                        kCFStringEncodingUTF8) == false) {
        s[0] = '\0';
        return(-1);
    }
    return(0);
}

// RETURN THE DIRECTORY NAME
//    Returns 0 on success, -1 on error.
//
int Fl_Native_File_Chooser::NavReply::get_dirname(char *s, int slen) {
    FSSpec fsspec;
    if ( AEDescToFSSpec(&_reply.selection, &fsspec) != noErr ) {
        // Conversion failed? Return empty name
        s[0] = 0;
        return(-1);
    }
    FSSpecToPath(fsspec, s, slen);
    return(0);
}

// RETURN MULTIPLE DIRECTORIES
//     Returns: 0 on success with pathnames[] containing pathnames selected,
//             -1 on error
//
int Fl_Native_File_Chooser::NavReply::get_pathnames(char **&pathnames,
                                                    int& tpathnames) {
    // How many items selected?
    long count = 0;
    if ( AECountItems(&_reply.selection, &count) != noErr )
        { return(-1); }

    // Allocate space for that many pathnames
    pathnames = new char*[count];
    memset((void*)pathnames, 0, count*sizeof(char*));
    tpathnames = count;

    // Walk list of pathnames selected
    for (short index=1; index<=count; index++) {
        AEKeyword keyWord;
        AEDesc    desc;
        if (AEGetNthDesc(&_reply.selection, index, typeFSS, &keyWord,
                                                           &desc) != noErr) {
            pathnames[index-1] = strnew("");
            continue;
        }
        FSSpec fsspec;
        if (AEGetDescData(&desc, &fsspec, sizeof(FSSpec)) != noErr ) {
            pathnames[index-1] = strnew("");
            continue;
        }
        char s[4096];
        FSSpecToPath(fsspec, s, sizeof(s)-1);
        pathnames[index-1] = strnew(s);
        AEDisposeDesc(&desc);
    }
    return(0);
}

// FREE PATHNAMES ARRAY, IF IT HAS ANY CONTENTS
void Fl_Native_File_Chooser::clear_pathnames() {
    if ( _pathnames ) {
        while ( --_tpathnames >= 0 ) {
            _pathnames[_tpathnames] = strfree(_pathnames[_tpathnames]);
        }
        delete [] _pathnames;
        _pathnames = NULL;
    }
    _tpathnames = 0;
}

// SET A SINGLE PATHNAME
void Fl_Native_File_Chooser::set_single_pathname(const char *s) {
    clear_pathnames();
    _pathnames = new char*[1];
    _pathnames[0] = strnew(s);
    _tpathnames = 1;
}

// GET THE 'Save As' FILENAME
//    Returns -1 on error, errmsg() has reason, filename == "".
//             0 if OK, filename() has filename chosen.
//
int Fl_Native_File_Chooser::get_saveas_basename(NavDialogRef& ref) {
    if ( ref == NULL ) {
        errmsg("get_saveas_basename: ref is NULL");
        return(-1);
    }
    NavReply reply;
    OSStatus err;
    if ((err = reply.get_reply(ref)) != noErr ) {
        errmsg("NavReply::get_reply() failed");
        clear_pathnames();
        return(-1);
    }

    char pathname[4096] = "";
    // Directory name..
    //    -2 leaves room to append '/'
    //
    if ( reply.get_dirname(pathname, sizeof(pathname)-2) < 0 ) {
        clear_pathnames();
        errmsg("NavReply::get_dirname() failed");
        return(-1);
    }
    // Append '/'
    int len = strlen(pathname);
    pathname[len++] = '/';
    pathname[len] = '\0';
    // Basename..
    if ( reply.get_saveas_basename(pathname+len, sizeof(pathname)-len) < 0 ) {
        clear_pathnames();
        errmsg("NavReply::get_saveas_basename() failed");
        return(-1);
    }
    set_single_pathname(pathname);
    return(0);
}

// GET (POTENTIALLY) MULTIPLE FILENAMES
//     Returns:
//         -1 -- error, errmsg() has reason, filename == ""
//          0 -- OK, pathnames()/filename() has pathname(s) chosen
//
int Fl_Native_File_Chooser::get_pathnames(NavDialogRef& ref) {
    if ( ref == NULL ) {
        errmsg("get_saveas_basename: ref is NULL");
        return(-1);
    }
    NavReply reply;
    OSStatus err;
    if ((err = reply.get_reply(ref)) != noErr ) {
        errmsg("NavReply::get_reply() failed");
        clear_pathnames();
        return(-1);
    }
    // First, clear pathnames array of any previous contents
    clear_pathnames();
    if ( reply.get_pathnames(_pathnames, _tpathnames) < 0 ) {
        clear_pathnames();
        errmsg("NavReply::get_dirname() failed");
        return(-1);
    }
    return(0);
}

// NAV CALLBACK EVENT HANDLER
void Fl_Native_File_Chooser::event_handler(
                                    NavEventCallbackMessage callBackSelector,
                                    NavCBRecPtr cbparm,
                                    void *data) {
    OSStatus err;
    Fl_Native_File_Chooser *nfb = (Fl_Native_File_Chooser*)data;
    switch (callBackSelector) {
        case kNavCBStart:
            if ( nfb->directory() || nfb->preset_file() ) {
                const char *pathname = nfb->directory() ? nfb->directory() : nfb->preset_file();
                FSSpec spec;
                if ( ( err = PathToFSSpec(pathname, spec) ) != noErr ) {
                    fprintf(stderr, "PathToFSSpec(%s) failed: err=%d\n",
                        pathname, (int)err);
                    break;
                }
                AEDesc desc;
                if ((err = AECreateDesc(typeFSS,
                                        &spec, sizeof(FSSpec), &desc)) != noErr) {
                    fprintf(stderr, "AECreateDesc() failed: err=%d\n",
                        (int)err);
                }
                if ((err = NavCustomControl(cbparm->context,
                                            kNavCtlSetLocation, &desc)) != noErr) {
                    fprintf(stderr, "NavCustomControl() failed: err=%d\n",
                        (int)err);
                }
                AEDisposeDesc(&desc);
            }
            if ( nfb->_btype == BROWSE_SAVE_FILE && nfb->preset_file() ) {
                 CFStringRef namestr =
                     CFStringCreateWithCString(NULL,
                                               nfb->preset_file(),
                                               kCFStringEncodingASCII);
                 NavDialogSetSaveFileName(cbparm->context, namestr);
                 CFRelease(namestr);
            }
            NavCustomControl(cbparm->context, kNavCtlSetActionState,
                                                           &nfb->_keepstate );

            // Select the right filter in pop-up menu
            if ( nfb->_filt_value == nfb->_filt_total ) {
                // Select All Documents
                NavPopupMenuItem kAll = kNavAllFiles;
                NavCustomControl(cbparm->context, kNavCtlSelectAllType, &kAll);
            } else if (nfb->_filt_value < nfb->_filt_total) {
                // Select custom filter
                nfb->_tempitem.version = kNavMenuItemSpecVersion;
                nfb->_tempitem.menuCreator = 'extn';
                nfb->_tempitem.menuType = nfb->_filt_value;
                *nfb->_tempitem.menuItemName = '\0';    // needed on 10.3+
                NavCustomControl(cbparm->context,
                                 kNavCtlSelectCustomType,
                                 &(nfb->_tempitem));
            }
            break;

        case kNavCBPopupMenuSelect:
            NavMenuItemSpecPtr ptr;
            // they really buried this one!
            ptr = (NavMenuItemSpecPtr)cbparm->eventData.eventDataParms.param;
            if ( ptr->menuCreator ) {
                // Gets index to filter ( menuCreator = 'extn' )
                nfb->_filt_value = ptr->menuType;
            } else {
                // All docs filter selected ( menuCreator = '\0\0\0\0' )
                nfb->_filt_value = nfb->_filt_total;
            }
            break;

        case kNavCBSelectEntry:
            NavActionState astate;
            switch ( nfb->_btype ) {
                // these don't need selection override
                case BROWSE_MULTI_FILE:
                case BROWSE_MULTI_DIRECTORY:
                case BROWSE_SAVE_FILE:
                    break;

                // These need to allow only one item, so disable
                // Open button if user tries to select multiple files
                case BROWSE_SAVE_DIRECTORY:
                case BROWSE_DIRECTORY:
                case BROWSE_FILE:
                    SInt32 selectcount;
                    AECountItems((AEDescList*)cbparm->
                                  eventData.eventDataParms.param,
                                  &selectcount);
                    if ( selectcount > 1 ) {
                        NavCustomControl(cbparm->context,
                                         kNavCtlSetSelection,
                                         NULL);
                        astate = nfb->_keepstate |
                                 kNavDontOpenState |
                                 kNavDontChooseState;
                        NavCustomControl(cbparm->context,
                                         kNavCtlSetActionState,
                                         &astate );
                    }
                    else {
                        astate= nfb->_keepstate | kNavNormalState;
                        NavCustomControl(cbparm->context,
                                         kNavCtlSetActionState,
                                         &astate );
                    }
                    break;
            }
            break;
    }
}

// CONSTRUCTOR
Fl_Native_File_Chooser::Fl_Native_File_Chooser(int val) {
    _btype          = val;
    NavGetDefaultDialogCreationOptions(&_opts);
    _opts.optionFlags |= kNavDontConfirmReplacement;    // no confirms for "save as"
    _options        = NO_OPTIONS;
    _ref            = NULL;
    memset(&_tempitem, 0, sizeof(_tempitem));
    _pathnames      = NULL;
    _tpathnames     = 0;
    _title          = NULL;
    _filter         = NULL;
    _filt_names     = NULL;
    memset(_filt_patt, 0, sizeof(char*) * MAXFILTERS);
    _filt_total     = 0;
    _filt_value     = 0;
    _directory      = NULL;
    _preset_file    = NULL;
    _errmsg         = NULL;
    _keepstate      = kNavNormalState;
}

// DESTRUCTOR
Fl_Native_File_Chooser::~Fl_Native_File_Chooser() {
    // _opts                    // nothing to manage
    if (_ref) { NavDialogDispose(_ref); _ref = NULL; }
    // _options                 // nothing to manage
    // _keepstate               // nothing to manage
    // _tempitem                // nothing to manage
    clear_pathnames();
    _directory   = strfree(_directory);
    _title       = strfree(_title);
    _preset_file = strfree(_preset_file);
    _filter      = strfree(_filter);
    //_filt_names               // managed by clear_filters()
    //_filt_patt[i]             // managed by clear_filters()
    //_filt_total               // managed by clear_filters()
    clear_filters();
    //_filt_value               // nothing to manage
    _errmsg = strfree(_errmsg);
}

// SET THE TYPE OF BROWSER
void Fl_Native_File_Chooser::type(int val) {
    _btype = val;
}

// GET TYPE OF BROWSER
int Fl_Native_File_Chooser::type() const {
    return(_btype);
}

// SET OPTIONS
void Fl_Native_File_Chooser::options(int val) {
    _options = val;
}

// GET OPTIONS
int Fl_Native_File_Chooser::options() const {
    return(_options);
}

// SHOW THE BROWSER WINDOW
//     Returns:
//         0 - user picked a file
//         1 - user cancelled
//        -1 - failed; errmsg() has reason
//
int Fl_Native_File_Chooser::show() {
    // Make sure fltk interface updates before posting our dialog
    Fl::flush();

    // BROWSER TITLE
    CFStringRef cfs_title;
    cfs_title = CFStringCreateWithCString(NULL,
                                          _title ? _title : "No Title",
                                          kCFStringEncodingASCII);
    _opts.windowTitle = cfs_title;

    _keepstate = kNavNormalState;

    // BROWSER FILTERS
    CFArrayRef filter_array = NULL;
    {
        // One or more filters specified?
        if ( _filt_total ) {
            // NAMES -> CFArrayRef
            CFStringRef tab = CFSTR("\t");
            CFStringRef tmp_cfs;
            tmp_cfs = CFStringCreateWithCString(NULL, _filt_names,
                                                kCFStringEncodingASCII);
            filter_array = CFStringCreateArrayBySeparatingStrings(
                                                NULL, tmp_cfs, tab);
            CFRelease(tmp_cfs);
            CFRelease(tab);
            _opts.popupExtension = filter_array;
            _opts.optionFlags |= kNavAllFilesInPopup;
        } else {
            filter_array = NULL;
            _opts.popupExtension = NULL;
            _opts.optionFlags |= kNavAllFilesInPopup;
        }
    }

    // HANDLE OPTIONS WE SUPPORT
    if ( _options & SAVEAS_CONFIRM ) {
        _opts.optionFlags &= ~kNavDontConfirmReplacement;       // enables confirm
    } else {
        _opts.optionFlags |= kNavDontConfirmReplacement;        // disables confirm
    }

    // POST BROWSER
    int err = post();

    // RELEASE _FILT_ARR
    if ( filter_array ) CFRelease(filter_array);
    filter_array = NULL;
    _opts.popupExtension = NULL;
    _filt_total = 0;

    // RELEASE TITLE
    if ( cfs_title ) CFRelease(cfs_title);
    cfs_title = NULL;

    return(err);
}

// POST BROWSER
//     Internal use only.
//     Assumes '_opts' has been initialized.
//
//     Returns:
//         0 - user picked a file
//         1 - user cancelled
//        -1 - failed; errmsg() has reason
//
int Fl_Native_File_Chooser::post() {

    // INITIALIZE BROWSER
    OSStatus err;
    if ( _filt_total == 0 ) {   // Make sure they match
        _filt_value = 0;        // TBD: move to someplace more logical?
    }

    switch (_btype) {
        case BROWSE_FILE:
        case BROWSE_MULTI_FILE:
            //_keepstate = kNavDontNewFolderState;
            // Prompt user for one or more files
            if ((err = NavCreateGetFileDialog(
                          &_opts,               // options
                          0,                    // file types
                          event_handler,        // event handler
                          0,                    // preview callback
                          filter_proc_cb,       // filter callback
                          (void*)this,          // callback data
                          &_ref)) != noErr ) {  // dialog ref
                errmsg("NavCreateGetFileDialog: failed");
                return(-1);
            }
            break;

        case BROWSE_DIRECTORY:
        case BROWSE_MULTI_DIRECTORY:
            _keepstate = kNavDontNewFolderState;
            //FALLTHROUGH

        case BROWSE_SAVE_DIRECTORY:
            // Prompts user for one or more files or folders
            if ((err = NavCreateChooseFolderDialog(
                          &_opts,               // options
                          event_handler,        // event callback
                          0,                    // filter callback
                          (void*)this,          // callback data
                          &_ref)) != noErr ) {  // dialog ref
                errmsg("NavCreateChooseFolderDialog: failed");
                return(-1);
            }
            break;

        case BROWSE_SAVE_FILE:
            // Prompt user for filename to 'save as'
            if ((err = NavCreatePutFileDialog(
                          &_opts,               // options
                          0,                    // file types
                          0,                    // file creator
                          event_handler,        // event handler
                          (void*)this,          // callback data
                          &_ref)) != noErr ) {  // dialog ref
                errmsg("NavCreatePutFileDialog: failed");
                return(-1);
            }
            break;
    }

    // SHOW THE DIALOG
    if ( ( err = NavDialogRun(_ref) ) != 0 ) {
        char msg[80];
        sprintf(msg, "NavDialogRun: failed (err=%d)", (int)err);
        errmsg(msg);
        return(-1);
    }

    // WHAT ACTION DID USER CHOOSE?
    NavUserAction act = NavDialogGetUserAction(_ref);
    if ( act == kNavUserActionNone ) {
        errmsg("Nothing happened yet (dialog still open)");
        return(-1);
    }
    else if ( act == kNavUserActionCancel ) {   // user chose 'cancel'
        return(1);
    }
    else if ( act == kNavUserActionSaveAs ) {   // user chose 'save as'
        return(get_saveas_basename(_ref));
    }

    // TOO MANY FILES CHOSEN?
    int ret = get_pathnames(_ref);
    if ( _btype == BROWSE_FILE && ret == 0 && _tpathnames != 1 ) {
        char msg[80];
        sprintf(msg, "Expected only one file to be chosen.. you chose %d.",
            (int)_tpathnames);
        errmsg(msg);
        return(-1);
    }
    return(err);
}

// SET ERROR MESSAGE
//     Internal use only.
//
void Fl_Native_File_Chooser::errmsg(const char *msg) {
    _errmsg = strfree(_errmsg);
    _errmsg = strnew(msg);
}

// RETURN ERROR MESSAGE
const char *Fl_Native_File_Chooser::errmsg() const {
    return(_errmsg ? _errmsg : "No error");
}

// GET FILENAME
const char* Fl_Native_File_Chooser::filename() const {
    if ( _pathnames && _tpathnames > 0 ) return(_pathnames[0]);
    return("");
}

// GET FILENAME FROM LIST OF FILENAMES
const char* Fl_Native_File_Chooser::filename(int i) const {
    if ( _pathnames && i < _tpathnames ) return(_pathnames[i]);
    return("");
}

// GET TOTAL FILENAMES CHOSEN
int Fl_Native_File_Chooser::count() const {
    return(_tpathnames);
}

// PRESET PATHNAME
//     Value can be NULL for none.
//
void Fl_Native_File_Chooser::directory(const char *val) {
    _directory = strfree(_directory);
    _directory = strnew(val);
}

// GET PRESET PATHNAME
//     Returned value can be NULL if none set.
//
const char* Fl_Native_File_Chooser::directory() const {
    return(_directory);
}

// SET TITLE
//     Value can be NULL if no title desired.
//
void Fl_Native_File_Chooser::title(const char *val) {
    _title = strfree(_title);
    _title = strnew(val);
}

// GET TITLE
//     Returned value can be NULL if none set.
//
const char *Fl_Native_File_Chooser::title() const {
    return(_title);
}

// SET FILTER
//     Can be NULL if no filter needed
//
void Fl_Native_File_Chooser::filter(const char *val) {
    _filter = strfree(_filter);
    _filter = strnew(val);

    // Parse filter user specified
    //     IN: _filter = "C Files\t*.{cxx,h}\nText Files\t*.txt"
    //    OUT: _filt_names   = "C Files\tText Files"
    //         _filt_patt[0] = "*.{cxx,h}"
    //         _filt_patt[1] = "*.txt"
    //         _filt_total   = 2
    //
    parse_filter(_filter);
}

// GET FILTER
//     Returned value can be NULL if none set.
//
const char *Fl_Native_File_Chooser::filter() const {
    return(_filter);
}

// CLEAR ALL FILTERS
//    Internal use only.
//
void Fl_Native_File_Chooser::clear_filters() {
    _filt_names = strfree(_filt_names);
    for (int i=0; i<_filt_total; i++) {
        _filt_patt[i] = strfree(_filt_patt[i]);
    }
    _filt_total = 0;
}

// PARSE USER'S FILTER SPEC
//    Parses user specified filter ('in'),
//    breaks out into _filt_patt[], _filt_names, and _filt_total.
//
//    Handles:
//    IN:                                   OUT:_filt_names    OUT: _filt_patt
//    ------------------------------------  ------------------ ---------------
//    "*.{ma,mb}"                           "*.{ma,mb} Files"  "*.{ma,mb}"
//    "*.[abc]"                             "*.[abc] Files"    "*.[abc]"
//    "*.txt"                               "*.txt Files"      "*.c"
//    "C Files\t*.[ch]"                     "C Files"          "*.[ch]"
//    "C Files\t*.[ch]\nText Files\t*.cxx"  "C Files"          "*.[ch]"
//
//    Parsing Mode:
//         IN:"C Files\t*.{cxx,h}"
//             |||||||  |||||||||
//       mode: nnnnnnn  wwwwwwwww
//             \_____/  \_______/
//              Name     Wildcard
//
void Fl_Native_File_Chooser::parse_filter(const char *in) {
    clear_filters();
    if ( ! in ) return;
    int has_name = strchr(in, '\t') ? 1 : 0;

    char mode = has_name ? 'n' : 'w';   // parse mode: n=title, w=wildcard
    char wildcard[1024] = "";           // parsed wildcard
    char name[1024] = "";

    // Parse filter user specified
    for ( ; 1; in++ ) {

        //// DEBUG
        //// printf("WORKING ON '%c': mode=<%c> name=<%s> wildcard=<%s>\n",
        ////                    *in,  mode,     name,     wildcard);

        switch (*in) {
            // FINISHED PARSING NAME?
            case '\t':
                if ( mode != 'n' ) goto regchar;
                mode = 'w';
                break;

            // ESCAPE NEXT CHAR
            case '\\':
                ++in;
                goto regchar;

            // FINISHED PARSING ONE OF POSSIBLY SEVERAL FILTERS?
            case '\r':
            case '\n':
            case '\0':
                // TITLE
                //     If user didn't specify a name, make one
                //
                if ( name[0] == '\0' ) {
                    sprintf(name, "%.*s Files", (int)sizeof(name)-10, wildcard);
                }
                // APPEND NEW FILTER TO LIST
                if ( wildcard[0] ) {
                    // Add to filtername list
                    //     Tab delimit if more than one. We later break
                    //     tab delimited string into CFArray with
                    //     CFStringCreateArrayBySeparatingStrings()
                    //
                    if ( _filt_total ) {
                        _filt_names = strapp(_filt_names, "\t");
                    }
                    _filt_names = strapp(_filt_names, name);

                    // Add filter to the pattern array
                    _filt_patt[_filt_total++] = strnew(wildcard);
                }
                // RESET
                wildcard[0] = name[0] = '\0';
                mode = strchr(in, '\t') ? 'n' : 'w';
                // DONE?
                if ( *in == '\0' ) return;      // done
                else continue;                  // not done yet, more filters

            // Parse all other chars
            default:                            // handle all non-special chars
            regchar:                            // handle regular char
                switch ( mode ) {
                    case 'n': chrcat(name, *in);     continue;
                    case 'w': chrcat(wildcard, *in); continue;
                }
                break;
        }
    }
    //NOTREACHED
}

// STATIC: FILTER CALLBACK
Boolean Fl_Native_File_Chooser::filter_proc_cb(AEDesc *theItem,
                                               void *info,
                                               void *callBackUD,
                                               NavFilterModes filterMode) {
     return((Fl_Native_File_Chooser*)callBackUD)->filter_proc_cb2(
                                        theItem, info, callBackUD, filterMode);
}

// FILTER CALLBACK
//     Return true if match,
//            false if no match.
//
Boolean Fl_Native_File_Chooser::filter_proc_cb2(AEDesc *theItem,
                                                void *info,
                                                void *callBackUD,
                                                NavFilterModes filterMode) {
    // All files chosen or no filters
    if ( _filt_value == _filt_total ) return(true);

    FSSpec fsspec;
    char pathname[4096];

    // On fail, filter should return true by default
    if ( AEDescToFSSpec(theItem, &fsspec) != noErr ) {
        return(true);
    }
    FSSpecToPath(fsspec, pathname, sizeof(pathname)-1);

    if ( fl_filename_isdir(pathname) ) return(true);
    if ( fl_filename_match(pathname, _filt_patt[_filt_value]) ) return(true);
    else return(false);
}

// SET PRESET FILE
//     Value can be NULL for none.
//
void Fl_Native_File_Chooser::preset_file(const char* val) {
    _preset_file = strfree(_preset_file);
    _preset_file = strnew(val);
}

// PRESET FILE
//     Returned value can be NULL if none set.
//
const char* Fl_Native_File_Chooser::preset_file() {
    return(_preset_file);
}

