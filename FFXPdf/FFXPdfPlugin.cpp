#include "FFXPdfPlugin.h"
#include "FFXApplication.h"
#include "FFXFileListView.h"
#include "FFXTaskPanel.h"
#include "FFXImageToPdfHandler.h"
#include "FFXMergePdfHandler.h"
#include "FFXPdfAddTextWatermarkHandler.h"
#include "FFXPdfAddImageWatermarkHandler.h"
#include "FFXExtractImageHandler.h"
#include "FFXPdfToImageHandler.h"
#include "FFXHandlerSettingDialog.h"
#include "FFXAddWatermarkToPdfHandler.h"

#include <QMenu>
#include <QAction>
#include <QVector4D>

namespace FFX {
	PdfPlugin::PdfPlugin(QObject* parent)
		: QObject(parent) {
		mPdfMenu = new QMenu(QObject::tr("&PDF"));
		mImageToPdfAction = new QAction(QObject::tr("Images to PDF"));
		mMergePdfAction = new QAction(QObject::tr("Merge"));
		mAddTextWatermarkAction = new QAction(QObject::tr("Text Watermark"));
		mAddImageWatermarkAction = new QAction(QObject::tr("Image Watermark"));
		mExtractImageAction = new QAction(QObject::tr("Extract Images"));
		mPdfToImageAction = new QAction(QObject::tr("Pdf to Images"));
		connect(mImageToPdfAction, &QAction::triggered, this, &PdfPlugin::OnImageToPdfAction);
		connect(mMergePdfAction, &QAction::triggered, this, &PdfPlugin::OnMergePdfAction);
		connect(mAddTextWatermarkAction, &QAction::triggered, this, &PdfPlugin::OnAddTextWatermarkAction);
		connect(mAddImageWatermarkAction, &QAction::triggered, this, &PdfPlugin::OnAddImageWatermarkAction);
		connect(mExtractImageAction, &QAction::triggered, this, &PdfPlugin::OnExtractImageAction);
		connect(mPdfToImageAction, &QAction::triggered, this, &PdfPlugin::OnPdfToImageAction);

		mPdfMenu->addAction(mImageToPdfAction);
		mPdfMenu->addAction(mMergePdfAction);
		QMenu* watermarkMenu = new QMenu(QObject::tr("Add Watermark"));
		watermarkMenu->addAction(mAddTextWatermarkAction);
		watermarkMenu->addAction(mAddImageWatermarkAction);
		mPdfMenu->addAction(watermarkMenu->menuAction());
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
		HandlerSettingDialog dialog(std::make_shared<ImageToPdfHandler>("D:/out.pdf", QSize(595, 842), QRect(), true, true, true));
		dialog.exec();
	}

	void PdfPlugin::OnMergePdfAction() {
		FileMainView* fmv = App()->FileMainViewPtr();
		QStringList files = fmv->SelectedFiles();
		HandlerSettingDialog dialog(std::make_shared<MergePdfHandler>("D:/m3.pdf"));
		dialog.exec();
	}

	void PdfPlugin::OnAddTextWatermarkAction() {
		FileMainView* fmv = App()->FileMainViewPtr();
		QStringList files = fmv->SelectedFiles();
		HandlerSettingDialog dialog(std::make_shared<PdfAddTextWatermarkHandler>("水印测试水印测试水印测试"));
		dialog.exec();
	}

	void PdfPlugin::OnAddImageWatermarkAction() {
		FileMainView* fmv = App()->FileMainViewPtr();
		QStringList files = fmv->SelectedFiles();
		HandlerSettingDialog dialog(std::make_shared<PdfAddImageWatermarkHandler>("水印测试水印测试水印测试"));
		dialog.exec();
	}

	void PdfPlugin::OnExtractImageAction() {
		FileMainView* fmv = App()->FileMainViewPtr();
		QStringList files = fmv->SelectedFiles();
		HandlerSettingDialog dialog(std::make_shared<ExtractImageHandler>("E:/新文件夹"));
		dialog.exec();
	}

	void PdfPlugin::OnPdfToImageAction() {
		FileMainView* fmv = App()->FileMainViewPtr();
		QStringList files = fmv->SelectedFiles();
		HandlerSettingDialog dialog(std::make_shared<PdfToImageHandler>("E:/新文件夹"));
		dialog.exec();
		//App()->TaskPanelPtr()->Submit(FileInfoList(files), std::make_shared<PdfToImageHandler>("E:/新文件夹", QString()));
	}

}

