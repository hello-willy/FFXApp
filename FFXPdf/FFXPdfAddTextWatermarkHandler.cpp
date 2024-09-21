#include "FFXPdfAddTextWatermarkHandler.h"
#include "FFXUtils.h"
#include <iostream>

namespace FFX {
	fz_matrix CalCentralTextMatrix(float tw, float th, const fz_rect& page_box, float r) {
		float w = page_box.x1 - page_box.x0;
		float h = page_box.y1 - page_box.y0;

		float w1 = th * std::sin(DegToRad(r));
		float nw = w1 + tw * std::cos(DegToRad(r));
		float nh = th * std::cos(DegToRad(r)) + th * std::sin(DegToRad(r));

		fz_matrix m = fz_make_matrix(1, 0, 0, 1, 0, 0);
		m = fz_concat(m, fz_rotate(r));
		m = fz_concat(m, fz_translate((w - nw) / 2 + w1, (h - nh) / 2));
		return m;
	}

	PdfAddTextWatermarkHandler::PdfAddTextWatermarkHandler(const QString& content, const QString& fontface, int fontSize, int rotate, int position, int opacity, bool saveas) {
		mArgMap["Content"] = Argument("Content", QObject::tr("Content"), QObject::tr("The text of watermark."), content, Argument::Normal, true);
		mArgMap["Font"] = Argument("Font", QObject::tr("Font"), QObject::tr("The fontface of watermark text."), fontface, Argument::Option);
		mArgMap["Font"].AddLimit("Helvetica").AddLimit("Times-Roman");
		mArgMap["FontSize"] = Argument("FontSize", QObject::tr("FontSize"), QObject::tr("The font size of watermark text."), fontSize);
		mArgMap["FontSize"].AddLimit("^([1-9]\\d?)$");
		mArgMap["Rotate"] = Argument("Rotate", QObject::tr("Rotate"), QObject::tr("The rotation angle of the watermark(measured in degrees). Only works when $Position is center."), rotate);
		mArgMap["Rotate"].AddLimit("^(360|3[0-5]\\d|[1-2]\\d\\d|[1-9]\\d?)$");
		mArgMap["Position"] = Argument("Position", QObject::tr("Position"), QObject::tr("Position of watermark."), position, Argument::Option);
		mArgMap["Position"].AddLimit("center").AddLimit("lower left corner").AddLimit("center left").AddLimit("upper left corner").AddLimit("center top").AddLimit("upper right corner").AddLimit("center right").AddLimit("lower right corner").AddLimit("center bottom");
		mArgMap["Opacity"] = Argument("Opacity", QObject::tr("Opacity"), QObject::tr("The transparency of the watermark, with a range of [1-100]."), opacity);
		mArgMap["Opacity"].AddLimit("^(100|[1-9]\\d?)$");
	}

	std::shared_ptr<FileHandler> PdfAddTextWatermarkHandler::Clone() {
		return FileHandlerPtr(new PdfAddTextWatermarkHandler(*this));
	}

	void PdfAddTextWatermarkHandler::Cancel() {
		mCancelled = true;
	}

	QFileInfoList PdfAddTextWatermarkHandler::DoHandle(const QFileInfoList& files, ProgressPtr progress) {
		QString content = mArgMap["Content"].Value().toString();
		int size = files.size();
		QFileInfoList result;
		for (int i = 0; i < size && !mCancelled; i++) {
			QFileInfo file = files[i];
			QString filePath = file.absoluteFilePath();
			double p = (i / (double)size) * 100;
			progress->OnProgress(p, QObject::tr("Watermarking file: %1").arg(filePath));

			pdf_document* doc = NULL;
			fz_try(mContext) {
				doc = pdf_open_document(mContext, filePath.toStdString().c_str());
				QString newPdf = file.absoluteDir().absoluteFilePath(QString("%1_Marked.pdf").arg(file.completeBaseName()));
				if (QFileInfo::exists(newPdf))
					QFile::remove(newPdf);

				AddTextWatermark(doc, content, newPdf.toStdString().c_str());

				progress->OnFileComplete(file, newPdf);
				result << newPdf;
			}
			fz_always(mContext)
				pdf_drop_document(mContext, doc);
			fz_catch(mContext) {
				progress->OnFileComplete(file, file, false, QObject::tr("Watermark %1 failed.").arg(filePath));
			}
		}
		progress->OnComplete(true, QObject::tr("Finish, %1 files watermarked.").arg(result.size()));
		return result;
	}

