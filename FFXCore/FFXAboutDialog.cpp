#include "FFXAboutDialog.h"
#include <QTableWidget>
#include <QGridLayout>
#include <QImage>
#include <QLabel>

namespace FFX {
	AboutDialog::AboutDialog(QWidget* parent)
		: QDialog(parent) {
		SetupUi();
	}

	AboutDialog::~AboutDialog()
	{}

	void AboutDialog::SetupUi() {
		mMainLayout = new QGridLayout;
		mAppNameLabel = new QLabel("Final File X");
		mAppVersionLabel = new QLabel("1.0.0-beta");
		mWechatQrCodeLabel = new QLabel;
		QImage qrCode(":/ffx/res/image/qr.jpg");
		QPixmap pixmap = QPixmap::fromImage(qrCode).scaled(256, 256);
		mWechatQrCodeLabel->setPixmap(pixmap);
		mWechatNameLabel = new QLabel(QObject::tr("轮子哥"));
		mEmailLabel = new QLabel("linweiling@163.com");
		mSlogonLabel = new QLabel(QObject::tr("<b>有文件处理需求或问题，扫码或邮件联系我</b>"));
		mSlogonLabel1 = new QLabel(QObject::tr("***不联系是你的问题***"));
		mSlogonLabel2 = new QLabel(QObject::tr("~~~解决不了是我的问题~~~"));
		mMainLayout->setAlignment(Qt::AlignCenter);
		mMainLayout->addWidget(mAppNameLabel, 0, 0, Qt::AlignCenter);
		mMainLayout->addWidget(mAppVersionLabel, 1, 0, Qt::AlignCenter);
		mMainLayout->addWidget(mWechatQrCodeLabel, 2, 0, Qt::AlignCenter);
		mMainLayout->addWidget(mWechatNameLabel, 3, 0, Qt::AlignCenter);
		mMainLayout->addWidget(mEmailLabel, 4, 0, Qt::AlignCenter);
		mMainLayout->addWidget(mSlogonLabel, 5, 0, Qt::AlignCenter);
		mMainLayout->addWidget(mSlogonLabel1, 6, 0, Qt::AlignCenter);
		mMainLayout->addWidget(mSlogonLabel2, 7, 0, Qt::AlignCenter);
		setLayout(mMainLayout);

		setWindowTitle(QObject::tr("About"));
		setWindowFlags(Qt::Dialog | Qt::WindowCloseButtonHint);
		setWindowIcon(QIcon(":/ffx/res/image/info.svg"));
		//resize(900, 1024);
	}
}

