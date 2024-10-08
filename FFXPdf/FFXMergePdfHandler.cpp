#include "FFXMergePdfHandler.h"

namespace FFX {
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
			}
			else {
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
		mArgMap["OutPdf"] = Argument("OutPdf", QObject::tr("Output pdf"), QObject::tr("Output PDF file, absolute path."), outPdf, Argument::SaveFile, true);
		mArgMap["OutPdf"].AddLimit("Pdf file(*.pdf)");

		mArgMap["Compress"] = Argument("Compress", QObject::tr("Compress"), QObject::tr("Compress the output PDF file, default is false."), outPdf, Argument::Bool);
	}

	void MergePdfHandler::Cancel() {
		mCancelled = true;
	}

	std::shared_ptr<FileHandler> MergePdfHandler::Clone() {
		return FileHandlerPtr(new MergePdfHandler(*this));
	}

	QFileInfoList MergePdfHandler::DoHandle(const QFileInfoList& files, ProgressPtr progress) {
		if (files.isEmpty()) {
			progress->OnComplete(true, QObject::tr("Finished, nothing to do"));
			return QFileInfoList();
		}
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
		}

		fz_try(mContext) {
			pdf_save_document(mContext, doc_des, out.toStdString().c_str(), &opts);
			progress->OnFileComplete(files[0], out);
		}
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