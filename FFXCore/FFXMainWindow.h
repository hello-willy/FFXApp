#pragma once
#include "FFXCore.h"
#include <QtWidgets/QMainWindow>

namespace FFX {
	class PluginManager;
	class FFXCORE_EXPORT MainWindow : public QMainWindow
	{
		Q_OBJECT
	public:
		static MainWindow* Instance();
	private:
		static MainWindow* sInstance;

	public:
		MainWindow(QWidget* parent = nullptr);
		~MainWindow();

	public:
		PluginManager* PluginManagerPtr();
	

	private:
		PluginManager* mPluginManager = nullptr;
	};

}
