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
		virtual void OnComplete(bool success, const QString& msg = QString()) = 0;
	};
	typedef std::shared_ptr<Progress> ProgressPtr;

	class FFXUTILS_EXPORT FileHandler {
	public:
		virtual ~FileHandler() = default;

	public:
		virtual bool Handle(const FileInfo& file, FileInfo& newFile, QString& error) = 0;
		virtual FileInfo Unhandle(const FileInfo& file) = 0;
		virtual QString Name() = 0;
		virtual QString Description() { return ""; }
		virtual bool Undoable() { return false; }
		virtual QString String();

	public:
		FileList Handle(const FileList& files);
		void Unhandle();
		void Cancel();
		FileHandler& SetArg(const QString& name, QVariant value);
		QVariant Arg(const QString& name, QVariant defaultValue = QVariant());
		ArgumentMap ArgMap() const { return mArgMap; }

	protected:
		virtual void Filter(const FileList& files) {}
		virtual bool DoingBefore() { return true; };
		virtual bool DointAfter() { return true; }

	protected:
		ArgumentMap mArgMap;
		bool mCancelled = false;
		FileList mSourceFiles;
		FileList mTargetFiles;
		QList<bool> mStateList;
		QList<QString> mErrorList;
	};

	typedef std::shared_ptr<FileHandler> FileHandlerPtr;

	class FFXUTILS_EXPORT ComposeFileHandler : public FileHandler {
	public:
		ComposeFileHandler(FileHandlerPtr h1, FileHandlerPtr h2)
			: mFileHandler1(h1)
			, mFileHandler2(h2) {}
	public:
		virtual QString Name() { return QStringLiteral("组合"); };
		virtual QString String();
		virtual QString Description() { return QStringLiteral("组合文件处理器"); }

	public:
		FileHandlerPtr FirstHandler() const { return mFileHandler1; }
		FileHandlerPtr SecondHandler() const { return mFileHandler2; }

	protected:
		virtual bool Handle(const FileInfo& file, FileInfo& FileInfo, QString& error);
		virtual FileInfo Unhandle(const FileInfo& file);

	protected:
		FileHandlerPtr mFileHandler1;
		FileHandlerPtr mFileHandler2;
	};

	class FFXUTILS_EXPORT PipelineFileHandler : public ComposeFileHandler {
	public:
		PipelineFileHandler(FileHandlerPtr first, FileHandlerPtr second)
			: ComposeFileHandler(first, second) {}
	public:
		virtual QString Name() { return QStringLiteral("管道"); };
		virtual QString Description() { return QStringLiteral("管道文件处理器"); }

	protected:
		virtual bool Handle(const FileInfo& file, FileInfo& newFile, QString& error);
	};

	class FFXUTILS_EXPORT RegExpReplaceHandler : public FileHandler {
	public:
		RegExpReplaceHandler(const QString& pattern, const QString& after, QRegExp::PatternSyntax syntax = QRegExp::Wildcard, 
			bool caseSensitive = true, bool suffixInclude = false);

	public:
		virtual QString Name() { return QStringLiteral("通配符文件名替换"); }
		virtual QString Description() { return QStringLiteral("通过通配符将文件名替换为指定的文本,不写盘"); }
		virtual bool Undoable() { return false; }

	protected:
		virtual bool Handle(const FileInfo& file, FileInfo& newFile, QString& error) override;
		virtual FileInfo Unhandle(const FileInfo& file) override;

	private:
		QRegExp mRegExp;
	};

	class FFXUTILS_EXPORT DuplicateHandler : public FileHandler {
	public:
		explicit DuplicateHandler(const QString& pattern = QStringLiteral("(N)"), int filedWidth = 4, int base = 10, QChar fill = '0', bool after = true);
	public:
		virtual QString Name() { return QStringLiteral("重名处理器"); }
		virtual QString Description() { return QStringLiteral("通过判别重名，对重名文件进行重命名"); }
		virtual bool Undoable() { return false; }

	protected:
		virtual bool Handle(const FileInfo& file, FileInfo& newFile, QString& error) override;
		virtual FileInfo Unhandle(const FileInfo& file) override;
	private:
		QMap<QString, int> mDuplicateChecker;
	};
}


#endif // _FFXFILEHANDLER_H_