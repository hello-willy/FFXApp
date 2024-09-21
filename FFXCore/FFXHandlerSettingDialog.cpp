#include "FFXHandlerSettingDialog.h"

#include "FFXFileHandlerSettingWidget.h"
#include "FFXMainWindow.h"
#include "FFXFileListView.h"
#include "FFXTaskPanel.h"
#include "FFXCommonFileListView.h"

#include <QToolButton>
#include <QPushButton>
#include <QGridLayout>
#include <QProgressBar>
#include <QTabWidget>
#include <QApplication>
#include <QClipboard>
#include <QMimeData>
#include <QTabBar>
#include <QMessageBox>

namespace FFX{

	HandlerSettingDialog::HandlerSettingDialog(FileHandlerPtr handler, QWidget*parent)
		: QDialog(parent)
		, mHandler(handler) {
		
		SetupUi();
		SetupSignalsConnect();
		InitFileList();
	}

	HandlerSettingDialog::~HandlerSettingDialog() {

	}

	void HandlerSettingDialog::SetupUi() {
		int fixHeight = 36;
		int miniWidth = 128;
		mTabWidget = new QTabWidget;
		mTabWidget->tabBar()->setIconSize(QSize(20, 20));
		//mTabWidget->tabBar()->setFixedHeight(45);
		mTabWidget->setTabShape(QTabWidget::Triangular);
		mLoadFileFromClipboardButton = new QToolButton;
		mLoadFileFromClipboardButton->setIcon(QIcon(":/ffx/res/image/clipboard.svg"));
		mLoadFileFromClipboardButton->setIconSize(QSize(24, 24));
		
		mTabWidget->setCornerWidget(mLoadFileFromClipboardButton);
		mFileListView = new CommonFileListView;
		mFileListView->setStyleSheet("QListView { border: transparent; }");
		mOutputFileListView = new CommonFileListView;
		mOutputFileListView->setStyleSheet("QListView { border: transparent; }");
		mFailedHandledFileListView = new CommonFileListView;
		mFailedHandledFileListView->setStyleSheet("QListView { border: transparent; }");
		mArgCollectorList = new ArgumentCollectorListWidget;
		mMainLayout = new QGridLayout;
		mRunInBackgroundButton = new QPushButton(QObject::tr(" Run in &backgroud"));
		mRunInBackgroundButton->setFixedHeight(fixHeight);
		mRunInBackgroundButton->setMinimumWidth(miniWidth);
		mRunInBackgroundButton->setEnabled(false);
		mRunButton = new QPushButton(QObject::tr(" &Run"));
		mRunButton->setFixedHeight(fixHeight);
		mRunButton->setMinimumWidth(miniWidth);
		mRunButton->setEnabled(false);
		mCloseButton = new QPushButton(QObject::tr(" &Close"));
		mCloseButton->setFixedHeight(fixHeight);
		mCloseButton->setMinimumWidth(miniWidth);
		mToolLayout = new QHBoxLayout;
		mToolLayout->addStretch(1);
		mToolLayout->addWidget(mRunInBackgroundButton);
		mToolLayout->addWidget(mRunButton);
		mToolLayout->addWidget(mCloseButton);

		mProgressLayout = new QHBoxLayout;
		mCancelButton = new QPushButton(QObject::tr(" Ca&ncel"));
		mCancelButton->setFixedHeight(fixHeight);
		mCancelButton->setMinimumWidth(miniWidth);
		mCancelButton->setEnabled(false);
		mHandleProgressBar = new QProgressBar;
		mHandleProgressBar->setFixedHeight(fixHeight);
		mHandleProgressBar->setAlignment(Qt::AlignCenter);
		mProgressLayout->addWidget(mHandleProgressBar, 1);
		mProgressLayout->addWidget(mCancelButton);

		mTabWidget->addTab(mFileListView, QIcon(":/ffx/res/image/file-txt.svg"), QObject::tr("Files(0)"));
		mTabWidget->addTab(mArgCollectorList, QIcon(":/ffx/res/image/settings.svg"), QObject::tr("Settings"));
		mTabWidget->addTab(mOutputFileListView, QIcon(":/ffx/res/image/file-txt.svg"), QObject::tr("Output Files(0)"));
		mTabWidget->addTab(mFailedHandledFileListView, QIcon(":/ffx/res/image/file-txt.svg"), QObject::tr("Unhandled Files(0)"));
		mMainLayout->setContentsMargins(6, 6, 6, 9);
		mMainLayout->addWidget(mTabWidget, 0, 0, 1, 1);
		mMainLayout->addLayout(mProgressLayout, 1, 0, 1, 1);
		mMainLayout->addLayout(mToolLayout, 2, 0, 1, 1);
		setLayout(mMainLayout);

		ArgumentMap& am = mHandler->ArgMap();
		ArgumentMap::iterator it = am.begin();
		for (; it != am.end(); it++) {
			mArgCollectorList->Add(it.value());
		}
		
		setWindowTitle(QObject::tr("Handler setting (%1)").arg(mHandler->DisplayName()));
		setWindowFlags(Qt::Dialog | Qt::WindowCloseButtonHint);
		resize(900, 1024);
	}

