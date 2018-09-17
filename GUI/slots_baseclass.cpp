#include "GUI/slots_baseclass.h"
#include "../includes.h"

    /* this handles (most) lineedit events*/
void GUI_slots_baseclass::lineedit_fun(QLineEdit* obj, tsvar<std::string> *var){
    if (var->set(obj->text().toStdString()))
        obj->setText(QString::fromStdString(var->get()));
    obj->clearFocus();
}
    /* this handles (most) spinbox events*/
void GUI_slots_baseclass::spinbox_fun(QSpinBox* obj, tsvar<int> *var){
    if (var->set(obj->value()))
        obj->setValue(var->get());
    obj->clearFocus();
}
void GUI_slots_baseclass::spinbox_fun(QSpinBox* obj, tsvar<unsigned> *var){
    if (var->set(obj->value()))
        obj->setValue(var->get());
    obj->clearFocus();
}
    /* this handles (most) slider events*/
void GUI_slots_baseclass::slider_fun(QSlider* obj,tsvar<int> *var,int value){
    if (var->set(value))
        obj->setValue(var->get());                  //TODO maybe revise this
}
