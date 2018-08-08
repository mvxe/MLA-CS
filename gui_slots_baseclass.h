#ifndef GUI_SLOTS_BASECLASS_H
#define GUI_SLOTS_BASECLASS_H

#include <QLineEdit>
#include <QSpinBox>
#include "mutex_containers.h"


class GUI_slots_baseclass{
protected:
    virtual void lineedit_fun(QLineEdit* obj, mxvar<std::string>* var);
    virtual void spinbox_fun(QSpinBox* obj, mxvar<int> *var);
};

#endif // GUI_SLOTS_BASECLASS_H
