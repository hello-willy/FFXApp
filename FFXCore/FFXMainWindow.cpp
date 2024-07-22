#include "FFXMainWindow.h"
#include "FFXPlugin.h"

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

		SetupUi();
		mPluginManager = new PluginManager;
	}

	MainWindow::~MainWindow()
	{}

	void MainWindow::SetupUi() {
		mFileMainView = new FileMainView(this);
		mFileMainView->SetRootPath(QString("D:\\"));
		setCentralWidget(mFileMainView);
	}

	MainWindow* MainWindow::Instance() {
		return sInstance;
	}

	PluginManager* MainWindow::PluginManagerPtr() {
		return mPluginManager;
	}
}

