#pragma once
#include "FFXCore.h"

#include <QtWidgets/QMainWindow>

class QMenuBar;
class QToolBar;
class QStatusBar;
class QToolButton;

namespace FFX {
	class PluginManager;
	class FileMainView;
	class FileSearchView;
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
		FileMainView* FileMainViewPtr();

	private:
		void SetupUi();

	private:
		QMenuBar* mMenuBar;
		QToolBar* mMainToolBar;
		QStatusBar* mStatusBar;
		QToolButton* mShowTaskBoardButton;

		PluginManager* mPluginManager = nullptr;
		FileMainView* mFileMainView;
		FileSearchView* mFileSearchView;
	};

}
