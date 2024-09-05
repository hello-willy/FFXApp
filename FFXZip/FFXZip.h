#pragma once

#include <QtCore/qglobal.h>
#include <QCoreApplication>

#ifndef BUILD_STATIC
# if defined(FFXZIP_LIB)
#  define FFXZIP_EXPORT Q_DECL_EXPORT
# else
#  define FFXZIP_EXPORT Q_DECL_IMPORT
# endif
#else
# define FFXZIP_EXPORT
#endif

#define PLUGIN_NAME "FFXZip"

inline QString PluginDir() {
	QString pluginPath = QDir(QCoreApplication::applicationDirPath()).absoluteFilePath("ffxplugins");
	return QDir(pluginPath).absoluteFilePath(PLUGIN_NAME);
}
