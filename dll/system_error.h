#pragma once

#include <string>

namespace am {

class system_error {
public:
	static const std::string& error2str(unsigned long error);
};

}
