#pragma once
#include "FFXPlugin.h"

class QMenu;
class QAction;
class QTranslator;

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

	private:
		void SetupUi();

	private slots:
		void OnImageToPdfAction();
		void OnMergePdfAction();
		void OnAddTextWatermarkAction();
		void OnAddImageWatermarkAction();
		void OnExtractImageAction();
		void OnPdfToImageAction();
		void OnPdfToImageActionOnClipboardPanel();

	private:
		QTranslator* mTranslator;
		QMenu* mPdfMenu;
		QMenu* mPdfMenuInClipboard;
		QAction* mImageToPdfAction;
		QAction* mImageToPdfActionOnClipboardPanel;
		QAction* mMergePdfAction;
		QAction* mMergePdfActionOnClipboardPanel;
		QAction* mAddTextWatermarkAction;
		QAction* mAddTextWatermarkActionOnClipboardPanel;
		QAction* mAddImageWatermarkAction;
		QAction* mAddImageWatermarkActionOnClipboardPanel;
		QAction* mExtractImageAction;
		QAction* mExtractImageActionOnClipboardPanel;
		QAction* mPdfToImageAction;
		QAction* mPdfToImageActionOnClipboardPanel;
	};

}
