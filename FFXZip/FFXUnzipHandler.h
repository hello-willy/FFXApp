#pragma once
#include "FFXFileHandler.h"

namespace FFX {
	class UnzipHandler : public FileHandler {
	public:
		UnzipHandler(const QString& outputDir = "", bool mkdir = true);

	public:
		virtual QFileInfoList Filter(const QFileInfoList& files) override;
		virtual QFileInfoList Handle(const QFileInfoList& files, ProgressPtr progress = G_DebugProgress) override;
		virtual std::shared_ptr<FileHandler> Clone() override;
		virtual QString Name() { return QStringLiteral("UnzipHandler"); }
		virtual QString DisplayName() { return QObject::tr("UnzipHandler"); }
		virtual QString Description() { return QObject::tr("Unzip file."); }

	private:
		QString MakeOutputDir(const QFileInfo& zipFile);
		void UnzipFile(const QFileInfo& zipFile, const QString& outputDir, ProgressPtr progress);

	private:
		FileFilterPtr mFileFilter;
	};
}