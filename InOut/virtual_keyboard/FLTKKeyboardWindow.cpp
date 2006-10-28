#include "FLTKKeyboardWindow.hpp"

FLTKKeyboardWindow::FLTKKeyboardWindow(CSOUND *csound, int W, int H, const char *L = 0)
    : Fl_Double_Window(W, H, L)
{

    this->begin();
    this->keyboard = new FLTKKeyboard(csound, 0,0,W, H, "Keyboard");

    this->end();

}
