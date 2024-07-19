#include "FFXFileHandler.h"
#include <QDebug>
#include <QDirIterator>

namespace FFX {
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
			(QRegExp::PatternSyntax)mArgMap["Pattern"].Value().toInt());

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
	FileDuplicateHandler::FileDuplicateHandler(const QString& pattern, int filedWidth, int base, QChar fill, bool after) {
		mArgMap["Pattern"]	= Argument("Pattern", QObject::tr("Pattern"), QObject::tr("Used to add string templates when file duplication occurs, such as (N), N_N, N_, [N] Among them, N must exist."), pattern);
		mArgMap["Position"] = Argument("Position", QObject::tr("Position"), QObject::tr("Should it be placed before or after the file name, by default after."), after);
		mArgMap["Width"]	= Argument("Width", QObject::tr("Width"), QObject::tr("Width of numbers4: 0001, 0002, ...; 2: 01, 02, 03, ..."), filedWidth);
		mArgMap["Fill"]		= Argument("Fill", QObject::tr("Char filling"), QObject::tr("Used in conjunction with Width, defaults is '0'."), fill);
		mArgMap["Base"]		= Argument("Base", QObject::tr("Base system"), QObject::tr("The base system for numerical output defaults to 10 (decimal)."), base);
	}

	QFileInfoList FileDuplicateHandler::Handle(const QFileInfoList& files, ProgressPtr progress) {
		QFileInfoList result;
		bool after		= mArgMap["Position"].Value().toBool();
		QChar fill		= mArgMap["Fill"].Value().toChar();
		int base		= mArgMap["Base"].Value().toInt();
		int width		= mArgMap["Width"].Value().toInt();
		QString pattern = mArgMap["Pattern"].Value().toString();

		if (!pattern.contains("N")) {
			progress->OnComplete(false, QObject::tr("Handle failed: %s").arg(QObject::tr("Invalid pattern.")));
			return result;
		}
		pattern.replace("N", "%1");

		QMap<QString, int> duplicateChecker;
		double step = files.size() / 100.;
		for (const QFileInfo& fileInfo : files) {
			QString newFile = fileInfo.filePath();
			if (fileInfo.exists() && duplicateChecker[fileInfo.filePath()] == 0) {
				duplicateChecker[fileInfo.filePath()]++;
			} else {
				if (duplicateChecker[fileInfo.filePath()] > 0) {
					while (true) {
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
	QFileInfoList FileStatHandler::Handle(const QFileInfoList& files, ProgressPtr progress) {
		if (files.isEmpty())
			return QFileInfoList();
		progress->OnProgress(-1, QObject::tr("Scanning..."));
		for (const QFileInfo& file : files) {
			if(file.isFile()) {
				AppendFile(file);
			} else if (file.isSymLink()) {
				AppendLink(file);
			} else if(file.isDir()) {
				mDirCount++;
				QDirIterator fit(file.absoluteFilePath(), QDir::Files | QDir::Dirs | QDir::System | QDir::Hidden | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
				while (fit.hasNext())
				{
					fit.next();
					QFileInfo fi = fit.fileInfo();
					if (fi.isSymLink()) {
						AppendLink(fi);
					} else if (fi.isFile()) {
						AppendFile(fi);
						continue;
					} else if (fi.isDir()) {
						mDirCount++;
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
	}

	void FileStatHandler::AppendLink(const QFileInfo& file) {
		QFile link(file.absoluteFilePath());
		link.open(QIODevice::ReadOnly);
		qint64 size = link.size();
		link.close();
		mTotalSize += size;
		mLinkFileCount++;
		if (file.isHidden())
			mHiddenFileCount++;
	}

	/************************************************************************************************************************
	 * Class： FileRenameByExp
	 *
	 *
	/************************************************************************************************************************/
	QFileInfoList FileRenameHandler::Handle(const QFileInfoList& files, ProgressPtr progress) {
		QFileInfoList tempFiles = PipeFileHandler::Handle(files);
		//QFileInfoList tempFiles = files;
		//for (FileHandlerPtr handler : mFileNameRegExpHandlerList) {
		//	tempFiles = handler->Handle(tempFiles);
		//}
		//tempFiles = mFileDuplicateHandler->Handle(tempFiles);
		if (files.size() != tempFiles.size()) {
			progress->OnComplete(false, QObject::tr("Handled failed: %s").arg(QObject::tr("count of files not equals.")));
			return files;
		}
		int size = files.size();
		double step = 100. / size;
		double pencent = 0;
		QFileInfoList result;
		for (int i = 0; i < size; i++) {
			bool flag = QFile::rename(files[i].absoluteFilePath(), tempFiles[i].absoluteFilePath());
			progress->OnFileComplete(files[i], tempFiles[i], flag);
			if (flag)
				result << tempFiles[i];
			progress->OnProgress(pencent += step);
		}
		progress->OnComplete();
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
			if (file.isFile()) {
				if (mFileFilter->Accept(file))
					result << file;
				continue;
			}
			if (file.isDir()) {
				QDirIterator fit(file.absoluteFilePath(), QDir::Files | QDir::Dirs | QDir::NoSymLinks | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
				while (fit.hasNext() && !mCancelled) {
					fit.next();
					QFileInfo fi = fit.fileInfo();
					if (mFileFilter->Accept(fi)) {
						result << fi;
						progress->OnFileComplete(file, fi, true);
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

	FileCopyHandler::FileCopyHandler(const QString& destPath, bool overwrite) {
		mArgMap["DestPath"] = Argument("DestPath", QObject::tr("DestPath"), QObject::tr("Target directory for files copying."), destPath);
		mArgMap["Overwrite"] = Argument("Overwrite", QObject::tr("Overwrite"), QObject::tr("Is it overwrite the existing file, default is false."), overwrite);
	}

	QFileInfoList FileCopyHandler::Handle(const QFileInfoList& files, ProgressPtr progress) {
		FileStatHandler scaner;
		scaner.Handle(files, progress);
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
		progress->OnComplete();
		return result;
	}

	std::shared_ptr<FileHandler> FileCopyHandler::Clone() {
		return FileHandlerPtr(new FileCopyHandler(*this));
	}

	void FileCopyHandler::CopyFile(const QFileInfo& file, const QString& dest, ProgressPtr progress) {
		if (QFile::exists(dest) && !mArgMap["Overwrite"].Value().toBool())
			return;

		if(QFile::exists(dest)) {
			QFile::setPermissions(dest, QFileDevice::ReadOther | QFileDevice::WriteOther);
			QFile::remove(dest);
		}
		double p = (mCopiedFile++ / (double)mTotalFile) * 100;
		progress->OnProgress(p, QObject::tr("Copying: %1").arg(file.absoluteFilePath()));
		bool flag = QFile::copy(file.absoluteFilePath(), dest);
		progress->OnFileComplete(file, dest, flag);
	}

	void FileCopyHandler::CopyDir(const QFileInfo& dir, const QString& dest, ProgressPtr progress) {
		QDirIterator fit(dir.absoluteFilePath(), QDir::Files | QDir::Dirs | QDir::System | QDir::Hidden | QDir::NoDotAndDotDot);
		while (fit.hasNext()) {
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
}
