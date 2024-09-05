#include "FFXUnzipHandler.h"
#include "FFXFileFilterExpr.h"
#include "FFXFile.h"
#include "FFXZip.h"

#include <QCoreApplication>

#include <bit7z/bitarchivereader.hpp>
#include <bit7z/bitfilecompressor.hpp>
#include <bit7z/bitfileextractor.hpp>
#include <bit7z/bitarchivewriter.hpp>
using namespace bit7z;

namespace FFX {
	bool UnzipProgressCallback(uint64_t size, const QFileInfo& file, ProgressPtr p)
	{
		uint64_t totalSize = FileSize(file);
		double process = ((1.0 * size) / totalSize);
		p->OnProgress((int)(process * 100), QObject::tr("Unzip:%1").arg(file.absoluteFilePath()));
		return true;
	}

	UnzipHandler::UnzipHandler(const QString& outputDir, bool mkdir) {
		mArgMap["OutputDir"] = Argument("OutputDir", QObject::tr("OutputDir"), QObject::tr("Extract files to this directory, default to the current directory of the compressed file."), outputDir);
		mArgMap["MkDir"] = Argument("MkDir", QObject::tr("MkDir"), QObject::tr("Create a folder with the compressed file name in the output directory"), mkdir);

		FileFilterPtr fileOnlyFilter = std::make_shared<OnlyFileFilter>();
		FileFilterExpr expr("*.zip|*.rar|*.gz|*.7z|*.ffx", false);
		FileFilterPtr wildcardFilter = expr.Filter();
		mFileFilter = std::make_shared<AndFileFilter>(fileOnlyFilter, wildcardFilter);
	}

	QFileInfoList UnzipHandler::Filter(const QFileInfoList& files) {
		QFileInfoList filesTodo;
		for (const QFileInfo& file : files) {
			if (mFileFilter->Accept(file)) {
				filesTodo << file;
			}
		}
		return filesTodo;
	}

	QFileInfoList UnzipHandler::Handle(const QFileInfoList& files, ProgressPtr progress) {
		QFileInfoList result;
		QFileInfoList zipFiles = Filter(files);
		int size = zipFiles.size();
		for (int i = 0; i < size; i++) {
			const QFileInfo& file = zipFiles[i];
			QString outputDir = MakeOutputDir(file);
			UnzipFile(file, outputDir, progress);
			result << outputDir;
		}
		progress->OnComplete(true, "Finish.");
		return result;
	}

	std::shared_ptr<FileHandler> UnzipHandler::Clone() {
		return FileHandlerPtr(new UnzipHandler(*this));
	}

	QString UnzipHandler::MakeOutputDir(const QFileInfo& zipFile) {
		QString outputDir = mArgMap["OutputDir"].StringValue();
		bool mkdir = mArgMap["MkDir"].BoolValue();
		if (outputDir.isEmpty()) {
			outputDir = zipFile.absolutePath();
		}
		if (!mkdir) {
			return outputDir;
		}
		File file(zipFile);
		QString fileName = file.BaseName();
		QDir d(outputDir);
		d.mkdir(fileName);
		return d.absoluteFilePath(fileName);
	}

	void UnzipHandler::UnzipFile(const QFileInfo& zipFile, const QString& outputDir, ProgressPtr progress) {
		QString appDir = PluginDir();
		Bit7zLibrary lib{ QDir(appDir).absoluteFilePath("7z.dll").toStdString() };
		try {
			BitFileExtractor extractor{ lib, BitFormat::Auto };
			extractor.test(zipFile.absoluteFilePath().toStdString());
			//! bind progress callback function: prototype is <bool calback(uint64_t size)>
			extractor.setProgressCallback(std::bind(UnzipProgressCallback, std::placeholders::_1, zipFile, progress));

			extractor.extract(zipFile.absoluteFilePath().toStdString(), outputDir.toStdString());
			progress->OnFileComplete(zipFile, outputDir);
			/*
			//! if the zip file is gzip or bzip, will try to unzip tar.
			if (fileName.endsWith(QString(".tar.gz"), Qt::CaseInsensitive) || fileName.endsWith(QString(".tar.bz2"), Qt::CaseInsensitive))
			{
				QDir t(outputDir.absoluteFilePath(fileBaseName));
				QString tarFile = t.absoluteFilePath(QString("%1.tar").arg(fileBaseName));
				if (QFile::exists(tarFile))
				{
					extractor.extract(tarFile.toStdString(), outputDir.absoluteFilePath(fileBaseName).toStdString());
					QFile::remove(tarFile);
				}
			}
			*/
		} catch (const bit7z::BitException& ex) {
			progress->OnFileComplete(zipFile, outputDir, false, ex.what());
		}
	}
}