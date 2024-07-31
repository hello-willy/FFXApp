#include "FFXPdfHandler.h"
#include "FFXFileFilterExpr.h"

namespace FFX {
	std::string FilterExp = "*.png | *.jpg | *.jpeg | *.gif | *.bmp";

	/************************************************************************************************************************
	 * Class： PdfHandler
	 *
	 *
	/************************************************************************************************************************/
	PdfHandler::PdfHandler() {
	}

	PdfHandler::~PdfHandler() {
		if (mContext != nullptr) {
			fz_drop_context(mContext);
		}
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

		FileFilterExpr expr(FilterExp, false);
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

	/************************************************************************************************************************
	 * Class： ImageToPdfHander
	 *
	 *
	/************************************************************************************************************************/
	ImageToPdfHandler::ImageToPdfHandler(const QString& outPdf, const QSize& size, const QRect& boundary, bool portrait, bool autoRotate, bool stretch) {
		mArgMap["OutPdf"] = Argument("OutPdf", QObject::tr("OutPdf"), QObject::tr("Output PDF file, absolute path."), outPdf);
		mArgMap["PageSize"] = Argument("PageSize", QObject::tr("PageSize"), QObject::tr("Size of output PDF file, default is A4(595, 842)."), size);
		mArgMap["Portrait"] = Argument("Portrait", QObject::tr("Portrait"), QObject::tr("Page orientaion, default is portrait."), portrait);
		mArgMap["AutoRotate"] = Argument("AutoRotate", QObject::tr("AutoRotate"), QObject::tr("Automatically determine whether the image has been rotated."), autoRotate);
		mArgMap["Boundary"] = Argument("Boundary", QObject::tr("Boundary"), QObject::tr("The maximum boundary occupied by the image."), boundary);
		mArgMap["Stretch"] = Argument("Stretch", QObject::tr("Stretch"), QObject::tr("If the image is smaller than the page, stretch it to cover the entire page. default is false"), stretch);
	}

	std::shared_ptr<FileHandler> ImageToPdfHandler::Clone() {
		return FileHandlerPtr(new ImageToPdfHandler(*this));
	}

	void ImageToPdfHandler::Cancel() {
		mCancelled = true;
	}

	QFileInfoList ImageToPdfHandler::DoHandle(const QFileInfoList& files, ProgressPtr progress) {
		pdf_document* doc = NULL;
		fz_rect	mediabox = { 0, 0, 595, 842 }; // A4
		int	rotate = 0;
		pdf_write_options opts = pdf_default_write_options;
		QString	out = mArgMap["OutPdf"].Value().toString();
		bool portrait = mArgMap["Portrait"].Value().toBool();

		fz_try(mContext) {
			doc = pdf_create_document(mContext);
			int size = files.size();
			char name[16];
			for (int i = 0; i < size && !mCancelled; i++) {
				QFileInfo file = files[i];
				double p = (i / (double)size) * 100;
				progress->OnProgress(p, QObject::tr("Writing image: %1").arg(file.absoluteFilePath()));
				sprintf(name, "I%d", i + 1);
				CreateImagePage(doc, file.absoluteFilePath().toStdString().c_str(), name, mediabox, portrait);
			}
			pdf_save_document(mContext, doc, out.toStdString().c_str(), &opts);
		}
		fz_always(mContext) {
			pdf_drop_document(mContext, doc);
		}
		fz_catch(mContext) {
			fz_report_error(mContext);
			progress->OnComplete(false, QObject::tr("Failed: %1").arg(fz_caught_message(mContext)));
			return QFileInfoList();
		}

		progress->OnComplete(true, QObject::tr("Finish, The PDF file is stored in: %1").arg(out));
		return FileInfoList(out);
	}

	bool ImageToPdfHandler::CreateImagePage(pdf_document* doc, const char* image, const char* imageName, const fz_rect& pageSize, bool portrait) {
		pdf_obj* resources = NULL;
		fz_buffer* contents = NULL;
		pdf_obj* page = NULL;
		fz_try(mContext) {
			contents = fz_new_buffer(mContext, 1024);
			resources = pdf_new_dict(mContext, doc, 2);
			contents = fz_new_buffer(mContext, 1024);

			fz_rect box = AddImage(doc, resources, imageName, image);

			fz_matrix m = CalcImageMatrix(box.x1 - box.x0, box.y1 - box.y0);

			char buf[128];
			sprintf(buf, "\nq\n%f %f %f %f %f %f cm\n/%s Do\nQ", m.a, m.b, m.c, m.d, m.e, m.f, imageName);
			fz_append_string(mContext, contents, buf);
			page = pdf_add_page(mContext, doc, pageSize, portrait ? 0 : 90, resources, contents);
			pdf_insert_page(mContext, doc, -1, page);
		}
		fz_always(mContext) {
			pdf_drop_obj(mContext, page);
			fz_drop_buffer(mContext, contents);
			pdf_drop_obj(mContext, resources);
		}
		fz_catch(mContext) {
			fz_report_error(mContext);
			return false;
		}
		return true;
	}

	fz_matrix ImageToPdfHandler::CalcImageMatrix(int width, int height) const {
		QSize pageSize = mArgMap["PageSize"].Value().toSize();
		bool portrait = mArgMap["Portrait"].Value().toBool();
		bool autoRotate = mArgMap["AutoRotate"].Value().toBool();
		QRect boundary = mArgMap["Boundary"].Value().toRect();
		bool stretch = mArgMap["Stretch"].Value().toBool();

		int marginLeft = 0;
		int marginRight = 0;
		int marginTop = 0;
		int marginBottom = 0;
		if (boundary.isValid()) {
			marginLeft = boundary.left();
			marginTop = boundary.top();
			marginRight = boundary.right() > pageSize.width() ? 0 : pageSize.width() - boundary.left() - boundary.width();
			marginBottom = boundary.bottom() > pageSize.height() ? 0 : pageSize.height() - boundary.top() - boundary.height();
		}

		int iw = width;
		int ih = height;
		int cw = pageSize.width() - marginLeft - marginRight;
		int ch = pageSize.height() - marginTop - marginBottom;
		double ir = width / (double)height;
		double pr = pageSize.width() / (double)pageSize.height();

		bool rotateImage = (ir > 1 && pr < 1) || (ir < 1 && pr > 1);
		
		int pmax = (std::max)(cw, ch);
		int pmin = (std::min)(cw, ch);
		int imax = (std::max)(width, height);
		int imin = (std::min)(width, height);
		if (autoRotate && rotateImage) {
			double nr = height / (double)width;
			if (nr < pr) {
				iw = stretch ? pmax : (std::min)(pmax, imax);
			} else {
				iw = stretch ? pmin : (std::min)(pmin, imin);
			}
			ih = iw / ir;
		} else {
			if (ir < pr) {
				ih = stretch ? ch : (std::min)(ih, ch);
				iw = ih * ir;
			} else {
				iw = stretch ? cw : (std::min)(iw, cw);
				ih = iw / ir;
			}
		}

		fz_matrix m = fz_make_matrix(iw, 0, 0, ih, 0, 0);
		if (rotateImage && autoRotate) {
			m = fz_concat(m, fz_rotate(90));
			int xw = portrait ? (std::min)(iw, ih) : (std::max)(iw, ih);
			int xh = portrait ? iw : ih;
			int tranx = (pageSize.width() - xw) / 2 + xw;
			int trany = (pageSize.height() - xh) / 2;
			m = fz_concat(m, fz_translate(tranx, trany));
		}
		else {
			int tranx = (pageSize.width() - iw) / 2;
			int trany = (pageSize.height() - ih) / 2;
			m = fz_concat(m, fz_translate(tranx, trany));
		}
		return m;
	}
}

