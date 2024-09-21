#include "FFXFile.h"

namespace FFX {
	QString G_FILE_VALIDATOR = "^[^/\\\\:*?\"<>|]+$";
	QSet<QString> File::CustomSuffix = { "shp.xml", "sbnand.sbx", "fbnand.fbx", "ainand.aih",
										 "tar.gz", "tar.bz2"
									   };

	QFileInfoList FileInfoList(const QStringList& files) {
		QFileInfoList fileInfoList;
		for (const QString& file : files) {
			fileInfoList << file;
		}
		return fileInfoList;
	}
	
	QFileInfoList FileInfoList(const QString& file) {
		QFileInfoList fileInfoList;
		fileInfoList << file;
		return fileInfoList;
	}

	QFileInfoList FileInfoList(const QFileInfo& file) {
		QFileInfoList fileInfoList;
		fileInfoList << file;
		return fileInfoList;
	}

	QFileInfoList FileInfoList(const QList<QUrl> urls) {
		QFileInfoList fileInfoList;
		for (const QUrl& url : urls)
			fileInfoList << url.toLocalFile();
		return fileInfoList;
	}

	QStringList StringList(const QFileInfoList& files) {
		QStringList ret;
		for (const QFileInfo& fi : files)
			ret << fi.absoluteFilePath();
		return ret;
	}

	qint64 SymbolLinkSize(const QFileInfo& file) {
		QFile link(file.absoluteFilePath());
		link.open(QIODevice::ReadOnly);
		qint64 size = link.size();
		link.close();
		return size;
	}

	qint64 FileSize(const QFileInfo& file) {
		if (file.isSymLink())
			return SymbolLinkSize(file);
		return file.size();
	}

	int PathDepth(const QString& path) {
		QString thePath(path);
		thePath = QDir::toNativeSeparators(QDir::cleanPath(thePath));
		return thePath.split(QDir::separator()).size();
	}

	void SortByDepth(QFileInfoList& files, bool asc) {
		std::sort(files.begin(), files.end(), [=](const QFileInfo& fi1, const QFileInfo& fi2) {
			if (fi1.isDir() && !fi2.isDir())
				return false;
			if (!fi1.isDir() && fi2.isDir())
				return true;
			int d1 = PathDepth(fi1.absoluteFilePath());
			int d2 = PathDepth(fi2.absoluteFilePath());
			return asc ? d1 < d2 : d1 > d2;
			});
	}
	void SortByDepth(QStringList& files, bool asc) {
		QFileInfoList fileInfoList;
		for (const QString& file : files) {
			fileInfoList << file;
		}
		SortByDepth(fileInfoList, asc);
		files.clear();
		for (const QFileInfo& file : fileInfoList) {
			files << file.absoluteFilePath();
		}
	}
}