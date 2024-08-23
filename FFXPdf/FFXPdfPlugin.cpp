#include "FFXPdfPlugin.h"
#include "FFXApplication.h"
#include "FFXFileListView.h"
#include "FFXTaskPanel.h"
#include "FFXImageToPdfHandler.h"
#include "FFXMergePdfHandler.h"
#include "FFXAddWatermarkToPdfHandler.h"
#include "FFXExtractImageHandler.h"
#include "FFXPdfToImageHandler.h"

#include <QMenu>
#include <QAction>
#include <QVector4D>

namespace FFX {
	PdfPlugin::PdfPlugin(QObject* parent)
		: QObject(parent) {
		mPdfMenu = new QMenu(QObject::tr("&PDF"));
		mImageToPdfAction = new QAction(QObject::tr("Images to PDF"));
		mMergePdfAction = new QAction(QObject::tr("Merge"));
		mAddWatermarkAction = new QAction(QObject::tr("Add Watermark"));
		mExtractImageAction = new QAction(QObject::tr("Extract Images"));
		mPdfToImageAction = new QAction(QObject::tr("Pdf to Images"));
		connect(mImageToPdfAction, &QAction::triggered, this, &PdfPlugin::OnImageToPdfAction);
		connect(mMergePdfAction, &QAction::triggered, this, &PdfPlugin::OnMergePdfAction);
		connect(mAddWatermarkAction, &QAction::triggered, this, &PdfPlugin::OnAddWatermarkAction);
		connect(mExtractImageAction, &QAction::triggered, this, &PdfPlugin::OnExtractImageAction);
		connect(mPdfToImageAction, &QAction::triggered, this, &PdfPlugin::OnPdfToImageAction);

		mPdfMenu->addAction(mImageToPdfAction);
		mPdfMenu->addAction(mMergePdfAction);
		mPdfMenu->addAction(mAddWatermarkAction);
		mPdfMenu->addAction(mExtractImageAction);
		mPdfMenu->addAction(mPdfToImageAction);
	}

	PdfPlugin::~PdfPlugin()	{}

	void PdfPlugin::Install() {
		App()->AddMenu(mPdfMenu);
	}

	void PdfPlugin::Uninstall() {
		App()->RemoveMenu(mPdfMenu);
	}

	void PdfPlugin::OnImageToPdfAction() {
		FileMainView* fmv = App()->FileMainViewPtr();
		QStringList files = fmv->SelectedFiles();
		App()->TaskPanelPtr()->Submit(FileInfoList(files), std::make_shared<ImageToPdfHandler>("D:/out.pdf", QSize(595, 842), 
			QRect(), true, true, true));
	}

	void PdfPlugin::OnMergePdfAction() {
		FileMainView* fmv = App()->FileMainViewPtr();
		QStringList files = fmv->SelectedFiles();
		App()->TaskPanelPtr()->Submit(FileInfoList(files), std::make_shared<MergePdfHandler>("D:/m3.pdf"));
	}

	void PdfPlugin::OnAddWatermarkAction() {
		FileMainView* fmv = App()->FileMainViewPtr();
		QStringList files = fmv->SelectedFiles();
		//App()->TaskPanelPtr()->Submit(FileInfoList(files), std::make_shared<AddWatermarkToPdfHandler>(QFileInfo("C:\\Users\\Coppe\\Pictures\\1.jpg"), 
		//	QSize(150, 100), true, 0, 8));

		App()->TaskPanelPtr()->Submit(FileInfoList(files), std::make_shared<AddWatermarkToPdfHandler>("Watermark水印测试Test",
			"", 24, 0, 8, 0.5));
	}

	void PdfPlugin::OnExtractImageAction() {
		FileMainView* fmv = App()->FileMainViewPtr();
		QStringList files = fmv->SelectedFiles();
		App()->TaskPanelPtr()->Submit(FileInfoList(files), std::make_shared<ExtractImageHandler>("E:/新文件夹"));
	}

	void PdfPlugin::OnPdfToImageAction() {
		FileMainView* fmv = App()->FileMainViewPtr();
		QStringList files = fmv->SelectedFiles();
		App()->TaskPanelPtr()->Submit(FileInfoList(files), std::make_shared<PdfToImageHandler>("E:/新文件夹", QString()));
	}
}

