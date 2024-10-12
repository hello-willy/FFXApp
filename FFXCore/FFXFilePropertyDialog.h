#pragma once

#include <QDialog>
#include <QFileInfo>
#include <QDateTime>

class QLabel;
class QFrame;
class QCheckBox;
class QGridLayout;
class QToolButton;
class QTabWidget;
class QTableWidget;
class QLineEdit;
class QHBoxLayout;

namespace FFX {
	class FileBasicPropertyWidget : public QWidget {
		Q_OBJECT

	public:
		FileBasicPropertyWidget(QWidget* parent = nullptr);
		friend class FilePropertyDialog;

	public:
		void SetLinkInfoVisible(bool v);

	private:
		void SetupUi();

	private:
		QLabel* mIconLabel;
		QLabel* mCountInfoLabel;
		QFrame* mLine1;
		QLabel* mTotalSizeLabel;
		QLabel* mTotalSizeInfoLabel;
		QLabel* mDateLabel;
		QLabel* mDateInfoLabel;
		QLabel* mLinkLabel;
		QLineEdit* mLineFileEdit;
		QFrame* mLine2;
		QLabel* mAttriLabel;
		QCheckBox* mReadOnlyCheckBox;
		QCheckBox* mHiddenCheckBox;

		QGridLayout* mMainLayout;
	};

	class FileSizeDetailPropertyWidget : public QWidget {
		Q_OBJECT
	public:
		FileSizeDetailPropertyWidget(QWidget* parent = nullptr);
		friend class FilePropertyDialog;

	private:
		void SetupUi();

	private:
		QTableWidget* mFileSizeTable;
	};

	class FilePropertyDialog : public QDialog {
		Q_OBJECT

	public:
		FilePropertyDialog(QFileInfoList files, QWidget* parent = nullptr);
		~FilePropertyDialog();

	private:
		void SetupUi();

	private slots:
		void OnFileHandled(int taskId, const QFileInfo& fileInput, const QFileInfo& fileOutput, bool success, const QString& message);
		virtual void reject();
		virtual void accept();

	private:
		QTabWidget* mTabWidget;
		FileBasicPropertyWidget* mFileBasicPropertyWidget;
		QToolButton* mOkButton;
		QToolButton* mCancelButton;
		QGridLayout* mMainLayout;
		QFileInfoList mFiles;
		int mTaskId = -1;
		// Basic Info
		int mDirCount = 0;
		int mFileCount = 0;
		int mHiddenDirCount = 0;
		int mHiddenFileCount = 0;
		int mReadonlyFileCount = 0;
		qint64 mTotalSize = 0;
		QDateTime mOldestTime = QDateTime::currentDateTime();
		QDateTime mNewestTime = QDateTime::fromTime_t(0);
	};

}