	void PdfAddTextWatermarkHandler::AddTextWatermark(pdf_document* doc, const QString& text, const char* pdfpath) {
		int fontsize = mArgMap["FontSize"].Value().toInt();

		pdf_write_options opts = pdf_default_write_options;
		int opacity = mArgMap["Opacity"].Value().toInt();
		if (opacity < 0) opacity = 0;
		if (opacity >= 100) opacity = 99;
		char opstr[16];
		sprintf(opstr, "fitzca%2d%2d", opacity, opacity); // is seem useless
		const char* ansifont = "Helv";
		const char* cjkfont = "Song";

		fz_try(mContext) {
			int pagecount = pdf_count_pages(mContext, doc);
			for (int i = 0; i < pagecount; i++) {
				pdf_page* page = pdf_load_page(mContext, doc, i);
				fz_rect bound = pdf_bound_page(mContext, page, FZ_MEDIA_BOX);

				pdf_obj* resources = pdf_dict_get(mContext, page->obj, PDF_NAME(Resources));
				pdf_obj* extg = pdf_dict_get(mContext, resources, PDF_NAME(ExtGState));
				if (extg == nullptr) {
					extg = pdf_dict_put_dict(mContext, resources, PDF_NAME(ExtGState), 2);
				}
				pdf_obj* opa = pdf_new_dict(mContext, doc, 3);
				pdf_dict_put_real(mContext, opa, PDF_NAME(CA), opacity / 100.);
				pdf_dict_put_real(mContext, opa, PDF_NAME(ca), opacity / 100.);
				pdf_dict_puts(mContext, extg, opstr, opa);

				AddFont(doc, resources, ansifont, "Helvetica", "");
				AddCjkFont(doc, resources, cjkfont, "zh-Hans", "", "");

				QString tjstr;
				fz_rect textbox = MakeTjStr(text, tjstr, ansifont, cjkfont, fontsize);

				fz_buffer* contents = fz_new_buffer(mContext, 2048);
				fz_matrix m = CalcTextMatrix(textbox, bound);

				//char rectBuf[128];
				//sprintf(rectBuf, "\nq\n%.0f %.0f m\n%.0f %.0f l\n%.0f %.0f l\n%.0f %.0f l\nf\nQ", textbox.x0, textbox.y0, textbox.x1, textbox.y0, textbox.x1, textbox.y1, textbox.x0, textbox.y1);
				char buf[512];
				sprintf(buf, "\nq/%s gs\n0 0 0 rg\n%f %f %f %f %f %f Tm\nBT %s ET\nQ", opstr, m.a, m.b, m.c, m.d, m.e, m.f, tjstr.toStdString().c_str());

				fz_append_string(mContext, contents, buf);
				JM_insert_contents(mContext, doc, page->obj, contents, 1);

				fz_drop_buffer(mContext, contents);
			}

			pdf_save_document(mContext, doc, pdfpath, &opts);
		}
		fz_catch(mContext) {
			fz_report_error(mContext);
		}
	}

	fz_matrix PdfAddTextWatermarkHandler::CalcTextMatrix(const fz_rect& textbox, const fz_rect& pagebox) {
		int fontsize = mArgMap["FontSize"].Value().toInt();
		QString position = mArgMap["Position"].Value().toString();
		int r = mArgMap["Rotate"].Value().toInt();

		float th = textbox.y1 - textbox.y0;
		float tw = textbox.x1 - textbox.x0;
		float pw = pagebox.x1 - pagebox.x0;
		float ph = pagebox.y1 - pagebox.y0;
		int margin = 5;
		fz_matrix m = fz_make_matrix(1, 0, 0, 1, 0, 0);
		switch (PositionMapping[position]) {
		case 0: // central
			m = CalCentralTextMatrix(tw, th, pagebox, r);
			break;
		case 1: // lower left corner
			m = fz_concat(m, fz_translate(margin, margin));
			break;
		case 2: // left center
			m = fz_concat(m, fz_rotate(-90));
			m = fz_concat(m, fz_translate(margin, tw + (ph - tw) / 2));
			break;
		case 3: // upper left corner
			m = fz_concat(m, fz_translate(margin, ph - th - margin));
			break;
		case 4: // center top
			m = fz_concat(m, fz_translate((pw - tw) / 2, ph - th - margin));
			break;
		case 5: // upper right corner
			m = fz_concat(m, fz_translate(pw - tw - margin, ph - th - margin));
			break;
		case 6: // right center
			m = fz_concat(m, fz_rotate(-90));
			m = fz_concat(m, fz_translate(pw - th - margin, (ph - tw) / 2 + tw));
			break;
		case 7: // lower right corner
			m = fz_concat(m, fz_translate(pw - tw - margin, margin));
			break;
		case 8: // center bottom
			m = fz_concat(m, fz_translate((pw - tw) / 2, margin));
			break;
		}
		return m;
	}
}