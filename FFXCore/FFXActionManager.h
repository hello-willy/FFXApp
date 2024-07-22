#pragma once

#include <QObject>
#include <QMap>

namespace FFX {
	class QAction;

	class ActionManager : public QObject
	{
		Q_OBJECT

	public:
		ActionManager(QObject* parent);
		~ActionManager();

	private:
		QMap<QString, QAction*> mActionMap;
	};
}

