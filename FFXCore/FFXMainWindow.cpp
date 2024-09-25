#include "FFXMainWindow.h"
#include "FFXPlugin.h"
#include "FFXFileListView.h"
#include "FFXFileSearchView.h"
#include "FFXTaskPanel.h"
#include "FFXString.h"
#include "FFXClipboardPanel.h"
#include "FFXFileQuickView.h"
#include "FFXFileHandler.h"
#include "FFXHandlerSettingDialog.h"

#include <QtWidgets/QMessageBox>
#include <QSplitter>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QToolButton>
#include <QDockWidget>
#include <QLabel>
#include <QClipboard>
#include <QApplication>
#include <QMimeData>
#include <QShortcut>

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
	}

	MainWindow::~MainWindow() {
		delete mAppConfig;
		delete mPluginManager;
		delete mHandlerFactory;
	}

	void MainWindow::SetupUi() {
		setObjectName("MainWindow");
		
		mMenuBar = new QMenuBar;
		mMainToolBar = new QToolBar;
		mStatusBar = new QStatusBar;
		mAppConfig = new AppConfig;
		mTaskPanel = new TaskPanel;
		mFileMainView = new FileMainView(this);
		mFileSearchView = new FileSearchView(this);
		mHandlerFactory = new HandlerFactory;
		mPluginManager = new PluginManager;

		//! Init menubar
		setMenuBar(mMenuBar);
		mFileMenu = new QMenu(QObject::tr("&File"));
		mViewMenu = new QMenu(QObject::tr("&View"));
		mPluginMenu = new QMenu(QObject::tr("&Plugin"));
		mHelpMenu = new QMenu;
		mMenuBar->addAction(mFileMenu->menuAction());
		mMenuBar->addAction(mViewMenu->menuAction());
		mMenuBar->addAction(mPluginMenu->menuAction());
		mMenuBar->addAction(mHelpMenu->menuAction());
		//! Init main toolbar
		addToolBar(mMainToolBar);
		mMainToolBar->addAction(mFileMainView->MakeDirAction());
		QList<QAction*> makefileActions = mFileMainView->MakeFileActions();
		mMainToolBar->addAction(makefileActions[0]);
		
		//! Init File Menu
		mFileMenu->addAction(mFileMainView->MakeDirAction());
		QMenu* makefileMenu = new QMenu(QObject::tr("Make File..."));
		makefileMenu->setIcon(QIcon(":/ffx/res/image/mk-file.svg"));
		mFileMenu->addAction(makefileMenu->menuAction());
		for (int i = 0; i < makefileActions.size(); i++) {
			makefileMenu->addAction(makefileActions[i]);
		}
		mFileMenu->addSeparator();
		mFileMenu->addAction(mFileMainView->PasteFilesAction());
		mFileMenu->addAction(mFileMainView->MoveFilesAction());
		mFileMenu->addAction(mFileMainView->RenameAction());
		mFileMenu->addAction(mFileMainView->EnvelopeFilesAction());
		mFileMenu->addAction(mFileMainView->ClearFolderAction());

		//! Init plugin menu
		mPluginMenu->addAction(mPluginManager->InstallPluginAction());

		mHandlerSettingAction = new QAction(QStringLiteral("Handler Setting"));
		connect(mHandlerSettingAction, &QAction::triggered, this, &MainWindow::OnHandlerSetting);
		mPluginMenu->addAction(mHandlerSettingAction);

		//! Init status bar
		mShowTaskBoardButton = new QToolButton;
		mCurrentDirInfoLabel = new QLabel;
		mSelectFilesInfoLabel = new QLabel;
		mFileSearchFileInfoLabel = new QLabel;
		mClipboardButton = new QToolButton;
		mTaskInfoLabel = new QLabel(QObject::tr("Ready"));
		//mClipboardButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
		
		mStatusBar->addWidget(mCurrentDirInfoLabel);
		mStatusBar->addWidget(mSelectFilesInfoLabel);
		mStatusBar->addPermanentWidget(mClipboardButton);
		mStatusBar->addPermanentWidget(mFileSearchFileInfoLabel);
		mStatusBar->addPermanentWidget(mShowTaskBoardButton);
		mStatusBar->addPermanentWidget(mTaskInfoLabel);
		setStatusBar(mStatusBar);

		//! Init central widget
		setCentralWidget(mFileMainView);

		//! Init task panel
		mTaskDocker = new QDockWidget;
		mTaskDocker->setWidget(mTaskPanel);
		mTaskDocker->setWindowTitle(QObject::tr("Task Panel"));
		addDockWidget(Qt::BottomDockWidgetArea, mTaskDocker);
		mViewMenu->addAction(mTaskDocker->toggleViewAction());
		mTaskDocker->toggleViewAction()->setIcon(QIcon(":/ffx/res/image/task.svg"));
		mTaskDocker->setHidden(true);

		//! Init file search panel
		mFileSearchDocker = new QDockWidget;
		mFileSearchDocker->setWidget(mFileSearchView);
		mFileSearchDocker->setWindowTitle(QObject::tr("File Search Panel"));
		addDockWidget(Qt::RightDockWidgetArea, mFileSearchDocker);
		mFileSearchDocker->setHidden(true);
		mViewMenu->addAction(mFileSearchDocker->toggleViewAction());
		mFileSearchDocker->toggleViewAction()->setIcon(QIcon(":/ffx/res/image/search.svg"));
		mFileSearchFileInfoLabel->setText(QObject::tr(" %1 files").arg(0));
		mClipboardButton->setDefaultAction(mFileSearchDocker->toggleViewAction());

		mShowTaskBoardButton->setDefaultAction(mTaskDocker->toggleViewAction());

		mActiveSearchShortcut = new QShortcut(QKeySequence("Ctrl+F"), this);
		mActiveSearchShortcut->setContext(Qt::WindowShortcut);
		connect(mActiveSearchShortcut, &QShortcut::activated, this, &MainWindow::OnActivateFileSearch);

		//! Setup signals/slots
		connect(mFileMainView, &FileMainView::CurrentPathChanged, this, [=](const QString& path) { 
			mFileSearchView->SetSearchDir(path); 
			UpdateCurrentDirInfo();
			UpdateSelectFilesInfo(QStringList());
			});
		connect(mTaskPanel, &TaskPanel::TaskComplete, mFileSearchView, &FileSearchView::OnSearchComplete);
		connect(mTaskPanel, &TaskPanel::TaskComplete, mFileMainView, &FileMainView::RefreshFileListView);
		connect(mTaskPanel, &TaskPanel::TaskComplete, this, &MainWindow::OnTaskInfoUpdate);
		connect(mTaskPanel, &TaskPanel::TaskSubmit, this, &MainWindow::OnTaskInfoUpdate);
		connect(mTaskPanel, &TaskPanel::TaskFileHandled, mFileMainView, &FileMainView::RefreshFileListView);
		connect(mTaskPanel, &TaskPanel::TaskFileHandled, mFileSearchView, &FileSearchView::OnSearchFileMatched);
		connect(mFileMainView, &FileMainView::SelectionChanged, this, [=](QStringList files) {
			UpdateSelectFilesInfo(files);
			});
		//QClipboard* clipboard = QApplication::clipboard();
		//connect(clipboard, &QClipboard::dataChanged, this, &MainWindow::OnClipboardDataChanged);

		mFileMainView->Restore(mAppConfig);

		mPluginManager->AutoLoad();
	}

	void MainWindow::closeEvent(QCloseEvent* event) {
		Save(mAppConfig);

		mFileMainView->Save(mAppConfig);
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

	AppConfig* MainWindow::AppConfigPtr() {
		return mAppConfig;
	}

	HandlerFactory* MainWindow::HandlerFactoryPtr() {
		return mHandlerFactory;
	}

	void MainWindow::Save(AppConfig* config) {
		// save window pos
		int isFull = this->isMaximized();
		if (!isFull)
		{
			QRect r = rect();
			QPoint p = pos();
			mAppConfig->WriteItem(objectName(), "WinPos", QRect(p.x(), p.y(), r.width(), r.height()));
			//mAppConfig->SaveMainWindowPos(QRect(p.x(), p.y(), r.width(), r.height()));
		}
		else {
			mAppConfig->WriteItem(objectName(), "WinPos", QRect(-1, -1, 0, 0));
			//mAppConfig->SaveMainWindowPos(QRect(-1, -1, 0, 0));
		}
	}

	void MainWindow::Restore(AppConfig* config) {
		QRect rect = config->ReadItem(objectName(), "WinPos").toRect();
		if (!rect.isValid()) {
			showMaximized();
			setWindowFlags(Qt::WindowMaximizeButtonHint | Qt::WindowCloseButtonHint | Qt::WindowMinimizeButtonHint);
		}
		else {
			resize(rect.width(), rect.height());
			move(rect.x(), rect.y());
		}
	}

	void MainWindow::AddMenu(QMenu* menu) {
		mMenuBar->addAction(menu->menuAction());
	}

	void MainWindow::RemoveMenu(QMenu* menu) {
		mMenuBar->removeAction(menu->menuAction());
	}

	void MainWindow::AddToolbar(QToolBar* toolbar, Qt::ToolBarArea area) {
		addToolBar(area, toolbar);
	}

	void MainWindow::RemoveToolbar(QToolBar* toolbar) {
		removeToolBar(toolbar);
	}

	void MainWindow::ShowMessage(const QString& message, int timeout) {
		mStatusBar->showMessage(message, timeout);
	}

	QString MainWindow::PluginPath() const {
		QDir currentDir(QCoreApplication::applicationDirPath());
		currentDir.cd("ffxplugins");
		return currentDir.absolutePath();
	}

	void MainWindow::UpdateCurrentDirInfo() {
		QString root = mFileMainView->RootPath();
		QDir f(root);
		QFileInfoList files = f.entryInfoList(QDir::AllEntries | QDir::Hidden | QDir::NoDotAndDotDot);
		FileStatHandler handler(false);
		handler.Handle(files);
		QString info = QObject::tr("%1 files%2 %3, %4 directories").arg(handler.FileCount()).arg(handler.HiddenFileCount() > 0 ? QObject::tr("(%1 Hidden files/directories)").arg(handler.HiddenFileCount()) : "")
			.arg(String::BytesHint(handler.TotalSize())).arg(handler.DirCount());

		mCurrentDirInfoLabel->setText(info);
	}

	void MainWindow::UpdateSelectFilesInfo(QStringList files) {
		if (files.isEmpty()) {
			mSelectFilesInfoLabel->setText("0 files selected");
			return;
		}
		FileStatHandler handler(false);
		handler.Handle(FileInfoList(files));
		QString info = QObject::tr("%1 files %2 directories selected (%3)").arg(handler.FileCount())
			.arg(handler.DirCount()).arg(String::BytesHint(handler.TotalSize()));
		mSelectFilesInfoLabel->setText(info);
	}

	void MainWindow::UpdateFileSearchInfo(int count) {
		mFileSearchFileInfoLabel->setText(QObject::tr(" %1 files").arg(count));
	}

	void MainWindow::OnTaskInfoUpdate() {
		int running = mTaskPanel->RunningTaskCount();
		if (running == 0) {
			mTaskInfoLabel->setText(QObject::tr("Ready"));
		} else {
			mTaskInfoLabel->setText(QObject::tr("%1 task running").arg(running));
		}
	}

	void MainWindow::OnActivateFileSearch() {
		if (mFileSearchDocker->isHidden()) {
			mFileSearchDocker->setHidden(false);
			mFileSearchView->ActivateSearch();
		} else {
			mFileSearchDocker->setHidden(true);
		}
	}

	void MainWindow::OnHandlerSetting() {
		HandlerSettingDialog dlg(std::make_shared<FileMoveHandler>("d:/", true), this);
		dlg.exec();
	}
}

