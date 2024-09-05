#include "FFXZipPlugin.h"
#include "FFXApplication.h"
#include "FFXFileListView.h"
#include "FFXUnzipHandler.h"
#include "FFXTaskPanel.h"

#include <QMenu>
#include <QCoreApplication>

namespace FFX {
	ZipPlugin::ZipPlugin(QObject* parent) : QObject(parent) {
		mMenu = new QMenu(QObject::tr("&Zip"));
		mUnzipAction = new QAction(QObject::tr("Unzip files"));
		mMenu->addAction(mUnzipAction);

		connect(mUnzipAction, &QAction::triggered, this, &ZipPlugin::OnUnzipAction);
	}

	ZipPlugin::~ZipPlugin() {

	}

	void ZipPlugin::Install() {
		App()->AddMenu(mMenu);
		App()->HandlerFactoryPtr()->Append(std::make_shared<UnzipHandler>());
	}

	void ZipPlugin::Uninstall() {
		App()->RemoveMenu(mMenu);
	}

	void ZipPlugin::OnUnzipAction() {
		FileMainView* fmv = App()->FileMainViewPtr();
		QStringList files = fmv->SelectedFiles();
		App()->TaskPanelPtr()->Submit(FileInfoList(files), std::make_shared<UnzipHandler>());
	}
}