	void HandlerSettingDialog::InitFileList() {
		QStringList files = MainWindow::Instance()->FileMainViewPtr()->SelectedFiles();
		mFileListView->AddItems(StringList(mHandler->Filter(FileInfoList(files))));
	}

	void HandlerSettingDialog::SetupSignalsConnect() {
		connect(mLoadFileFromClipboardButton, &QToolButton::clicked, this, &HandlerSettingDialog::OnLoadFileFromClipboard);
		connect(mFileListView, &CommonFileListView::itemChanged, this, &HandlerSettingDialog::OnFileListDataChanged);
		connect(mOutputFileListView, &CommonFileListView::itemChanged, this, &HandlerSettingDialog::OnOutputFileListDataChanged);
		connect(mFailedHandledFileListView, &CommonFileListView::itemChanged, this, &HandlerSettingDialog::OnFailedFileListDataChanged);

		connect(mTabWidget->tabBar(), &QTabBar::currentChanged, this, &HandlerSettingDialog::OnTabChanged);
		connect(mCloseButton, &QPushButton::clicked, this, &HandlerSettingDialog::OnClose);
		connect(mRunButton, &QPushButton::clicked, this, &HandlerSettingDialog::OnRun);
		connect(mRunInBackgroundButton, &QPushButton::clicked, this, &HandlerSettingDialog::OnRunInBackground);
		connect(MainWindow::Instance()->TaskPanelPtr(), &TaskPanel::TaskComplete, this, &HandlerSettingDialog::OnTaskComplete);
		connect(MainWindow::Instance()->TaskPanelPtr(), &TaskPanel::TaskProgressChanged, this, &HandlerSettingDialog::OnTaskProgressChanged);
		connect(MainWindow::Instance()->TaskPanelPtr(), &TaskPanel::TaskFileHandled, this, &HandlerSettingDialog::OnTaskFileHandled);
	}

	void HandlerSettingDialog::OnFileListDataChanged() {
		bool runnable = mFileListView->model()->rowCount() > 0;
		mRunInBackgroundButton->setEnabled(runnable);
		mRunButton->setEnabled(runnable);

		mTabWidget->setTabText(0, QObject::tr("Files(%1)").arg(mFileListView->Count()));
	}

	void HandlerSettingDialog::OnFailedFileListDataChanged() {
		mTabWidget->setTabText(3, QObject::tr("Unhandled Files(%1)").arg(mFailedHandledFileListView->Count()));
	}

	void HandlerSettingDialog::OnOutputFileListDataChanged() {
		mTabWidget->setTabText(2, QObject::tr("Output Files(%1)").arg(mOutputFileListView->Count()));
	}

