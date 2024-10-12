#include "FFXPdfAddImageWatermarkHandler.h"
#include "FFXFile.h" // for G_FILE_VALIDATOR

namespace FFX {

	PdfAddImageWatermarkHandler::PdfAddImageWatermarkHandler(const QString& imagePath, const QSize& size, bool fixRatio, float rotate, int position, int opacity, bool saveas) {
		mArgMap["ImagePath"] = Argument("ImagePath", QObject::tr("Image Path"), QObject::tr("Image's image path of the watermark."), imagePath, Argument::File, true);
		mArgMap["ImagePath"].AddLimit("Image file(*.jpg *.jpeg *.png *.bmp)");

		mArgMap["Size"] = Argument("Size", QObject::tr("Size"), QObject::tr("The size of image watermark."), size, Argument::Size);
		mArgMap["KeepAspectRatio"] = Argument("KeepAspectRatio", QObject::tr("Keep Aspect Ratio"), QObject::tr("Does the image maintain its aspect ratio."), fixRatio, Argument::Bool);
		mArgMap["Position"] = Argument("Position", QObject::tr("Position"), QObject::tr("Position of watermark, 0: center, 1: lower left corner, 2: center left, 3: upper left corner, 4: center top, 5: upper right corner, 6: center right, 7: lower right corner, 8: center bottom."), position, Argument::Option);
		mArgMap["Position"].AddLimit("center").AddLimit("lower left corner").AddLimit("center left").AddLimit("upper left corner").AddLimit("center top").AddLimit("upper right corner").AddLimit("center right").AddLimit("lower right corner").AddLimit("center bottom");
		mArgMap["Opacity"] = Argument("Opacity", QObject::tr("Opacity"), QObject::tr("The transparency of the watermark, with a range of [1-100]."), opacity);
		mArgMap["Opacity"].AddLimit("^(100|[1-9]\\d?)$");
	}

	std::shared_ptr<FileHandler> PdfAddImageWatermarkHandler::Clone() {
		return FileHandlerPtr(new PdfAddImageWatermarkHandler(*this));
	}

	void PdfAddImageWatermarkHandler::Cancel() {
		mCancelled = true;
	}

	QFileInfoList PdfAddImageWatermarkHandler::DoHandle(const QFileInfoList& files, ProgressPtr progress) {
		int wt = mArgMap["WatermarkType"].Value().toInt();
		int opacity = mArgMap["Opacity"].IntValue();
		if (opacity <= 0 || opacity > 100) opacity = 100;
		opacity = (int)(255 * (opacity / 100.));

		QString content = mArgMap["ImagePath"].Value().toString();
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

				AddImageWatermark(doc, content.toStdString().c_str(), newPdf.toStdString().c_str(), opacity);

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

	void PdfAddImageWatermarkHandler::AddImageWatermark(pdf_document* doc, const char* image, const char* pdfpath, int opacity) {
		pdf_write_options opts = pdf_default_write_options;

		fz_try(mContext) {
			int pagecount = pdf_count_pages(mContext, doc);
			for (int i = 0; i < pagecount; i++) {
				pdf_page* page = pdf_load_page(mContext, doc, i);
				pdf_obj* resources = pdf_dict_get_inheritable(mContext, page->obj, PDF_NAME(Resources));
				fz_buffer* contents = fz_new_buffer(mContext, 1024);

				fz_rect pagebox = pdf_bound_page(mContext, page, FZ_MEDIA_BOX);
				fz_rect box = AddImage(doc, resources, "watermark888", image, opacity);

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

	fz_matrix PdfAddImageWatermarkHandler::CalcImageMatrix(const fz_rect& imagebox, const fz_rect& pagebox) const {
		QSize size = mArgMap["Size"].Value().toSize();
		bool keepAspectRatio = mArgMap["KeepAspectRatio"].Value().toBool();
		QString position = mArgMap["Position"].Value().toString();

		fz_rect newImageBox;
		std::memcpy(&newImageBox, &imagebox, sizeof(fz_rect));

		float iw = newImageBox.x1 - newImageBox.x0;
		float ih = newImageBox.y1 - newImageBox.y0;
		float pw = pagebox.x1 - pagebox.x0;
		float ph = pagebox.y1 - pagebox.y0;

		if (!size.isEmpty()) {
			double ir = iw / ih;
			if (keepAspectRatio) {
				iw = size.width();
				ih = iw / ir;
			} else {
				iw = size.width();
				ih = size.height();
				/*
				double sr = size.width() / (double)size.height();
				if (ir < sr) {
					ih = (std::min)((int)ih, size.height());
					iw = ih * ir;
				}
				else {
					iw = (std::min)((int)iw, size.width());
					ih = iw / ir;
				}
				*/
			}
		}
		int margin = 5;
		fz_matrix m = fz_make_matrix(iw, 0, 0, ih, 0, 0);
		switch (PositionMapping[position]) {
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
}