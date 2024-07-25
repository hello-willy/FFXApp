#include "FFXMainWindow.h"
#include "FFXPlugin.h"
#include "FFXFileListView.h"
#include "FFXFileSearchView.h"
#include "FFXTaskPanel.h"

#include <QtWidgets/QMessageBox>
#include <QSplitter>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QToolButton>
#include <QDockWidget>

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
		mMenuBar = new QMenuBar;
		mMainToolBar = new QToolBar;
		mStatusBar = new QStatusBar;
		mTaskPanel = new TaskPanel;
		mFileMainView = new FileMainView(this);
		mFileSearchView = new FileSearchView(this);

		//! Init menubar
		setMenuBar(mMenuBar);
		mFileMenu = new QMenu(QObject::tr("&File"));
		mViewMenu = new QMenu(QObject::tr("&View"));
		mMenuBar->addAction(mFileMenu->menuAction());
		mMenuBar->addAction(mViewMenu->menuAction());

		//! Init main toolbar
		addToolBar(mMainToolBar);

		//! Init status bar
		mShowTaskBoardButton = new QToolButton;
		mStatusBar->addPermanentWidget(mShowTaskBoardButton);
		setStatusBar(mStatusBar);

		//! Init central widget
		QSplitter* splitterMain = new QSplitter(Qt::Horizontal);
		splitterMain->addWidget(mFileMainView);
		splitterMain->addWidget(mFileSearchView);
		splitterMain->setStretchFactor(0, 4);
		splitterMain->setStretchFactor(1, 1);
		setCentralWidget(splitterMain);

		//! Init task panel
		mTaskDocker = new QDockWidget;
		mTaskDocker->setWidget(mTaskPanel);
		mTaskDocker->setWindowTitle(QObject::tr("Task Panel"));
		addDockWidget(Qt::BottomDockWidgetArea, mTaskDocker);
		mViewMenu->addAction(mTaskDocker->toggleViewAction());

		mTaskDocker->toggleViewAction()->setIcon(QIcon(":/ffx/res/image/task.svg"));
		mShowTaskBoardButton->setDefaultAction(mTaskDocker->toggleViewAction());

		//! Setup signals/slots
		connect(mFileMainView, &FileMainView::CurrentPathChanged, this, [=](const QString& path) { mFileSearchView->SetSearchDir(path); });
		connect(mTaskPanel, &TaskPanel::TaskComplete, mFileSearchView, &FileSearchView::OnSearchComplete);
		connect(mTaskPanel, &TaskPanel::TaskFileHandled, mFileSearchView, &FileSearchView::OnSearchFileMatched);

		mFileMainView->Goto(QString("D:\\"));
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

	TaskPanel* MainWindow::TaskPanelPtr() {
		return mTaskPanel;
	}
}

