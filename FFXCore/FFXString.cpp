#include "FFXString.h"

namespace FFX {
	namespace String {
		void Trim(std::string& str, bool left, bool right)
		{
			static const std::string delims = " \t\r\n";
			if (right)
				str.erase(str.find_last_not_of(delims) + 1); // trim right
			if (left)
				str.erase(0, str.find_first_not_of(delims)); // trim left
		}
	}
}