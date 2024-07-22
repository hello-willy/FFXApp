#include "FFXActionManager.h"
#include <QAction>

namespace FFX {
	ActionManager::ActionManager(QObject* parent)
		: QObject(parent)
	{}

	ActionManager::~ActionManager()
	{}
}

