#ifndef _FFXFILEHANDLER_H_
#define _FFXFILEHANDLER_H_
#include "FFXFile.h"


#include <QFileInfo>
#include <QDir>
#include <QSet>
#include <QVariant>
#include <memory> // for shared_ptr

namespace FFX {
	class Argument {
	public:
		Argument() = default;
		Argument(const QString& name)
			: mName(name)
			, mDisplayName(name)
			, mDescription(name) {}
		Argument(const QString& name, QVariant value)
			: mName(name)
			, mDisplayName(name)
			, mDescription(name)
			, mValue(value) {}
		Argument(const QString& name, const QString& displayName, const QString& description, QVariant value)
			: mName(name)
			, mDisplayName(displayName)
			, mDescription(description)
			, mValue(value) {}

	public:
		QString Name() { return mName; }
		QString DisplayName() { return mDisplayName; }
		QString Description() { return mDescription; }
		QVariant Value() { return mValue; }
		Argument& SetName(const QString& name) {
			mName = name;
			return *this;
		}
		Argument& SetDisplayName(const QString& name) {
			mDisplayName = name;
			return *this;
		}
		Argument& SetDescription(const QString& description) {
			mDescription = description;
			return *this;
		}
		Argument& SetValue(QVariant value) {
			mValue = value;
			return *this;
		}
	public:
		bool operator == (const Argument& other) {
			return mName == other.mName;
		}
	private:
		QString mName;
		QString mDisplayName;
		QString mDescription;
		QVariant mValue;
	};
	typedef QHash<QString, Argument> ArgumentMap;

	class Progress {
	public:
		virtual void OnProgress(double percent, const QString& msg = QString()) = 0;
		virtual void OnFileComplete(const QFileInfo& input, const QFileInfo& output, bool success = true, const QString& msg = QString()) = 0;
		virtual void OnComplete(bool success = true, const QString& msg = QString()) = 0;
	};
	typedef std::shared_ptr<Progress> ProgressPtr;

	class FFXCORE_EXPORT DebugProgress : public Progress {
	public:
		virtual void OnProgress(double percent, const QString& msg = QString());
		virtual void OnFileComplete(const QFileInfo& input, const QFileInfo& output, bool success = true, const QString& msg = QString());
		virtual void OnComplete(bool success = true, const QString& msg = QString());
	};

	class FFXCORE_EXPORT FileHandler {
	public:
		virtual ~FileHandler() = default;

	public:
		virtual QFileInfoList Handle(const QFileInfoList& files, ProgressPtr progress = std::make_shared<DebugProgress>()) = 0;
		virtual void Unhandle() = 0;
		virtual QString Name() = 0;
		virtual QString DisplayName() { return Name(); }
		virtual QString Description() { return ""; }
		virtual bool Undoable() { return false; }
		virtual QString String();
		virtual bool Batchable() { return false; }
		virtual bool Cancellable() { return false; }
		virtual void Cancel() {}

	public:
		FileHandler& SetArg(const QString& name, QVariant value);
		QVariant Arg(const QString& name, QVariant defaultValue = QVariant());
		ArgumentMap ArgMap() const { return mArgMap; }

	protected:
		ArgumentMap mArgMap;
	};

	typedef std::shared_ptr<FileHandler> FileHandlerPtr;

	class FFXCORE_EXPORT CombineFileHandler : public FileHandler {
	public:
		CombineFileHandler() = default;
		CombineFileHandler(FileHandlerPtr handler){
			mHandlers << handler;
		}

	public:
		void Append(FileHandlerPtr handler) { mHandlers << handler; }
		FileHandlerPtr Handler(int i) { return mHandlers[i]; }
		int Count() const { return mHandlers.size(); }

	public:
		virtual QFileInfoList Handle(const QFileInfoList& files, ProgressPtr progress = std::make_shared<DebugProgress>()) override;
		virtual void Unhandle() override;
		virtual QString Name() { return QStringLiteral("CombineHandler"); }
		virtual QString DisplayName() { return QObject::tr("CombineHandler"); }
		virtual QString Description() { return QObject::tr("Input files to be handled by handler1 and hander2 in order."); }
		virtual bool Undoable() { return false; }
	protected:
		QList<FileHandlerPtr> mHandlers;
	};

