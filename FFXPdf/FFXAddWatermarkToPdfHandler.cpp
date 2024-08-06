#include "FFXAddWatermarkToPdfHandler.h"
#include "FFXUtils.h"
#include <iostream>

#include <QFontMetrics>

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

	int JM_insert_contents(fz_context* ctx, pdf_document* pdf,
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
			} else {
				pdf_obj* carr = pdf_new_array(ctx, pdf, 5);
				if (overlay) {
					if (contents) 
						pdf_array_push(ctx, carr, contents);
					pdf_array_push_drop(ctx, carr, newconts);
				} else {
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

	AddWatermarkToPdfHandler::AddWatermarkToPdfHandler(const QString& content, const QString& fontface, int fontSize, float rotate, int position, float opacity, bool saveas) {
		mArgMap["WatermarkType"] = Argument("WatermarkType", QObject::tr("WatermarkType"), QObject::tr("Watermark type, 0 is text, 1 is image."), 0);
		mArgMap["Content"] = Argument("Content", QObject::tr("Content"), QObject::tr("If the watermark type is text, it is watermark text; if the watermark type is image, it is image path."), content);
		mArgMap["Font"] = Argument("Font", QObject::tr("Font"), QObject::tr("The fontface of watermark text."), fontface);
		mArgMap["FontSize"] = Argument("FontSize", QObject::tr("FontSize"), QObject::tr("The font size of watermark text."), fontSize);
		mArgMap["SaveAs"] = Argument("SaveAs", QObject::tr("SaveAs"), QObject::tr("Save watermarked files as new files."), saveas);
		mArgMap["Rotate"] = Argument("Rotate", QObject::tr("Rotate"), QObject::tr("The rotation angle of the watermark, measured in degrees."), rotate);
		mArgMap["Position"] = Argument("Position", QObject::tr("Position"), QObject::tr("Position of watermark, 0: center, 1: lower left corner, 2: center left, 3: upper left corner, 4: center top, 5: upper right corner, 6: center right, 7: lower right corner, 8: center bottom."), position);
		mArgMap["Opacity"] = Argument("Opacity", QObject::tr("Opacity"), QObject::tr("The transparency of the watermark, with a value range of [0-1]."), opacity);
	}

	AddWatermarkToPdfHandler::AddWatermarkToPdfHandler(const QFileInfo& image, const QSize& size, bool fixRatio, float rotate, int position, float opacity, bool saveas) {
		mArgMap["WatermarkType"] = Argument("WatermarkType", QObject::tr("WatermarkType"), QObject::tr("Watermark type, 0 is text, 1 is image."), 1);
		mArgMap["Content"] = Argument("Content", QObject::tr("Content"), QObject::tr("If the watermark type is text, it is watermark text; if the watermark type is image, it is image path."), image.absoluteFilePath());
		mArgMap["Size"] = Argument("Size", QObject::tr("Size"), QObject::tr("The size of image watermark."), size);
		mArgMap["FixRatio"] = Argument("FixRatio", QObject::tr("FixRatio"), QObject::tr("Does the image maintain its aspect ratio."), fixRatio);
		mArgMap["SaveAs"] = Argument("SaveAs", QObject::tr("SaveAs"), QObject::tr("Save watermarked files as new files."), saveas);
		mArgMap["Rotate"] = Argument("Rotate", QObject::tr("Rotate"), QObject::tr("The rotation angle of the watermark, measured in degrees."), rotate);
		mArgMap["Position"] = Argument("Position", QObject::tr("Position"), QObject::tr("Position of watermark, 0: center, 1: lower left corner, 2: center left, 3: upper left corner, 4: center top, 5: upper right corner, 6: center right, 7: lower right corner, 8: center bottom."), position);
		mArgMap["Opacity"] = Argument("Opacity", QObject::tr("Opacity"), QObject::tr("The transparency of the watermark, with a value range of [0-1]."), opacity);
	}

	std::shared_ptr<FileHandler> AddWatermarkToPdfHandler::Clone() {
		return FileHandlerPtr(new AddWatermarkToPdfHandler(*this));
	}

	void AddWatermarkToPdfHandler::Cancel() {
		mCancelled = true;
	}

	QFileInfoList AddWatermarkToPdfHandler::DoHandle(const QFileInfoList& files, ProgressPtr progress) {
		int wt = mArgMap["WatermarkType"].Value().toInt();

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

				if (wt == 1)
					AddImageWatermark(doc, content.toStdString().c_str(), newPdf.toStdString().c_str());
				else
					AddTextWatermark(doc, content, newPdf.toStdString().c_str());

				progress->OnFileComplete(file, file);
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

	void AddWatermarkToPdfHandler::AddImageWatermark(pdf_document* doc, const char* image, const char* pdfpath) {
		pdf_write_options opts = pdf_default_write_options;
		
		fz_try(mContext) {
			int pagecount = pdf_count_pages(mContext, doc);
			for (int i = 0; i < pagecount; i++) {
				pdf_page*  page = pdf_load_page(mContext, doc, i);
				pdf_obj* resources = pdf_dict_get_inheritable(mContext, page->obj, PDF_NAME(Resources));
				fz_buffer* contents = fz_new_buffer(mContext, 1024);

				fz_rect pagebox = pdf_bound_page(mContext, page, FZ_MEDIA_BOX);
				fz_rect box = AddImage(doc, resources, "watermark888", image);

				fz_matrix m = CalcImageMatrix(box, pagebox);

				char buf[128];
				sprintf(buf, "\nq\n%f %f %f %f %f %f cm\n/watermark888 Do\nQ", m.a, m.b, m.c, m.d, m.e, m.f);

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

	void AddWatermarkToPdfHandler::AddTextWatermark(pdf_document* doc, const QString& text, const char* pdfpath) {
		int fontsize = mArgMap["FontSize"].Value().toInt();

		pdf_write_options opts = pdf_default_write_options;
		int opacity = (int)(mArgMap["Opacity"].Value().toFloat() * 100);
		if (opacity < 0) opacity = 0;
		if (opacity >= 100) opacity = 99;
		char opstr[16];
		sprintf(opstr, "fitzca%2d%2d", opacity, opacity);
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
				pdf_dict_put_real(mContext, opa, PDF_NAME(CA), 0.5);
				pdf_dict_put_real(mContext, opa, PDF_NAME(ca), 0.5);
				pdf_dict_puts(mContext, extg, opstr, opa);

				AddFont(doc, resources, ansifont, "Helvetica", "");
				AddCjkFont(doc, resources, cjkfont, "zh-Hans", "", "");

				QString tjstr;
				fz_rect textbox = MakeTjStr(text, tjstr, ansifont, cjkfont, fontsize);

				fz_buffer* contents = fz_new_buffer(mContext, 2048);
				fz_matrix m = CalcTextMatrix(textbox, bound);

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

	fz_matrix AddWatermarkToPdfHandler::CalcImageMatrix(const fz_rect& imagebox, const fz_rect& pagebox) const {
		QSize size = mArgMap["Size"].Value().toSize();
		bool fixratio = mArgMap["FixRatio"].Value().toBool();
		int position = mArgMap["Position"].Value().toInt();

		fz_rect newImageBox;
		std::memcpy(&newImageBox, &imagebox, sizeof(fz_rect));
		float iw = newImageBox.x1 - newImageBox.x0;
		float ih = newImageBox.y1 - newImageBox.y0;
		float pw = pagebox.x1 - pagebox.x0;
		float ph = pagebox.y1 - pagebox.y0;

		if (!size.isEmpty()) {
			double ir = iw / ih;
			double sr = size.width() / (double)size.height();
			if (ir < sr) {
				ih = (std::min)((int)ih, size.height());
				iw = ih * ir;
			}
			else {
				iw = (std::min)((int)iw, size.width());
				ih = iw / ir;
			}
		}
		int margin = 5;
		fz_matrix m = fz_make_matrix(iw, 0, 0, ih, 0, 0);
		switch (position) {
		case 0: // central
			m = fz_concat(m, fz_translate((pw - iw) / 2, (ph - ih) / 2));
			break;
		case 1: // lower left corner
			m = fz_concat(m, fz_translate(margin, margin));
			break;
		case 2: // left center
			m = fz_concat(m, fz_translate(margin, (ph - ih) / 2));
			break;
		case 3: // upper left corner
			m = fz_concat(m, fz_translate(margin, ph - ih - margin));
			break;
		case 4: // center top
			m = fz_concat(m, fz_translate((pw - iw) / 2, ph - ih - margin));
			break;
		case 5: // upper right corner
			m = fz_concat(m, fz_translate(pw - iw - margin, ph - ih - margin));
			break;
		case 6: // right center
			m = fz_concat(m, fz_translate(pw - iw - margin, (ph - ih) / 2 - margin));
			break;
		case 7: // lower right corner
			m = fz_concat(m, fz_translate(pw - iw - margin, margin));
			break;
		case 8: // center bottom
			m = fz_concat(m, fz_translate((pw - iw) / 2, margin));
			break;
		}
		return m;
	}

	fz_matrix AddWatermarkToPdfHandler::CalcTextMatrix(const fz_rect& textbox, const fz_rect& pagebox) {
		int fontsize = mArgMap["FontSize"].Value().toInt();
		int position = mArgMap["Position"].Value().toInt();
		float r = mArgMap["Rotate"].Value().toFloat();

		float th = textbox.y1 - textbox.y0;
		float tw = textbox.x1 - textbox.x0;
		float pw = pagebox.x1 - pagebox.x0;
		float ph = pagebox.y1 - pagebox.y0;
		int margin = 5;
		fz_matrix m = fz_make_matrix(1, 0, 0, 1, 0, 0);
		switch (position) {
		case 0: // central
			m = CalCentralTextMatrix(tw, th, pagebox, r);
			break;
		case 1: // lower left corner
			m = fz_concat(m, fz_translate(margin, margin));
			break;
		case 2: // left center
			m = fz_concat(m, fz_rotate(-90));
			m = fz_concat(m, fz_translate(margin, (ph - th) / 2));
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
			m = fz_concat(m, fz_translate(pw - th - margin, (ph - tw) / 2  + tw));
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