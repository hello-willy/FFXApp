#ifndef _FFXFILEHANDLER_H_
#define _FFXFILEHANDLER_H_
#include "FFXFile.h"
#include "FFXFileFilter.h"

#include <QFileInfo>
#include <QDir>
#include <QSet>
#include <QVariant>
#include <QDateTime>
#include <QSize>
#include <QRect>

#include <memory> // for shared_ptr

namespace FFX {
	class Argument {
	public:
		enum Type {
			Normal,
			Options,
			Scope,
			Files,
			Dirs
		};
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
		QString Name() const { return mName; }
		QString DisplayName() const { return mDisplayName; }
		QString Description() const { return mDescription; }
		QVariant Value() const { return mValue; }
		QString StringValue() const { return mValue.toString(); }
		int IntValue() const { return mValue.toInt(); }
		double DoubleValue() const { return mValue.toDouble(); }
		bool BoolValue() const { return mValue.toBool(); }
		QSize SizeValue() const { return mValue.toSize(); }
		QRect RectValue() const { return mValue.toRect(); }
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
		void AddLimit(QVariant limit) {
			mLimit << limit;
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
		Type mType = Normal;
		/// <summary>
		/// Options: contains all options
		/// Scope: contains one or more scope, for example: 1, 5, 8, 19, means value must be in [1, 5] or [8, 9]. only for numeric value.
		/// Files: contains file filter expression, for example: *.tif|*.tiff|*.jpg|*.jpeg.
		/// </summary>
		QVariantList mLimit;
	};
	typedef QHash<QString, Argument> ArgumentMap;

	class Progress {
	public:
		virtual void OnProgress(double percent, const QString& msg = QString()) = 0;
		virtual void OnFileComplete(const QFileInfo& input, const QFileInfo& output, bool success = true, const QString& msg = QString()) = 0;
		virtual void OnComplete(bool success = true, const QString& msg = QString()) = 0;
	};
	typedef Progress* ProgressPtr;

	class FFXCORE_EXPORT DebugProgress : public Progress {
	public:
		virtual void OnProgress(double percent, const QString& msg = QString());
		virtual void OnFileComplete(const QFileInfo& input, const QFileInfo& output, bool success = true, const QString& msg = QString());
		virtual void OnComplete(bool success = true, const QString& msg = QString());
	};
	extern ProgressPtr G_DebugProgress;

	class FFXCORE_EXPORT FileHandler {
	public:
		virtual ~FileHandler() = default;

	public:
		virtual QFileInfoList Filter(const QFileInfoList& files) { return files; }
		virtual QFileInfoList Handle(const QFileInfoList& files, ProgressPtr progress = G_DebugProgress) = 0;
		virtual QString Name() = 0;
		virtual std::shared_ptr<FileHandler> Clone() = 0;
		virtual void Cancel() {}
		virtual QString DisplayName() { return Name(); }
		virtual QString Description() { return ""; }
		virtual QString String();

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
		CombineFileHandler(const CombineFileHandler& other);

	public:
		void Append(FileHandlerPtr handler) { mHandlers << handler; }
		FileHandlerPtr Handler(int i) { return mHandlers[i]; }
		int Count() const { return mHandlers.size(); }

	public:
		virtual QFileInfoList Handle(const QFileInfoList& files, ProgressPtr progress = G_DebugProgress) override;
		virtual std::shared_ptr<FileHandler> Clone() override;
		virtual QString Name() { return QStringLiteral("CombineHandler"); }
		virtual QString DisplayName() { return QObject::tr("CombineHandler"); }
		virtual QString Description() { return QObject::tr("Input files to be handled by handler1 and hander2 in order."); }
	protected:
		QList<FileHandlerPtr> mHandlers;
	};

	class FFXCORE_EXPORT PipeFileHandler : public CombineFileHandler {
	public:
		PipeFileHandler() = default;
		PipeFileHandler(FileHandlerPtr handler)
			: CombineFileHandler(handler) {}
		PipeFileHandler(const PipeFileHandler& other);

	public:
		virtual QFileInfoList Handle(const QFileInfoList& files, ProgressPtr progress = G_DebugProgress) override;
		virtual std::shared_ptr<FileHandler> Clone() override;
		virtual QString Name() { return QStringLiteral("PipeFileHandler"); }
		virtual QString DisplayName() { return QObject::tr("PipeFileHandler"); }
		virtual QString Description() { return QObject::tr("Input files to be handled by handler1, the result of handler1 will be handled by handler2."); }
	};

	class FFXCORE_EXPORT FileNameReplaceByExpHandler : public FileHandler {
	public:
		FileNameReplaceByExpHandler(const QString& pattern, const QString& after, QRegExp::PatternSyntax syntax = QRegExp::Wildcard,
			bool caseSensitive = true, bool suffixInclude = false);

	public:
		virtual QFileInfoList Handle(const QFileInfoList& files, ProgressPtr progress = G_DebugProgress) override;
		virtual std::shared_ptr<FileHandler> Clone() override;
		virtual QString Name() { return QStringLiteral("FileNameReplaceByExpHandler"); }
		virtual QString DisplayName() { return QObject::tr("FileNameReplaceByExpHandler"); }
		virtual QString Description() { return QObject::tr("Replace file name with specified text through expression matching, without writing to disk."); }
	};

