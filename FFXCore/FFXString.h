#pragma once
#include "FFXCore.h"

#include <string>
#include <QString>

namespace FFX {
	namespace String {
		FFXCORE_EXPORT void Trim(std::string& str, bool left = true, bool right = true);
		FFXCORE_EXPORT QString TimeHint(qint64 t);
		FFXCORE_EXPORT QString BytesHint(qint64 size);
	}
}
