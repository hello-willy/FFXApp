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
		virtual std::string FilterExpression() = 0;
		virtual QFileInfoList DoHandle(const QFileInfoList& files, ProgressPtr progress) = 0;

	protected:
		fz_context* mContext = nullptr;
		FileFilterPtr mFilter;
	};

	class ImageToPdfHandler : public PdfHandler {
	public:
		ImageToPdfHandler(const QString& outPdf, const QSize& size = QSize(595, 842), const QRect& boundary = QRect(),
			bool portrait = true, bool autoRotate = false, bool strech = false);

	public:
		virtual std::string FilterExpression();
		virtual QString Name() override { return QStringLiteral("ImageToPdfHander"); }
		virtual std::shared_ptr<FileHandler> Clone() override;
		virtual void Cancel() override;
		virtual QString DisplayName() override { return QObject::tr("ImageToPdfHander"); }
		virtual QString Description() override { return QObject::tr("Convert images to PDF file."); }

	protected:
		virtual QFileInfoList DoHandle(const QFileInfoList& files, ProgressPtr progress) override;

	private:
		bool CreateImagePage(pdf_document* doc, const char* image, const char* imageName, const fz_rect& pageSize, bool portrait);
		fz_matrix CalcImageMatrix(int width, int height) const;

	private:
		bool mCancelled = false;
	};

	class AddWaterMarkToPdfHandler : public PdfHandler {

	};

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
		virtual std::string FilterExpression() override;
		virtual QFileInfoList DoHandle(const QFileInfoList& files, ProgressPtr progress) override;

	private:
		void Merge(pdf_document* doc_src, pdf_document* doc_des);

	private:
		bool mCancelled = false;
		int mTotalMergedPageCount = 0;
	};
}

