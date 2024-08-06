#include "FFXExtractImageHandler.h"

namespace FFX {
	ExtractImageHandler::ExtractImageHandler(const QString& outputDir) {
		mArgMap["OutputDir"] = Argument("OutputDir", QObject::tr("OutputDir"), QObject::tr("Storage directory for images."), outputDir);
	}

	std::shared_ptr<FileHandler> ExtractImageHandler::Clone() {
		return FileHandlerPtr(new ExtractImageHandler(*this));
	}

	void ExtractImageHandler::Cancel() {

	}

	QFileInfoList ExtractImageHandler::DoHandle(const QFileInfoList& files, ProgressPtr progress) {
		int size = files.size();
		QFileInfoList result;
		for (int i = 0; i < size && !mCancelled; i++) {
			QFileInfo file = files[i];
			QString filePath = file.absoluteFilePath();
			pdf_document* doc = NULL;
			fz_try(mContext) {
				doc = pdf_open_document(mContext, filePath.toStdString().c_str());
				int len = pdf_count_objects(mContext, doc);
				for (int o = 1; o < len && !mCancelled; o++) {
					pdf_obj* ref = pdf_new_indirect(mContext, doc, o, 0);
					pdf_obj* type = pdf_dict_get(mContext, ref, PDF_NAME(Subtype));
					if (pdf_name_eq(mContext, type, PDF_NAME(Image))) {
						result << SaveImage(doc, ref);
					}
					fz_empty_store(mContext);
				}
			}
			fz_always(mContext)
				pdf_drop_document(mContext, doc);
			fz_catch(mContext) {
				fz_report_error(mContext);
			}
		}
		return result;
	}

	QString ExtractImageHandler::SaveImage(pdf_document* doc, pdf_obj* ref) {
		QString outputDir = mArgMap["OutputDir"].Value().toString();

		fz_image* image = NULL;
		fz_pixmap* pix = NULL;
		fz_pixmap* mask = NULL;
		fz_compressed_buffer* cbuf;
		int type;
		QString file;

		fz_var(image);
		fz_var(pix);

		fz_try(mContext) {
			image = pdf_load_image(mContext, doc, ref);
			cbuf = fz_compressed_image_buffer(mContext, image);
			//fz_snprintf(buf, sizeof(buf), "image-%04d", pdf_to_num(mContext, ref));
			type = cbuf == NULL ? FZ_IMAGE_UNKNOWN : cbuf->params.type;

			if (image->use_colorkey)
				type = FZ_IMAGE_UNKNOWN;
			if (image->use_decode)
				type = FZ_IMAGE_UNKNOWN;
			if (image->mask)
				type = FZ_IMAGE_UNKNOWN;

			enum fz_colorspace_type ctype = fz_colorspace_type(mContext, image->colorspace);
			if (ctype != FZ_COLORSPACE_RGB && ctype != FZ_COLORSPACE_GRAY)
				type = FZ_IMAGE_UNKNOWN;

			if (type == FZ_IMAGE_JPEG) {
				unsigned char* data;
				size_t len = fz_buffer_storage(mContext, cbuf->buffer, &data);
				file = QDir(outputDir).absoluteFilePath(QString("image-%1.jpg").arg(pdf_to_num(mContext, ref)));
				fz_output* out = fz_new_output_with_path(mContext, file.toStdString().c_str(), 0);
				fz_write_data(mContext, out, data, len);
				fz_close_output(mContext, out);
				fz_drop_output(mContext, out);
			} else {
				pix = fz_get_pixmap_from_image(mContext, image, NULL, NULL, 0, 0);
				if (image->mask) {
					mask = fz_get_pixmap_from_image(mContext, image->mask, NULL, NULL, 0, 0);
					if (mask->w == pix->w && mask->h == pix->h) {
						fz_pixmap* apix = fz_new_pixmap_from_color_and_mask(mContext, pix, mask);
						fz_drop_pixmap(mContext, pix);
						pix = apix;
					} else {
						fz_warn(mContext, "cannot combine image with smask if different resolution");
					}
				}
				fz_pixmap* rgb = NULL;
				if (pix->colorspace && pix->colorspace != fz_device_rgb(mContext)) {
					rgb = fz_convert_pixmap(mContext, pix, fz_device_rgb(mContext), NULL, NULL, fz_default_color_params /* FIXME */, 1);
					pix = rgb;
				}
				if (!pix->colorspace || pix->colorspace->type == FZ_COLORSPACE_GRAY || pix->colorspace->type == FZ_COLORSPACE_RGB) {
					file = QDir(outputDir).absoluteFilePath(QString("image-%1.png").arg(pdf_to_num(mContext, ref)));
					fz_save_pixmap_as_png(mContext, pix, file.toStdString().c_str());
				} else {
					file = QDir(outputDir).absoluteFilePath(QString("image-%1.pam").arg(pdf_to_num(mContext, ref)));
					fz_save_pixmap_as_pam(mContext, pix, file.toStdString().c_str());
				}
				fz_drop_pixmap(mContext, rgb);
			}
		}
		fz_always(mContext) {
			fz_drop_image(mContext, image);
			fz_drop_pixmap(mContext, mask);
			fz_drop_pixmap(mContext, pix);
		}
		fz_catch(mContext) {
			fz_rethrow(mContext);
		}
		return file;
	}
}