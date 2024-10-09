#include "FFXPdfPlugin.h"
#include "FFXApplication.h"
#include "FFXFileListView.h"
#include "FFXTaskPanel.h"
#include "FFXClipboardPanel.h"
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
#include <QTranslator>
#include <QCoreApplication>
#include <QDebug>

namespace FFX {
	PdfPlugin::PdfPlugin(QObject* parent)
		: QObject(parent) {	
		// add language
		mTranslator = new QTranslator;
		if (mTranslator->load(QLocale(), QLatin1String("ffx-pdf"), QLatin1String("_"), QLatin1String(":/ffx/res/i18n"))) {
			bool flag = QCoreApplication::installTranslator(mTranslator);
		}

		SetupUi();
	}

	PdfPlugin::~PdfPlugin()	{}

	void PdfPlugin::SetupUi() {
		mPdfMenu = new QMenu(QObject::tr("&PDF"));
		mImageToPdfAction = new QAction(QObject::tr("Images to PDF"));
		mImageToPdfActionOnClipboardPanel = new QAction(QObject::tr("Images to PDF"));
		mMergePdfAction = new QAction(QObject::tr("Merge"));
		mMergePdfActionOnClipboardPanel = new QAction(QObject::tr("Merge"));
		mAddTextWatermarkAction = new QAction(QObject::tr("Text Watermark"));
		mAddTextWatermarkActionOnClipboardPanel = new QAction(QObject::tr("Text Watermark"));
		mAddImageWatermarkAction = new QAction(QObject::tr("Image Watermark"));
		mAddImageWatermarkActionOnClipboardPanel = new QAction(QObject::tr("Image Watermark"));
		mExtractImageAction = new QAction(QObject::tr("Extract Images"));
		mExtractImageActionOnClipboardPanel = new QAction(QObject::tr("Extract Images"));
		mPdfToImageAction = new QAction(QObject::tr("Pdf to Images"));
		mPdfToImageActionOnClipboardPanel = new QAction(QObject::tr("Pdf to Images"));

		connect(mImageToPdfAction, &QAction::triggered, this, &PdfPlugin::OnImageToPdfAction);
		connect(mImageToPdfActionOnClipboardPanel, &QAction::triggered, this, []() {
			HandlerSettingDialog dialog(std::make_shared<ImageToPdfHandler>("", QSize(595, 842), QRect(), true, true, true), false);
			dialog.exec(); });

		connect(mMergePdfAction, &QAction::triggered, this, &PdfPlugin::OnMergePdfAction);
		connect(mMergePdfActionOnClipboardPanel, &QAction::triggered, this, []() {
			HandlerSettingDialog dialog(std::make_shared<MergePdfHandler>(""), false);
			dialog.exec(); });

		connect(mAddTextWatermarkAction, &QAction::triggered, this, &PdfPlugin::OnAddTextWatermarkAction);
		connect(mAddTextWatermarkActionOnClipboardPanel, &QAction::triggered, this, []() {
			HandlerSettingDialog dialog(std::make_shared<PdfAddTextWatermarkHandler>("水印测试"), false);
			dialog.exec(); });

		connect(mAddImageWatermarkAction, &QAction::triggered, this, &PdfPlugin::OnAddImageWatermarkAction);
		connect(mAddImageWatermarkActionOnClipboardPanel, &QAction::triggered, this, []() {
			HandlerSettingDialog dialog(std::make_shared<PdfAddImageWatermarkHandler>(""), false);
			dialog.exec(); });

		connect(mExtractImageAction, &QAction::triggered, this, &PdfPlugin::OnExtractImageAction);
		connect(mExtractImageActionOnClipboardPanel, &QAction::triggered, this, []() {
			HandlerSettingDialog dialog(std::make_shared<ExtractImageHandler>(""), false);
			dialog.exec(); });

		connect(mPdfToImageAction, &QAction::triggered, this, &PdfPlugin::OnPdfToImageAction);
		connect(mPdfToImageActionOnClipboardPanel, &QAction::triggered, this, &PdfPlugin::OnPdfToImageActionOnClipboardPanel);

		mPdfMenu->addAction(mImageToPdfAction);
		mPdfMenu->addAction(mMergePdfAction);
		QMenu* watermarkMenu = new QMenu(QObject::tr("Add Watermark"));
		watermarkMenu->addAction(mAddTextWatermarkAction);
		watermarkMenu->addAction(mAddImageWatermarkAction);
		mPdfMenu->addAction(watermarkMenu->menuAction());
		mPdfMenu->addAction(mExtractImageAction);
		mPdfMenu->addAction(mPdfToImageAction);

		mPdfMenuInClipboard = App()->ClipboardPanelPtr()->Header()->AddMenuAction("PDF");
		mPdfMenuInClipboard->addAction(mImageToPdfActionOnClipboardPanel);
		mPdfMenuInClipboard->addAction(mMergePdfActionOnClipboardPanel);
		QMenu* watermarkMenuOnClipboardPanel = new QMenu(QObject::tr("Add Watermark"));
		watermarkMenuOnClipboardPanel->addAction(mAddTextWatermarkActionOnClipboardPanel);
		watermarkMenuOnClipboardPanel->addAction(mAddImageWatermarkActionOnClipboardPanel);
		mPdfMenuInClipboard->addAction(watermarkMenuOnClipboardPanel->menuAction());
		mPdfMenuInClipboard->addAction(mExtractImageActionOnClipboardPanel);
		mPdfMenuInClipboard->addAction(mPdfToImageActionOnClipboardPanel);
	}

	void PdfPlugin::Install() {
		App()->AddMenu(mPdfMenu);
		App()->FileMainViewPtr()->AddContextMenu(mPdfMenu);
	}

	void PdfPlugin::Uninstall() {
		App()->RemoveMenu(mPdfMenu);
		App()->ClipboardPanelPtr()->Header()->RemoveMenu(mPdfMenuInClipboard);
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
		HandlerSettingDialog dialog(std::make_shared<PdfToImageHandler>("E:/新文件夹"));
		dialog.exec();
	}

	void PdfPlugin::OnPdfToImageActionOnClipboardPanel() {
		HandlerSettingDialog dialog(std::make_shared<PdfToImageHandler>("E:/新文件夹"), false);
		dialog.exec();
	}
}

