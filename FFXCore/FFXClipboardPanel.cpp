#include "FFXClipboardPanel.h"
#include "FFXMainWindow.h"
#include "FFXFileHandler.h"
#include "FFXTaskPanel.h"

#include <QGridLayout>
#include <QToolButton>
#include <QApplication>
#include <QClipboard>
#include <QMimeData>
#include <QUrl>
#include <QAction>
#include <QPainter>
#include <QLabel>
#include <QMenu>
#include <QFileDialog>

namespace FFX {
	ClipboardPanelHeader::ClipboardPanelHeader(QWidget* parent)
		: QWidget(parent) {
		SetupUi();
	}

	void ClipboardPanelHeader::AddWidget(QWidget* widget) {
		mMainLayout->insertWidget(1, widget);
	}

	void ClipboardPanelHeader::AddAction(QAction* action) {
		mOperatorMenu->addAction(action);
	}

	QMenu* ClipboardPanelHeader::AddMenuAction(const QString& name) {
		QMenu* menu = new QMenu(name);
		mOperatorMenu->addAction(menu->menuAction());
		return menu;
	}

	void ClipboardPanelHeader::RemoveMenu(const QMenu* menu) {
		mOperatorMenu->removeAction(menu->menuAction());
	}

	void ClipboardPanelHeader::AddSeperator() {
		QFrame* seperator = new QFrame;
		seperator->setFrameShape(QFrame::VLine);
		seperator->setFrameShadow(QFrame::Sunken);
		mMainLayout->addWidget(seperator);
	}

	void ClipboardPanelHeader::SetupUi() {
		mHeaderLabel = new QLabel(QObject::tr("Clipboard Panel"));
		mHeaderLabel->setFixedHeight(32);
		mMainLayout = new QHBoxLayout;
		mOperatorButton = new QToolButton;
		mOperatorMenu = new QMenu;

		mMainLayout->setContentsMargins(0, 9, 0, 0);

		mMainLayout->addWidget(mHeaderLabel, 1);
		AddSeperator();

		mOperatorButton->setText(QObject::tr("Operator"));
		mOperatorButton->setIcon(QIcon(":/ffx/res/image/menu-ext.svg"));
		mOperatorButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
		mOperatorButton->setFixedHeight(32);
		mOperatorButton->setIconSize(QSize(24, 24));
		mOperatorButton->setPopupMode(QToolButton::InstantPopup);
		mOperatorButton->setMenu(mOperatorMenu);

		mMainLayout->addWidget(mOperatorButton);

		setLayout(mMainLayout);
	}

	void ClipboardPanelHeader::paintEvent(QPaintEvent* event) {
		QPainter painter(this);
		painter.setRenderHint(QPainter::Antialiasing);
		painter.fillRect(rect().marginsAdded(QMargins(0, -9, 0, 0)), QColor("#EAEAEA"));
	}

	ClipboardPanel::ClipboardPanel(QWidget*parent)
		: QWidget(parent) {
		SetupUi();
	}

	ClipboardPanel::~ClipboardPanel()
	{}

	void ClipboardPanel::SetupUi() {
		mClipboardPanelHeader = new ClipboardPanelHeader;
		mMainLayout = new QGridLayout;
		mItemListView = new CommonFileListView;
		mClearButton = new QToolButton;
		mClearButton->setText(QObject::tr("Clear"));
		mClearButton->setIcon(QIcon(":/ffx/res/image/delete.svg"));
		mClearButton->setFixedHeight(32);
		mClearButton->setIconSize(QSize(24, 24));

		mRemoveSelectionAction = new QAction(QIcon(":/ffx/res/image/remove-item.svg"), QObject::tr("Remove Selected Item(s)"));
		mCopyFileToAction = new QAction(QObject::tr("Copy to..."));
		mMoveFileToAction = new QAction(QObject::tr("Move to..."));

		mRemoveSelectionButton = new QToolButton;
		mRemoveSelectionButton->setDefaultAction(mRemoveSelectionAction);
		mRemoveSelectionButton->setFixedHeight(32);
		mRemoveSelectionButton->setIconSize(QSize(24, 24));

		mGotoParentDirButton = new QToolButton;
		mGotoParentDirButton->setDefaultAction(mItemListView->Action("Goto"));
		mGotoParentDirButton->setFixedHeight(32);
		mGotoParentDirButton->setIconSize(QSize(24, 24));

		mClipboardPanelHeader->AddWidget(mClearButton);
		mClipboardPanelHeader->AddWidget(mRemoveSelectionButton);
		mClipboardPanelHeader->AddWidget(mGotoParentDirButton);

		mMainLayout->addWidget(mClipboardPanelHeader, 0, 0, 1, 1);
		mMainLayout->addWidget(mItemListView, 1, 0, 1, 1);
		mMainLayout->setRowStretch(1, 1);
		mMainLayout->setContentsMargins(0, 0, 5, 0);
		setLayout(mMainLayout);

		QClipboard* clipboard = QApplication::clipboard();
		connect(clipboard, &QClipboard::dataChanged, this, &ClipboardPanel::OnClipboardDataChanged);
		
		connect(mRemoveSelectionAction, &QAction::triggered, this, &ClipboardPanel::OnRemoveSelection);
		connect(mClearButton, &QToolButton::clicked, this, &ClipboardPanel::OnClear);
		connect(mCopyFileToAction, &QAction::triggered, this, &ClipboardPanel::OnCopyFileTo);
		connect(mMoveFileToAction, &QAction::triggered, this, &ClipboardPanel::OnMoveFileTo);

		connect(MainWindow::Instance()->TaskPanelPtr(), &TaskPanel::TaskComplete, this, &ClipboardPanel::OnTaskComplete);
		mItemListView->AddAction("RemoveSelection", mRemoveSelectionAction);
		
		mClipboardPanelHeader->AddAction(mCopyFileToAction);
		mClipboardPanelHeader->AddAction(mMoveFileToAction);
	}

