#include "FFXPdfHandler.h"
#include "FFXFileFilterExpr.h"
#include <QFont>
#include <QFontMetrics>

namespace FFX {
	QMap<QString, int> PositionMapping = {
		{"center", 0},
		{"lower left corner", 1},
		{"center left", 2},
		{"upper left corner", 3},
		{"center top", 4},
		{"upper right corner", 5},
		{"center right", 6},
		{"lower right corner", 7},
		{"center bottom", 8}
	};

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
		if (mFilter == nullptr) {
			FileFilterExpr expr(FilterExpression(), false);
			mFilter = expr.Filter();
		}

		if (mFilter == nullptr) {
			return files;
		}

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
		return true;
	}

	int PdfHandler::JM_insert_contents(fz_context* ctx, pdf_document* pdf,
		pdf_obj* pageref, fz_buffer* newcont, int overlay) {
		int xref = 0;
		fz_try(ctx) {
			pdf_obj* contents = pdf_dict_get(ctx, pageref, PDF_NAME(Contents));
			pdf_obj* newconts = pdf_add_stream(ctx, pdf, newcont, NULL, 0);
			xref = pdf_to_num(ctx, newconts);
			if (pdf_is_array(ctx, contents)) {
				if (overlay)
					pdf_array_push_drop(ctx, contents, newconts);
				else
					pdf_array_insert_drop(ctx, contents, newconts, 0);
			}
			else {
				pdf_obj* carr = pdf_new_array(ctx, pdf, 5);
				if (overlay) {
					if (contents)
						pdf_array_push(ctx, carr, contents);
					pdf_array_push_drop(ctx, carr, newconts);
				}
				else {
					pdf_array_push_drop(ctx, carr, newconts);
					if (contents)
						pdf_array_push(ctx, carr, contents);
				}
				pdf_dict_put_drop(ctx, pageref, PDF_NAME(Contents), carr);
			}
		}
		fz_catch(ctx)
			fz_rethrow(ctx);
		return xref;
	}

	void SetAlpha(fz_context* ctx, fz_pixmap* pix, int value) {
		unsigned char* s;
		int w, h, n;
		ptrdiff_t stride, len;
		int alpha = pix->alpha;
		if (!alpha)
			return;

		w = pix->w;
		h = pix->h;
		if (w < 0 || h < 0)
			return;

		n = pix->n;
		stride = pix->stride;
		len = (ptrdiff_t)w * n;

		s = pix->samples;
		int k, x, y;
		stride -= len;
		for (y = 0; y < pix->h; y++) {
			for (x = 0; x < pix->w; x++) {
				for (k = 0; k < pix->n - 1; k++) {
					*s++ = fz_mul255(*s, value);
				}
				*s++ = value;
			}
			s += stride;
		}
	}

	void ClonePixmapAndSetAlpha(fz_context* ctx, fz_pixmap* pix, fz_pixmap* src, int value)	{
		unsigned char* s;
		int w, h, n;
		ptrdiff_t stride, len;
		int alpha = pix->alpha;

		w = pix->w;
		h = pix->h;
		if (w < 0 || h < 0)
			return;

		n = pix->n;
		stride = pix->stride;
		len = (ptrdiff_t)w * n;

		s = pix->samples;
		unsigned char* s_src = src->samples;

		int k, x, y;
		stride -= len;
		for (y = 0; y < pix->h; y++) {
			for (x = 0; x < pix->w; x++) {
				for (k = 0; k < pix->n - 1; k++)
					*s++ = fz_mul255(*s_src++, value);
				*s++ = value;
			}
			s += stride;
		}
	}

	fz_rect PdfHandler::AddImage(pdf_document* doc, pdf_obj* resources, const char* name, const char* path, int opacity) {
		fz_image* image;
		pdf_obj* subres, * ref;

		image = fz_new_image_from_file(mContext, path);
		fz_rect rect = { 0, 0, (float)image->w, (float)image->h };

		fz_image* alphaImage = 0;
		fz_pixmap* pix = fz_get_pixmap_from_image(mContext, image, NULL, NULL, 0, 0);
		int alpha = fz_pixmap_alpha(mContext, pix);
		if (alpha == 0) {
			fz_pixmap* alphaPixmap = fz_new_pixmap_with_bbox(mContext, pix->colorspace, fz_pixmap_bbox(mContext, pix), 0, 1);
			ClonePixmapAndSetAlpha(mContext, alphaPixmap, pix, opacity);
			alphaImage = fz_new_image_from_pixmap(mContext, alphaPixmap, 0);
			fz_drop_pixmap(mContext, alphaPixmap);
		} else {
			SetAlpha(mContext, pix, opacity);
			alphaImage = fz_new_image_from_pixmap(mContext, pix, 0);
		}
		fz_drop_image(mContext, image);
		fz_drop_pixmap(mContext, pix);
		subres = pdf_dict_get(mContext, resources, PDF_NAME(XObject));
		if (!subres) {
			subres = pdf_new_dict(mContext, doc, 10);
			pdf_dict_put_drop(mContext, resources, PDF_NAME(XObject), subres);
		}

		ref = pdf_add_image(mContext, doc, alphaImage);
		pdf_dict_puts(mContext, subres, name, ref);
		pdf_drop_obj(mContext, ref);
		
		fz_drop_image(mContext, alphaImage);

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
					s += QString("/%1 %2 Tf (").arg(ansifont).arg(fontsize);
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
					s += QString("/%1 %2 Tf <").arg(cjkfont).arg(fontsize);
				}
				s += QString("%1").arg(b.unicode(), 4, 16, QChar('0'));
				isAsciiCode = false;
			}
		}
		if (!s.isEmpty()) {
			s += isAsciiCode ? QString(") Tj") : QString("> Tj");
			tjstr += s;
		}
		int notAsc = content.size() - asc;
		//! 1.9 Value obtained based on experience, the best results are obtained
		fz_rect r = { 0., 0., asc * fontsize / (float)1.9 + notAsc * (float)fontsize, (float)fontsize };
		return r;
	}

}

