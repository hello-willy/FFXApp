#include "FFXMainWindow.h"
#include "FFXPlugin.h"
#include "FFXFileListView.h"
#include "FFXFileSearchView.h"
#include "FFXTaskPanel.h"
#include "FFXString.h"
#include "FFXClipboardPanel.h"

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
		mMainToolBar->addAction(mFileMainView->MakeDirAction());
		QList<QAction*> makefileActions = mFileMainView->MakeFileActions();
		mMainToolBar->addAction(makefileActions[0]);
		
		//! Init File Menu
		mFileMenu->addAction(mFileMainView->MakeDirAction());
		QMenu* makefileMenu = new QMenu(QObject::tr("Make File..."));
		makefileMenu->setIcon(QIcon(":/ffx/res/image/plus.svg"));
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

		//! Init status bar
		mShowTaskBoardButton = new QToolButton;
		mCurrentDirInfoLabel = new QLabel;
		mSelectFilesInfoLabel = new QLabel;
		mClipboardInfoLabel = new QLabel;
		mClipboardButton = new QToolButton;
		//mClipboardButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
		
		mStatusBar->addPermanentWidget(mCurrentDirInfoLabel);
		mStatusBar->addPermanentWidget(mSelectFilesInfoLabel);
		mStatusBar->addPermanentWidget(mClipboardButton);
		mStatusBar->addPermanentWidget(mClipboardInfoLabel);
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

		//! Init clipboard panel
		mClipboardPanel = new ClipboardPanel;
		mClipboardPanelDocker = new QDockWidget;
		mClipboardPanelDocker->setWidget(mClipboardPanel);
		mClipboardPanelDocker->setWindowTitle(QObject::tr("Clipboard Panel"));
		addDockWidget(Qt::RightDockWidgetArea, mClipboardPanelDocker);
		mClipboardPanelDocker->setHidden(true);
		mViewMenu->addAction(mClipboardPanelDocker->toggleViewAction());
		mClipboardPanelDocker->toggleViewAction()->setIcon(QIcon(":/ffx/res/image/clipboard.svg"));
		mClipboardInfoLabel->setText(QObject::tr(" %1 items").arg(0));
		mClipboardButton->setDefaultAction(mClipboardPanelDocker->toggleViewAction());

		//QSize statusBarSize = mStatusBar->sizeHint();
		//mShowTaskBoardButton->setFixedSize(QSize(statusBarSize.height() - 8, statusBarSize.height() - 8));
		//mShowTaskBoardButton->setIconSize(QSize(20, 20));
		mShowTaskBoardButton->setDefaultAction(mTaskDocker->toggleViewAction());

		//! Setup signals/slots
		connect(mFileMainView, &FileMainView::CurrentPathChanged, this, [=](const QString& path) { 
			mFileSearchView->SetSearchDir(path); 
			UpdateCurrentDirInfo();
			UpdateSelectFilesInfo(QStringList());
			});
		connect(mTaskPanel, &TaskPanel::TaskComplete, mFileSearchView, &FileSearchView::OnSearchComplete);
		connect(mTaskPanel, &TaskPanel::TaskComplete, mFileMainView, &FileMainView::RefreshFileListView);
		connect(mTaskPanel, &TaskPanel::TaskFileHandled, mFileMainView, &FileMainView::RefreshFileListView);
		connect(mTaskPanel, &TaskPanel::TaskFileHandled, mFileSearchView, &FileSearchView::OnSearchFileMatched);
		connect(mFileMainView, &FileMainView::SelectionChanged, this, [=](QStringList files) {
			UpdateSelectFilesInfo(files);
			});
		QClipboard* clipboard = QApplication::clipboard();
		connect(clipboard, &QClipboard::dataChanged, this, &MainWindow::OnClipboardDataChanged);

		
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

	void MainWindow::AddMenu(QMenu* menu) {
		mMenuBar->insertAction(mViewMenu->menuAction(), menu->menuAction());
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

	void MainWindow::UpdateCurrentDirInfo() {
		QString root = mFileMainView->RootPath();
		QDir f(root);
		QFileInfoList files = f.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot);
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

	void MainWindow::OnClipboardDataChanged() {
		QClipboard* clipboard = QApplication::clipboard();
		const QMimeData* mimeData = clipboard->mimeData();
		int count = 0;
		if (!(mimeData == nullptr || !mimeData->hasUrls())) {
			count = mimeData->urls().size();
		}
		mClipboardInfoLabel->setText(QObject::tr(" %1 items").arg(count));
	}
}

