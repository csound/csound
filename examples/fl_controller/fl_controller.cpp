// Copyright (c) 2004 by Iain Duncan and Michael Gogins. All rights reserved.
// fl_controller is licensed under the terms of the GNU Lesser General Public License.
// Csound instruments should expect p3 through p6 to receive values 
// normalized over the range from 0 through 1.

#include <csound.h>

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Dial.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Round_Button.H>
#include <FL/Fl_Output.H>
#include <FL/Fl_Multiline_Output.H>
#include <FL/Fl_File_Chooser.H>

#include <cstdlib>
#include <cstdio>
#include <map>
#include <string>
#include <vector>

using namespace std;

class ControlWindow : public Fl_Window
{	
public:
	ControlWindow(int w, int h, const char* title );
	virtual ~ControlWindow();
	std::map<Fl_Dial *, Fl_Output *> outputs_for_dials;
	std::map<Fl_Dial *, int> numbers_for_dials;
	std::map<int, double> dial_values;
	// switch for dynamic versus snapshot patch loading
	Fl_Round_Button *button_dyn_table;
	Fl_Button	*button_play_note;
	Fl_Button	*button_load_snapshot;
	Fl_Button	*button_start_csound;
	Fl_Button	*button_stop_csound;
	Fl_Button	*button_load_csd;
	Fl_Multiline_Output *output_box;
protected:
    // FLTK callbacks:
	static void cb_dial_gen_( Fl_Dial*, void* p_data );
	void cb_dial_gen( Fl_Dial*);
	static void cb_button_play_note_( Fl_Button*, void* p_data );
	void cb_button_play_note( Fl_Button*);
	static void cb_button_load_snapshot_( Fl_Button*, void* p_data );
	void cb_button_load_snapshot( Fl_Button*);
	static void cb_button_start_csound_( Fl_Button*, void* p_data );
	void cb_button_start_csound( Fl_Button*);
	static void cb_button_stop_csound_( Fl_Button*, void* p_data );
	void cb_button_stop_csound( Fl_Button*);
	static void cb_button_load_csd_( Fl_Button*, void* p_data );
	void cb_button_load_csd( Fl_Button*);
	// Csound yield callback:
	static int cb_thread_yield(void *csound);
	void csound_start();
	// Csound rendering thread routine:
	static int csound_thread_routine_(void* p_data );
	int csound_thread_routine( );
	void csound_stop();
	// Other Csound routines:
	void csound_load_csd();
	void csound_play_note( double dur, double amp, double pitch, double p5 );
	void csound_load_ftable( int table_num, double val_0, double val_0, double val_0, double val_0 );
	void csound_update_ftable( int table_num, int table_indx, double table_value );
	// Csound state:
	void *csound;
	int cs_ftable_num;;
	bool cs_performing;
	bool go;
	string csd_filename;
};

ControlWindow::ControlWindow(int w, int h, const char* title) : 
    Fl_Window(w,h,title),
    cs_ftable_num(1),
    cs_performing(0)
{
    csound = csoundCreate(this);
	begin();
	// loop to put the dials on screen
	int dial_x = 10;
	int dial_y = 10;
	// put the dial names here	
	const char *dial_label[8] = {"Dur", "Amp", "Pitch", "P5", "FC", "FQ", "FC Env", "PW" };
	// put the dials on screen, with text boxes underneath them
	for ( int dial_num = 0; dial_num < 8; dial_num++, dial_x+=50 ) {	
		// a spacer for the two sets of dials
		if ( dial_num == 4 ) dial_x+=40;
		// put the dials and value boxes on screen
		Fl_Dial *dial = new Fl_Dial( dial_x,dial_y, 40,40, dial_label[dial_num] ); 		
		Fl_Output *output = new Fl_Output( dial_x, dial_y+60, 40,20, "" );
		outputs_for_dials[dial] = output;
		numbers_for_dials[dial] = dial_num;
		dial->callback((Fl_Callback*)cb_dial_gen_, this);	
	}	
	button_play_note = new Fl_Button(10,100, 180,30, "Play Csound Note" );	
	button_play_note->callback((Fl_Callback*)cb_button_play_note_, this);
	button_load_snapshot = new Fl_Button(250,100, 180,30, "Send Patch Snapshot" );	
	button_load_snapshot->callback((Fl_Callback*)cb_button_load_snapshot_, this);
	button_dyn_table = new Fl_Round_Button( 480,20, 200,30, "Dynamically load patch table." ); 
	button_start_csound = new Fl_Button( 480,60, 180,30, "Start Csound" );	
	button_start_csound->callback((Fl_Callback*)cb_button_start_csound_, this);
	button_stop_csound = new Fl_Button( 480,120, 180,30, "Stop Csound" );	
	button_stop_csound->callback((Fl_Callback*)cb_button_stop_csound_, this);
	button_load_csd = new Fl_Button( 480,180, 180,30, "Load Csd File" );
	button_load_csd->callback((Fl_Callback*)cb_button_load_csd_, this);
	output_box = new Fl_Multiline_Output( 10,200, 400,100, "" );
	output_box->value("Starting value for output");
	end();
	resizable(this);
	show();
}

