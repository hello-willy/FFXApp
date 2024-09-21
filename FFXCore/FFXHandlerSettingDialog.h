#pragma once
#include "FFXFileHandler.h"

#include <QDialog>

class QGridLayout;
class QHBoxLayout;
class QPushButton;
class QProgressBar;
class QTabWidget;
class QToolButton;

namespace FFX {
	
	class ArgumentCollectorListWidget;
	class CommonFileListView;

	class FFXCORE_EXPORT HandlerSettingDialog : public QDialog	{
		Q_OBJECT
	public:
		HandlerSettingDialog(FileHandlerPtr handler, QWidget* parent = nullptr);
		~HandlerSettingDialog();

	private:
		void SetupUi();
		void InitFileList();
		void SetupSignalsConnect();
		bool ValidateArgument();

	private slots:
		void OnRunInBackground();
		void OnRun();
		void OnCancel();
		void OnClose();
		void OnTaskComplete(int taskId, bool success);
		void OnTaskProgressChanged(int taskId, const QString& message, int pos);
		void OnTaskFileHandled(int taskId, const QFileInfo& fileInput, const QFileInfo& fileOutput, bool success, const QString& message);
		void OnLoadFileFromClipboard();
		void OnFileListDataChanged();
		void OnFailedFileListDataChanged();
		void OnOutputFileListDataChanged();
		void OnTabChanged(int index);

	private:
		QTabWidget* mTabWidget;
		QToolButton* mLoadFileFromClipboardButton;
		CommonFileListView* mFileListView;
		CommonFileListView* mOutputFileListView;
		CommonFileListView* mFailedHandledFileListView;
		ArgumentCollectorListWidget* mArgCollectorList;
		FileHandlerPtr mHandler;
		QGridLayout* mMainLayout;
		QHBoxLayout* mToolLayout;
		QPushButton* mRunInBackgroundButton;
		QPushButton* mRunButton;
		QPushButton* mCloseButton;
		QHBoxLayout* mProgressLayout;
		QPushButton* mCancelButton;
		QProgressBar* mHandleProgressBar;
		int mTaskId = -1;
	};
}

