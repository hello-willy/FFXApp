#pragma once
#include "FFXPlugin.h"

class QMenu;
class QAction;

namespace FFX {
	class ZipPlugin : public QObject, public Plugin {
		Q_OBJECT
			Q_PLUGIN_METADATA(IID FFX_Plugin_IID)
			Q_INTERFACES(FFX::Plugin)
	public:
		ZipPlugin(QObject* parent = nullptr);
		~ZipPlugin();

	public:
		virtual void Install() override;
		virtual void Uninstall() override;
		virtual QString Id() override { return "zip.finalfilex.com"; };
		virtual QString Name() override { return QObject::tr("Zip tools"); }

	private slots:
		void OnUnzipAction();

	private:
		QMenu* mMenu;
		QAction* mUnzipAction;
	};

}


