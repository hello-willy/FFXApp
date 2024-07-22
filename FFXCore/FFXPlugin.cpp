#include "FFXPlugin.h"
#include <QPluginLoader>

namespace FFX {
    PluginManager::PluginManager(QObject* parent) 
        : QObject(parent) {

    }

	void PluginManager::LoadPlugin(const QString& pluginPath) {
        if (!QLibrary::isLibrary(pluginPath))
            return;

        QPluginLoader* loader = new QPluginLoader(pluginPath);
        if (loader->load()) {
            Plugin* plugin = qobject_cast<Plugin*>(loader->instance());
            if (plugin) {
                plugin->Install();
                mPluginMap[plugin->Id()] = loader;
            } else {
                delete loader;
                loader = nullptr;
            }
        }
	}

	void PluginManager::UnloadPlugin(const QString& pluginId) {
        QMap<QString, QPluginLoader*>::iterator it = mPluginMap.find(pluginId);
        if (it == mPluginMap.end())
            return;
        QPluginLoader* loader = it.value();
        Plugin* p = qobject_cast<Plugin*>(loader->instance());
        if (p) {
            p->Uninstall();
        }
        loader->unload();
        delete loader;
        mPluginMap.erase(it);
	}
}