	class FFXCORE_EXPORT CaseTransformHandler : public FileHandler {
	public:
		CaseTransformHandler(bool toUpper = true, bool suffixInc = true);
	public:
		virtual QFileInfoList Handle(const QFileInfoList& files, ProgressPtr progress = G_DebugProgress) override;
		virtual std::shared_ptr<FileHandler> Clone() override;
		virtual QString Name() { return QStringLiteral("CaseTransformHandler"); }
		virtual QString DisplayName() { return QObject::tr("CaseTransformHandler"); }
		virtual QString Description() { return QObject::tr("Set the file name to upper or lower case, without writing to disk."); }
	};

	class FFXCORE_EXPORT FileDuplicateHandler : public FileHandler {
	public:
		explicit FileDuplicateHandler(const QString& pattern = QStringLiteral("(N)"), bool firstFileIgnored = true, bool after = true, int filedWidth = 4, int base = 10, QChar fill = '0');
	public:
		virtual QFileInfoList Handle(const QFileInfoList& files, ProgressPtr progress = G_DebugProgress) override;
		virtual std::shared_ptr<FileHandler> Clone() override;
		virtual QString Name() { return QStringLiteral("DuplicateHandler"); }
		virtual QString DisplayName() { return QObject::tr("DuplicateHandler"); }
		virtual QString Description() { return QObject::tr("Rename duplicate files by identifying duplicates without writing them to disk."); }

	};

	class FFXCORE_EXPORT FileStatHandler : public FileHandler {
	public:
		FileStatHandler(bool recursion = true);
	public:
		virtual QFileInfoList Handle(const QFileInfoList& files, ProgressPtr progress = G_DebugProgress) override;
		virtual std::shared_ptr<FileHandler> Clone() override;
		virtual QString Name() { return QStringLiteral("FileRenameHandler"); }
		virtual QString DisplayName() { return QObject::tr("FileRenameHandler"); }
		virtual QString Description() { return QObject::tr("Replace file name with specified text through expression matching."); }

	public:
		int DirCount() { return mDirCount; }
		int FileCount() { return mFileCount + mLinkFileCount; }
		int HiddenFileCount() { return mHiddenFileCount; }
		int HiddenDirCount() { return mHiddenDirCount; }
		qint64 TotalSize() { return mTotalSize; }
		QDateTime OldestTime() const { return mOldestTime; }
		QDateTime NewestTime() const { return mNewestTime; }

	private:
		void AppendFile(const QFileInfo& file);
		void AppendLink(const QFileInfo& file);
		void AppendDir(const QFileInfo& file);

	private:
		int mDirCount = 0;
		int mFileCount = 0;
		int mLinkFileCount = 0;
		int mHiddenDirCount = 0;
		int mHiddenFileCount = 0;
		qint64 mTotalSize = 0;
		QDateTime mOldestTime = QDateTime::currentDateTime();
		QDateTime mNewestTime = QDateTime::fromTime_t(0);
	};

	class FFXCORE_EXPORT FileRenameHandler : public PipeFileHandler {
	public:
		FileRenameHandler(const QString& after, bool caseSensitive = true, bool suffixInc = false);
		FileRenameHandler(FileHandlerPtr handler)
			: PipeFileHandler(handler) {}
		
	public:
		virtual QFileInfoList Filter(const QFileInfoList& files) override;
		virtual QFileInfoList Handle(const QFileInfoList& files, ProgressPtr progress = G_DebugProgress) override;
		virtual std::shared_ptr<FileHandler> Clone() override;
		virtual QString Name() { return QStringLiteral("FileRenameHandler"); }
		virtual QString DisplayName() { return QObject::tr("FileRenameHandler"); }
		virtual QString Description() { return QObject::tr("Replace file name with specified text through expression matching."); }
	};

	class FFXCORE_EXPORT FileSearchHandler : public FileHandler {
	public:
		FileSearchHandler(FileFilterPtr filter);
	public:
		virtual QFileInfoList Handle(const QFileInfoList& files, ProgressPtr progress = G_DebugProgress) override;
		virtual std::shared_ptr<FileHandler> Clone() override;
		virtual QString Name() { return QStringLiteral("FileSearchHandler"); }
		virtual QString DisplayName() { return QObject::tr("FileSearchHandler"); }
		virtual QString Description() { return QObject::tr("Search for files that meet the criteria in the specified location."); }
		virtual void Cancel() { mCancelled = true; }
	private:
		FileFilterPtr mFileFilter;
		bool mCancelled = false;
	};

	class FFXCORE_EXPORT FileModifyAttributeHandler : public FileHandler {
	public:
		FileModifyAttributeHandler(bool readonly = true, bool hidden = false, bool recursion = false);

