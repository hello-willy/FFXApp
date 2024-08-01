#pragma once
#include "FFXPlugin.h"

class QMenu;
class QAction;
namespace FFX {
	class PdfPlugin : public QObject, public Plugin	{
		Q_OBJECT
		Q_PLUGIN_METADATA(IID FFX_Plugin_IID)
		Q_INTERFACES(FFX::Plugin)
	public:
		PdfPlugin(QObject* parent = nullptr);
		~PdfPlugin();

	public:
		virtual void Install() override;
		virtual void Uninstall() override;
		virtual QString Id() override { return "pdf.finalfilex.com"; };
		virtual QString Name() override { return QObject::tr("Pdf tools"); }

	private slots:
		void OnImageToPdfAction();
		void OnMergePdfAction();

	private:
		QMenu* mPdfMenu;
		QAction* mImageToPdfAction;
		QAction* mMergePdfAction;
	};

}
