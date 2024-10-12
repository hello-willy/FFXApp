#include "FFXFilePropertyDialog.h"
#include "FFXTaskPanel.h"
#include "FFXMainWindow.h"
#include "FFXFile.h"
#include "FFXString.h"

#include <QLabel>
#include <QGridLayout>
#include <QFrame>
#include <QCheckBox>
#include <QSpacerItem>
#include <QToolButton>
#include <QTabWidget>
#include <QTableWidget>
#include <QLineEdit>

namespace FFX {
	FileBasicPropertyWidget::FileBasicPropertyWidget(QWidget* parent)
		: QWidget(parent) {
		SetupUi();
	}

	void FileBasicPropertyWidget::SetupUi() {
		QTabWidget* mTabWidget = new QTabWidget;
		mIconLabel = new QLabel;
		QIcon fileIcon(":/ffx/res/image/stat-file.svg");
		mIconLabel->setPixmap(fileIcon.pixmap(QSize(64, 64)));
		mCountInfoLabel = new QLabel("772 files 89 directories");
		mLine1 = new QFrame;
		mLine1->setFrameShape(QFrame::HLine);
		mLine1->setFrameShadow(QFrame::Sunken);
		mTotalSizeLabel = new QLabel(QObject::tr("Size: "));
		mTotalSizeInfoLabel = new QLabel(QObject::tr("239MB (12394304032 Bytes)"));
		mDateLabel = new QLabel(QObject::tr("Date: "));
		mDateInfoLabel = new QLabel;
		mLinkLabel = new QLabel(QObject::tr("Linked File Location: "));
		mLineFileEdit = new QLineEdit;
		mLineFileEdit->setReadOnly(true);

		mLine2 = new QFrame;
		mLine2->setFrameShape(QFrame::HLine);
		mLine2->setFrameShadow(QFrame::Sunken);
		mAttriLabel = new QLabel(QObject::tr("Attribute: "));
		mReadOnlyCheckBox = new QCheckBox(QObject::tr("Readonly"));
		mHiddenCheckBox = new QCheckBox(QObject::tr("Hidden"));
		

		mMainLayout = new QGridLayout;
		mMainLayout->addWidget(mIconLabel, 0, 0, 1, 1);
		mMainLayout->addWidget(mCountInfoLabel, 0, 1, 1, 3);
		mMainLayout->addWidget(mLine1, 1, 0, 1, 4);
		mMainLayout->addWidget(mTotalSizeLabel, 2, 0, 1, 1);
		mMainLayout->addWidget(mTotalSizeInfoLabel, 2, 1, 1, 3);
		mMainLayout->addWidget(mDateLabel, 3, 0, 1, 1);
		mMainLayout->addWidget(mDateInfoLabel, 3, 1, 1, 3);

		mMainLayout->addWidget(mLinkLabel, 4, 0, 1, 1);
		mMainLayout->addWidget(mLineFileEdit, 4, 1, 1, 3);

		mMainLayout->addWidget(mLine2, 5, 0, 1, 4);
		mMainLayout->addWidget(mAttriLabel, 6, 0, 1, 1);
		mMainLayout->addWidget(mReadOnlyCheckBox, 6, 1, 1, 3);
		mMainLayout->addWidget(mHiddenCheckBox, 7, 1, 1, 3);
		mMainLayout->addItem(new QSpacerItem(20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding), 8, 0);
		
		mMainLayout->setColumnStretch(1, 1);
		mMainLayout->setVerticalSpacing(24);
		setLayout(mMainLayout);
	}

	void FileBasicPropertyWidget::SetLinkInfoVisible(bool v) {
		mLinkLabel->setVisible(v);
		mLineFileEdit->setVisible(v);
	}

	FileSizeDetailPropertyWidget::FileSizeDetailPropertyWidget(QWidget* parent) 
		: QWidget(parent) {
		SetupUi();
	}

	void FileSizeDetailPropertyWidget::SetupUi() {
		mFileSizeTable = new QTableWidget;
	}

	FilePropertyDialog::FilePropertyDialog(QFileInfoList files, QWidget *parent)
		: QDialog(parent) 
		, mFiles(files) {
		SetupUi();
		if (files.size() == 1 && files[0].isSymLink()) {
			mFileBasicPropertyWidget->SetLinkInfoVisible(true);
			mFileBasicPropertyWidget->mLineFileEdit->setText(files[0].symLinkTarget());
		} else {
			mFileBasicPropertyWidget->SetLinkInfoVisible(false);
		}
		mTaskId = MainWindow::Instance()->TaskPanelPtr()->Submit(files, std::make_shared<FileSearchHandler>(FileFilterPtr(new EmptyFilter())), false);
	}

	FilePropertyDialog::~FilePropertyDialog()
	{}

	void FilePropertyDialog::reject() {
		MainWindow::Instance()->TaskPanelPtr()->Cancel(mTaskId);
		QDialog::reject();
	}

