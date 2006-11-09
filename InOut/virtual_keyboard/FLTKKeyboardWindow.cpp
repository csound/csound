#include "FLTKKeyboardWindow.hpp"

static void allNotesOff(Fl_Widget *widget, void * v) {
        FLTKKeyboardWindow *win = (FLTKKeyboardWindow *)v;
        win->keyboard->allNotesOff();
}

static void channelChange(Fl_Widget *widget, void * v) {
        Fl_Spinner *spinner = (Fl_Spinner *)widget;
        FLTKKeyboardWindow *win = (FLTKKeyboardWindow *)v;

        win->lock();


        win->keyboardMapping->setCurrentChannel((int)spinner->value() - 1);

        win->bankChoice->value(win->keyboardMapping->getCurrentBank());

        win->setProgramNames();

        win->unlock();
}

static void bankChange(Fl_Widget *widget, void * v) {
        Fl_Choice *choice = (Fl_Choice *)widget;
        FLTKKeyboardWindow *win = (FLTKKeyboardWindow *)v;

        win->lock();

        win->keyboardMapping->setCurrentBank((int)choice->value());

        win->setProgramNames();

        win->unlock();
}

static void programChange(Fl_Widget *widget, void * v) {
        Fl_Choice *choice = (Fl_Choice *)widget;
        FLTKKeyboardWindow *win = (FLTKKeyboardWindow *)v;

        win->lock();

        win->keyboardMapping->setCurrentProgram((int)choice->value());

        win->unlock();
}

FLTKKeyboardWindow::FLTKKeyboardWindow(CSOUND *csound,
          const char *deviceMap,
          int W, int H, const char *L = 0)
    : Fl_Double_Window(W, H, L)
{

        this->csound = csound;
        this->mutex = csound->Create_Mutex(0);

        this->keyboardMapping = new KeyboardMapping(csound, deviceMap);

    this->begin();

    this->channelSpinner = new Fl_Spinner(60, 0, 80, 20, "Channel");
    channelSpinner->maximum(16);
    channelSpinner->minimum(1);
    this->channelSpinner->callback((Fl_Callback*) channelChange, this);

        this->bankChoice = new Fl_Choice(180, 0, 180, 20, "Bank");
        this->programChoice = new Fl_Choice(420, 0, 200, 20, "Program");

        bankChoice->clear();

        for(unsigned int i = 0; i < keyboardMapping->banks.size(); i++) {
                bankChoice->add(keyboardMapping->banks[i]->name);
        }

        bankChoice->value(0);

        setProgramNames();

        this->bankChoice->callback((Fl_Callback*)bankChange, this);
        this->programChoice->callback((Fl_Callback*)programChange, this);


    this->allNotesOffButton = new Fl_Button(0, 20, W, 20, "All Notes Off");
    this->allNotesOffButton->callback((Fl_Callback*) allNotesOff, this);

    this->keyboard = new FLTKKeyboard(csound, 0, 40, W, 80, "Keyboard");

    this->end();

}

FLTKKeyboardWindow::~FLTKKeyboardWindow() {
        if (mutex) {
        csound->DestroyMutex(mutex);
        mutex = (void*) 0;
    }
    delete keyboardMapping;
}

void FLTKKeyboardWindow::setProgramNames() {

        Bank* bank = keyboardMapping->banks[keyboardMapping->getCurrentBank()];

        programChoice->clear();

        for( vector<Program>::iterator iter = bank->programs.begin();
                iter != bank->programs.end(); iter++ ) {
                programChoice->add((*iter).name);
        }

        programChoice->value(bank->currentProgram);
}

int FLTKKeyboardWindow::handle(int event) {
    //this->csound->Message(this->csound, "Keyboard event: %d\n", event);

    switch(event) {
        case FL_KEYDOWN:
                return this->keyboard->handle(event);
        case FL_KEYUP:
                return this->keyboard->handle(event);
//        case FL_DEACTIVATE:
//              this->keyboard->allNotesOff();
//              csound->Message(csound, "Deactivate\n");
//              return 1;
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
