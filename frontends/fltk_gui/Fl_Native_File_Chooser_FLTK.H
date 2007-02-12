//
// Fl_Native_File_Chooser_DEFAULT.H -- FLTK native OS file chooser widget
//
// Copyright 2005 by Nathan Vander Wilt.
// March 2005 - wrapper around Fl_File_Chooser
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

// Use FLTK BROWSER
#include <FL/Fl_File_Chooser.H>
#include <string.h>

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
private:
    int   _btype;			// kind-of browser to show()
    int   _options;			// general options
    char *_filter;			// user supplied filter
    char *_parsedfilt;			// parsed filter
    int   _filtvalue;			// selected filter
    char *_preset_file;
    char *_prevvalue;			// Returned filename
    char *_directory;
    char *_errmsg;			// error message
    Fl_File_Chooser *file_chooser;
    int _nfilters;

    // Private methods
    void errmsg(const char *msg);
    int type_fl_file(int);
    void parse_filter();
    void keeplocation();

public:
    Fl_Native_File_Chooser(int val=BROWSE_FILE);
    ~Fl_Native_File_Chooser();

    // Public methods
    void type(int);
    int type() const;
    void options(int);
    int options() const;
    int count() const;
    const char *filename() const;
    const char *filename(int i) const;
    void directory(const char *val);
    const char *directory() const;
    void title(const char *);
    const char* title() const;
    const char *filter() const;
    void filter(const char *);
    int filters() const { return(_nfilters); }
    void filter_value(int i);
    int filter_value() const;
    void preset_file(const char*);
    const char* preset_file() const;
    const char *errmsg() const;
    int show();
};
