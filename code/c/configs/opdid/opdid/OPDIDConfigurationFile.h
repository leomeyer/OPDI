#pragma once

#include "Poco/Util/IniFileConfiguration.h"
#include <map>
#include <istream>


class OPDIDConfigurationFile: public Poco::Util::IniFileConfiguration
{
protected:
	/** Override getRaw to allow values in quotes. Quotes are removed before returning the value. 
	*   This allows whitespace to appear in front or at the end of values.
	*/
	virtual bool getRaw(const std::string & key, std::string & value) const;

public:
	OPDIDConfigurationFile(const std::string& path, std::map<std::string, std::string> parameters);
};
