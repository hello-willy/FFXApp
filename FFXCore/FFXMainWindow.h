#pragma once
#include "FFXCore.h"
#include "FFXApplication.h"
#include "FFXAppConfig.h"

#include <QtWidgets/QMainWindow>

class QMenuBar;
class QToolBar;
class QStatusBar;
class QToolButton;
class QDockWidget;
class QLabel;
class QShortcut;

namespace FFX {
	class PluginManager;
	class FileMainView;
	class FileSearchView;
	class TaskPanel;
	class FileQuickView;
	class ClipboardPanel;
	class AppConfig;
	class HandlerFactory;

	class FFXCORE_EXPORT MainWindow : public QMainWindow, public Application, public Configurable
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
		AppConfig* AppConfigPtr();

	public:
		virtual void AddMenu(QMenu* menu);
		virtual void RemoveMenu(QMenu* menu);
		virtual void AddToolbar(QToolBar* toolbar, Qt::ToolBarArea area = Qt::TopToolBarArea);
		virtual void RemoveToolbar(QToolBar* toolbar);
		virtual void ShowMessage(const QString& message, int timeout = 5);
		virtual QString PluginPath() const;
		virtual TaskPanel* TaskPanelPtr();
		virtual FileMainView* FileMainViewPtr();
		virtual HandlerFactory* HandlerFactoryPtr();
		virtual ClipboardPanel* ClipboardPanelPtr();

	public:
		virtual void Save(AppConfig* config);
		virtual void Restore(AppConfig* config);

	public:
		void UpdateFileSearchInfo(int count);

	protected:
		void closeEvent(QCloseEvent* event) override;

	private:
		void SetupUi();
		void UpdateCurrentDirInfo();
		void UpdateSelectFilesInfo(QStringList files);

	private slots:
		void OnTaskInfoUpdate();
		void OnActivateFileSearch();
		void OnAbout();

	private:
		QMenuBar* mMenuBar;
		QMenu* mFileMenu;
		QMenu* mViewMenu;
		QMenu* mPluginMenu;
		QMenu* mHelpMenu;

		QToolBar* mMainToolBar;
		
		QStatusBar* mStatusBar;
		QToolButton* mShowTaskBoardButton;
		QLabel* mTaskInfoLabel;
		QLabel* mCurrentDirInfoLabel;
		QLabel* mSelectFilesInfoLabel;
		QToolButton* mClipboardButton;
		QLabel* mFileSearchFileInfoLabel;

		PluginManager* mPluginManager = nullptr;
		FileMainView* mFileMainView;
		FileSearchView* mFileSearchView;
		QDockWidget* mTaskDocker;
		TaskPanel* mTaskPanel;
		
		QDockWidget* mFileSearchDocker;
		AppConfig* mAppConfig;
		HandlerFactory* mHandlerFactory;

		QShortcut* mActiveSearchShortcut;
		// test
		QAction* mShowAboutAction;
	};
}
