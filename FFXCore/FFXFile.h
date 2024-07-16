#pragma once
#include "FFXCore.h"

#include <QFileInfo>
#include <QDir>
#include <QSet>

namespace FFX {
	class FileInfo {
	public:
		static FFXCORE_EXPORT QSet<QString> CustomSuffix;

	public:
		FileInfo() = default;
		FileInfo(const QString& path, bool isFile = true)
			: mPath(path)
			, mIsFile(isFile) {
			mPath = QDir::toNativeSeparators(QDir::cleanPath(mPath));
			mDepth = mPath.split(QDir::separator()).size();
		}

	public:
		void SetPath(const QString& path) {
			mPath = path;
		}
		void SetFile(bool isFile) {
			mIsFile = isFile;
		}
		bool IsFile() const {
			return mIsFile;
		}
		QString Path() const {
			return mPath;
		}
		bool IsValid() const {
			return !mPath.isEmpty();
		}
		int Depth() const {
			return mDepth;
		}
		QString Suffix() const {
			for (const QString& suffix : CustomSuffix) {
				if (mPath.endsWith(suffix, Qt::CaseInsensitive)) {
					return suffix;
				}
			}
			return QFileInfo(mPath).suffix();
		}
		QString BaseName() const {
			QFileInfo fi(mPath);
			QString fileName = fi.fileName();
			for (const QString& suffix : CustomSuffix) {
				if (fileName.endsWith(suffix, Qt::CaseInsensitive)) {
					return fileName.left(fileName.size() - suffix.size() - 1);
				}
			}
			return fi.completeBaseName();
		}
		QString FileName() const {
			QFileInfo fi(mPath);
			return fi.fileName();
		}
		QDir ParentDir() const {
			return QFileInfo(mPath).absoluteDir();
		}
		bool operator>(const FileInfo& other) const {
			return mDepth > other.mDepth;
		}
		bool operator>=(const FileInfo& other) const {
			return mDepth >= other.mDepth;
		}
		bool operator<(const FileInfo& other) const {
			return mDepth < other.mDepth;
		}
		bool operator<=(const FileInfo& other) const {
			return mDepth <= other.mDepth;
		}
		bool operator==(const FileInfo& other) const {
			return mPath == other.mPath;
		}

	private:
		QString mPath;
		int mDepth = 0;
		bool mIsFile = true;
	};
	Q_DECLARE_METATYPE(FileInfo)
	typedef QList<FileInfo> FileList;
}

