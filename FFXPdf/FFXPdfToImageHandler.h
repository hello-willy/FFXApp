#pragma once

#include "FFXPdfHandler.h"

namespace FFX {
	class PdfToImageHandler : public PdfHandler	{
	public:
		PdfToImageHandler(const QString& outputDir = "", const QString& pages = "1-N", int dpi = 72);

	public:
		virtual QString Name() override { return QStringLiteral("PdfToImageHandler"); }
		virtual std::shared_ptr<FileHandler> Clone() override;
		virtual void Cancel() override;
		virtual QString DisplayName() override { return QObject::tr("PdfToImageHandler"); }
		virtual QString Description() override { return QObject::tr("Convert PDF to image files per page."); }

	protected:
		virtual QFileInfoList DoHandle(const QFileInfoList& files, ProgressPtr progress) override;

	private:
		QString MakeOutput(const QFileInfo& file);

		QList<int> FetchPageNumbers(fz_context* ctx, const char* range, int count);
		void RunPage(fz_document* doc, fz_document_writer* writer, int pageNum);

	private:
		bool mCancelled = false;
	};
}


