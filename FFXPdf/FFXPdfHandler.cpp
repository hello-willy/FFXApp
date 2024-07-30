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
		if(mContext != nullptr) {
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
	ImageToPdfHandler::ImageToPdfHandler(const QString& outPdf, const QSize& size, bool portrait, bool autoRotate, const QRect& margins) {
		mArgMap["OutPdf"] = Argument("OutPdf", QObject::tr("OutPdf"), QObject::tr("Output PDF file, absolute path."), outPdf);
		mArgMap["PageSize"] = Argument("PageSize", QObject::tr("PageSize"), QObject::tr("Size of output PDF file, default is A4(595, 842)."), size);
		mArgMap["Portrait"] = Argument("Portrait", QObject::tr("Portrait"), QObject::tr("Page orientaion, default is portrait."), portrait);
		mArgMap["AutoRotate"] = Argument("AutoRotate", QObject::tr("AutoRotate"), QObject::tr("Automatically determine whether the image has been rotated."), autoRotate);
		mArgMap["Margins"] = Argument("Margins", QObject::tr("Margins"), QObject::tr("Page margin."), margins);
	}

	std::shared_ptr<FileHandler> ImageToPdfHandler::Clone() {
		return FileHandlerPtr(new ImageToPdfHandler(*this));
	}

	void ImageToPdfHandler::Cancel() {
		mCancelled = true;
	}

	QFileInfoList ImageToPdfHandler::DoHandle(const QFileInfoList& files, ProgressPtr progress) {
		pdf_document*	doc = NULL;
		pdf_obj*		resources = NULL;
		fz_buffer*		contents = NULL;
		pdf_obj*		page = NULL;
		fz_rect			mediabox = { 0, 0, 595, 842 }; // A4
		int				rotate = 0;
		pdf_write_options opts = pdf_default_write_options;
		QString			out = mArgMap["OutPdf"].Value().toString();

		fz_try(mContext) {
			doc = pdf_create_document(mContext);
			resources = pdf_new_dict(mContext, doc, 2);
			contents = fz_new_buffer(mContext, 1024);

			int size = files.size();
			char name[16];
			for (int i = 0; i < size && !mCancelled; i++) {
				QFileInfo file = files[i];
				sprintf(name, "I%d", i + 1);
				fz_rect box = AddImage(doc, resources, name, file.absoluteFilePath().toStdString().c_str());
				int image_width = box.x1 - box.x0;
				int image_height = box.y1 - box.y0;
				
				fz_matrix m = CalcImageMatrix(image_width, image_height);

				char buf[128];
				sprintf(buf, "\nq\n%f %f %f %f %f %f cm\n/%s Do\nQ", m.a, m.b, m.c, m.d, m.e, m.f, name);
				fz_append_string(mContext, contents, buf);
				page = pdf_add_page(mContext, doc, mediabox, rotate, resources, contents);
				pdf_insert_page(mContext, doc, -1, page);
			}
			pdf_save_document(mContext, doc, out.toStdString().c_str(), &opts);
		}
		fz_always(mContext) {
			pdf_drop_obj(mContext, page);
			fz_drop_buffer(mContext, contents);
			pdf_drop_obj(mContext, resources);
			pdf_drop_document(mContext, doc);
		}
		fz_catch(mContext) {
			fz_report_error(mContext);
		}
		return QFileInfoList();
	}

	fz_matrix ImageToPdfHandler::CalcImageMatrix(int width, int height) const {
		QSize pageSize	= mArgMap["PageSize"].Value().toSize();
		int rotate		= mArgMap["Portrait"].Value().toBool() ? 0 : 90;
		bool autoRotate = mArgMap["AutoRotate"].Value().toBool();
		QRect margins	= mArgMap["Margins"].Value().toRect();

		int iw = width;
		int ih = height;
		double r = width / (double)height;
		if (width > height) {
			iw = (std::min)(iw, pageSize.width());
			ih = iw / r;
		} else {
			ih = (std::min)(ih, pageSize.height());
			iw = ih * r;
		}

		fz_matrix m = fz_make_matrix(iw, 0, 0, ih, 0, 0);
		m = fz_concat(m, fz_rotate((autoRotate && width > height) ? 90 : 0));

		int tranx = (pageSize.width() - iw) / 2;
		int trany = (pageSize.height() - ih) / 2;
		m = fz_concat(m, fz_translate(tranx, trany));

		return m;
	}
}

