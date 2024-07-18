#include "FFXFileHandler.h"

namespace FFX {
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
	* Class RegExpReplaceHandler
	*
	*************************************************************************************************************************/
	FileNameRegExpReplace::FileNameRegExpReplace(const QString& pattern, const QString& after, QRegExp::PatternSyntax syntax,
		bool caseSensitive, bool suffixInclude) {
		mArgMap["Pattern"] = Argument("Pattern", QObject::tr("Pattern"), QObject::tr("Wildcard template string, * represents multiple characters, ? Representing a character."), pattern);
		mArgMap["After"] = Argument("After", QObject::tr("String to be replaced"), QObject::tr("String to be replaced."), after);
		mArgMap["Case"] = Argument("Case", QObject::tr("Case sensitivity"), QObject::tr("Whether to distinguish case when matching, default distinction."), caseSensitive);
		mArgMap["Syntax"] = Argument("Syntax", QObject::tr("Syntax"), QObject::tr("Can be a regular expression or wildcard, default is wildcard."), syntax);
		mArgMap["SuffixInc"] = Argument("SuffixInc", QObject::tr("Suffix inclusion"), QObject::tr("Whether to include file (including directory) suffix in the matching range, default is No"), suffixInclude);
	}

	QFileInfoList FileNameRegExpReplace::Handle(const QFileInfoList& files, ProgressPtr progress) {
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

	void FileNameRegExpReplace::Unhandle() {
		
	}

	/************************************************************************************************************************
	 * Class： DuplicateHandler
	 * 
	 * 
	/************************************************************************************************************************/
	FileDuplicateHandler::FileDuplicateHandler(const QString& pattern, int filedWidth, int base, QChar fill, bool after) {
		mArgMap["Pattern"] = Argument("Pattern", QObject::tr("Pattern"), QObject::tr("Used to add string templates when file duplication occurs, such as (N), N_N, N_, [N] Among them, N must exist."), pattern);
		mArgMap["Position"] = Argument("Position", QObject::tr("Position"), QObject::tr("Should it be placed before or after the file name, by default after."), after);
		mArgMap["Width"] = Argument("Width", QObject::tr("Width"), QObject::tr("Width of numbers4: 0001, 0002, ...; 2: 01, 02, 03, ..."), filedWidth);
		mArgMap["Fill"] = Argument("Fill", QObject::tr("Char filling"), QObject::tr("Used in conjunction with Width, defaults is '0'."), fill);
		mArgMap["Base"] = Argument("Base", QObject::tr("Base system"), QObject::tr("The base system for numerical output defaults to 10 (decimal)."), base);
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
			}
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
			result << newFile;
		}
		return result;
	}

	void FileDuplicateHandler::Unhandle() {

	}

	FileRenameByExp::FileRenameByExp(const QString& pattern, const QString& after, QRegExp::PatternSyntax syntax,
		bool caseSensitive, bool suffixInclude) {
		AddExp(pattern, after, syntax, caseSensitive, suffixInclude);
		mFileDuplicateHandler = std::make_shared<FileDuplicateHandler>();
	}

	void FileRenameByExp::AddExp(const QString& pattern, const QString& after, QRegExp::PatternSyntax syntax,
		bool caseSensitive, bool suffixInclude) {
		mFileNameRegExpHandlerList << std::make_shared<FileNameRegExpReplace>(pattern, after, syntax, caseSensitive, suffixInclude);
	}

	void FileRenameByExp::ClearExp() {
		mFileNameRegExpHandlerList.clear();
	}

	void FileRenameByExp::SetDuplicatedPolicy(const QString& pattern, int filedWidth, int base, QChar fill, bool after) {
		mFileDuplicateHandler->SetArg("Pattern", pattern);
	}

	QFileInfoList FileRenameByExp::Handle(const QFileInfoList& files, ProgressPtr progress) {
		QFileInfoList result;
		return result;
	}

	void FileRenameByExp::Unhandle() {

	}
}
