#include "FFXString.h"
#include <QStringList>
#include <QObject>

namespace FFX {
	namespace String {
		void Trim(std::string& str, bool left, bool right) {
			static const std::string delims = " \t\r\n";
			if (right)
				str.erase(str.find_last_not_of(delims) + 1); // trim right
			if (left)
				str.erase(0, str.find_first_not_of(delims)); // trim left
		}

		QString TimeHint(qint64 t) {
			QStringList unitList;
			unitList << QObject::tr("secs") << QStringLiteral("mins") << QStringLiteral("hours") << QStringLiteral("days");
			QList<qint64> stepList;
			stepList << 1000 << 60 << 60 << 24;
			QStringListIterator i(unitList);
			QString unit(QObject::tr("ms"));
			double num = (double)t;
			//while (num > 1000 && i.hasNext())
			for (int i = 0; i < unitList.size(); i++) {
				if (num < stepList[i])
					break;
				unit = unitList[i];
				num /= stepList[i];
			}
			return QString().setNum(num, 'f', t < 1000 ? 0 : 3) + " " + unit;
		}

		QString BytesHint(qint64 size) {
			QStringList list;
			list << "KB" << "MB" << "GB" << "TB";
			QStringListIterator i(list);
			QString unit("Bytes");
			double num = (double)size;
			while (num >= 1024.0 && i.hasNext()) {
				unit = i.next();
				num /= 1024.0;
			}
			return QString().setNum(num, 'f', size < 1024 ? 0 : 2) + " " + unit;
		}
	}
}