	class FFXCORE_EXPORT PipeFileHandler : public CombineFileHandler {
	public:
		PipeFileHandler() = default;
		PipeFileHandler(FileHandlerPtr handler)
			: CombineFileHandler(handler) {}

	public:
		virtual QFileInfoList Handle(const QFileInfoList& files, ProgressPtr progress = std::make_shared<DebugProgress>()) override;
		virtual void Unhandle() override;
		virtual QString Name() { return QStringLiteral("PipeFileHandler"); }
		virtual QString DisplayName() { return QObject::tr("PipeFileHandler"); }
		virtual QString Description() { return QObject::tr("Input files to be handled by handler1, the result of handler1 will be handled by handler2."); }
	};

	class FFXCORE_EXPORT FileNameReplaceByExpHandler : public FileHandler {
	public:
		FileNameReplaceByExpHandler(const QString& pattern, const QString& after, QRegExp::PatternSyntax syntax = QRegExp::Wildcard,
			bool caseSensitive = true, bool suffixInclude = false);

	public:
		virtual QFileInfoList Handle(const QFileInfoList& files, ProgressPtr progress = std::make_shared<DebugProgress>()) override;
		virtual void Unhandle() override;
		virtual QString Name() { return QStringLiteral("FileNameReplaceByExpHandler"); }
		virtual QString DisplayName() { return QObject::tr("FileNameReplaceByExpHandler"); }
		virtual QString Description() { return QObject::tr("Replace file name with specified text through expression matching, without writing to disk."); }
		virtual bool Undoable() { return false; }
	};

	class FFXCORE_EXPORT CaseTransformHandler : public FileHandler {
	public:
		CaseTransformHandler(bool toUpper = true, bool suffixInc = true);
	public:
		virtual QFileInfoList Handle(const QFileInfoList& files, ProgressPtr progress = std::make_shared<DebugProgress>()) override;
		virtual void Unhandle() override;
		virtual QString Name() { return QStringLiteral("CaseTransformHandler"); }
		virtual QString DisplayName() { return QObject::tr("CaseTransformHandler"); }
		virtual QString Description() { return QObject::tr("Set the file name to upper or lower case, without writing to disk."); }
		virtual bool Undoable() { return false; }
	};

	class FFXCORE_EXPORT FileDuplicateHandler : public FileHandler {
	public:
		explicit FileDuplicateHandler(const QString& pattern = QStringLiteral("(N)"), int filedWidth = 4, int base = 10, QChar fill = '0', bool after = true);
	public:
		virtual QFileInfoList Handle(const QFileInfoList& files, ProgressPtr progress = std::make_shared<DebugProgress>()) override;
		virtual void Unhandle() override;
		virtual QString Name() { return QStringLiteral("DuplicateHandler"); }
		virtual QString DisplayName() { return QObject::tr("DuplicateHandler"); }
		virtual QString Description() { return QObject::tr("Rename duplicate files by identifying duplicates without writing them to disk."); }
		virtual bool Undoable() { return false; }

	};

	class FFXCORE_EXPORT FileRenameHandler : public PipeFileHandler {
	public:
		FileRenameHandler() = default;
		FileRenameHandler(FileHandlerPtr handler)
			: PipeFileHandler(handler) {}
		
	public:
		virtual QFileInfoList Handle(const QFileInfoList& files, ProgressPtr progress = std::make_shared<DebugProgress>()) override;
		virtual void Unhandle() override;
		virtual QString Name() { return QStringLiteral("FileRenameByExp"); }
		virtual QString DisplayName() { return QObject::tr("FileRenameByExp"); }
		virtual QString Description() { return QObject::tr("Replace file name with specified text through expression matching."); }
		virtual bool Undoable() { return false; }
	};
}


#endif // _FFXFILEHANDLER_H_