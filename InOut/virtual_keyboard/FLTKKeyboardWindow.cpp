#include "FLTKKeyboardWindow.hpp"


static void allNotesOff(Fl_Widget *widget, void * v) {
	FLTKKeyboardWindow *win = (FLTKKeyboardWindow *)v;
	win->keyboard->allNotesOff();
}

static void channelChange(Fl_Widget *widget, void * v) {
	Fl_Spinner *spinner = (Fl_Spinner *)widget;
	FLTKKeyboardWindow *win = (FLTKKeyboardWindow *)v;

	win->lock();
	win->channel = (int)spinner->value() - 1;

	win->programSpinner->value(win->program[win->channel] + 1);

	win->unlock();
}

static void programChange(Fl_Widget *widget, void * v) {
	Fl_Spinner *spinner = (Fl_Spinner *)widget;
	FLTKKeyboardWindow *win = (FLTKKeyboardWindow *)v;

	win->lock();
	win->program[win->channel] = (int)spinner->value() - 1;
	win->unlock();
}

FLTKKeyboardWindow::FLTKKeyboardWindow(CSOUND *csound, int W, int H, const char *L = 0)
    : Fl_Double_Window(W, H, L)
{

	this->csound = csound;
	this->mutex = csound->Create_Mutex(0);

    this->begin();

    this->channelSpinner = new Fl_Spinner(60, 0, 80, 20, "Channel");
    channelSpinner->maximum(128);
    channelSpinner->minimum(1);
    this->channelSpinner->callback((Fl_Callback*) channelChange, this);

    this->programSpinner = new Fl_Spinner(210, 0, 80, 20, "Program");
    programSpinner->maximum(128);
    programSpinner->minimum(1);
    this->programSpinner->callback((Fl_Callback*) programChange, this);


    this->allNotesOffButton = new Fl_Button(0, 20, W, 20, "All Notes Off");
    this->allNotesOffButton->callback((Fl_Callback*) allNotesOff, this);

    this->keyboard = new FLTKKeyboard(csound, 0, 40, W, 80, "Keyboard");

    this->end();

	channel = 0;

    for(int i = 0; i < 16; i++) {
    	program[i] = 0;
    	previousProgram[i] = 0;
    }

}

FLTKKeyboardWindow::~FLTKKeyboardWindow() {
	if (mutex) {
      	csound->DestroyMutex(mutex);
      	mutex = (void*) 0;
    }
}

int FLTKKeyboardWindow::handle(int event) {
    //this->csound->Message(this->csound, "Keyboard event: %d\n", event);

    switch(event) {
    	case FL_KEYDOWN:
    		return this->keyboard->handle(event);
    	case FL_KEYUP:
      		return this->keyboard->handle(event);
//        case FL_DEACTIVATE:
//        	this->keyboard->allNotesOff();
//        	csound->Message(csound, "Deactivate\n");
//        	return 1;
        default:
            return Fl_Double_Window::handle(event);
    }

}

void FLTKKeyboardWindow::lock() {
	if(mutex) {
      	csound->LockMutex(mutex);
	}
}

void FLTKKeyboardWindow::unlock() {
	if(mutex) {
      	csound->UnlockMutex(mutex);
	}
}
