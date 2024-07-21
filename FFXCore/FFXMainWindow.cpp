#include "FFXMainWindow.h"
#include <QtWidgets/QMessageBox>

namespace FFX {
	MainWindow* MainWindow::sInstance = nullptr;
	MainWindow::MainWindow(QWidget* parent)
		: QMainWindow(parent) {
		if (sInstance) {
			QMessageBox::critical(
				this,
				QObject::tr("Error"),
				QObject::tr("Final File X is aready started."));
			abort();
		}
		sInstance = this;
	}

	MainWindow::~MainWindow()
	{}

	MainWindow* MainWindow::Instance() {
		return sInstance;
	}
}

