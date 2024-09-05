#pragma once
#include <QString>
class QMenu;
class QToolBar;

namespace FFX {
	class TaskPanel;
	class FileMainView;
	class HandlerFactory;

	class Application {
	public:
		virtual void AddMenu(QMenu* menu) = 0;
		virtual void RemoveMenu(QMenu* menu) = 0;
		virtual void AddToolbar(QToolBar* toolbar, Qt::ToolBarArea area = Qt::TopToolBarArea) = 0;
		virtual void RemoveToolbar(QToolBar* toolbar) = 0;
		virtual void ShowMessage(const QString& message, int timeout = 5) = 0;
		virtual QString PluginPath() const = 0;
		virtual TaskPanel* TaskPanelPtr() = 0;
		virtual FileMainView* FileMainViewPtr() = 0;
		virtual HandlerFactory* HandlerFactoryPtr() = 0;
	};
}