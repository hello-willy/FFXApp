#pragma once
#include "FFXCore.h"
#include <QString>
#include <QtPlugin>

class QPluginLoader;
namespace FFX {
	class FFXCORE_EXPORT Plugin {
	public:
		virtual ~Plugin() {}
	public:
		virtual void Install() = 0;
		virtual void Uninstall() = 0;
		virtual QString Id() = 0;
		QString name() { return "Unknown"; }
		QString description() { return ""; }
	};

	class FFXCORE_EXPORT PluginManager : public QObject {
		Q_OBJECT
	public:
		PluginManager(QObject* parent = nullptr);
		~PluginManager() = default;

	public:
		void LoadPlugin(const QString& pluginPath);
		void UnloadPlugin(const QString& pluginId);
	private:
		QMap<QString, QPluginLoader*> mPluginMap;
	};
}

#define FFX_Plugin_IID "FFX.Plugin.Interface"

Q_DECLARE_INTERFACE(FFX::Plugin, FFX_Plugin_IID)


