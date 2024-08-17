#pragma once
#include "FFXFileHandler.h"
#include <QWidget>
#include <QThreadPool>
#include <QMutex>

class QGridLayout;
class QToolButton;
class QLabel;
class QComboBox;
class QLineEdit;
class QTableWidget;
class QFrame;
class QProgressBar;
class QVBoxLayout;

namespace FFX {
	class Task;
	class TaskIdGenerator {
	public:
		virtual int Id();
	private:
		int mAutoincreamentId = 1;
	};

	class TaskProgressBar : public QWidget {
		Q_OBJECT
	public:
		TaskProgressBar(int height = 20, QWidget* parent = nullptr);

	public:
		void setMinimum(int minimum);
		void setMaximum(int maximum);
		void setValue(int value);

	private:
		QVBoxLayout* mLayout;
		QProgressBar* mProgressBar;
	};

	class FFXCORE_EXPORT TaskPanel : public QWidget {
		Q_OBJECT
	public:
		TaskPanel(QWidget* parent = nullptr);
		~TaskPanel();

	public:
		int Submit(const QFileInfoList& files, FileHandlerPtr handler);
		void Cancel(int taskId);

	Q_SIGNALS:
		void TaskComplete(int taskId, bool success);
		void TaskFileHandled(int taskId, const QFileInfo& fileInput, const QFileInfo& fileOutput, bool success, const QString& message);

	private:
		void SetupUi();
		int RowOf(int taskId);
		void UpdateTaskTable();

	private slots:
		void OnTaskTableItemSelectionChanged();
		void OnTaskTableItemChanged();
		void OnCancelTaskButtonClicked();
		void OnRemoveTaskButtonClicked();

		void OnTaskStateChanged(int taskId, int oldState, int state);
		void OnTaskComplete(int taskId, bool success, const QString& msg, qint64 timeCost);
		void OnTaskProgressChanged(int taskId, const QString& message, int pos);
		void OnTaskFileHandled(int taskId, const QFileInfo& fileInput, const QFileInfo& fileOutput, bool success, const QString& message);

	private:
		QThreadPool* mWorkerGroup = QThreadPool::globalInstance();
		TaskIdGenerator mTaskIdGenerator;
		QMap<int, Task*> mTaskMap;
		QMutex mTaskMapLocker;

		QGridLayout* mMainGridLayout;
		QToolButton* mRemoveTaskButton;
		QLabel* mFilterLabel;
		QComboBox* mStateFilterCombo;
		QLineEdit* mTaskSearchEdit;
		QToolButton* mNewTaskButton;
		QToolButton* mCancelTaskButton;
		QTableWidget* mTaskTable;
		QFrame* mSeperator;
	};

}
