#ifndef GUI_SLOTS_BASECLASS_H
#define GUI_SLOTS_BASECLASS_H

#include "GUI/includes.h"

class GUI_slots_baseclass{
protected:
    virtual void lineedit_fun(QLineEdit* obj, tsvar<std::string>* var);
    virtual void spinbox_fun(QSpinBox* obj, tsvar<unsigned> *var);
    virtual void spinbox_fun(QSpinBox* obj, tsvar<int> *var);
    virtual void slider_fun(QSlider* obj, tsvar<int> *var, int value);

        //TODO remove these
    virtual void lineedit_fun(QLineEdit* obj, mxvar<std::string>* var);
    virtual void spinbox_fun(QSpinBox* obj, mxvar<int> *var);
    virtual void slider_fun(QSlider* obj, mxvar<int> *var, int value);
};

#endif // GUI_SLOTS_BASECLASS_H
