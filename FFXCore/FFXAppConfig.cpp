#include "FFXAppConfig.h"
#include "FFXMainWindow.h"

#include <QSettings>

QString MainWindowPos = "MainWindowPos";
QString QuickItem = "QuickItem";
QString CurrentRoot = "CurrentRoot";

namespace FFX {
	AppConfig::AppConfig(QObject *parent)
		: QObject(parent)
		, mConfigFile("app.ini") {}

	AppConfig::~AppConfig()
	{}

	void AppConfig::SaveMainWindowPos(const QRect& loc) {
		QSettings settings(mConfigFile, QSettings::IniFormat);

		settings.beginGroup(MainWindowPos);
		settings.setValue("x", loc.x());
		settings.setValue("y", loc.y());
		settings.setValue("width", loc.width());
		settings.setValue("height", loc.height());
		settings.endGroup();
	}

	QRect AppConfig::RestoreMainWindowPos() {
		QSettings settings(mConfigFile, QSettings::IniFormat);
		settings.beginGroup(MainWindowPos);
		QVariant x = settings.value("x");
		QVariant y = settings.value("y");
		QVariant width = settings.value("width");
		QVariant height = settings.value("height");
		settings.endGroup();
		return QRect(x.toInt(), y.toInt(), width.toInt(), height.toInt());
	}

	void AppConfig::SaveQuickItem(const QList<QPair<QString, QVariant>>& items) {
		WritePairItemArray(QuickItem, items);
	}

	QList<QPair<QString, QVariant>> AppConfig::RestoreQuickItem() {
		return ReadPairItemArray(QuickItem);
	}

	void AppConfig::SaveCurrentRoot(const QString& root) {
		QSettings settings(mConfigFile, QSettings::IniFormat);

		settings.beginGroup(CurrentRoot);
		settings.setValue("RootPath", root);
		settings.endGroup();
	}

	QString AppConfig::RestoreCurrentRoot() {
		QSettings settings(mConfigFile, QSettings::IniFormat);
		settings.beginGroup(CurrentRoot);
		QVariant rootPath = settings.value("RootPath");
		settings.endGroup();
		return rootPath.toString();
	}

	void AppConfig::WriteItem(const QString& group, const QString& key, const QVariant& value) {
		QSettings settings(mConfigFile, QSettings::IniFormat);

		settings.beginGroup(group);
		settings.setValue(key, value);
		settings.endGroup();
	}

	void AppConfig::WriteItemArray(const QString& group, const QVariantList& values) {
		QSettings settings(mConfigFile, QSettings::IniFormat);
		settings.beginWriteArray(group);
		for (int i = 0; i < values.size(); i++) {
			settings.setArrayIndex(i);
			settings.setValue("value", values[i]);
		}
		settings.endArray();
	}

	void AppConfig::WritePairItemArray(const QString& group, const QList<QPair<QString, QVariant>>& values) {
		QSettings settings(mConfigFile, QSettings::IniFormat);
		settings.beginWriteArray(group);
		for (int i = 0; i < values.size(); i++) {
			settings.setArrayIndex(i);
			settings.setValue("key", values[i].first);
			settings.setValue("value", values[i].second);
		}
		settings.endArray();
	}

	QVariant AppConfig::ReadItem(const QString& group, const QString& key) {
		QSettings settings(mConfigFile, QSettings::IniFormat);

		settings.beginGroup(group);
		QVariant value = settings.value(key);
		settings.endGroup();
		return value;
	}

	QVariantList AppConfig::ReadItemArray(const QString& group) {
		QSettings settings(mConfigFile, QSettings::IniFormat);
		QVariantList result;
		int size = settings.beginReadArray(group);
		for (int i = 0; i < size; i++) {
			settings.setArrayIndex(i);
			result.append(settings.value("value"));
		}
		settings.endArray();
		return result;
	}

	QList<QPair<QString, QVariant>> AppConfig::ReadPairItemArray(const QString& group) {
		QSettings settings(mConfigFile, QSettings::IniFormat);
		QList<QPair<QString, QVariant>> result;
		int size = settings.beginReadArray(group);
		for (int i = 0; i < size; i++) {
			settings.setArrayIndex(i);
			QPair<QString, QVariant> pair = qMakePair(settings.value("key").toString(), settings.value("value"));
			result.append(pair);
		}
		settings.endArray();
		return result;
	}
}
