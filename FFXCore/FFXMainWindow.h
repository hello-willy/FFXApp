#pragma once
#include "FFXCore.h"
#include "FFXApplication.h"

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

	class FFXCORE_EXPORT MainWindow : public QMainWindow, public Application
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

	public:
		virtual void AddMenu(QMenu* menu);
		virtual void RemoveMenu(QMenu* menu);
		virtual void AddToolbar(QToolBar* toolbar, Qt::ToolBarArea area = Qt::TopToolBarArea);
		virtual void RemoveToolbar(QToolBar* toolbar);
		virtual void ShowMessage(const QString& message, int timeout = 5);
		virtual TaskPanel* TaskPanelPtr();
		virtual FileMainView* FileMainViewPtr();

	private:
		void SetupUi();

	private:
		QMenuBar* mMenuBar;
		QMenu* mFileMenu;
		QMenu* mViewMenu;

		QToolBar* mMainToolBar;
		QStatusBar* mStatusBar;
		QToolButton* mShowTaskBoardButton;

		PluginManager* mPluginManager = nullptr;
		FileMainView* mFileMainView;
		FileSearchView* mFileSearchView;
		QDockWidget* mTaskDocker;
		TaskPanel* mTaskPanel;
	};
}
