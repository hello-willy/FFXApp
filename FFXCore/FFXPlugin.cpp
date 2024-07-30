#include "FFXPlugin.h"
#include "FFXMainWindow.h"

#include <QPluginLoader>
#include <QDir>
#include <QCoreApplication>

namespace FFX {
    Application* Plugin::App() {
        return MainWindow::Instance();
    }

    PluginManager::PluginManager(QObject* parent) 
        : QObject(parent) {
        AutoLoad();
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

    void PluginManager::AutoLoad() {
        QDir currentDir(QCoreApplication::applicationDirPath());
        currentDir.cd("ffxplugins");
        currentDir.setFilter(QDir::Dirs | QDir::NoDotAndDotDot);
        QFileInfoList fileList = currentDir.entryInfoList();
        foreach(QFileInfo fileInfo, fileList) {
            QDir pluginDir(fileInfo.absoluteFilePath());
            QString pluginPath = pluginDir.absoluteFilePath(fileInfo.fileName() + ".dll");
            LoadPlugin(pluginPath);
        }
    }
}