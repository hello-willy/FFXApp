#pragma once
#include "FFXCore.h"
#include <QtWidgets/QMainWindow>

namespace FFX {
	class FFXCORE_EXPORT MainWindow : public QMainWindow
	{
		Q_OBJECT

	public:
		MainWindow(QWidget* parent = nullptr);
		~MainWindow();

	public:
		static MainWindow* Instance();

	private:
		static MainWindow* sInstance;
	};

}
