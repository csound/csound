#include "FLTKKeyboardWindow.hpp"


static void allNotesOff(Fl_Widget *widget, void * v) {
	FLTKKeyboardWindow *win = (FLTKKeyboardWindow *)v;
	win->keyboard->allNotesOff();
}

FLTKKeyboardWindow::FLTKKeyboardWindow(CSOUND *csound, int W, int H, const char *L = 0)
    : Fl_Double_Window(W, H, L)
{

    this->begin();
    this->allNotesOffButton = new Fl_Button(0, 0, W, 20, "All Notes Off");
    this->allNotesOffButton->callback((Fl_Callback*) allNotesOff, this);
    this->keyboard = new FLTKKeyboard(csound, 0, 20, W, 80, "Keyboard");
	this->csound = csound;

    this->end();

}

int FLTKKeyboardWindow::handle(int event) {
    //this->csound->Message(this->csound, "Keyboard event: %d\n", event);

    switch(event) {
    	case FL_KEYDOWN:
    		return 0;
    	case FL_KEYUP:
      		if (Fl::focus() == this) {

      		}
        	return 0;
        default:
            return Fl_Double_Window::handle(event);
    }

}

