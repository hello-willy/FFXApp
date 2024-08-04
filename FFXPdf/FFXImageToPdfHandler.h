#pragma once
#include "FFXPdfHandler.h"

#include <QRect>
#include <QSize>

namespace FFX {
	class ImageToPdfHandler : public PdfHandler {
	public:
		ImageToPdfHandler(const QString& outPdf, const QSize& size = QSize(595, 842), const QRect& boundary = QRect(),
			bool portrait = true, bool autoRotate = false, bool strech = false);

	public:
		virtual QString Name() override { return QStringLiteral("ImageToPdfHander"); }
		virtual std::shared_ptr<FileHandler> Clone() override;
		virtual void Cancel() override;
		virtual QString DisplayName() override { return QObject::tr("ImageToPdfHander"); }
		virtual QString Description() override { return QObject::tr("Convert images to PDF file."); }

	protected:
		virtual std::string FilterExpression();
		virtual QFileInfoList DoHandle(const QFileInfoList& files, ProgressPtr progress) override;

	private:
		bool CreateImagePage(pdf_document* doc, const char* image, const char* imageName, const fz_rect& pageSize, bool portrait);
		fz_matrix CalcImageMatrix(int width, int height) const;

	private:
		bool mCancelled = false;
	};
}
