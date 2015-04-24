#pragma once

#include "Poco/Util/IniFileConfiguration.h"
#include <map>
#include <istream>


class OPDIDConfigurationFile: public Poco::Util::IniFileConfiguration
{
public:
	OPDIDConfigurationFile(const std::string& path, std::map<std::string, std::string> parameters);
};
