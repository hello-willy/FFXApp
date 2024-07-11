#include "FFXFileHandler.h"

namespace FFX {

	/************************************************************************************************************************
	* Class FileHandler
	*
	*************************************************************************************************************************/
	FileList FileHandler::Handle(const FileList& files) {
		Filter(files);
		if (!DoingBefore())
			return mTargetFiles;
		for (const FileInfo& file : mSourceFiles) {
			if (mCancelled)
				break;
			FileInfo target;
			QString error("OK");
			bool flag = Handle(file, target, error);
			mStateList << flag;
			mErrorList << error;
			mTargetFiles << target;
		}
		DointAfter();
		return mTargetFiles;
	}

	void FileHandler::Unhandle() {
		int size = mTargetFiles.size();
		for (int i = 0; i < size; i++) {
			FileInfo file = mTargetFiles[i];
			if (file.IsValid() || mStateList[i])
				Unhandle(file);
		}
		mTargetFiles.clear();
		mStateList.clear();
		mErrorList.clear();
	}

	void FileHandler::Cancel() {
		mCancelled = true;
	}

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
	* Class ComposeFileHandler
	*
	*************************************************************************************************************************/
	bool ComposeFileHandler::Handle(const FileInfo& file, FileInfo& newFile, QString& error) {
		if (mFileHandler1->Handle(file, newFile, error))
			return mFileHandler2->Handle(file, newFile, error);
		return false;
	}

	FileInfo ComposeFileHandler::Unhandle(const FileInfo& file) {
		if (Undoable()) {
			FileInfo tmpFile = mFileHandler2->Unhandle(file);
			return mFileHandler1->Unhandle(tmpFile);
		}
		return file;
	}

	QString ComposeFileHandler::String() {
		return QString("%1(%2, %3)").arg(Name(), mFileHandler1->String()).arg(mFileHandler1->String());
	}

	/************************************************************************************************************************
	* Class PipelineFileHandler
	*
	*************************************************************************************************************************/
	bool PipelineFileHandler::Handle(const FileInfo& file, FileInfo& newFile, QString& error) {
		FileInfo tmpFile;
		if (mFileHandler1->Handle(file, tmpFile, error))
			return mFileHandler2->Handle(tmpFile, newFile, error);
		return false;
	}

	/************************************************************************************************************************
	* Class WildcardReplaceHandler
	*
	*************************************************************************************************************************/
	RegExpReplaceHandler::RegExpReplaceHandler(const QString& pattern, const QString& after, QRegExp::PatternSyntax syntax, 
		bool caseSensitive, bool suffixInclude) {
		mArgMap["Pattern"] = Argument("Pattern", QStringLiteral("模板"), QStringLiteral("通配符模板字符串，*代表多个字符，?代表一个字符"), pattern);
		mArgMap["After"] = Argument("After", QStringLiteral("替换字符串"), QStringLiteral("替换字符串"), after);
		mArgMap["Case"] = Argument("Pattern", QStringLiteral("大小写敏感"), QStringLiteral("匹配时是否区分大小写，默认区分"), caseSensitive);
		mArgMap["Syntax"] = Argument("Syntax", QStringLiteral("匹配语法"), QStringLiteral("可以是正则表达式或通配符，默认为通配符（1）"), syntax);
		mArgMap["SuffixInc"] = Argument("SuffixInc", QStringLiteral("后缀纳入匹配"), QStringLiteral("是否将文件(包括目录)后缀纳入匹配范围，默认为否"), suffixInclude);
		mRegExp = QRegExp(pattern, caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive, syntax);
	}

	bool RegExpReplaceHandler::Handle(const FileInfo& file, FileInfo& newFile, QString& error) {
		bool suffixInc = mArgMap["SuffixInc"].Value().toBool();
		QString after = mArgMap["After"].Value().toString();

		QString strToMatch = suffixInc ? file.FileName() : file.BaseName();
		
		int size = strToMatch.size();
		int pos = 0;
		QSet<QString> tokens;
		while ((pos = mRegExp.indexIn(strToMatch, pos)) != -1 && pos < size) {
			tokens << mRegExp.cap();
			pos += mRegExp.matchedLength();
		}
		for (const QString& token : tokens)
			strToMatch = strToMatch.replace(token, after);
		if (!suffixInc) {
			QString suffix = file.Suffix();
			strToMatch = QString("%1%2").arg(strToMatch).arg(suffix.isEmpty() ? "" : QString(".%1").arg(suffix));
		}
		newFile = file.ParentDir().absoluteFilePath(strToMatch);
		return true;
	}

	FileInfo RegExpReplaceHandler::Unhandle(const FileInfo& file) {
		return FileInfo();
	}

	/************************************************************************************************************************
	 * Class： DuplicateHandler
	 * 
	 * 
	/************************************************************************************************************************/
	DuplicateHandler::DuplicateHandler(const QString& pattern, int filedWidth, int base, QChar fill, bool after) {
		mArgMap["Pattern"] = Argument("Pattern", QStringLiteral("模板"), QStringLiteral("用于出现文件重复时添加字符串模板，如(N), _N, N_, [N]...其中N必须存在"), pattern);
		mArgMap["After"] = Argument("After", QStringLiteral("位置"), QStringLiteral("放置于文件名的前面还是后面，默认后面"), after);
		mArgMap["Width"] = Argument("Pattern", QStringLiteral("宽度"), QStringLiteral("数字的宽度，4: 0001, 0002, ...; 2: 01, 02, 03, ..."), filedWidth);
		mArgMap["Fill"] = Argument("Syntax", QStringLiteral("填充字符"), QStringLiteral("与Width配合使用，默认为0"), fill);
		mArgMap["Base"] = Argument("Syntax", QStringLiteral("进制"), QStringLiteral("数字输出的进制，默认10(十进制)"), base);
	}

	bool DuplicateHandler::Handle(const FileInfo& file, FileInfo& newFile, QString& error) {
		QFileInfo fileInfo(file.Path());
		newFile = file;
		bool after = mArgMap["After"].Value().toBool();
		QChar fill = mArgMap["Fill"].Value().toChar();
		int base = mArgMap["Base"].Value().toInt();
		int width = mArgMap["Width"].Value().toInt();
		QString pattern = mArgMap["Pattern"].Value().toString();
		if (!pattern.contains("N"))
			return false;
		pattern.replace("N", "%1");

		if (fileInfo.exists() && mDuplicateChecker[file.Path()] == 0) {
			mDuplicateChecker[file.Path()]++;
		}
		if (mDuplicateChecker[file.Path()] > 0) {
			while (true) {
				QString newFileName;
				QString suffix = fileInfo.suffix();
				if (after) {
					newFileName = QString("%1%2%3").arg(fileInfo.completeBaseName()).
						arg(pattern.arg(mDuplicateChecker[file.Path()], width, base, fill)).arg(suffix.isEmpty() ? "" : QString(".%1").arg(suffix));
				} else {
					newFileName = QString("%1%2.%3").arg(pattern.arg(mDuplicateChecker[file.Path()], width, base, fill)).
						arg(fileInfo.completeBaseName()).arg(suffix.isEmpty() ? "" : QString(".%1").arg(suffix));
				}
				newFile = fileInfo.absoluteDir().absoluteFilePath(newFileName);
				if (!QFileInfo::exists(newFile.Path())) {
					break;
				}
				mDuplicateChecker[file.Path()]++;
			}
		}
		
		mDuplicateChecker[file.Path()]++;
		return true;
	}

	FileInfo DuplicateHandler::Unhandle(const FileInfo& file) {
		return FileInfo();
	}
}