ControlWindow::~ControlWindow()
{
    csoundDestroy(csound);
}

void ControlWindow::cb_dial_gen_( Fl_Dial* dial, void *p_data )
{
    ((ControlWindow *)p_data)->cb_dial_gen(dial);
}

void ControlWindow::cb_button_play_note_( Fl_Button* button, void *p_data )
{
    ((ControlWindow *)p_data)->cb_button_play_note(button);
}

void ControlWindow::cb_button_load_snapshot_( Fl_Button* button, void *p_data )
{
    ((ControlWindow *)p_data)->cb_button_load_snapshot(button);
}

void ControlWindow::cb_button_start_csound_( Fl_Button* button, void *p_data )
{
    ((ControlWindow *)p_data)->cb_button_start_csound(button);
}

void ControlWindow::cb_button_stop_csound_( Fl_Button* button, void *p_data )
{
    ((ControlWindow *)p_data)->cb_button_stop_csound(button);
}

void ControlWindow::cb_button_load_csd_( Fl_Button* button, void *p_data )
{
    ((ControlWindow *)p_data)->cb_button_load_csd(button);
}
     
// If the dynamic table load button is ON, we also send a table update to csound.

void ControlWindow::cb_dial_gen( Fl_Dial *p_dial)
{
	double dial_value = p_dial->value();
	Fl_Output *output = outputs_for_dials[p_dial];
	int dial_number = numbers_for_dials[p_dial];
    dial_values[dial_number] = dial_value;	
	char buffer[0xff];
	sprintf(buffer, "%5.2f", dial_value );
	// index point is dial_num - 4 because of the dial layout in the parent window
	int table_index = dial_number - 4;
	output->value( buffer );
	int dyn_table_on = button_dyn_table->value();
	char outstring[50];
	if ( dyn_table_on && table_index >= 0 && cs_performing ) {
		csound_update_ftable( cs_ftable_num, table_index, dial_value );
		sprintf( outstring, "You moved dial %i.\nUpdating table %i, index %i, with value %5.2f.\n", \
				 dial_number, cs_ftable_num, table_index, dial_value );		
	} else {
		sprintf( outstring, "You moved dial %i to value %5.3f.\n", dial_number, dial_value );
	}
	output_box->value( outstring );	
}

void ControlWindow::cb_button_play_note( Fl_Button *p_button)
{ 
	if ( !cs_performing ) {
		char *output_str = "Csound is not running.";
		output_box->value( output_str );
		return;
	}
	double amp, pitch, dur, p5;
	dur = dial_values[0];
	amp = dial_values[1];
	pitch = dial_values[2];
	p5 = dial_values[3];
	char output_str[100];
	sprintf( output_str, "Playing csound note:\ni1\t0\t%5.2f\t%5.2f\t%5.2f\t%5.2f", \
			 dur, amp, pitch, p5 );		
	output_box->value( output_str );
	csound_play_note( dur, amp, pitch, p5 );
}

void ControlWindow::cb_button_load_snapshot( Fl_Button *p_button)
{ 
	if ( !cs_performing ){
		char *output_str = "Csound is not running.";
		output_box->value( output_str );
		return;
	}
	double val_0 = dial_values[4];
	double val_1 = dial_values[5];
	double val_2 = dial_values[6];
	double val_3 = dial_values[7];
	char output_str[100];
	sprintf( output_str, "Loading csound ftable #%i with snapshot.\n%5.2f\t%5.2f\t%5.2f\t%5.2f", \
			 cs_ftable_num, val_0, val_1, val_2, val_3 );		
	output_box->value( output_str );
	csound_load_ftable( cs_ftable_num, val_0, val_1, val_2, val_3 );	
}

