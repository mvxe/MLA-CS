#include "GUI/slots_baseclass.h"


    /* this handles (most) lineedit events*/
void GUI_slots_baseclass::lineedit_fun(QLineEdit* obj, mxvar<std::string> *var){
    if (var->set(obj->text().toStdString()))
        obj->setText(QString::fromStdString(var->get()));
    obj->clearFocus();
}
    /* this handles (most) spinbox events*/
void GUI_slots_baseclass::spinbox_fun(QSpinBox* obj, mxvar<int> *var){
    if (var->set(obj->value()))
        obj->setValue(var->get());
    obj->clearFocus();
}
    /* this handles (most) slider events*/
void GUI_slots_baseclass::slider_fun(QSlider* obj,mxvar<int> *var,int value){
    if (var->set(value))
        obj->setValue(var->get());                  //TODO maybe revise this
}
