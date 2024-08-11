#include "FFXPdfToImageHandler.h"

namespace FFX {

	PdfToImageHandler::PdfToImageHandler(const QString& outputDir, const QString& pages, int dpi) {
		mArgMap["OutputDir"] = Argument("OutputDir", QObject::tr("OutputDir"), QObject::tr("Storage directory for images."), outputDir);
		mArgMap["PageRange"] = Argument("PageRange", QObject::tr("PageRange"), QObject::tr("Comma separated list of page ranges, 1,2,4 or 3-7,3,7-10, default is 1-N"), pages);
		mArgMap["DPI"] = Argument("DPI", QObject::tr("DPI"), QObject::tr("DPI of output image, default 72"), dpi);
	}

	std::shared_ptr<FileHandler> PdfToImageHandler::Clone() {
		return FileHandlerPtr(new PdfToImageHandler(*this));
	}

	void PdfToImageHandler::Cancel() {
		mCancelled = true;
	}

	QFileInfoList PdfToImageHandler::DoHandle(const QFileInfoList& files, ProgressPtr progress) {
		fz_try(mContext)
			fz_register_document_handlers(mContext);
		fz_catch(mContext) {
			fprintf(stderr, "cannot register document handlers\n");
			return QFileInfoList();
		}
		
		QString pageRange = mArgMap["PageRange"].Value().toString();

		float layout_w = FZ_DEFAULT_LAYOUT_W;
		float layout_h = FZ_DEFAULT_LAYOUT_H;
		float layout_em = FZ_DEFAULT_LAYOUT_EM;

		int alphabits = 8;
		fz_set_aa_level(mContext, alphabits);
		fz_set_use_document_css(mContext, 1);

		int size = files.size();
		if (size > 1 || pageRange.isEmpty()) {
			pageRange = QString("1-N");
		}

		QFileInfoList result;
		for (int i = 0; i < size && !mCancelled; i++) {
			QFileInfo file = files[i];
			QString filePath = file.absoluteFilePath();
			QString out = MakeOutput(file);

			fz_document* doc = NULL;
			fz_document_writer* writer = NULL;
			
			fz_try(mContext) {
				doc = fz_open_accelerated_document(mContext, filePath.toStdString().c_str(), NULL);
				writer = fz_new_document_writer(mContext, out.toStdString().c_str(), "png", "");
				fz_layout_document(mContext, doc, layout_w, layout_h, layout_em);
				int count = fz_count_pages(mContext, doc);

				QList<int> pages = FetchPageNumbers(mContext, pageRange.toStdString().c_str(), count);
				if (pages.isEmpty())
					continue;

				int pageCount = pages.size();
				for (int p = 0; p < pageCount; p++) {
					RunPage(doc, writer, pages[p]);
					double prog = ((p + 1.) / (double)pageCount) * 100;
					progress->OnProgress(prog, QObject::tr("Converting file: %1").arg(file.absoluteFilePath()));
				}
			}
			fz_always(mContext) {
				fz_drop_document(mContext, doc);
				fz_drop_document_writer(mContext, writer);
			}
			fz_catch(mContext) {
				fz_drop_document(mContext, doc);
				fz_report_error(mContext);
			}
		}
		progress->OnComplete(true, QObject::tr("Finish."));
		return QFileInfoList();
	}

	QString PdfToImageHandler::MakeOutput(const QFileInfo& file) {
		QString outputDir = mArgMap["OutputDir"].Value().toString();
		QString fileBaseName = file.completeBaseName();
		QDir root;
		if (outputDir.isEmpty()) {
			root = file.absoluteDir();
		} else {
			root = QDir(outputDir);
			if(!root.exists())
				root.mkpath(outputDir);
		}
		
		// if the file name exists but the file is not a directory, try to rename.
		QString dirname = fileBaseName;
		QFileInfo imageOutputDirInfo(root.absoluteFilePath(dirname));

		int counter = 1;
		while (imageOutputDirInfo.exists() && !imageOutputDirInfo.isDir()) {
			dirname = QString("%1_%2").arg(fileBaseName).arg(counter++);
			imageOutputDirInfo.setFile(dirname);
		}
		root.mkdir(dirname);

		QDir imageOutputDir(imageOutputDirInfo.absoluteFilePath());
		return imageOutputDir.absoluteFilePath("Page%04d.png");
	}

	QList<int> PdfToImageHandler::FetchPageNumbers(fz_context* ctx, const char* range, int count) {
		int start, end, i;
		QList<int> pages;
		while ((range = fz_parse_page_range(ctx, range, &start, &end, count))) {
			if (start < end)
				for (i = start; i <= end; ++i)
					pages << i;
			else
				for (i = start; i >= end; --i)
					pages << i;
		}
		return pages;
	}

	void PdfToImageHandler::RunPage(fz_document* doc, fz_document_writer* writer, int pageNum) {
		int dpi = mArgMap["DPI"].Value().toInt();
		if (dpi <= 0)
			dpi = 72;

		fz_rect box;
		fz_page* page;
		fz_device* dev = NULL;
		fz_matrix ctm;

		page = fz_load_page(mContext, doc, pageNum - 1);

		fz_var(dev);

		fz_try(mContext) {
			box = fz_bound_page_box(mContext, page, FZ_CROP_BOX);
			float zoom = (float)dpi / 72;
			ctm = fz_pre_scale(fz_rotate(0), zoom, zoom);
			// Realign page box on 0,0
			//ctm = fz_translate(-box.x0, -box.y0);
			box = fz_transform_rect(box, ctm);

			dev = fz_begin_page(mContext, writer, box);
			fz_run_page(mContext, page, dev, ctm, NULL);
			fz_end_page(mContext, writer);
		}
		fz_always(mContext) {
			fz_drop_page(mContext, page);
		}
		fz_catch(mContext)
			fz_rethrow(mContext);
	}
}