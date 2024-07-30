#pragma once
#include "FFXFileHandler.h"
#include "FFXFileFilter.h"

//! Mupdf library
#include "mupdf/fitz.h"
#include "mupdf/pdf.h"

#include <QSize>
#include <QRect>

namespace FFX {
	class PdfHandler : public FileHandler {
	public:
		PdfHandler();
		virtual ~PdfHandler();

	public:
		virtual QFileInfoList Filter(const QFileInfoList& files) override;
		virtual QFileInfoList Handle(const QFileInfoList& files, ProgressPtr progress = G_DebugProgress) override;

	public:
		fz_rect AddImage(pdf_document* doc, pdf_obj* resources, const char* name, const char* path);

	protected:
		bool Init(ProgressPtr progress);
		virtual QFileInfoList DoHandle(const QFileInfoList& files, ProgressPtr progress) = 0;

	protected:
		fz_context* mContext = nullptr;
		FileFilterPtr mFilter;
	};

	class ImageToPdfHandler : public PdfHandler {
	public:
		ImageToPdfHandler(const QString& outPdf, const QSize& size = QSize(595, 842), 
			bool portrait = true, bool autoRotate = false, const QRect& margins = QRect(0, 0, 0, 0));

	public:
		virtual QString Name() override { return QStringLiteral("ImageToPdfHander"); }
		virtual std::shared_ptr<FileHandler> Clone() override;
		virtual void Cancel() override;
		virtual QString DisplayName() override { return QObject::tr("ImageToPdfHander"); }
		virtual QString Description() override { return QObject::tr("Convert images to PDF file."); }

	protected:
		virtual QFileInfoList DoHandle(const QFileInfoList& files, ProgressPtr progress) override;

	private:
		fz_matrix CalcImageMatrix(int width, int height) const;

	private:
		bool mCancelled = false;
	};
}

