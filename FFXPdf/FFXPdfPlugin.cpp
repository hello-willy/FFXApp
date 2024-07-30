#include "FFXPdfPlugin.h"
#include "FFXApplication.h"
#include "FFXPdfHandler.h"
#include "FFXFileListView.h"
#include "FFXTaskPanel.h"

#include <QMenu>
#include <QAction>

namespace FFX {
	PdfPlugin::PdfPlugin(QObject* parent)
		: QObject(parent) {
		mPdfMenu = new QMenu(QObject::tr("&PDF"));
		mImageToPdfAction = new QAction(QObject::tr("Images to PDF"));
		connect(mImageToPdfAction, &QAction::triggered, this, &PdfPlugin::OnImageToPdfAction);
		mPdfMenu->addAction(mImageToPdfAction);
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
		App()->TaskPanelPtr()->Submit(FileInfoList(files), std::make_shared<ImageToPdfHandler>("D:/out.pdf"));
	}
}

