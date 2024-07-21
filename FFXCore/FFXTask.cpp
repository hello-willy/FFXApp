#include "FFXTask.h"
#include <QDateTime>

namespace FFX {
	QString Task::StateText(State state) {
		switch (state)
		{
		case State::Queued:
			return QObject::tr("Queued");
		case State::Holded:
			return QObject::tr("Holded");
		case State::Running:
			return QObject::tr("Running");
		case State::Succeeded:
			return QObject::tr("Succeeded");
		case State::Failed:
			return QObject::tr("Failed");
		}
		return QObject::tr("Unknown");
	}

	Task::Task(long taskId, const QFileInfoList& files, FileHandlerPtr handler, QObject* parent)
		: QObject(parent)
	{}

	Task::~Task()
	{}

	Task::State Task::Status() {
		QMutexLocker lock(&mStateMutex);
		return mState;
	}

	void Task::SetStatus(State state) {
		if (state == mState)
			return;

		QMutexLocker lock(&mStateMutex);
		State oldState = mState;
		mState = state;
		emit TaskStateChanged(Id(), static_cast<int>(oldState), static_cast<int>(mState));
	}

	void Task::run() {
		SetStatus(State::Running);
		mTimeStart = QDateTime::currentMSecsSinceEpoch();
		mResultFiles = mHandler->Handle(mSourceFiles, ProgressPtr(this));
	}

	void Task::OnProgress(double percent, const QString& msg) {
		emit TaskProgressChanged(Id(), msg, (int)percent);
	}

	void Task::OnFileComplete(const QFileInfo& input, const QFileInfo& output, bool success, const QString& msg) {
		if (!success)
			mFailedFiles << input;
		emit TaskFileHandled(Id(), input, output, success, msg);
	}

	void Task::OnComplete(bool success, const QString& msg) {
		SetStatus(success ? State::Succeeded : State::Failed);
		emit TaskComplete(Id(), success, QDateTime::currentMSecsSinceEpoch() - mTimeStart);
	}
}
