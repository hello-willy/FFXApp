#include "FFXPdfHandler.h"
#include "FFXFileFilterExpr.h"

namespace FFX {

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

	std::string ImageToPdfHandler::FilterExpression() {
		return "*.png | *.jpg | *.jpeg | *.gif | *.bmp";
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

	/************************************************************************************************************************
	 * Class： MergePdfHandler
	 *
	 *
	/************************************************************************************************************************/
	
	void page_merge(fz_context* ctx, pdf_document* doc_src, int page_from, int page_to, pdf_graft_map* graft_map) {
		pdf_graft_mapped_page(ctx, graft_map, page_to - 1, doc_src, page_from - 1);
	}

	typedef struct
	{
		fz_context* ctx;
		fz_outline_iterator* it_dst;
		fz_outline_iterator* it_src;
		const char* range;
		int page_count;
		int max;
		int len;
		fz_outline_item* items;
		int copied_to_depth;
		int page_output_base;
	} cor_state;

	int position_in_range(fz_context* ctx, const char* range, int count, int page) {
		int start, end;
		int n = 0;

		while ((range = fz_parse_page_range(ctx, range, &start, &end, count))) {
			if (start < end) {
				if (start <= page && page <= end)
					return n + page - start + 1;
				n += end - start + 1;
			} else {
				if (end <= page && page <= start)
					return n + page - end + 1;
				n += start - end + 1;
			}
		}

		return 0;
	}

	void copy_item(cor_state* cor) {
		fz_context* ctx = cor->ctx;

		while (cor->copied_to_depth < cor->len) {
			/* All items copied in a run get the same uri - that of the last one. */
			fz_outline_item item = cor->items[cor->copied_to_depth];
			item.uri = cor->items[cor->len - 1].uri;
			fz_outline_iterator_insert(ctx, cor->it_dst, &item);
			cor->copied_to_depth++;
			fz_outline_iterator_prev(ctx, cor->it_dst);
			fz_outline_iterator_down(ctx, cor->it_dst);
		}
	}

	char* rewrite_page(fz_context* ctx, const char* uri, int n) {
		const char* p;

		if (uri == NULL)
			return NULL;

		if (strncmp(uri, "#page=", 6) != 0)
			return fz_strdup(ctx, uri);
		p = strchr(uri + 6, '&');
		if (p == NULL)
			return fz_asprintf(ctx, "#page=%d", n);

		return fz_asprintf(ctx, "#page=%d%s", n, p);
	}

	void do_copy_outline_range(pdf_document* doc_src, cor_state* cor) {
		fz_context* ctx = cor->ctx;

		do {
			int has_children;
			float x, y;
			fz_outline_item* item = fz_outline_iterator_item(ctx, cor->it_src);
			int page_num = fz_page_number_from_location(ctx, (fz_document*)doc_src, fz_resolve_link(ctx, (fz_document*)doc_src, item->uri, &x, &y));
			int page_in_range = position_in_range(ctx, cor->range, cor->page_count, page_num + 1);
			int new_page_number = page_in_range + cor->page_output_base;

			if (cor->len == cor->max)
			{
				int newmax = cor->max ? cor->max * 2 : 8;
				cor->items = fz_realloc_array(ctx, cor->items, newmax, fz_outline_item);
				cor->max = newmax;
			}
			cor->len++;
			cor->items[cor->len - 1].title = NULL;
			cor->items[cor->len - 1].uri = NULL;
			cor->items[cor->len - 1].is_open = item->is_open;
			cor->items[cor->len - 1].title = item->title ? fz_strdup(ctx, item->title) : NULL;
			cor->items[cor->len - 1].uri = rewrite_page(ctx, item->uri, new_page_number);

			if (page_in_range != 0)
				copy_item(cor);

			has_children = fz_outline_iterator_down(ctx, cor->it_src);
			if (has_children == 0)
				do_copy_outline_range(doc_src, cor);
			if (has_children >= 0)
				fz_outline_iterator_up(ctx, cor->it_src);

			cor->len--;
			if (cor->copied_to_depth > cor->len)
			{
				cor->copied_to_depth = cor->len;
				fz_outline_iterator_up(ctx, cor->it_dst);
			}
			fz_outline_iterator_next(ctx, cor->it_dst);
			fz_free(ctx, cor->items[cor->len].title);
			fz_free(ctx, cor->items[cor->len].uri);
		} while (fz_outline_iterator_next(ctx, cor->it_src) == 0);
	}

	void copy_outline_range(fz_context* ctx, pdf_document* doc_src, fz_outline_iterator* it_dst, fz_outline_iterator* it_src, const char* range, int page_count, int page_output_base)
	{
		cor_state cor;

		cor.ctx = ctx;
		cor.it_dst = it_dst;
		cor.it_src = it_src;
		cor.max = 0;
		cor.len = 0;
		cor.copied_to_depth = 0;
		cor.range = range;
		cor.items = NULL;
		cor.page_count = page_count;
		cor.page_output_base = page_output_base;

		fz_try(ctx)
			do_copy_outline_range(doc_src, &cor);
		fz_always(ctx) {
			int i;

			for (i = 0; i < cor.len; i++) {
				fz_free(ctx, cor.items[i].title);
				fz_free(ctx, cor.items[i].uri);
			}
			fz_free(ctx, cor.items);
		}
		fz_catch(ctx)
			fz_rethrow(ctx);
	}

	MergePdfHandler::MergePdfHandler(const QString& outPdf, bool compress) {
		mArgMap["OutPdf"] = Argument("OutPdf", QObject::tr("OutPdf"), QObject::tr("Output PDF file, absolute path."), outPdf);
		mArgMap["Compress"] = Argument("Compress", QObject::tr("Compress"), QObject::tr("Compress the output PDF file, default is false."), outPdf);
	}

	void MergePdfHandler::Cancel() {
		mCancelled = true;
	}

	std::shared_ptr<FileHandler> MergePdfHandler::Clone() {
		return FileHandlerPtr(new MergePdfHandler(*this));
	}

	std::string MergePdfHandler::FilterExpression() {
		return "*.pdf";
	}

	QFileInfoList MergePdfHandler::DoHandle(const QFileInfoList& files, ProgressPtr progress) {
		QString	out = mArgMap["OutPdf"].Value().toString();
		bool compress = mArgMap["Compress"].Value().toBool();

		pdf_write_options opts = pdf_default_write_options;
		if (compress) {
			opts.do_compress = opts.do_compress_images = opts.do_compress_fonts = 1;
		}
		pdf_document* doc_des = NULL;

		fz_try(mContext) {
			doc_des = pdf_create_document(mContext);
		}
		fz_catch(mContext) {
			progress->OnComplete(false, QObject::tr("Failed: Cannot create destination document"));
			return QFileInfoList();
		}

		int size = files.size();
		for (int i = 0; i < size && !mCancelled; i++) {
			QFileInfo file = files[i];
			QString filePath = file.absoluteFilePath();
			double p = (i / (double)size) * 100;
			progress->OnProgress(p, QObject::tr("Merging file: %1").arg(filePath));

			pdf_document* doc_src = NULL;
			fz_try(mContext) {
				doc_src = pdf_open_document(mContext, filePath.toStdString().c_str());
				Merge(doc_src, doc_des);
			}
			fz_always(mContext)
				pdf_drop_document(mContext, doc_src);
			fz_catch(mContext) {
				progress->OnFileComplete(file, file, false, QObject::tr("Merge %1 failed.").arg(filePath));
			}
			progress->OnFileComplete(file, file);
		}

		fz_try(mContext)
			pdf_save_document(mContext, doc_des, out.toStdString().c_str(), &opts);
		fz_catch(mContext) {
			progress->OnComplete(false, QObject::tr("Failed: Cannot save output file."));
			return QFileInfoList();
		}
		progress->OnComplete(true, QObject::tr("Finish, The PDF file is stored in: %1").arg(out));
		return FileInfoList(out);
	}

	void MergePdfHandler::Merge(pdf_document* doc_src, pdf_document* doc_des) {
		int start, end, i, count;
		pdf_graft_map* graft_map;
		const char* r;
		fz_outline_iterator* it_src = NULL;
		fz_outline_iterator* it_dst = NULL;
		int pages_merged = 0;
		
		count = pdf_count_pages(mContext, doc_src);
		graft_map = pdf_new_graft_map(mContext, doc_des);

		fz_var(it_src);
		fz_var(it_dst);
		const char* range = "1-N";
		fz_try(mContext) {
			r = range;
			while ((r = fz_parse_page_range(mContext, r, &start, &end, count))) {
				if (start < end)
					for (i = start; i <= end; ++i) {
						page_merge(mContext, doc_src, i, 0, graft_map);
						pages_merged++;
					}
				else
					for (i = start; i >= end; --i) {
						page_merge(mContext, doc_src, i, 0, graft_map);
						pages_merged++;
					}
			}

			it_src = fz_new_outline_iterator(mContext, (fz_document*)doc_src);
			if (it_src == NULL)
				break;
			it_dst = fz_new_outline_iterator(mContext, (fz_document*)doc_des);
			if (it_dst == NULL)
				break;

			if (fz_outline_iterator_item(mContext, it_dst) != NULL)
			{
				while (fz_outline_iterator_next(mContext, it_dst) == 0);
			}

			if (fz_outline_iterator_item(mContext, it_src) != NULL)
				copy_outline_range(mContext, doc_src, it_dst, it_src, range, count, mTotalMergedPageCount);

			mTotalMergedPageCount += pages_merged;
		}
		fz_always(mContext) {
			fz_drop_outline_iterator(mContext, it_src);
			fz_drop_outline_iterator(mContext, it_dst);
			pdf_drop_graft_map(mContext, graft_map);
		}
		fz_catch(mContext) {
			fz_rethrow(mContext);
		}
	}

}

