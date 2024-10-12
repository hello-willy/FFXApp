#include "FFXFileSearchView.h"
#include "FFXMainWindow.h"
#include "FFXFileHandler.h"
#include "FFXFileFilterExpr.h"
#include "FFXTaskPanel.h"
#include "FFXCommonFileListView.h"

#include <QPainter>
#include <QFileIconProvider>
#include <QAction>
#include <QGridLayout>
#include <QLineEdit>
#include <QToolButton>
#include <QShortcut>

namespace FFX {
	/************************************************************************************************************************
	 * Class：	FileSearchView
	 * Purpose:	File search view
	 *
	/***********************************************************************************************************************/
	FileSearchView::FileSearchView(QWidget *parent)
		: QWidget(parent) {
		SetupUi();
	}

	FileSearchView::~FileSearchView()
	{}

	void FileSearchView::SetupUi() {
		mSearchFileListView = new CommonFileListView;
		mSearchEdit = new QLineEdit;
		mSearchEdit->setFixedHeight(32);
		connect(mSearchEdit, &QLineEdit::returnPressed, this, &FileSearchView::OnSearch);

		mSearchAction = new QAction(QIcon(":/ffx/res/image/search.svg"), "", mSearchFileListView);
		connect(mSearchAction, &QAction::triggered, this, &FileSearchView::OnSearchActionTriggered);
		

		mSearchEdit->addAction(mSearchAction, QLineEdit::TrailingPosition);
		mSearchFileOnlyButton = new QToolButton;
		mSearchFileOnlyButton->setIcon(QIcon(":/ffx/res/image/search-file-only.svg"));
		mSearchFileOnlyButton->setIconSize(QSize(20,20));
		mSearchFileOnlyButton->setFixedSize(QSize(32, 32));
		mSearchFileOnlyButton->setCheckable(true);
		mSearchFileOnlyButton->setChecked(true);
		mSearchCaseButton = new QToolButton;
		mSearchCaseButton->setIcon(QIcon(":/ffx/res/image/search-case-sen.svg"));
		mSearchCaseButton->setIconSize(QSize(20, 20));
		mSearchCaseButton->setFixedSize(QSize(32, 32));
		mSearchCaseButton->setCheckable(true);
		mSearchCaseButton->setChecked(true);
		mMainLayout = new QGridLayout;
		mMainLayout->addWidget(mSearchEdit, 0, 0, 1, 1);
		mMainLayout->addWidget(mSearchFileOnlyButton, 0, 1, 1, 1);
		mMainLayout->addWidget(mSearchCaseButton, 0, 2, 1, 1);
		mMainLayout->addWidget(mSearchFileListView, 1, 0, 1, 3);
		mMainLayout->setRowStretch(0, 1);
		mMainLayout->setColumnStretch(1, 1);
		mMainLayout->setContentsMargins(0, 9, 5, 0); // Set the right margin to 5 pixels.

		setLayout(mMainLayout);
	}

	void FileSearchView::SetSearchDir(const QString& dir) {
		mSearchDir = dir;
		QFileInfo fileInfo(mSearchDir);
		QString name = fileInfo.fileName();
		if (name.isEmpty()) {
			name = fileInfo.absoluteFilePath();
		}
		mSearchEdit->setPlaceholderText(QObject::tr("Search in [%1]").arg(name));
	}

	void FileSearchView::OnSearchComplete(int taskId, bool success) {
		if (mSearchTaskId == taskId) {
			mSearchTaskId = -1;
			SetWorking(false);
		}
	}

	void FileSearchView::OnSearchFileMatched(int taskId, const QFileInfo& fileInput, const QFileInfo& fileOutput, bool success, const QString& message) {
		if (taskId != mSearchTaskId || !success)
			return;
		mSearchFileListView->AddItem(fileOutput.absoluteFilePath());

		MainWindow::Instance()->UpdateFileSearchInfo(mSearchFileListView->Count());
	}

	void FileSearchView::OnSearchActionTriggered() {
		if (mSearchTaskId > 0) {
			MainWindow::Instance()->TaskPanelPtr()->Cancel(mSearchTaskId);
			return;
		}
		OnSearch();
	}

	void FileSearchView::SetWorking(bool work) {
		mSearchEdit->setReadOnly(work);
		mSearchFileOnlyButton->setEnabled(!work);
		mSearchCaseButton->setEnabled(!work);
		mSearchAction->setIcon(work ? QIcon(":/ffx/res/image/cancel.svg") : QIcon(":/ffx/res/image/search.svg"));
	}

	void FileSearchView::OnSearch() {
		if (mSearchTaskId > 0) {
			return;
		}

		mSearchFileListView->RemoveAll();
		QString expression = mSearchEdit->text();
		if (expression.isEmpty()) {
			return;
		}
		FileFilterExpr fe(expression.toStdString(), mSearchCaseButton->isChecked());
		FileFilterPtr filter = fe.Filter();
		if (mSearchFileOnlyButton->isChecked()) {
			filter = std::make_shared<AndFileFilter>(filter, std::make_shared<OnlyFileFilter>());
		}
		SetWorking();
		mSearchTaskId = MainWindow::Instance()->TaskPanelPtr()->Submit(FileInfoList(mSearchDir), std::make_shared<FileSearchHandler>(filter));
	}

	void FileSearchView::ActivateSearch() {
		mSearchEdit->setFocus();
	}
}
