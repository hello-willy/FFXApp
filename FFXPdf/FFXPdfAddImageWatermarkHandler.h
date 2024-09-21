#pragma once
#include "FFXPdfHandler.h"
#include <QRect>

namespace FFX {
	class PdfAddImageWatermarkHandler : public PdfHandler {
	public:
		explicit PdfAddImageWatermarkHandler(const QString& imagePath, const QSize& size = QSize(), bool fixRatio = true, float rotate = 0, int position = 0, int opacity = 100, bool saveas = true);

	public:
		virtual QString Name() override { return QStringLiteral("PdfAddImageWatermarkHandler"); }
		virtual std::shared_ptr<FileHandler> Clone() override;
		virtual void Cancel() override;
		virtual QString DisplayName() override { return QObject::tr("Add Image Watermark"); }
		virtual QString Description() override { return QObject::tr("Add image watermark to each page of PDF."); }

	protected:
		virtual QFileInfoList DoHandle(const QFileInfoList& files, ProgressPtr progress) override;

	private:
		void AddImageWatermark(pdf_document* doc, const char* image, const char* pdfpath, int opacity = 255);
		fz_matrix CalcImageMatrix(const fz_rect& box, const fz_rect& pagebox) const;

	private:
		bool mCancelled = false;
	};
}