	public:
		virtual QFileInfoList Handle(const QFileInfoList& files, ProgressPtr progress = G_DebugProgress) override;
		virtual std::shared_ptr<FileHandler> Clone() override;
		virtual QString Name() { return QStringLiteral("FileModifyAttributeHandler"); }
		virtual QString DisplayName() { return QObject::tr("FileModifyAttributeHandler"); }
		virtual QString Description() { return QObject::tr("Set files to readonly, hidden, etc."); }
		virtual void Cancel() { mCancelled = true; }

	private:
		void SetFileReadonly(const QFileInfo& file);
		void SetFileHidden(const QFileInfo& file);
	private:
		bool mCancelled = false;
	};

	class FFXCORE_EXPORT FileCopyHandler : public FileHandler {
	public:
		FileCopyHandler(const QString& destPath, int dupMode = 0);
	public:
		virtual QFileInfoList Handle(const QFileInfoList& files, ProgressPtr progress = G_DebugProgress) override;
		virtual std::shared_ptr<FileHandler> Clone() override;
		virtual QString Name() { return QStringLiteral("FileCopyHandler"); }
		virtual QString DisplayName() { return QObject::tr("FileCopyHandler"); }
		virtual QString Description() { return QObject::tr("Copy files to the specified location."); }
		virtual void Cancel() { mCancelled = true; }

	private:
		void CopyFile(const QFileInfo& file, const QString& dest, ProgressPtr progress = G_DebugProgress);
		void CopyDir(const QFileInfo& dir, const QString& dest, ProgressPtr progress = G_DebugProgress);

	private:
		bool mCancelled = false;
		int mCopiedFile = 0;
		int mTotalFile = 0;
	};

	class FFXCORE_EXPORT FileMoveHandler : public FileHandler {
	public:
		FileMoveHandler(const QString& destPath, bool overwrite = false);
	public:
		virtual QFileInfoList Handle(const QFileInfoList& files, ProgressPtr progress = G_DebugProgress) override;
		virtual std::shared_ptr<FileHandler> Clone() override;
		virtual QString Name() { return QStringLiteral("FileMoveHandler"); }
		virtual QString DisplayName() { return QObject::tr("FileMoveHandler"); }
		virtual QString Description() { return QObject::tr("Move files to the specified location."); }
		virtual void Cancel() { mCancelled = true; }

	private:
		void MoveFile(const QFileInfo& file, const QString& dest, ProgressPtr progress = G_DebugProgress);
		void MoveDir(const QFileInfo& dir, const QString& dest, ProgressPtr progress = G_DebugProgress);

	private:
		bool mCancelled = false;
		int mMovedFile = 0;
		int mTotalFile = 0;
		int mMovedOkCount = 0;
	};

	class FFXCORE_EXPORT FileDeleteHandler : public FileHandler {
	public:
		FileDeleteHandler(bool forced = false);
	public:
		virtual QFileInfoList Handle(const QFileInfoList& files, ProgressPtr progress = G_DebugProgress) override;
		virtual std::shared_ptr<FileHandler> Clone() override;
		virtual QString Name() { return QStringLiteral("FileDeleteHandler"); }
		virtual QString DisplayName() { return QObject::tr("FileDeleteHandler"); }
		virtual QString Description() { return QObject::tr("Delete files to the specified location."); }
		virtual void Cancel() { mCancelled = true; }

	protected:
		void DeleteFile(const QFileInfo& file, ProgressPtr progress = G_DebugProgress);

	protected:
		bool mCancelled = false;
		int mDeletedFile = 0;
		int mTotalFile = 0;
	};

	class FFXCORE_EXPORT FileEnvelopeByDirHandler : public FileHandler {
	public:
		FileEnvelopeByDirHandler();
	public:
		virtual QFileInfoList Handle(const QFileInfoList& files, ProgressPtr progress = G_DebugProgress) override;
		virtual std::shared_ptr<FileHandler> Clone() override;
		virtual QString Name() { return QStringLiteral("FileEnvelopeByDirHandler"); }
		virtual QString DisplayName() { return QObject::tr("FileEnvelopeByDirHandler"); }
		virtual QString Description() { return QObject::tr("Move the file to a directory with the same name as it."); }

	};

	class FFXCORE_EXPORT ClearFolderHandler : public FileDeleteHandler {
	public:
		ClearFolderHandler();
	public:
		virtual QFileInfoList Handle(const QFileInfoList& files, ProgressPtr progress = G_DebugProgress) override;
		virtual std::shared_ptr<FileHandler> Clone() override;
		virtual QString Name() { return QStringLiteral("ClearFolderHandler"); }
		virtual QString DisplayName() { return QObject::tr("ClearFolderHandler"); }
		virtual QString Description() { return QObject::tr("Clear all files in the directory, but preserve the directory structure."); }

	private:
		void ClearDir(const QFileInfo& dir, ProgressPtr progress);
	};

	class FFXCORE_EXPORT HandlerFactory {
	public:
		HandlerFactory();

	public:
		void Append(FileHandlerPtr handler);
		void Remove(const QString& name);
		FileHandlerPtr Handler(const QString& name) const;

	private:
		QMap<QString, FileHandlerPtr> mFileHandlerMap;
	};
}


#endif // _FFXFILEHANDLER_H_