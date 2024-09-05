#include "FFXFileHandler.h"
#include <QDebug>
#include <QDirIterator>
#include <QThread>

namespace FFX {
	static DebugProgress dp;
	ProgressPtr G_DebugProgress = &dp;

	/************************************************************************************************************************
	* Class DebugProgress
	*
	*************************************************************************************************************************/
	void DebugProgress::OnProgress(double percent, const QString& msg) {
		qDebug() << percent << "% " << msg;
	}

	void DebugProgress::OnFileComplete(const QFileInfo& input, const QFileInfo& output, bool success, const QString& msg) {
		qDebug() << input << "->" << output << ":" << success << ", " << msg;
	}

	void DebugProgress::OnComplete(bool success, const QString& msg) {
		qDebug() << "Complete:" << success << ":" << msg;
	}

	/************************************************************************************************************************
	* Class FileHandler
	*
	*************************************************************************************************************************/
	FileHandler& FileHandler::SetArg(const QString& name, QVariant value) {
		mArgMap[name].SetValue(value);
		return *this;
	}

	QVariant FileHandler::Arg(const QString& name, QVariant defaultValue) {
		if (mArgMap.contains(name))
			return mArgMap[name].Value();
		return defaultValue.isValid() ? defaultValue : QVariant();
	}

	QString FileHandler::String() {
		ArgumentMap::iterator it = mArgMap.begin();
		QStringList arglist;
		for (; it != mArgMap.end(); it++) {
			arglist << QString("%1=%2").arg(it.key()).arg(it.value().Value().toString());
		}
		return QString("%1(%2)").arg(Name()).arg(arglist.join(","));
	}

	/************************************************************************************************************************
	* Class CombineFileHandler
	*
	*************************************************************************************************************************/
	CombineFileHandler::CombineFileHandler(const CombineFileHandler& other) {
		for (FileHandlerPtr handler : other.mHandlers) {
			mHandlers << handler->Clone();
		}
	}
	QFileInfoList CombineFileHandler::Handle(const QFileInfoList& files, ProgressPtr progress) {
		int size = mHandlers.size();
		QFileInfoList result;
		for (int i = 0; i < size; i++) {
			result = mHandlers[i]->Handle(files);
		}
		return result;
	}

	std::shared_ptr<FileHandler> CombineFileHandler::Clone() {
		return FileHandlerPtr(new CombineFileHandler(*this));
	}

	/************************************************************************************************************************
	* Class PipeFileHandler
	*
	*************************************************************************************************************************/
	PipeFileHandler::PipeFileHandler(const PipeFileHandler& other) {
		for (FileHandlerPtr handler : other.mHandlers) {
			mHandlers << handler->Clone();
		}
	}

	QFileInfoList PipeFileHandler::Handle(const QFileInfoList& files, ProgressPtr progress) {
		int size = mHandlers.size();
		QFileInfoList result = files;
		for (int i = 0; i < size; i++) {
			result = mHandlers[i]->Handle(result);
		}
		return result;
	}

	std::shared_ptr<FileHandler> PipeFileHandler::Clone() {
		return FileHandlerPtr(new PipeFileHandler(*this));
	}

	/************************************************************************************************************************
	* Class RegExpReplaceHandler
	*
	*************************************************************************************************************************/
	FileNameReplaceByExpHandler::FileNameReplaceByExpHandler(const QString& pattern, const QString& after, QRegExp::PatternSyntax syntax,
		bool caseSensitive, bool suffixInclude) {
		mArgMap["Pattern"] = Argument("Pattern", QObject::tr("Pattern"), QObject::tr("Wildcard template string, * represents multiple characters, ? Representing a character."), pattern);
		mArgMap["After"] = Argument("After", QObject::tr("String to be replaced"), QObject::tr("String to be replaced."), after);
		mArgMap["Case"] = Argument("Case", QObject::tr("Case sensitivity"), QObject::tr("Whether to distinguish case when matching, default distinction."), caseSensitive);
		mArgMap["Syntax"] = Argument("Syntax", QObject::tr("Syntax"), QObject::tr("Can be a regular expression or wildcard, default is wildcard."), syntax);
		mArgMap["SuffixInc"] = Argument("SuffixInc", QObject::tr("Suffix inclusion"), QObject::tr("Whether to include file (including directory) suffix in the handling range, default is No"), suffixInclude);
	}

