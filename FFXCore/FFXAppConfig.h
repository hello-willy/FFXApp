#pragma once
#include "FFXCore.h"
#include <QPair>
#include <QObject>

namespace FFX {
	class FFXCORE_EXPORT AppConfig : public QObject	{
		Q_OBJECT

	public:
		AppConfig(QObject* parent = nullptr);
		~AppConfig();

	public:
		void SaveMainWindowPos(const QRect& pos);
		QRect RestoreMainWindowPos();

		void SaveQuickItem(const QList<QPair<QString, QVariant>>& items);
		QList<QPair<QString, QVariant>> RestoreQuickItem();

		void SaveCurrentRoot(const QString& root);
		QString RestoreCurrentRoot();

	private:
		void WriteItem(const QString& group, const QString& key, const QVariant& value);
		void WriteItemArray(const QString& group, const QVariantList& values);
		void WritePairItemArray(const QString& group, const QList<QPair<QString, QVariant>>& values);

		QVariant ReadItem(const QString& group, const QString& key);
		QVariantList ReadItemArray(const QString& group);
		QList<QPair<QString, QVariant>> ReadPairItemArray(const QString& group);

	private:
		QString mConfigFile;
	};

}
