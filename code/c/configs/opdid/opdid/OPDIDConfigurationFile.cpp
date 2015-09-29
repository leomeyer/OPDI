
#include <sstream>

#include "Poco/FileStream.h"

#include "OPDIDConfigurationFile.h"

OPDIDConfigurationFile::OPDIDConfigurationFile(const std::string& path, std::map<std::string, std::string> parameters) {

	// load the file content
	Poco::FileInputStream istr(path, std::ios::in);

	if (!istr.good())
		throw Poco::OpenFileException(path);
	
	std::string content;
	std::string line;
	while (std::getline(istr, line)) {
		content += line;
		content.push_back('\n');
	}

	// replace parameters in content
	typedef std::map<std::string, std::string>::iterator it_type;
	for (it_type iterator = parameters.begin(); iterator != parameters.end(); ++iterator) {
		std::string key = iterator->first;
		std::string value = iterator->second;

		size_t start = 0;
		while ((start = content.find(key, start)) != std::string::npos) {
			content.replace(start, key.length(), value);
		}
	}

	// load the configuration from the new content
	std::stringstream newContent;
	newContent.str(content);
	this->load(newContent);
}