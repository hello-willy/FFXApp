#pragma once

#include <QDialog>

namespace FFX {
	class AboutDialog : public QDialog {
		Q_OBJECT

	public:
		AboutDialog(QWidget* parent = nullptr);
		~AboutDialog();

	private:

	};
}

