#include "FLTKKeyboardWindow.hpp"

FLTKKeyboardWindow::FLTKKeyboardWindow(CSOUND *csound, int W, int H, const char *L = 0)
    : Fl_Double_Window(W, H, L)
{

    this->begin();
    this->keyboard = new FLTKKeyboard(csound, 0,0,W, H, "Keyboard");

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

