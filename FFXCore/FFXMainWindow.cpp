#include "FFXMainWindow.h"
#include "FFXPlugin.h"
#include "FFXFileListView.h"
#include "FFXFileSearchView.h"
#include <QtWidgets/QMessageBox>
#include <QSplitter>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QToolButton>

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
		//! Init menubar
		mMenuBar = new QMenuBar;
		setMenuBar(mMenuBar);

		//! Init main toolbar
		mMainToolBar = new QToolBar;
		addToolBar(mMainToolBar);

		//! Init status bar
		mStatusBar = new QStatusBar;
		mShowTaskBoardButton = new QToolButton;
		mShowTaskBoardButton->setIcon(QIcon(":/ffx/res/image/task.svg"));
		mStatusBar->addPermanentWidget(mShowTaskBoardButton);
		setStatusBar(mStatusBar);

		//! Init central widget
		mFileMainView = new FileMainView(this);
		mFileMainView->Goto(QString("D:\\"));
		mFileSearchView = new FileSearchView(this);
		QSplitter* splitterMain = new QSplitter(Qt::Horizontal);
		splitterMain->addWidget(mFileMainView);
		splitterMain->addWidget(mFileSearchView);
		splitterMain->setStretchFactor(0, 4);
		splitterMain->setStretchFactor(1, 1);
		setCentralWidget(splitterMain);
	}

	MainWindow* MainWindow::Instance() {
		return sInstance;
	}

	PluginManager* MainWindow::PluginManagerPtr() {
		return mPluginManager;
	}

	FileMainView* MainWindow::FileMainViewPtr() {
		return mFileMainView;
	}
}