	void ClipboardPanel::OnClear() {
		mItemListView->RemoveAll();
		QClipboard* clipboard = QApplication::clipboard();
		clipboard->clear();
	}

	void ClipboardPanel::OnClipboardDataChanged() {
		QClipboard* clipboard = QApplication::clipboard();
		const QMimeData* mimeData = clipboard->mimeData();
		if (mimeData == nullptr || !mimeData->hasUrls()) {
			mItemListView->RemoveAll();
			return;
		}

		QStringList filesInClipboard;
		QList<QUrl> urls = mimeData->urls();
		for (const QUrl& url : urls) {
			filesInClipboard << url.toLocalFile();
		}

		QStringList filesInList;
		QStringList allFileEntry = mItemListView->AllRow();
		int count = allFileEntry.size();
		for (int i = count - 1; i >= 0; i--) {
			QString f = allFileEntry[i];
			if (filesInClipboard.contains(f)) {
				filesInList << f;
				continue;
			}
			mItemListView->RemoveRow(i);
		}

		for (QString f : filesInClipboard) {
			if (filesInList.contains(f))
				continue;
			mItemListView->AddItem(f);
		}

		allFileEntry = mItemListView->AllRow();
		int dirCount = 0;
		int fileCount = 0;
		for (const QString& f : allFileEntry) {
			QFileInfo fi(f);
			if (fi.isDir())
				dirCount++;
			if (fi.isFile() || fi.isSymLink())
				fileCount++;
		}
	}

	void ClipboardPanel::OnRemoveSelection() {
		QClipboard* clipboard = QApplication::clipboard();
		const QMimeData* mimeData = clipboard->mimeData();
		if (mimeData == nullptr || !mimeData->hasUrls()) {
			mItemListView->RemoveAll();
			return;
		}

		QList<QString> files = mItemListView->SelectedFiles();
		if (files.isEmpty())
			return;

		QList<QUrl> filesInClipboard;
		QList<QUrl> urls = mimeData->urls();
		for (const QUrl& url : urls) {
			QString file = url.toLocalFile();
			if (files.contains(file))
				continue;
			filesInClipboard << url;
		}

		QMimeData* newMimeData = new QMimeData;
		newMimeData->setUrls(filesInClipboard);
		clipboard->clear();
		clipboard->setMimeData(newMimeData);
	}

	ClipboardPanelHeader* ClipboardPanel::Header () {
		return mClipboardPanelHeader;
	}

	void ClipboardPanel::OnTaskComplete(int taskId, bool success) {
		QClipboard* clipboard = QApplication::clipboard();
		const QMimeData* mimeData = clipboard->mimeData();
		if (mimeData == nullptr || !mimeData->hasUrls()) {
			mItemListView->RemoveAll();
			return;
		}
		
		QList<QUrl> filesInClipboard;
		QList<QUrl> urls = mimeData->urls();
		for (const QUrl& url : urls) {
			QString file = url.toLocalFile();
			if(!QFileInfo::exists(file))
				continue;
			filesInClipboard << url;
		}

		QMimeData* newMimeData = new QMimeData;
		newMimeData->setUrls(filesInClipboard);
		clipboard->clear();
		clipboard->setMimeData(newMimeData);
	}

	void ClipboardPanel::OnTaskFileHandled(int taskId, const QFileInfo& fileInput, const QFileInfo& fileOutput, bool success, const QString& message) {
	
	}

	void ClipboardPanel::OnCopyFileTo() {
		QStringList files = mItemListView->AllFiles();
		if (files.isEmpty())
			return;

		QString outputDir = QFileDialog::getExistingDirectory(nullptr, QObject::tr("Select output directory"), MainWindow::Instance()->FileMainViewPtr()->RootPath());
		MainWindow::Instance()->TaskPanelPtr()->Submit(FileInfoList(files), std::make_shared<FileCopyHandler>(outputDir));
	}

	void ClipboardPanel::OnMoveFileTo() {
		QStringList files = mItemListView->AllFiles();
		if (files.isEmpty())
			return;

		QString outputDir = QFileDialog::getExistingDirectory(nullptr, QObject::tr("Select output directory"), MainWindow::Instance()->FileMainViewPtr()->RootPath());
		MainWindow::Instance()->TaskPanelPtr()->Submit(FileInfoList(files), std::make_shared<FileMoveHandler>(outputDir));
	}
}
