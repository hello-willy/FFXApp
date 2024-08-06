#pragma once
#include "FFXPdfHandler.h"

namespace FFX {
	class ExtractImageHandler : public PdfHandler {
	public:
		ExtractImageHandler(const QString& ouputDir);

	public:
		virtual QString Name() override { return QStringLiteral("ExtractImageHandler"); }
		virtual std::shared_ptr<FileHandler> Clone() override;
		virtual void Cancel() override;
		virtual QString DisplayName() override { return QObject::tr("Extract Image"); }
		virtual QString Description() override { return QObject::tr("Extract images from PDF files."); }

	protected:
		virtual QFileInfoList DoHandle(const QFileInfoList& files, ProgressPtr progress) override;

	private:
		QString SaveImage(pdf_document* doc, pdf_obj* ref);

	private:
		bool mCancelled = false;
	};
}
