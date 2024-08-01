#include "FFXClipboardPanel.h"
#include <QGridLayout>
#include <QToolButton>
#include <QApplication>
#include <QClipboard>
#include <QMimeData>
#include <QUrl>
#include <QAction>

namespace FFX {
	ClipboardPanel::ClipboardPanel(QWidget*parent)
		: QWidget(parent) {
		SetupUi();
	}

	ClipboardPanel::~ClipboardPanel()
	{}

	void ClipboardPanel::SetupUi() {
		mMainLayout = new QGridLayout;
		mItemListView = new CommonFileListView;
		mClearButton = new QToolButton;
		mClearButton->setIcon(QIcon(":/ffx/res/image/delete.svg"));
		mRemoveSelection = new QAction(QObject::tr("Remove"));

		mMainLayout->setContentsMargins(0, 5, 0, 0);
		mMainLayout->addWidget(mClearButton, 0, 0, 1, 1);
		mMainLayout->addItem(new QSpacerItem(1, 1, QSizePolicy::Minimum, QSizePolicy::Expanding), 0, 1, 1, 1);
		mMainLayout->addWidget(mItemListView, 1, 0, 1, 2);
		mMainLayout->setRowStretch(1, 1);
		setLayout(mMainLayout);

		QClipboard* clipboard = QApplication::clipboard();
		connect(clipboard, &QClipboard::dataChanged, this, &ClipboardPanel::OnClipboardDataChanged);
		
		connect(mRemoveSelection, &QAction::triggered, this, &ClipboardPanel::OnRemoveSelection);
		connect(mClearButton, &QToolButton::clicked, this, &ClipboardPanel::OnClear);

		mItemListView->AddAction("RemoveSelection", mRemoveSelection);
	}

	void ClipboardPanel::OnClear() {
		mItemListView->RemoveAll();
		QClipboard* clipboard = QApplication::clipboard();
		clipboard->clear();
	}

	void ClipboardPanel::OnClipboardDataChanged() {
		QClipboard* clipboard = QApplication::clipboard();
		const QMimeData* mimeData = clipboard->mimeData();
		if (mimeData == nullptr || !mimeData->hasUrls())
			return;

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

}