	void FilePropertyDialog::accept() {
		MainWindow::Instance()->TaskPanelPtr()->Cancel(mTaskId);

		Qt::CheckState readonly = mFileBasicPropertyWidget->mReadOnlyCheckBox->checkState();
		Qt::CheckState hidden = mFileBasicPropertyWidget->mHiddenCheckBox->checkState();
		if (readonly != Qt::PartiallyChecked || hidden != Qt::PartiallyChecked) {
			MainWindow::Instance()->TaskPanelPtr()->Submit(mFiles, std::make_shared<FileModifyAttributeHandler>(readonly == Qt::Checked, hidden == Qt::Checked));
		}

		QDialog::accept();
	}

	void FilePropertyDialog::SetupUi() {
		setWindowTitle(QObject::tr("Property"));
		resize(QSize(768, 1024));
		setWindowFlags(Qt::WindowCloseButtonHint);

		mTabWidget = new QTabWidget;
		mMainLayout = new QGridLayout;

		mOkButton = new QToolButton;
		mOkButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
		mOkButton->setText(QObject::tr("Ok"));
		mOkButton->setIcon(QIcon(":/ffx/res/image/ok.svg"));
		mCancelButton = new QToolButton;
		mCancelButton->setText(QObject::tr("Cancel"));
		mCancelButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
		mCancelButton->setIcon(QIcon(":/ffx/res/image/cancel.svg"));

		mFileBasicPropertyWidget = new FileBasicPropertyWidget;
		mTabWidget->addTab(mFileBasicPropertyWidget, QObject::tr("Basic Info"));
		mMainLayout->addWidget(mTabWidget, 0, 0, 1, 3);
		mMainLayout->addItem(new QSpacerItem(20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum), 1, 0, 1, 1);
		mMainLayout->addWidget(mOkButton, 1, 1, 1, 1);
		mMainLayout->addWidget(mCancelButton, 1, 2, 1, 1);
		mMainLayout->setRowStretch(0, 1);
		setLayout(mMainLayout);

		connect(MainWindow::Instance()->TaskPanelPtr(), &TaskPanel::TaskFileHandled, this, &FilePropertyDialog::OnFileHandled);
		connect(mCancelButton, &QToolButton::clicked, this, &FilePropertyDialog::reject);
		connect(mOkButton, &QToolButton::clicked, this, &FilePropertyDialog::accept);
	}

	void FilePropertyDialog::OnFileHandled(int taskId, const QFileInfo& fileInput, const QFileInfo& fileOutput, bool success, const QString& message) {
		if (taskId != mTaskId)
			return;

		if (fileOutput.isDir()) {
			mDirCount++;
			if (fileOutput.isHidden())
				mHiddenDirCount++;
		}
		else if (fileOutput.isFile()) {
			mFileCount++;
			mTotalSize += fileOutput.size();
			if (fileOutput.isHidden())
				mHiddenFileCount++;
		}
		else if (fileOutput.isSymLink()) {
			mFileCount++;
			mTotalSize = SymbolLinkSize(fileOutput);
		}

		//mHiddenDirCount += handler.HiddenDirCount();
		//mHiddenFileCount += handler.HiddenFileCount();
		QDateTime dt = fileOutput.lastModified();
		if (dt < mOldestTime)
			mOldestTime = dt;
		if (dt > mNewestTime)
			mNewestTime = dt;
		QString dateStr;
		if(mOldestTime == mNewestTime) {
			dateStr = QString("%1").arg(mOldestTime.toString("yyyy-MM-dd hh:mm:ss"));
		} else {
			dateStr = QString("%1 ~ %2").arg(mOldestTime.toString("yyyy-MM-dd hh:mm:ss")).arg(mNewestTime.toString("yyyy-MM-dd hh:mm:ss"));
		}
		if (fileOutput.isFile() && !(fileOutput.permissions() & QFile::WriteOther)) {
			mReadonlyFileCount++;
		}
		if(mDirCount > 0)
			mFileBasicPropertyWidget->mReadOnlyCheckBox->setCheckState(Qt::PartiallyChecked);
		else
		if (mReadonlyFileCount > 0) {
			if (mReadonlyFileCount == mFileCount) {
				mFileBasicPropertyWidget->mReadOnlyCheckBox->setCheckState(Qt::Checked);
			} else {
				mFileBasicPropertyWidget->mReadOnlyCheckBox->setCheckState(Qt::PartiallyChecked);
			}
		}

		if (mHiddenDirCount + mHiddenDirCount > 0)
			mFileBasicPropertyWidget->mHiddenCheckBox->setCheckState(Qt::PartiallyChecked);
		mFileBasicPropertyWidget->mDateInfoLabel->setText(dateStr);
		mFileBasicPropertyWidget->mCountInfoLabel->setText(QObject::tr("%1 files %2 directories").arg(mFileCount).arg(mDirCount));
		mFileBasicPropertyWidget->mTotalSizeInfoLabel->setText(QString("%1 (%2 Bytes)").arg(String::BytesHint(mTotalSize)).arg(mTotalSize));
	}
}

