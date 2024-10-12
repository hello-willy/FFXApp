#pragma once

#include <QDialog>

class QLabel;
class QGridLayout;

namespace FFX {
	class AboutDialog : public QDialog {
		Q_OBJECT

	public:
		AboutDialog(QWidget* parent = nullptr);
		~AboutDialog();

	private:
		void SetupUi();

	private:
		QGridLayout* mMainLayout;
		QLabel* mAppNameLabel;
		QLabel* mAppVersionLabel;
		QLabel* mWechatQrCodeLabel;
		QLabel* mWechatNameLabel;
		QLabel* mEmailLabel;
		QLabel* mSlogonLabel;
		QLabel* mSlogonLabel1;
		QLabel* mSlogonLabel2;
	};
}

