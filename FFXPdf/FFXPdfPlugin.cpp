#include "FFXPdfPlugin.h"
#include "FFXApplication.h"
#include "FFXPdfHandler.h"
#include "FFXFileListView.h"
#include "FFXTaskPanel.h"

#include <QMenu>
#include <QAction>
#include <QVector4D>

namespace FFX {
	PdfPlugin::PdfPlugin(QObject* parent)
		: QObject(parent) {
		mPdfMenu = new QMenu(QObject::tr("&PDF"));
		mImageToPdfAction = new QAction(QObject::tr("Images to PDF"));
		mMergePdfAction = new QAction(QObject::tr("Merge"));
		connect(mImageToPdfAction, &QAction::triggered, this, &PdfPlugin::OnImageToPdfAction);
		connect(mMergePdfAction, &QAction::triggered, this, &PdfPlugin::OnMergePdfAction);
		mPdfMenu->addAction(mImageToPdfAction);
		mPdfMenu->addAction(mMergePdfAction);
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
			QRect(50, 50, 495, 742), true, false, true));
	}

	void PdfPlugin::OnMergePdfAction() {
		FileMainView* fmv = App()->FileMainViewPtr();
		QStringList files = fmv->SelectedFiles();
		App()->TaskPanelPtr()->Submit(FileInfoList(files), std::make_shared<MergePdfHandler>("D:/m3.pdf"));
	}
}