	QFileInfoList FileNameReplaceByExpHandler::Handle(const QFileInfoList& files, ProgressPtr progress) {
		QFileInfoList result;
		if (files.isEmpty()) {
			progress->OnComplete(true, QObject::tr("The file list to be handled is empty"));
			return result;
		}
		QRegExp exp(mArgMap["Pattern"].Value().toString(),
			mArgMap["Case"].Value().toBool() ? Qt::CaseSensitive : Qt::CaseInsensitive,
			(QRegExp::PatternSyntax)mArgMap["Syntax"].Value().toInt());
		if (exp.isEmpty())
			return files;
		
		bool suffixInc = mArgMap["SuffixInc"].Value().toBool();
		QString after = mArgMap["After"].Value().toString();
		double step = files.size() / 100.;
		for (const QFileInfo& fi : files) {
			File file(fi);
			QString strToMatch = suffixInc ? file.FileName() : file.BaseName();

			int size = strToMatch.size();
			int pos = 0;
			QSet<QString> tokens;
			while ((pos = exp.indexIn(strToMatch, pos)) != -1 && pos < size) {
				tokens << exp.cap();
				pos += exp.matchedLength();
			}
			for (const QString& token : tokens)
				strToMatch = strToMatch.replace(token, after);
			if (!suffixInc) {
				QString suffix = file.Suffix();
				strToMatch = QString("%1%2").arg(strToMatch).arg(suffix.isEmpty() ? "" : QString(".%1").arg(suffix));
			}
			result << file.ParentDir().absoluteFilePath(strToMatch);
		}
		
		return result;
	}

	std::shared_ptr<FileHandler> FileNameReplaceByExpHandler::Clone() {
		return FileHandlerPtr(new FileNameReplaceByExpHandler(*this));
	}

	/************************************************************************************************************************
	 * Class： CaseTransformHandler
	 * 
	 * 
	/************************************************************************************************************************/
	CaseTransformHandler::CaseTransformHandler(bool toUpper, bool suffixInc) {
		mArgMap["Upper"] = Argument("Upper", QObject::tr("Upper"), QObject::tr(""), toUpper);
		mArgMap["SuffixInc"] = Argument("SuffixInc", QObject::tr("SuffixInc"), QObject::tr("Whether to include file (including directory) suffix in the handling range, default is No"), suffixInc);
	}

	QFileInfoList CaseTransformHandler::Handle(const QFileInfoList& files, ProgressPtr progress) {
		QFileInfoList result;
		bool suffixInc = mArgMap["SuffixInc"].Value().toBool();
		bool toUpper = mArgMap["Upper"].Value().toBool();
		int size = files.size();
		for (int i = 0; i < size; i++) {
			QString str = suffixInc ? files[i].fileName() : files[i].completeBaseName();
			QString suffix = files[i].suffix();
			str = toUpper ? str.toUpper() : str.toLower();
			if (!suffix.isEmpty() && !suffixInc) {
				str = QString("%1.%2").arg(str).arg(suffix);
			}
			result << files[i].absoluteDir().absoluteFilePath(str);
		}
		return result;
	}

	std::shared_ptr<FileHandler> CaseTransformHandler::Clone() {
		return FileHandlerPtr(new CaseTransformHandler(*this));
	}

	/************************************************************************************************************************
	 * Class： DuplicateHandler
	 * 
	 * 
	/************************************************************************************************************************/
	FileDuplicateHandler::FileDuplicateHandler(const QString& pattern, bool firstFileIgnored, bool after, int filedWidth, int base, QChar fill) {
		mArgMap["Pattern"] = Argument("Pattern", QObject::tr("Pattern"), QObject::tr("Used to add string templates when file duplication occurs, such as (N), N_N, N_, [N] Among them, N must exist."), pattern);
		mArgMap["Position"] = Argument("Position", QObject::tr("Position"), QObject::tr("Should it be placed before or after the file name, by default after."), after);
		mArgMap["Width"] = Argument("Width", QObject::tr("Width"), QObject::tr("Width of numbers4: 0001, 0002, ...; 2: 01, 02, 03, ..."), filedWidth);
		mArgMap["Fill"] = Argument("Fill", QObject::tr("Char filling"), QObject::tr("Used in conjunction with Width, defaults is '0'."), fill);
		mArgMap["Base"] = Argument("Base", QObject::tr("Base system"), QObject::tr("The base system for numerical output defaults to 10 (decimal)."), base);
		mArgMap["FirstFileIgnored"] = Argument("FirstFileIgnored", QObject::tr("FirstFileIgnored"), QObject::tr("The first file does not participate in numbering."), firstFileIgnored);
	}