void ControlWindow::cb_button_start_csound( Fl_Button *p_button)
{
	if (csd_filename.empty()) {
		char *output_str = "You need to load a csd file.";
		output_box->value( output_str );
		return;
	}
	if (!cs_performing) {
		char *output_str = "Starting Csound performance.";
		output_box->value( output_str );
		csound_start();
	} else {
		char *output_str = "Csound is already running!";
		output_box->value( output_str );
	}
}

void ControlWindow::cb_button_stop_csound( Fl_Button *p_button)
{
	if (cs_performing) {
		char *output_str = "Stopping Csound performance.";
		output_box->value( output_str );
		csound_stop();
	} else {
		char *output_str = "Csound is not running!";
		output_box->value( output_str );
	}
}

void ControlWindow::cb_button_load_csd( Fl_Button *p_button)
{
    csound_load_csd();
}

/**
* While Csound is rendering,
* safely dispatch FLTK events.
*/
int ControlWindow::cb_thread_yield(void *)
{
    Fl::lock();
    Fl::wait(0.0);
    Fl::unlock();
    return 1;
}

int ControlWindow::csound_thread_routine_(void *p_data)
{
   return ((ControlWindow *)p_data)->csound_thread_routine();
}    

/**
* To permit the FLTK GUI to work during performance,
* Csound must perform in a separate thread.
*/
int ControlWindow::csound_thread_routine()
{
	const char *argv[] = {"csound", csd_filename.c_str()};
    csoundCompile(csound, 2, (char **)argv);
    // cs_performing is set by Csound, and go is set by the user.
    for(cs_performing = true, go = true; cs_performing && go; ) {
        // Csound will call cb_thread_yield.
        cs_performing = !csoundPerformKsmps(csound);
    }    
    csoundCleanup(csound);
    cs_performing = false;
    return true;
}

/**
* Use the Csound API to create the Csound performance thread, 
* first making sure that Csound will call the FLTK event dispatcher callback.
*/
void ControlWindow::csound_start()
{
    csoundSetYieldCallback(csound, cb_thread_yield); 
    csoundCreateThread(csound, &ControlWindow::csound_thread_routine_, this);
}

/** 
* Simply set a flag to indicate to the performance thread that it should
* end prematurely.
*/
void ControlWindow::csound_stop()
{
	go = false;
}

/**
* Don't actually load a CSD file, just store the filename of the CSD file
* for Csound to render.
*/
void ControlWindow::csound_load_csd()
{
	printf("Loading Csound csd file.\n");
	csd_filename = fl_file_chooser("Select .csd file", "*.csd", "*.csd" );
}

/**
* Send a note to instr 1: p1 = 1, p2 = 0, p3 = duration, p4=amplitude, p5=pitch, p6.
*/
void ControlWindow::csound_play_note( double cs_dur, double cs_amp, double cs_pitch, double cs_p6 )
{
    char buffer[0xff];
    sprintf(buffer, "i 1.0 0.0 %5.2f %5.2f %5.2f %5.2f\n", cs_dur, cs_amp, cs_pitch, cs_p6);
    printf( "Sending note to Csound: %s", buffer);
    csoundInputMessage(csound, buffer);
 }

void ControlWindow::csound_load_ftable( int cs_table_num, double cs_val_0, double cs_val_1, double cs_val_2, double cs_val_3 )
{
	printf( "Loading Csound table #%i:\n %5.2f %5.2f %5.2f %5.2f\n", \
		cs_table_num, cs_val_0, cs_val_1, cs_val_2, cs_val_3 );
	csoundTableSet(csound, cs_table_num, 0, cs_val_0);
	csoundTableSet(csound, cs_table_num, 1, cs_val_1);
	csoundTableSet(csound, cs_table_num, 2, cs_val_2);
	csoundTableSet(csound, cs_table_num, 3, cs_val_3);
}

void ControlWindow::csound_update_ftable( int cs_table_num, int cs_table_indx, double cs_table_value )
{
	printf( "Updating Csound table #%i, index %i, value %5.2f\n", \
		cs_table_num, cs_table_indx, cs_table_value );
	csoundTableSet(csound, cs_table_num, cs_table_indx, cs_table_value);
}

int main()
{
	ControlWindow *my_control_win = new ControlWindow(800,400, "Csound Controller" );
	return Fl::run();
}


