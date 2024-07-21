#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_FFXApp.h"

class FFXApp : public QMainWindow, public Ui::FFXAppClass
{
    Q_OBJECT

public:
    FFXApp(QWidget *parent = nullptr);
    ~FFXApp();

public:
    static FFXApp* Instance();

private:
    static FFXApp* sInstance;
};
