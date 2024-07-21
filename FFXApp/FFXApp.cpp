#include "FFXApp.h"
#include <QMessageBox>

FFXApp* FFXApp::sInstance = nullptr;
FFXApp::FFXApp(QWidget *parent)
    : QMainWindow(parent)
{
	if (sInstance) {
		QMessageBox::critical(
			this,
			QObject::tr("Error"),
			QObject::tr("Final File X is aready started."));
		abort();
	}
	sInstance = this;
    setupUi(this);
}

FFXApp::~FFXApp()
{}

FFXApp* FFXApp::Instance() {
    return sInstance;
}