//
// Fl_Native_File_Chooser_MAC.H -- FLTK native OS file chooser widget
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
//      10        20        30        40        50        60        70
//       |         |         |         |         |         |         |
// 4567890123456789012345678901234567890123456789012345678901234567890123456789

// OSX-SPECIFIC NATIVE BROWSER
#ifdef __APPLE_CC__
#include <Carbon/Carbon.h>
#else
#include <Carbon.h>
#endif

#include <FL/filename.H>
#define MAXFILTERS	80

class Fl_Native_File_Chooser {
public:
    enum Type {
	BROWSE_FILE = 0,
	BROWSE_DIRECTORY,
	BROWSE_MULTI_FILE,
	BROWSE_MULTI_DIRECTORY,
	BROWSE_SAVE_FILE,
	BROWSE_SAVE_DIRECTORY
    };
    enum Option {
        NO_OPTIONS     = 0x0000,	// no options enabled
	SAVEAS_CONFIRM = 0x0001,	// Show native 'Save As' overwrite
					// confirm dialog (if supported)
	NEW_FOLDER     = 0x0002,	// Show 'New Folder' icon
					// (if supported)
	PREVIEW        = 0x0004,	// enable preview mode
    };
protected:
    NavDialogCreationOptions _opts;	// file navigation options
private:
    int             _btype;		// kind-of browser to show()
    int             _options;		// general options
    NavDialogRef    _ref;		// file navigation reference
    NavActionState  _keepstate;		// holds button permissions
    NavMenuItemSpec _tempitem;   	// Popup menu selection
    char          **_pathnames;		// array of pathnames
    int             _tpathnames;	// total pathnames
    char           *_directory;		// default pathname to use
    char           *_title;		// title for window
    char           *_preset_file;	// the 'save as' filename

    char           *_filter;		// user-side search filter, eg:
					// C Files\t*.[ch]\nText Files\t*.txt"

    char           *_filt_names;	// filter names (tab delimited)
    					// eg. "C Files\tText Files"

    char           *_filt_patt[MAXFILTERS];
                                        // array of filter patterns, eg:
					//     _filt_patt[0]="*.{cxx,h}"
					//     _filt_patt[1]="*.txt"

    int             _filt_total;	// parse_filter() # of filters loaded
    int             _filt_value;	// index of the selected filter
    char           *_errmsg;		// error message

    // PRIVATE CLASS TO HANDLE NAVIGATION DIALOG REPLY STRUCT
    //     Class-ified, mainly to ensure proper cleanup.
    //
    class NavReply {
        int _valid_reply;
	NavReplyRecord _reply;
    public:
        NavReply();
	~NavReply();
	int get_reply(NavDialogRef& ref);
	int get_saveas_basename(char *s, int slen);
	int get_dirname(char *s, int slen);
	int get_pathnames(char **&pathnames, int& tpathnames);
    };

    // Private methods
    void errmsg(const char *msg);
    void clear_pathnames();
    void set_single_pathname(const char *s);
    int get_saveas_basename(NavDialogRef& ref);
    int get_pathnames(NavDialogRef& ref);
    static void event_handler(NavEventCallbackMessage callBackSelector, 
    					     NavCBRecPtr cbparm, void *data);

    void clear_filters();
    void add_filter(const char *, const char *);
    void parse_filter(const char *from);
    static Boolean filter_proc_cb(AEDesc *, void *, void *, NavFilterModes);
    Boolean filter_proc_cb2(AEDesc*, void*, void*, NavFilterModes);
    int post();
    
public:
    Fl_Native_File_Chooser(int val = BROWSE_FILE);
    ~Fl_Native_File_Chooser();

    // Public methods
    void type(int);
    int type() const;
    void options(int);
    int options() const;
    int count() const;
    const char *filename() const;
    const char *filename(int i) const;
    void directory(const char *);
    const char *directory() const;
    void title(const char *);
    const char *title() const;
    const char *filter() const;
    void filter(const char *);
    void filter_value(int i) { _filt_value = i; }
    int filter_value() { return(_filt_value); }
    int filters() { return(_filt_total); }
    void preset_file(const char *);
    const char *preset_file();
    const char *errmsg() const;
    int show();
};