	void HandlerSettingDialog::OnTabChanged(int index) {
		mLoadFileFromClipboardButton->setVisible(index == 0);
	}

	void HandlerSettingDialog::OnRunInBackground() {
		if (!ValidateArgument())
			return;
		//mArgCollectorList->GetArgumentMap(mHandler->ArgMap());
		QStringList files = mFileListView->AllFiles();
		mTaskId = MainWindow::Instance()->TaskPanelPtr()->Submit(FileInfoList(files), mHandler);
		accept();
	}

	void HandlerSettingDialog::OnRun() {
		if (!ValidateArgument())
			return;

		mRunButton->setEnabled(false);
		mRunInBackgroundButton->setEnabled(false);
		mOutputFileListView->RemoveAll();
		mFailedHandledFileListView->RemoveAll();

		//mArgCollectorList->GetArgumentMap(mHandler->ArgMap());
		QStringList files = mFileListView->AllFiles();
		mTaskId = MainWindow::Instance()->TaskPanelPtr()->Submit(FileInfoList(files), mHandler, false);
		mCancelButton->setEnabled(mTaskId > 0);
	}

	void HandlerSettingDialog::OnCancel() {
		if (mTaskId < 0)
			return;
		MainWindow::Instance()->TaskPanelPtr()->Cancel(mTaskId);
	}

	void HandlerSettingDialog::OnClose() {
		OnCancel();
		reject();
	}

	void HandlerSettingDialog::OnTaskComplete(int taskId, bool success) {
		if (mTaskId != taskId)
			return;

		mTaskId = -1;
		mCancelButton->setEnabled(mTaskId > 0);
		mHandleProgressBar->setValue(100);
		if (mHandler->IsIdempotent()) {
			mRunButton->setEnabled(true);
			mRunInBackgroundButton->setEnabled(true);
		}
	}

	void HandlerSettingDialog::OnTaskProgressChanged(int taskId, const QString& message, int pos) {
		if (mTaskId != taskId)
			return;
		if (pos < 0) {
			mHandleProgressBar->setMinimum(0);
			mHandleProgressBar->setMaximum(0);
		} else {
			mHandleProgressBar->setMinimum(0);
			mHandleProgressBar->setMaximum(100);
			mHandleProgressBar->setValue(pos);
		}
	}

	void HandlerSettingDialog::OnTaskFileHandled(int taskId, const QFileInfo& fileInput, const QFileInfo& fileOutput, bool success, const QString& message) {
		if (mTaskId == taskId && success) {
			mOutputFileListView->AddItem(fileOutput.absoluteFilePath());
		}
		if(mTaskId == taskId && !success) {
			mFailedHandledFileListView->AddItem(fileInput.absoluteFilePath());
		}
	}

	bool HandlerSettingDialog::ValidateArgument() {
		mArgCollectorList->GetArgumentMap(mHandler->ArgMap());
		const ArgumentMap& argmap = mHandler->ArgMap();
		
		ArgumentMap::const_iterator it = argmap.begin();
		for (; it != argmap.end(); it++) {
			if (it.value().IsRequired() && !it.value().Value().isNull()) {
				QMessageBox::information(this, QObject::tr("Infomation"), QObject::tr("Argument (%1) can not be empty").arg(it.key()));
				mArgCollectorList->FocusAt(it.key());
				return false;
			}
		}
		return true;
	}

	void HandlerSettingDialog::OnLoadFileFromClipboard() {
		mFileListView->RemoveAll();
		QClipboard* clipboard = QApplication::clipboard();
		const QMimeData* mimeData = clipboard->mimeData();
		QList<QUrl> curUrls = mimeData->urls();

		QStringList files;
		int size = curUrls.size();
		for (int i = 0; i < size; i++) {
			QString file = curUrls[i].toLocalFile();
			files << file;
		}
		mFileListView->AddItems(files);
	}
}