	QFileInfoList FileDuplicateHandler::Handle(const QFileInfoList& files, ProgressPtr progress) {
		QFileInfoList result;
		bool after = mArgMap["Position"].Value().toBool();
		QChar fill = mArgMap["Fill"].Value().toChar();
		int base = mArgMap["Base"].Value().toInt();
		int width = mArgMap["Width"].Value().toInt();
		QString pattern = mArgMap["Pattern"].Value().toString();
		bool firstFileIgnored = mArgMap["FirstFileIgnored"].Value().toBool();

		if (!pattern.contains("N")) {
			progress->OnComplete(false, QObject::tr("Handle failed: %s").arg(QObject::tr("Invalid pattern.")));
			return files;
		}
		pattern.replace("N", "%1");

		QMap<QString, int> duplicateChecker;
		double step = files.size() / 100.;
		for (const QFileInfo& fileInfo : files) {
			QString newFile = fileInfo.filePath();
			//if(!firstFileIgnored) {
			//	duplicateChecker[fileInfo.filePath()]++;
			//}
			if (fileInfo.exists() && duplicateChecker[fileInfo.filePath()] == 0 && firstFileIgnored) {
				duplicateChecker[fileInfo.filePath()]++;
			} else {
				if (duplicateChecker[fileInfo.filePath()] > 0 || !firstFileIgnored) {
					while (true) {
						if (duplicateChecker[fileInfo.filePath()] == 0)
							duplicateChecker[fileInfo.filePath()]++;
						QString newFileName;
						QString suffix = fileInfo.suffix();
						if (after) {
							newFileName = QString("%1%2%3").arg(fileInfo.completeBaseName()).
								arg(pattern.arg(duplicateChecker[fileInfo.filePath()], width, base, fill)).arg(suffix.isEmpty() ? "" : QString(".%1").arg(suffix));
						}
						else {
							newFileName = QString("%1%2%3").arg(pattern.arg(duplicateChecker[fileInfo.filePath()], width, base, fill)).
								arg(fileInfo.completeBaseName()).arg(suffix.isEmpty() ? "" : QString(".%1").arg(suffix));
						}
						newFile = fileInfo.absoluteDir().absoluteFilePath(newFileName);
						if (!QFileInfo::exists(newFile)) {
							break;
						}
						duplicateChecker[fileInfo.filePath()]++;
					}
				}
				duplicateChecker[fileInfo.filePath()]++;
			}
			result << newFile;
		}
		return result;
	}

	std::shared_ptr<FileHandler> FileDuplicateHandler::Clone() {
		return FileHandlerPtr(new FileDuplicateHandler(*this));
	}

	/************************************************************************************************************************
	 * Class： FileStatHandler
	 *
	 *
	/************************************************************************************************************************/
	FileStatHandler::FileStatHandler(bool recursion) {
		mArgMap["Recursion"] = Argument("Recursion", QObject::tr("Recursion"), QObject::tr("Recursive statistics of all directories, default is true"), recursion);
	}

