#pragma once
#include "FFXFileHandler.h"

#include <QObject>
#include <QRunnable>
#include <QMutex>

namespace FFX {
	class FFXCORE_EXPORT Task : public QObject, public QRunnable, public Progress
	{
		Q_OBJECT
	public:
		enum class State
		{
			Queued,		//!< Handler is queued and has not begun
			Holded,		//!< Handler is queued but on hold and will not be started
			Running,	//!< Handler is currently running
			Succeeded,	//!< Handler successfully completed
			Failed,		//!< Handler was terminated or errored
		};
	public:
		static QString StateText(State state);

	public:
		Task(long taskId, const QFileInfoList& files, FileHandlerPtr handler, QObject* parent = nullptr);
		~Task();

	public:
		virtual void run() override;
		virtual void OnProgress(double percent, const QString& msg = QString()) override;
		virtual void OnFileComplete(const QFileInfo& input, const QFileInfo& output, bool success = true, const QString& msg = QString()) override;
		virtual void OnComplete(bool success = true, const QString& msg = QString()) override;

	public:
		long Id() const { return mTaskId; }
		State Status();
		void SetStatus(State state);
		void Cancel();

	private:
		QMutex mStateMutex;
		State mState = State::Queued;
		long mTaskId = 0;
		qint64 mTimeStart = 0;
		FileHandlerPtr mHandler;
		QFileInfoList mSourceFiles;
		QFileInfoList mResultFiles;
		QFileInfoList mFailedFiles;

	Q_SIGNALS:
		void TaskStateChanged(int taskId, int oldState, int state);
		void TaskComplete(int taskId, bool success, const QString& msg, qint64 timeCost);
		void TaskProgressChanged(int taskId, const QString& message, int pos);
		void TaskFileHandled(int taskId, const QFileInfo& fileInput, const QFileInfo& fileOutput, bool success, const QString& message);
	};
	typedef std::shared_ptr<Task> TaskPtr;
}

