#pragma once
#include "FFXPdfHandler.h"

namespace FFX {
	class MergePdfHandler : public PdfHandler {
	public:
		MergePdfHandler(const QString& outPdf, bool compress = false);

	public:
		virtual QString Name() override { return QStringLiteral("MergePdfHandler"); }
		virtual std::shared_ptr<FileHandler> Clone() override;
		virtual void Cancel() override;
		virtual QString DisplayName() override { return QObject::tr("MergePdfHandler"); }
		virtual QString Description() override { return QObject::tr("Merge the given PDF files into one PDF file."); }

	protected:
		virtual QFileInfoList DoHandle(const QFileInfoList& files, ProgressPtr progress) override;

	private:
		void Merge(pdf_document* doc_src, pdf_document* doc_des);

	private:
		bool mCancelled = false;
		int mTotalMergedPageCount = 0;
	};
}
