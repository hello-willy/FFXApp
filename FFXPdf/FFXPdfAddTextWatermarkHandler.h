#pragma once
#include "FFXPdfHandler.h"

namespace FFX {
	class PdfAddTextWatermarkHandler : public PdfHandler {
	public:
		explicit PdfAddTextWatermarkHandler(const QString& content, const QString& fontface = "", int fontSize = 24, int rotate = 0, int position = 0, int opacity = 100, bool saveas = true);

	public:
		virtual QString Name() override { return QStringLiteral("AddWatermarkToPdfHandler"); }
		virtual std::shared_ptr<FileHandler> Clone() override;
		virtual void Cancel() override;
		virtual QString DisplayName() override { return QObject::tr("Add Watermark"); }
		virtual QString Description() override { return QObject::tr("Add watermark to each page of PDF."); }

	protected:
		virtual QFileInfoList DoHandle(const QFileInfoList& files, ProgressPtr progress) override;

	private:
		void AddTextWatermark(pdf_document* doc, const QString& text, const char* pdfpath);
		fz_matrix CalcTextMatrix(const fz_rect& textbox, const fz_rect& pagebox);

	private:
		bool mCancelled = false;
	};
}


