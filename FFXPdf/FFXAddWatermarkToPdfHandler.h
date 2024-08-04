#pragma once

#include "FFXPdfHandler.h"
#include <QRect>
namespace FFX {
	class AddWatermarkToPdfHandler : public PdfHandler {
	public:
		explicit AddWatermarkToPdfHandler(const QString& content, const QString& fontface = "", int fontSize = 24, float rotate = 0, int position = 0, float opacity = 0.99, bool saveas = true);
		explicit AddWatermarkToPdfHandler(const QFileInfo& image, const QSize& size = QSize(), bool fixRatio = true, float rotate = 0, int position = 0, float opacity = 0.99, bool saveas = true);

	public:
		virtual QString Name() override { return QStringLiteral("AddWatermarkToPdfHandler"); }
		virtual std::shared_ptr<FileHandler> Clone() override;
		virtual void Cancel() override;
		virtual QString DisplayName() override { return QObject::tr("Add Watermark"); }
		virtual QString Description() override { return QObject::tr("Add watermark to each page of PDF."); }

	protected:
		virtual QFileInfoList DoHandle(const QFileInfoList& files, ProgressPtr progress) override;

	private:
		void AddImageWatermark(pdf_document* doc, const char* image, const char* pdfpath);
		void AddTextWatermark(pdf_document* doc);
		fz_matrix CalcImageMatrix(const fz_rect& box, const fz_rect& page_box) const;
		fz_matrix CalcTextMatrix();
	private:
		bool mCancelled = false;
	};
}
