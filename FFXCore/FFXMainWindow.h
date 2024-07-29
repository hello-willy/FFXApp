#pragma once
#include "FFXCore.h"

#include <QtWidgets/QMainWindow>

class QMenuBar;
class QToolBar;
class QStatusBar;
class QToolButton;
class QDockWidget;
namespace FFX {
	class PluginManager;
	class FileMainView;
	class FileSearchView;
	class TaskPanel;
	class FileQuickView;

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
		TaskPanel* TaskPanelPtr();

	private:
		void SetupUi();

	private:
		QMenuBar* mMenuBar;
		QMenu* mFileMenu;
		QMenu* mViewMenu;

		QToolBar* mMainToolBar;
		QStatusBar* mStatusBar;
		QToolButton* mShowTaskBoardButton;
		//QToolButton* mMakeFileButton;
		//QMenu* mMakeFileMenu;

		PluginManager* mPluginManager = nullptr;
		FileMainView* mFileMainView;
		FileSearchView* mFileSearchView;
		QDockWidget* mTaskDocker;
		TaskPanel* mTaskPanel;
	};
}
