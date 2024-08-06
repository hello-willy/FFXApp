#include "FFXPdfHandler.h"
#include "FFXFileFilterExpr.h"

namespace FFX {
	PdfHandler::PdfHandler() {
	}

	PdfHandler::~PdfHandler() {
		if (mContext != nullptr) {
			fz_drop_context(mContext);
		}
	}

	std::string PdfHandler::FilterExpression() {
		return "*.pdf";
	}

	QFileInfoList PdfHandler::Filter(const QFileInfoList& files) {
		QFileInfoList filesTodo;
		for (const QFileInfo& file : files) {
			if (mFilter->Accept(file)) {
				filesTodo << file;
			}
		}
		return filesTodo;
	}

	QFileInfoList PdfHandler::Handle(const QFileInfoList& files, ProgressPtr progress) {
		if (!Init(progress)) {
			return QFileInfoList();
		}

		QFileInfoList filesTodo = Filter(files);
		if (filesTodo.isEmpty()) {
			progress->OnComplete(true, QObject::tr("Finish, nothing to do."));
			return QFileInfoList();
		}

		return DoHandle(filesTodo, progress);
	}

	bool PdfHandler::Init(ProgressPtr progress) {
		mContext = fz_new_context(NULL, NULL, FZ_STORE_UNLIMITED);
		if (!mContext) {
			progress->OnComplete(false, QObject::tr("Context initialise failed."));
			return false;
		}
		FileFilterExpr expr(FilterExpression(), false);
		mFilter = expr.Filter();
		if (mFilter == nullptr) {
			progress->OnComplete(false, QObject::tr("Filter initialise failed."));
			return false;
		}
		return true;
	}

	fz_rect PdfHandler::AddImage(pdf_document* doc, pdf_obj* resources, const char* name, const char* path) {
		fz_image* image;
		pdf_obj* subres, * ref;

		image = fz_new_image_from_file(mContext, path);
		fz_rect rect = { 0, 0, (float)image->w, (float)image->h };

		subres = pdf_dict_get(mContext, resources, PDF_NAME(XObject));
		if (!subres) {
			subres = pdf_new_dict(mContext, doc, 10);
			pdf_dict_put_drop(mContext, resources, PDF_NAME(XObject), subres);
		}

		ref = pdf_add_image(mContext, doc, image);
		pdf_dict_puts(mContext, subres, name, ref);
		pdf_drop_obj(mContext, ref);

		fz_drop_image(mContext, image);

		return rect;
	}

	void PdfHandler::AddCjkFont(pdf_document* doc, pdf_obj* resources, const char* name, const char* lang, const char* wm, const char* style) {
		const unsigned char* data;
		int size, index, ordering, wmode, serif;
		fz_font* font;
		pdf_obj* subres, * ref;

		ordering = fz_lookup_cjk_ordering_by_language(lang);

		if (wm && !strcmp(wm, "V"))
			wmode = 1;
		else
			wmode = 0;

		if (style && (!strcmp(style, "sans") || !strcmp(style, "sans-serif")))
			serif = 0;
		else
			serif = 1;

		data = fz_lookup_cjk_font(mContext, ordering, &size, &index);
		font = fz_new_font_from_memory(mContext, NULL, data, size, index, 0);
		//font = fz_load_system_font(mContext, "黑体", 0, 0, 0);
		subres = pdf_dict_get(mContext, resources, PDF_NAME(Font));
		if (!subres) {
			subres = pdf_new_dict(mContext, doc, 10);
			pdf_dict_put_drop(mContext, resources, PDF_NAME(Font), subres);
		}

		ref = pdf_add_cjk_font(mContext, doc, font, ordering, wmode, serif);
		pdf_dict_puts(mContext, subres, name, ref);
		pdf_drop_obj(mContext, ref);

		fz_drop_font(mContext, font);
	}

	void PdfHandler::AddFont(pdf_document* doc, pdf_obj* resources, const char* name, const char* path, const char* encname) {
		const unsigned char* data;
		int size, enc;
		fz_font* font;
		pdf_obj* subres, * ref;

		data = fz_lookup_base14_font(mContext, path, &size);
		if (data)
			font = fz_new_font_from_memory(mContext, path, data, size, 0, 0);
		else
			font = fz_new_font_from_file(mContext, NULL, path, 0, 0);

		subres = pdf_dict_get(mContext, resources, PDF_NAME(Font));
		if (!subres) {
			subres = pdf_new_dict(mContext, doc, 10);
			pdf_dict_put_drop(mContext, resources, PDF_NAME(Font), subres);
		}

		enc = PDF_SIMPLE_ENCODING_LATIN;
		if (encname) {
			if (!strcmp(encname, "Latin") || !strcmp(encname, "Latn"))
				enc = PDF_SIMPLE_ENCODING_LATIN;
			else if (!strcmp(encname, "Greek") || !strcmp(encname, "Grek"))
				enc = PDF_SIMPLE_ENCODING_GREEK;
			else if (!strcmp(encname, "Cyrillic") || !strcmp(encname, "Cyrl"))
				enc = PDF_SIMPLE_ENCODING_CYRILLIC;
		}

		ref = pdf_add_simple_font(mContext, doc, font, enc);
		pdf_dict_puts(mContext, subres, name, ref);
		pdf_drop_obj(mContext, ref);

		fz_drop_font(mContext, font);
	}

	fz_rect PdfHandler::MakeTjStr(const QString& content, QString& tjstr, const char* ansifont, const char* cjkfont, int fontsize) {
		QString s;
		bool isAsciiCode = true;
		int asc = 0;

		for (QChar b : content) {
			ushort u = b.unicode();
			if (u < 256) {
				if (!isAsciiCode && !s.isEmpty()) {
					s += QString("> Tj ");
					tjstr += s;
					s = "";
				}
				if (s.isEmpty()) {
					s += QString("/%1 24 Tf (").arg(ansifont);
				}
				s += b;
				isAsciiCode = true;
				asc++;
			}
			else {
				if (isAsciiCode && !s.isEmpty()) {
					s += QString(") Tj ");
					tjstr += s;
					s = "";
				}
				if (s.isEmpty()) {
					s += QString("/%1 24 Tf <").arg(cjkfont);
				}
				s += QString("%1").arg(b.unicode(), 4, 16, QChar('0'));
				isAsciiCode = false;
			}
		}
		if (!s.isEmpty()) {
			s += isAsciiCode ? QString(") Tj") : QString("> Tj");
			tjstr += s;
		}
		//! 1.9 Value obtained based on experience, the best results are obtained
		fz_rect r = { 0., 0., asc * fontsize / (float)1.9 + (content.size() - asc) * (float)fontsize, (float)fontsize };
		return r;
	}
}