	QFileInfoList FileStatHandler::Handle(const QFileInfoList& files, ProgressPtr progress) {
		if (files.isEmpty())
			return QFileInfoList();
		bool r = mArgMap["Recursion"].Value().toBool();
		progress->OnProgress(-1, QObject::tr("Scanning..."));
		for (const QFileInfo& file : files) {
			if(file.isFile()) {
				AppendFile(file);
			} else if (file.isSymLink()) {
				AppendLink(file);
			} else if(file.isDir()) {
				AppendDir(file);
				QDirIterator fit(file.absoluteFilePath(), QDir::AllEntries | QDir::Hidden | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
				while (fit.hasNext() && r)
				{
					fit.next();
					QFileInfo fi = fit.fileInfo();
					if (fi.isSymLink()) {
						AppendLink(fi);
					} else if (fi.isFile()) {
						AppendFile(fi);
						continue;
					} else if (fi.isDir()) {
						AppendDir(fi);
					}
				}
			}
		}
		progress->OnComplete();
		return QFileInfoList();
	}

	std::shared_ptr<FileHandler> FileStatHandler::Clone() {
		return FileHandlerPtr(new FileStatHandler(*this));
	}

	void FileStatHandler::AppendFile(const QFileInfo& file) {
		mTotalSize += file.size();
		mFileCount++;
		if (file.isHidden())
			mHiddenFileCount++;
		QDateTime dt = file.lastModified();
		if (dt < mOldestTime)
			mOldestTime = dt;
		if (dt > mNewestTime)
			mNewestTime = dt;
	}

	void FileStatHandler::AppendLink(const QFileInfo& file) {
		mTotalSize += SymbolLinkSize(file);
		mLinkFileCount++;
		if (file.isHidden())
			mHiddenFileCount++;

		QDateTime dt = file.lastModified();
		if (dt < mOldestTime)
			mOldestTime = dt;
		if (dt > mNewestTime)
			mNewestTime = dt;
	}

	void FileStatHandler::AppendDir(const QFileInfo& file) {
		mDirCount++;
		if (file.isHidden())
			mHiddenDirCount++;

		QDateTime dt = file.lastModified();
		if (dt < mOldestTime)
			mOldestTime = dt;
		if (dt > mNewestTime)
			mNewestTime = dt;
	}

	/************************************************************************************************************************
	 * Class： FileRenameHandler
	 *
	 *
	/************************************************************************************************************************/
	FileRenameHandler::FileRenameHandler(const QString& after, bool caseSensitive, bool suffixInc) {
		mHandlers << std::make_shared<FFX::FileNameReplaceByExpHandler>("*", after, QRegExp::Wildcard, true, suffixInc);
		mHandlers << std::make_shared<FFX::FileDuplicateHandler>();
	}

	QFileInfoList FileRenameHandler::Filter(const QFileInfoList& files) {
		QFileInfoList result(files);
		SortByDepth(result, false);
		return result;
	}

	QFileInfoList FileRenameHandler::Handle(const QFileInfoList& files, ProgressPtr progress) {
		QFileInfoList allfiles = Filter(files);
		QFileInfoList tempFiles = PipeFileHandler::Handle(allfiles);
		if (allfiles.size() != tempFiles.size()) {
			progress->OnComplete(false, QObject::tr("Handled failed: %s").arg(QObject::tr("count of files not equals.")));
			return files;
		}
		int size = allfiles.size();
		double step = 100. / size;
		double pencent = 0;
		QFileInfoList result;
		for (int i = 0; i < size; i++) {
			bool flag = QFile::rename(allfiles[i].absoluteFilePath(), tempFiles[i].absoluteFilePath());
			progress->OnFileComplete(allfiles[i], tempFiles[i], flag);
			if (flag)
				result << tempFiles[i];
			progress->OnProgress(pencent += step);
		}
		progress->OnComplete(true, QObject::tr("Finish."));
		return result;
	}

	std::shared_ptr<FileHandler> FileRenameHandler::Clone() {
		return FileHandlerPtr(new FileRenameHandler(*this));
	}

	/************************************************************************************************************************
	 * Class： FileSearchHandler
	 *
	 *
	/************************************************************************************************************************/
	FileSearchHandler::FileSearchHandler(FileFilterPtr filter)
		: mFileFilter(filter) {}

	QFileInfoList FileSearchHandler::Handle(const QFileInfoList& files, ProgressPtr progress) {
		QFileInfoList result;
		for (QFileInfo file : files) {
			progress->OnProgress(-1, QObject::tr("Matching: %1").arg(file.absoluteFilePath()));
			if (mFileFilter->Accept(file)) {
				result << file;
				progress->OnFileComplete(file, file, true);
			}
			if (file.isDir()) {
				QDirIterator fit(file.absoluteFilePath(), QDir::AllEntries | QDir::Hidden | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
				while (fit.hasNext() && !mCancelled) {
					fit.next();
					QFileInfo fi = fit.fileInfo();
					progress->OnProgress(-1, QObject::tr("Matching: %1").arg(fi.absoluteFilePath()));
					if (mFileFilter->Accept(fi)) {
						result << fi;
						progress->OnFileComplete(file, fi, true);
						QThread::usleep(1);
					}
				}
			}
		}
		progress->OnComplete(true, QObject::tr("Finish, %1 files matched.").arg(result.size()));
		return result;
	}

	std::shared_ptr<FileHandler> FileSearchHandler::Clone() {
		return FileHandlerPtr(new FileSearchHandler(*this));
	}

	/************************************************************************************************************************
	 * Class： FileModifyAttributeHandler
	 *
	 *
	/************************************************************************************************************************/
	FileModifyAttributeHandler::FileModifyAttributeHandler(bool readonly, bool hidden, bool recursion) {
		mArgMap["Recursion"] = Argument("Recursion", QObject::tr("Recursion"), QObject::tr("Recursive of all directories to set the attributes, default is false"), recursion);
		mArgMap["Readonly"] = Argument("Readonly", QObject::tr("Readonly"), QObject::tr("Set the files to readonly or not."), readonly);
		mArgMap["Hidden"] = Argument("Hidden", QObject::tr("Hidden"), QObject::tr("Set the files to hidden or not."), hidden);
	}

	QFileInfoList FileModifyAttributeHandler::Handle(const QFileInfoList& files, ProgressPtr progress) {
		bool recursion = mArgMap["Recursion"].Value().toBool();
		QFileInfoList result;
		for (QFileInfo file : files) {
			progress->OnProgress(-1, QObject::tr("Handling: %1").arg(file.absoluteFilePath()));
			SetFileReadonly(file);
			if (file.isDir() && recursion) {
				QDirIterator fit(file.absoluteFilePath(), QDir::AllEntries | QDir::Hidden | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
				while (fit.hasNext() && !mCancelled) {
					fit.next();
					QFileInfo fi = fit.fileInfo();
					progress->OnProgress(-1, QObject::tr("Matching: %1").arg(fi.absoluteFilePath()));
					SetFileReadonly(fi);
				}
			}
		}
		progress->OnComplete(true, QObject::tr("Finish.").arg(result.size()));
		return result;
	}

	void FileModifyAttributeHandler::SetFileHidden(const QFileInfo& file) {
		if (!file.exists())
			return;

		bool hidden = mArgMap["Hidden"].Value().toBool();
		QFile f(file.absoluteFilePath());
		if (hidden) {
		}
	}

	void FileModifyAttributeHandler::SetFileReadonly(const QFileInfo& file) {
		if (!file.exists())
			return;

		bool readonly = mArgMap["Readonly"].Value().toBool();
		QFile theFile(file.absoluteFilePath());
		if(readonly) {
			theFile.setPermissions(QFile::ReadOther);
		} else {
			theFile.setPermissions(QFile::ReadOther | QFile::WriteOther);
		}
		
	}

	std::shared_ptr<FileHandler> FileModifyAttributeHandler::Clone() {
		return FileHandlerPtr(new FileModifyAttributeHandler(*this));
	}

	/************************************************************************************************************************
	 * Class： FileCopyHandler
	 *
	 *
	/************************************************************************************************************************/
	FileCopyHandler::FileCopyHandler(const QString& destPath, int dupMode) {
		mArgMap["DestPath"] = Argument("DestPath", QObject::tr("DestPath"), QObject::tr("Target directory for files copying."), destPath);
		mArgMap["DupMode"] = Argument("DupMode", QObject::tr("DupMode"), QObject::tr("How to handle duplicate files, 0: rename, 1: overwrite, 2: ignored."), dupMode);
	}

	QFileInfoList FileCopyHandler::Handle(const QFileInfoList& files, ProgressPtr progress) {
		FileStatHandler scaner;
		progress->OnProgress(-1, QObject::tr("Scanning..."));
		scaner.Handle(files);
		mTotalFile = scaner.FileCount();

		QFileInfoList result;
		QString targetPath = mArgMap["DestPath"].Value().toString();
		QDir targetDir(targetPath);
		for (const QFileInfo& file : files) {
			QString targetFile = targetDir.absoluteFilePath(file.fileName());
			if (file.isDir()) {
				targetDir.mkdir(file.fileName());
				CopyDir(file, targetFile, progress);
			} else {
				CopyFile(file, targetFile, progress);
			}
			result << targetFile;
		}
		progress->OnComplete(true, QObject::tr("Finish, Total %1 files copied.").arg(mTotalFile));
		return result;
	}

	std::shared_ptr<FileHandler> FileCopyHandler::Clone() {
		return FileHandlerPtr(new FileCopyHandler(*this));
	}

	void FileCopyHandler::CopyFile(const QFileInfo& file, const QString& dest, ProgressPtr progress) {
		int dupMode = mArgMap["Overwrite"].IntValue();

		QString theTargetFile(dest);
		if (QFile::exists(dest) && dupMode == 2)
			return;

		if(QFile::exists(dest) && dupMode == 1) {
			QFile::setPermissions(dest, QFileDevice::ReadOther | QFileDevice::WriteOther);
			QFile::remove(dest);
		}

		if (QFile::exists(dest) && dupMode == 0) {
			FileDuplicateHandler duph("_N", false);
			QFileInfoList r = duph.Handle(FileInfoList(dest));
			theTargetFile = r[0].absoluteFilePath();
		}
		double p = (mCopiedFile++ / (double)mTotalFile) * 100;
		progress->OnProgress(p, QObject::tr("Copying: %1").arg(file.absoluteFilePath()));
		bool flag = QFile::copy(file.absoluteFilePath(), theTargetFile);
		progress->OnFileComplete(file, theTargetFile, flag);
	}

	void FileCopyHandler::CopyDir(const QFileInfo& dir, const QString& dest, ProgressPtr progress) {
		QDirIterator fit(dir.absoluteFilePath(), QDir::Files | QDir::Dirs | QDir::System | QDir::Hidden | QDir::NoDotAndDotDot);
		while (fit.hasNext() && !mCancelled) {
			fit.next();
			QFileInfo fi = fit.fileInfo();
			QDir targetDir(dest);
			if (fi.isDir()) {
				// make dir first.
				targetDir.mkdir(fi.fileName());
				CopyDir(fi, targetDir.absoluteFilePath(fi.fileName()), progress);
				continue;
			}
			QString theFilePath = fi.absoluteFilePath();
			CopyFile(theFilePath, targetDir.absoluteFilePath(fi.fileName()), progress);
		}
	}

	/************************************************************************************************************************
	 * Class： FileMoveHandler
	 *
	 *
	/************************************************************************************************************************/
	FileMoveHandler::FileMoveHandler(const QString& destPath, bool overwrite) {
		mArgMap["DestPath"] = Argument("DestPath", QObject::tr("DestPath"), QObject::tr("Target directory for files moving."), destPath);
		mArgMap["Overwrite"] = Argument("Overwrite", QObject::tr("Overwrite"), QObject::tr("Is it overwrite the existing file, default is false."), overwrite);
	}

	QFileInfoList FileMoveHandler::Handle(const QFileInfoList& files, ProgressPtr progress) {
		FileStatHandler scaner;
		progress->OnProgress(-1, QObject::tr("Scanning..."));
		scaner.Handle(files);
		mTotalFile = scaner.FileCount();
		QFileInfoList result;
		QString targetPath = mArgMap["DestPath"].Value().toString();
		QDir targetDir(targetPath);
		for (const QFileInfo& file : files) {
			QString targetFile = targetDir.absoluteFilePath(file.fileName());
			if (file.isDir()) {
				targetDir.mkdir(file.fileName());
				MoveDir(file, targetFile, progress);
				// Remove the dir
				//QDir(file.absoluteFilePath()).removeRecursively();
			} else {
				MoveFile(file, targetFile, progress);
			}
			result << targetFile;
		}
		progress->OnComplete(true, QObject::tr("Finish, Total %1 files moved, %2 failed.").arg(mMovedOkCount).arg(mTotalFile - mMovedOkCount));
		return result;
	}

	std::shared_ptr<FileHandler> FileMoveHandler::Clone() {
		return FileHandlerPtr(new FileMoveHandler(*this));
	}

	void FileMoveHandler::MoveFile(const QFileInfo& file, const QString& dest, ProgressPtr progress) {
		double p = (mMovedFile++ / (double)mTotalFile) * 100;
		progress->OnProgress(p, QObject::tr("Moving: %1").arg(file.absoluteFilePath()));

		if (QFile::exists(dest) && !mArgMap["Overwrite"].Value().toBool()) {
			progress->OnFileComplete(file, dest, false);
			return;
		}

		if (QFile::exists(dest)) {
			QFile::setPermissions(dest, QFileDevice::ReadOther | QFileDevice::WriteOther);
			QFile::remove(dest);
		}
		
		bool flag = QFile::rename(file.absoluteFilePath(), dest);
		progress->OnFileComplete(file, dest, flag);
		if (flag) mMovedOkCount++;
	}

	void FileMoveHandler::MoveDir(const QFileInfo& dir, const QString& dest, ProgressPtr progress) {
		QDirIterator fit(dir.absoluteFilePath(), QDir::Files | QDir::Dirs | QDir::System | QDir::Hidden | QDir::NoDotAndDotDot);
		while (fit.hasNext() && !mCancelled) {
			fit.next();
			QFileInfo fi = fit.fileInfo();
			QDir targetDir(dest);
			if (fi.isDir()) {
				// make dir first.
				targetDir.mkdir(fi.fileName());
				MoveDir(fi, targetDir.absoluteFilePath(fi.fileName()), progress);
				continue;
			}
			QString theFilePath = fi.absoluteFilePath();
			MoveFile(theFilePath, targetDir.absoluteFilePath(fi.fileName()), progress);
		}
		QString dstr = dir.absoluteFilePath();
		QDir d(dstr);
		int count = d.entryList(QDir::Files | QDir::Dirs | QDir::System | QDir::Hidden | QDir::NoDotAndDotDot).size();
		if(count == 0)
			d.removeRecursively();
	}

	/************************************************************************************************************************
	 * Class： FileDeleteHandler
	 *
	 *
	/************************************************************************************************************************/
	FileDeleteHandler::FileDeleteHandler(bool forced) {
		mArgMap["Forced"] = Argument("Overwrite", QObject::tr("Overwrite"), QObject::tr("Is it overwrite the existing file, default is false."), forced);
	}

	QFileInfoList FileDeleteHandler::Handle(const QFileInfoList& files, ProgressPtr progress) {
		bool forced = mArgMap["Forced"].Value().toBool();
		if (forced) {
			FileStatHandler scaner;
			scaner.Handle(files);
			mTotalFile = scaner.FileCount();
			for (const QFileInfo& file : files) {
				if (file.isFile())
					DeleteFile(file, progress);
				if (file.isDir()) {
					QDirIterator fit(file.absoluteFilePath(), QDir::Files | QDir::System | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
					while (fit.hasNext()) {
						fit.next();
						DeleteFile(fit.fileInfo(), progress);
					}
					QDir(file.absoluteFilePath()).removeRecursively();
				}
			}
		} else {
			for (const QFileInfo& file : files) {
				bool flag = QFile::moveToTrash(file.absoluteFilePath());
				progress->OnFileComplete(file, QFileInfo(), flag);
			}
		}
		progress->OnComplete(true, QObject::tr("Finish."));
		return QFileInfoList();
	}

	std::shared_ptr<FileHandler> FileDeleteHandler::Clone() {
		return FileHandlerPtr(new FileDeleteHandler(*this));
	}

	void FileDeleteHandler::DeleteFile(const QFileInfo& file, ProgressPtr progress) {
		// Modify the file attributes to delete.
		QFile::setPermissions(file.absoluteFilePath(), QFileDevice::ReadOther | QFileDevice::WriteOther);
		double p = (mDeletedFile++ / (double)mTotalFile) * 100;
		progress->OnProgress(p, QObject::tr("Deleting: %1").arg(file.absoluteFilePath()));
		QFile f(file.absoluteFilePath());
		bool flag = f.remove();
		QString fs = file.absoluteFilePath();
		progress->OnFileComplete(file, QFileInfo(), flag, flag ? "" : f.errorString());
	}

	FileEnvelopeByDirHandler::FileEnvelopeByDirHandler() {

	}

	QFileInfoList FileEnvelopeByDirHandler::Handle(const QFileInfoList& files, ProgressPtr progress) {
		QFileInfoList result;

		int size = files.size();
		for (int i = 0; i < size; i++) {
			const QFileInfo& file = files[i];
			double p = ((i + 1) / (double)size) * 100;
			progress->OnProgress(p, QObject::tr("Enveloping: %1").arg(file.absoluteFilePath()));

			if (!file.isFile())
				continue;

			QDir root = file.absoluteDir();
			File f(file);
			QString baseName = f.BaseName();
			QString targetDirPath = root.absoluteFilePath(baseName);
			
			//! if there is a file with the same name, system can not make the dir, so omit.
			if (QFileInfo::exists(targetDirPath) && !QFileInfo(targetDirPath).isDir()) {
				continue;
			}

			root.mkdir(baseName);
			QDir targetDir(targetDirPath);
			QString targetFile = targetDir.absoluteFilePath(file.fileName());
			bool flag = QFile::rename(file.absoluteFilePath(), targetFile);
			progress->OnFileComplete(file, QFileInfo(), flag);
			if(flag) result << targetFile;
		}
		progress->OnComplete(true, QObject::tr("Finish, Total %1 files done.").arg(result.size()));
		return result;
	}

	std::shared_ptr<FileHandler> FileEnvelopeByDirHandler::Clone() {
		return FileHandlerPtr(new FileEnvelopeByDirHandler(*this));
	}

	ClearFolderHandler::ClearFolderHandler() {

	}

	QFileInfoList ClearFolderHandler::Handle(const QFileInfoList& files, ProgressPtr progress) {
		FileStatHandler scaner;
		progress->OnProgress(-1, QObject::tr("Scanning..."));
		scaner.Handle(files);
		int totalFiles = scaner.FileCount();

		int size = files.size();
		for (int i = 0; i < size; i++) {
			const QFileInfo& file = files[i];
			if (file.isDir()) {
				ClearDir(file, progress);
			} else {
				DeleteFile(file, progress);
			}
			progress->OnFileComplete(file, QFileInfo());
		}
		progress->OnComplete(true, QObject::tr("Finish."));
		return QFileInfoList();
	}

	std::shared_ptr<FileHandler> ClearFolderHandler::Clone() {
		return FileHandlerPtr(new ClearFolderHandler(*this));
	}

	void ClearFolderHandler::ClearDir(const QFileInfo& dir, ProgressPtr progress) {
		QDirIterator fit(dir.absoluteFilePath(), QDir::Files | QDir::Dirs | QDir::System | QDir::Hidden | QDir::NoDotAndDotDot);
		while (fit.hasNext() && !mCancelled) {
			fit.next();
			QFileInfo fi = fit.fileInfo();
			if (fi.isDir()) {
				ClearDir(fi, progress);
				continue;
			}
			QString theFilePath = fi.absoluteFilePath();
			DeleteFile(theFilePath, progress);
		}
	}

	HandlerFactory::HandlerFactory() {
		Append(std::make_shared<FileRenameHandler>(""));
		Append(std::make_shared<FileCopyHandler>(""));
		Append(std::make_shared<FileMoveHandler>(""));
		Append(std::make_shared<FileDeleteHandler>());
		Append(std::make_shared<FileEnvelopeByDirHandler>());
		Append(std::make_shared<ClearFolderHandler>());
	}

	void HandlerFactory::Append(FileHandlerPtr handler) {
		mFileHandlerMap[handler->Name()] = handler;
	}

	void HandlerFactory::Remove(const QString& name) {
		mFileHandlerMap.remove(name);
	}

	FileHandlerPtr HandlerFactory::Handler(const QString& name) const {
		if (mFileHandlerMap.contains(name))
			return mFileHandlerMap[name];
		return FileHandlerPtr();
	}
}
