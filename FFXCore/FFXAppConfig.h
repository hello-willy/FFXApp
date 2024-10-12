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
		QRect RestoreMainWindowPos();

		void WriteItem(const QString& group, const QString& key, const QVariant& value);
		void WriteItemArray(const QString& group, const QVariantList& values);
		void WritePairItemArray(const QString& group, const QList<QPair<QString, QVariant>>& values);
		//void WriteRectItem(const QString& group, const QRect& rect);

		QVariant ReadItem(const QString& group, const QString& key);
		QVariantList ReadItemArray(const QString& group);
		QList<QPair<QString, QVariant>> ReadPairItemArray(const QString& group);
		//QRect ReadRectItem(const QString& group);

	private:
		QString mConfigFile;
	};

	class Configurable
	{
	public:
		virtual void Save(AppConfig* config) = 0;
		virtual void Restore(AppConfig* config) = 0;
	};
}
