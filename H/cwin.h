/*
 * "cwin.h"                                   Copyright (C) 1994, A C Norman
 *
 * This defines the public interface supported by the "cwin" window
 * interface.  Although this header file is copyright it only defines an
 * interface - anybody is permitted to read it, make copies of it, edit,
 * re-implement packages to its specification and pass it on to others
 * either freely or for commercial purposes.
 */

/*
 * The code here is provides a windowed framework in which reasonably
 * ordinary C code can run.  The functions described here are the
 * interface.  At present "cwin" supports just one window, somewhat
 * similar to the Windows NT "console" windows.
 */

/* Signature: 5ed950af 30-Jun-1994 */

#ifndef __CWIN_H_LOADED
#define __CWIN_H_LOADED 1

#ifdef BUILDING_DLL
#define DLLexport __declspec(dllexport)
#else
#define DLLexport
#endif

#ifdef __cplusplus
extern "C" {
class CTheApp : public CWinApp          // application class
{
public:
   BOOL InitInstance();                 // override default InitInstance()
   int Run();                           // override top level loop (!)
};
extern CTheApp theApp;
#endif

/*
 * The "C" code will eventually be entered at main() in what looks like a
 * normal way.  Either main() should return, or it should call cwin_exit()
 * but it should not just call the regular C exit() function directly.
 */
extern int csoundMain(void *cs, int argc, char *argv[]);

/*
 * cwin_full_program_name is a string like "d:\xxx\yyy\cwin.exe"  This is
 * made available so that applications can edit it to generate names of
 * resource files (eg by just altering the ".exe" bit on the end into some
 * other suffix.
 */
DLLexport extern char *cwin_full_program_name;

/*
 * Something that is SPECIAL and IMPORTANT to this code is that all the
 * code executed via main() must arrange to call cwin_poll_window_manager()
 * every so often.  If this does not happen the window system will become
 * unresponsive.
 */
DLLexport extern int cwin_poll_window_manager();

/*
 * To finish off you can either return from main(), or you can go
 *        cwin_exit(n);
 * The system will forcibly close down for you if the EXIT item on
 * the FILE menu or the CLOSE item on the SYSTEM menu gets selected.  But
 * direct use of the C function "exit()" is not considered proper.
 */

DLLexport extern void cwin_exit(int return_code);

/*
 * cwin_atexit() is analogous to the regular C function atexit().
 */

typedef void ExitFunction(void);

DLLexport extern int cwin_atexit(ExitFunction*);

/*
 * If, when the program is stopping, cwin_pause_at_end has been set to
 * be non-zero (by default it will be zero) then an alert box is displayed
 * forcing the user to give positive confirmation before the main window
 * is closed.  This does not give an opportunity to cancel the exit, just to
 * read the final state of the screen...   This effect does not occur if
 * program exit is caused by selecting EXIT from the FILE menu or CLOSE
 * from the system menu.
 */
DLLexport extern int cwin_pause_at_end;

/*
 * Rather than using putchar() and printf(), here are the calls
 * the can be made to get output onto the screen.  NOTE that cwin_puts()
 * is more like fputs than puts in that it just dumps the characters in its
 * string to the screen [it does not add an extra newline in the way that
 * puts does].
 * These functions support printable ASCII characters. TABs are expanded
 * to multiples of 8 spaces.
 */
DLLexport extern void cwin_putchar(int c);
DLLexport extern void cwin_puts(char *s);
DLLexport extern void cwin_printf(char *fmt, ...);
DLLexport extern void cwin_fprintf(FILE *err, char *fmt, ...);
DLLexport extern void cwin_vfprintf(char *fmt, va_list a);


/*
 * ensure_screen() causes the display to catch up with whatever else has
 * been going on.
 */
DLLexport extern int cwin_ensure_screen();

/*
 * almost_ensure_screen() causes the display to be brought up to date
 * except that partial lines of output may not be moved to the screen,
 * especially if displaying them could force the window to scroll left or
 * right.
 */
DLLexport extern void cwin_almost_ensure_screen();

/*
 * cwin_getchar() behaves rather as one might expect getchar() to - it
 * grabs a character from the keyboard input buffer.
 */
DLLexport extern int cwin_getchar();

/*
 * cwin_getchar_nowait() is just like cwin_getchar() except that if
 * no character is immediately available it returns EOF instead of
 * waiting.
 */
DLLexport extern int cwin_getchar_nowait();

/*
 * If any characters had already been typed and were waiting to be
 * read, this abandons them.
 */
DLLexport extern void cwin_discard_input();

/*
 * cwin_pause_pending can be set by the "pause" menu and is intended
 * to be used to halt calculations in the main program. It gets set to 1
 * on "PAUSE" and to 3 on "BACKTRACE".
 */
DLLexport extern int cwin_pause;

/*
 * Short messages can be displayed at the left middle and right of the
 * main title-ribbon of your window.  These functions set the text to be
 * displayed there.  If there is not much room then only the middle one
 * will remain visible.  Each message should be limited to around 30 chars
 * (and will be best if kept shorter than that).  The default position is that
 * the left position displays the time & date, the middle one the name of
 * the program being run and the right one is blank. cwin_report_left(NULL)
 * or cwin_report_mid(NULL) re-instate the default display. Use
 * cwin_report_left("") to get an empty left message.
 */
DLLexport extern void cwin_report_left(char *msg);
DLLexport extern void cwin_report_mid(char *msg);
DLLexport extern void cwin_report_right(char *msg);

/*
 * The following four strings may be updated (but PLEASE keep within the
 * length limit) to make the display in the "ABOUT" box reflect your
 * particular application.
 */
DLLexport extern char about_box_title[32];       /* "About XXX";           */
DLLexport extern char about_box_description[32]; /* "XXX version 1.1";     */
                                                 /* <icon appears here>    */
DLLexport extern char about_box_rights_1[32];    /* "Copyright Whoever";   */
DLLexport extern char about_box_rights_2[32];    /* "Date or whatever";    */

/*
 *  Now a start at a graphics interface...
 */

typedef double transform[3][3];

DLLexport extern void cwin_line(unsigned colour, float x0, float y0, float x1, float y1);

DLLexport extern void cwin_line_dash(unsigned colour, float x0, float y0, float x1, float y1);

DLLexport extern void cwin_show();

DLLexport extern void cwin_caption(char *name);

DLLexport extern void cwin_clear();

DLLexport extern void cwin_unTop();

DLLexport extern void cwin_newgraph();

DLLexport extern void cwin_paint();

#ifdef __cplusplus
}
#endif
#endif

/* end of "cwin.h" */

