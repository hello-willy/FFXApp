#pragma once
#include "FFXFileHandler.h"
#include "FFXFileFilter.h"

//! Mupdf library
#include "mupdf/fitz.h"
#include "mupdf/pdf.h"

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
		void AddCjkFont(pdf_document* doc, pdf_obj* resources, const char* name, const char* lang, const char* wm, const char* style);

	protected:
		bool Init(ProgressPtr progress);
		virtual std::string FilterExpression();
		virtual QFileInfoList DoHandle(const QFileInfoList& files, ProgressPtr progress) = 0;

	protected:
		fz_context* mContext = nullptr;
		FileFilterPtr mFilter;
	};
	
}

