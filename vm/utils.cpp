#include "utils.h"

std::string escape(std::string str) {
	std::string ret = "";
	auto it = str.begin();
	while (it != str.end()) {
		char c = *(it++);
		if (c == '\\' && it != str.end()) {
			switch (*(it++)) {
				case 'n':
					c = '\n';
					break;
				case 't':
					c = '\t';
					break;
				case '"':
					c = '\"';
					break;
				case '\\':
					c = '\\';
					break;
				default:
					break;
			}
		}
		ret += c;
	}
	return ret;
}

std::string unescape(std::string str) {
	std::string ret = "";
	for (auto it = str.begin(); it != str.end(); ++it) {
		switch (*it) {
			case '\n':
				ret += "\\n";
				break;
			case '\t':
				ret += "\\t";
				break;
			case '"':
				ret += "\\\"";
				break;
			case '\\':
				ret += "\\\\";
				break;
			default:
				ret += (*it);
				break;
		}
	}
	return ret;
}
