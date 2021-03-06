#include <sstream>
#include <exception>
#include <memory>

#include "Poco/Tuple.h"
#include "Poco/Runnable.h"
#include "Poco/Mutex.h"
#include "Poco/ScopedLock.h"
#include "Poco/FileStreamFactory.h"
#include <Poco/Net/HTTPStreamFactory.h>
#include <Poco/Net/FTPStreamFactory.h>
#include <Poco/StreamCopier.h>
#include <Poco/Path.h>
#include <Poco/URI.h>
#include <Poco/URIStreamOpener.h>
#include <Poco/Exception.h>
#include <Poco/Net/NetException.h>
#include "Poco/DOM/DOMParser.h"
#include "Poco/DOM/Document.h"
#include "Poco/DOM/NodeIterator.h"
#include "Poco/DOM/NodeFilter.h"
#include "Poco/DOM/NodeList.h"
#include "Poco/DOM/AutoPtr.h"
#include "Poco/SAX/InputSource.h"
#include "Poco/NumberParser.h"
#include "Poco/DateTimeParser.h"
#include "Poco/Timezone.h"
#include "Poco/RegularExpression.h"
#include "Poco/JSON/Parser.h"

#include "opdi_constants.h"
#include "opdi_platformfuncs.h"

#include "AbstractOPDID.h"

namespace {

/** Interface for weather ports */
class WeatherPort {
	std::string id;
public:
	WeatherPort(std::string id) : id(id) {};

	virtual std::string getDataElement(void) = 0;

	virtual void invalidate(void) = 0;

	virtual void extract(const std::string& rawValue) = 0;
};

/** A WeatherGaugePort extracts a data value from the weather station's data source.
* It is represented as a DialPort internally.
* Its extract() method is fed the raw string data value. It applies the specifed regex
* to match the value into a string that is then parsed as a double.
* The resulting double is multiplied with the numerator and divided by the denominator.
* This allows to scale the value into a meaningful integer value. The resulting integer
* value is set as the position of the dial port.
*/
class WeatherGaugePort : public opdi::DialPort, public WeatherPort {

protected:
	opdid::AbstractOPDID* opdid;
		
	bool isValid;
	mutable bool lastRequestedValidState;
	std::string dataElement;
	std::string regexMatch;
	std::string regexReplace;
	std::string replaceBy;
	int numerator;
	int denominator;

	std::string rawValue;	// stores the value for in-thread processing

	mutable Poco::Mutex mutex;	// mutex for thread-safe accessing

public:

	WeatherGaugePort(opdid::AbstractOPDID* opdid, const char* id);

	virtual std::string getDataElement(void);

	virtual void configure(Poco::Util::AbstractConfiguration* nodeConfig, opdi::LogVerbosity defaultLogVerbosity);

	virtual void invalidate(void);

	virtual void extract(const std::string& rawValue);

	virtual uint8_t doWork(uint8_t canSend) override;

	virtual void prepare(void) override;

	virtual void getState(int64_t* position) const override;

	// important! Override this method and use isValid directly because
	// the standard method will continuously query the state, thus rendering the
	// lastRequestedValidState flag useless and cause perpetual self-refreshes!
	virtual bool hasError(void) const override;
};

}	// end anonymous namespace

WeatherGaugePort::WeatherGaugePort(opdid::AbstractOPDID* opdid, const char* id) : opdi::DialPort(id), WeatherPort(id) {
	this->opdid = opdid;
	this->numerator = 1;
	this->denominator = 1;
	this->isValid = false;
	this->lastRequestedValidState = false;
}

std::string WeatherGaugePort::getDataElement(void){
	return this->dataElement;
}

void WeatherGaugePort::configure(Poco::Util::AbstractConfiguration* nodeConfig, opdi::LogVerbosity defaultLogVerbosity) {
	opdid->configureDialPort(nodeConfig, this);
	this->logVerbosity = opdid->getConfigLogVerbosity(nodeConfig, defaultLogVerbosity);

	this->dataElement = opdid->getConfigString(nodeConfig, this->ID(), "DataElement", "", true);
	this->regexMatch = opdid->getConfigString(nodeConfig, this->ID(), "RegexMatch", "", false);
	this->regexReplace = opdid->getConfigString(nodeConfig, this->ID(), "RegexReplace", "", false);
	this->replaceBy = opdid->getConfigString(nodeConfig, this->ID(), "ReplaceBy", "", false);
	this->numerator = nodeConfig->getInt("Numerator", this->numerator);
	this->denominator = nodeConfig->getInt("Denominator", this->denominator);
	if (this->denominator == 0) 
		throw Poco::InvalidArgumentException(this->ID() + ": The Denominator may not be 0");
}

void WeatherGaugePort::prepare(void) {
	// cannot change weather parameters on the master; so better set this to readonly
	this->setReadonly(true);
	if (this->refreshMode == RefreshMode::REFRESH_NOT_SET)
		// automatically refresh when the value changes
		this->setRefreshMode(RefreshMode::REFRESH_AUTO);
	opdi::DialPort::prepare();
}

void WeatherGaugePort::invalidate(void) {
	Poco::Mutex::ScopedLock lock(this->mutex);
	this->isValid = false;
}

bool WeatherGaugePort::hasError(void) const {
	Poco::Mutex::ScopedLock lock(this->mutex);
	return !this->isValid;
}

void WeatherGaugePort::extract(const std::string& rawValue) {
	// important! Do not process on the weather plugin's thread;
	// instead, store the value for processing on the OPDID thread
	Poco::Mutex::ScopedLock lock(this->mutex);
	this->rawValue = rawValue;
}

uint8_t WeatherGaugePort::doWork(uint8_t canSend) {
	opdi::DialPort::doWork(canSend);

	Poco::Mutex::ScopedLock lock(this->mutex);

	std::string rawValue = this->rawValue;

	// no value to process?
	if (rawValue == "")
		return OPDI_STATUS_OK;

	// mark as invalid
	this->isValid = false;

	std::string value = rawValue;
	if (this->regexMatch != "") {
		Poco::RegularExpression regex(this->regexMatch, 0, true);
		this->logDebug("WeatherGaugePort for element " + this->dataElement + ": Matching regex against weather data: " + rawValue);
		if (regex.extract(rawValue, value, 0) == 0) {
		this->logDebug("WeatherGaugePort for element " + this->dataElement + ": Warning: Matching regex returned no result; weather data: " + rawValue);
		}
	}
	if (this->regexReplace != "") {
		Poco::RegularExpression regex(this->regexReplace, 0, true);
		if (regex.subst(value, this->replaceBy, Poco::RegularExpression::RE_GLOBAL) == 0) {
			this->logDebug("WeatherGaugePort for element " + this->dataElement + ": Warning: Replacement regex did not match in value: " + value);
		}
	}

	// clear the raw value
	this->rawValue = "";

	// try to parse the value as double
	double result = 0;
	try {
		result = Poco::NumberParser::parseFloat(value);
	} catch (Poco::Exception e) {
		this->logDebug("WeatherGaugePort for element " + this->dataElement + ": Warning: Unable to parse weather data: " + value);
		return OPDI_STATUS_OK;
	}

	// scale the result
	int64_t newPos = (int64_t)(result * this->numerator / this->denominator * 1.0);
	this->logDebug("WeatherGaugePort for element " + this->dataElement + ": Extracted value is: " + to_string(newPos));

	// correct value; a standard dial port will throw exceptions
	// but exceptions must be avoided in this threaded code because they will cause strange messages on Linux
	if (newPos < this->minValue) {
		this->logDebug("Warning: Value too low (" + to_string(newPos) + " < " + to_string(this->minValue) + "), correcting");
		newPos = this->minValue;
	}
	if (newPos > this->maxValue) {
		this->logDebug("Warning: Value too high (" + to_string(newPos) + " > " + to_string(this->maxValue) + "), correcting");
		newPos = this->maxValue;
	}
	this->setPosition(newPos);

	// mark as valid
	this->isValid = true;

	// if the master's last state is invalid, and we're now valid, we should self-refresh
	// to update the master (except it's disabled)
	if (!this->lastRequestedValidState && (this->refreshMode != RefreshMode::REFRESH_OFF))
		this->refreshRequired = true;

	return OPDI_STATUS_OK;
}

// function that fills in the current port state
void WeatherGaugePort::getState(int64_t* position) const {

	// ensure thread safety for this block
	// also ensures that the doWork method and getState do not cross
	Poco::Mutex::ScopedLock lock(mutex);

	// remember whether the master has a valid state or not
	this->lastRequestedValidState = this->isValid;

	if (!this->isValid) {
		throw PortError(this->ID() + ": Reading is not valid");
	}

	opdi::DialPort::getState(position);
}

////////////////////////////////////////////////////////////////////////
// Plugin main class
////////////////////////////////////////////////////////////////////////

class WeatherPlugin : public IOPDIDPlugin, public opdid::IOPDIDConnectionListener, public Poco::Runnable {

protected:
	std::string nodeID;

	opdi::LogVerbosity logVerbosity;

	std::string url;
	int timeoutSeconds;

	// weather data provider identification
	std::string provider;

	std::string xpath;
	int64_t dataValiditySeconds;

	int refreshTime;

	std::string sid;

	Poco::Thread workThread;

	typedef std::vector<WeatherPort*> WeatherPortList;
	WeatherPortList weatherPorts;

public:
	opdid::AbstractOPDID* opdid;

	virtual void masterConnected(void) override;
	virtual void masterDisconnected(void) override;

	virtual void refreshData(void);

	// weather data collection thread method
	virtual void run(void);

	virtual void setupPlugin(opdid::AbstractOPDID* abstractOPDID, const std::string& node, Poco::Util::AbstractConfiguration* config) override;
};

void WeatherPlugin::setupPlugin(opdid::AbstractOPDID* abstractOPDID, const std::string& node, Poco::Util::AbstractConfiguration* config) {
	this->opdid = abstractOPDID;
	this->nodeID = node;
	this->timeoutSeconds = 10;
	this->refreshTime = 30;	// default: 30 seconds
	this->dataValiditySeconds = 120;	// default: two minutes (JSON skin time resolution does not include seconds, allow extra time)

	Poco::AutoPtr<Poco::Util::AbstractConfiguration> nodeConfig = config->createView(node);

	this->logVerbosity = opdid->getConfigLogVerbosity(nodeConfig, opdi::LogVerbosity::UNKNOWN);

	this->url = abstractOPDID->getConfigString(nodeConfig, node, "Url", "", true);
	this->provider = abstractOPDID->getConfigString(nodeConfig, node, "Provider", "", true);

	if (this->provider == "Weewx") {
		// configure xpath for default skin
		this->xpath = nodeConfig->getString("XPath", "html/body/div/div[@id='stats_group']/div/table/tbody");
	} else
	if (this->provider == "Weewx-JSON") {
		// nothing to do
	} else
		throw Poco::DataException(node + ": Provider not supported: " + this->provider);

	this->timeoutSeconds = nodeConfig->getInt("Timeout", this->timeoutSeconds);

	// store main node's group (will become the default of ports)
	std::string group = nodeConfig->getString("Group", "");
	this->refreshTime = nodeConfig->getInt("RefreshTime", this->refreshTime);
	if (this->refreshTime <= 0)
		throw Poco::DataException(node + ": Please specify a non-negative meaningful RefreshTime in seconds");
	if (this->refreshTime > 600)
		throw Poco::DataException(node + ": Please do not specify more than 10 minutes RefreshTime");

	this->dataValiditySeconds = nodeConfig->getInt64("DataValidity", this->dataValiditySeconds);
	if (this->dataValiditySeconds <= 0)
		throw Poco::DataException(node + ": Please specify a non-negative meaningful DataValidity in seconds");

	// enumerate keys of the plugin's nodes (in specified order)
	this->opdid->logVerbose(node + ": Enumerating Weather nodes: " + node + ".Nodes", this->logVerbosity);

	Poco::AutoPtr<Poco::Util::AbstractConfiguration> nodes = config->createView(node + ".Nodes");

	// get ordered list of ports
	Poco::Util::AbstractConfiguration::Keys portKeys;
	nodes->keys("", portKeys);

	typedef Poco::Tuple<int, std::string> Item;
	typedef std::vector<Item> ItemList;
	ItemList orderedItems;

	// create ordered list of port keys (by priority)
	for (auto it = portKeys.begin(), ite = portKeys.end(); it != ite; ++it) {

		int itemNumber = nodes->getInt(*it, 0);
		// check whether the item is active
		if (itemNumber < 0)
			continue;

		// insert at the correct position to create a sorted list of items
		auto nli = orderedItems.begin();
		auto nlie = orderedItems.end();
		while (nli != nlie) {
			if (nli->get<0>() > itemNumber)
				break;
			++nli;
		}
		Item item(itemNumber, *it);
		orderedItems.insert(nli, item);
	}

	if (orderedItems.size() == 0) {
		this->opdid->logNormal("Warning: No ports configured in node " + node + ".Nodes; is this intended?", this->logVerbosity);
	}

	// go through items, create ports in specified order
	auto nli = orderedItems.begin();
	auto nlie = orderedItems.end();
	while (nli != nlie) {

		std::string nodeName = nli->get<1>();

		this->opdid->logVerbose("Setting up Weather port(s) for node: " + nodeName, this->logVerbosity);

		// get port section from the configuration
		Poco::Util::AbstractConfiguration* portConfig = config->createView(nodeName);

		// get port type (required)
		std::string portType = abstractOPDID->getConfigString(portConfig, nodeName, "Type", "", true);

		if (portType == "WeatherGaugePort") {

			WeatherGaugePort* port = new WeatherGaugePort(this->opdid, nodeName.c_str());
			port->setGroup(group);
			port->configure(portConfig, this->logVerbosity);
			opdid->addPort(port);
			// add port to internal list
			this->weatherPorts.push_back(port);
		} else
			throw Poco::DataException("This plugin does not support the port type", portType);

		++nli;
	}

	this->opdid->addConnectionListener(this);

	// initialize and start thread
	this->workThread.setName(node + " work thread");
	this->workThread.start(*this);

	this->opdid->logVerbose(this->nodeID + ": WeatherPlugin setup completed successfully", this->logVerbosity);
}

void WeatherPlugin::masterConnected() {
}

void WeatherPlugin::masterDisconnected() {
}

void replaceAll(std::string& str, const std::string& from, const std::string& to) {
    if(from.empty())
        return;
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
    }
}

static std::string GetJSONStringValue(Poco::JSON::Object::Ptr aoJsonObject, std::string aszKey) {
    Poco::Dynamic::Var loVariable;
    std::string lsReturn;

    loVariable = aoJsonObject->get(aszKey);
    lsReturn = loVariable.convert<std::string>();

    return lsReturn;
}

static Poco::JSON::Object::Ptr GetJSONObject(Poco::JSON::Object::Ptr aoJsonObject, std::string aszKey) {
    Poco::Dynamic::Var loVariable;
    Poco::JSON::Object::Ptr lsReturn;

    loVariable = aoJsonObject->get(aszKey);
    lsReturn = loVariable.extract<Poco::JSON::Object::Ptr>();

    return lsReturn;
}

void WeatherPlugin::refreshData(void) {
	try {
		// invalidate all ports
		auto it = this->weatherPorts.begin();
		auto ite = this->weatherPorts.end();
		while (it != ite) {
			(*it)->invalidate();
			++it;
		}

		this->opdid->logDebug(this->nodeID + ": Fetching content of URL: " + this->url, this->logVerbosity);

		Poco::URI uri(this->url);
		std::unique_ptr<std::istream> pStr(Poco::URIStreamOpener::defaultOpener().open(uri));

		// save stream content
		std::string content;
		Poco::StreamCopier::copyToString(*pStr.get(), content);

		if (this->provider == "Weewx") {
			// this code is not reliable (experimental only)
			// likely to break with minor changes to the HTML; does not support complete data set

			// parse content as XML
			// cleanup structure errors in Weewx default skin
			replaceAll(content, "select NAME=noaaselect", "select name=\"noaaselect\"");
			replaceAll(content, "option selected", "option selected=\"1\"");

			this->opdid->logDebug(this->nodeID + ": Parsing weather data content as XML", this->logVerbosity);

			std::stringstream in(content);
			Poco::XML::InputSource src(in);
			Poco::XML::DOMParser parser;
			Poco::AutoPtr<Poco::XML::Document> pDoc = parser.parse(&src);

			Poco::XML::Node* node;

			node = pDoc->getNodeByPath(this->xpath);
			if (node == nullptr) {
				this->opdid->logNormal(this->nodeID + ": Error: File format mismatch for weather data provider " + this->provider, this->logVerbosity);
				this->opdid->logNormal(this->nodeID + ": Check XPath expression: " + this->xpath, this->logVerbosity);
				return;
			}
			this->opdid->logDebug(this->nodeID + ": Found weather data XML node: " + node->localName(), this->logVerbosity);
			Poco::XML::NodeList* children = node->childNodes();
			for (size_t i = 0; i < children->length(); i++) {
				if (children->item(i)->localName() != "tr")
					continue;

				Poco::XML::Node* labelNode = children->item(i)->getNodeByPath("td[@class='stats_label']");
				Poco::XML::Node* dataNode = children->item(i)->getNodeByPath("td[@class='stats_data']");

				if ((labelNode == nullptr) || (dataNode == nullptr) || (labelNode == dataNode))
					continue;

				std::string dataElement = labelNode->innerText();

				// find weather port for the data element
				auto it = this->weatherPorts.begin();
				auto ite = this->weatherPorts.end();
				while (it != ite) {
					if ((*it)->getDataElement() == dataElement) {
						this->opdid->logDebug(this->nodeID + ": Evaluating XML weather data element: " + dataElement + " with data: " + dataNode->innerText(), this->logVerbosity);
						(*it)->extract(dataNode->innerText());
					}
					++it;
				}
			}
			children->release();
		} else
		if (this->provider == "Weewx-JSON") {

			// parse content as JSON
			this->opdid->logDebug(this->nodeID + ": Parsing weather data content as JSON", this->logVerbosity);

			Poco::JSON::Parser jsonParser;
			Poco::Dynamic::Var loParsedJson = jsonParser.parse(content);
			Poco::Dynamic::Var loParsedJsonResult = jsonParser.result();
			Poco::JSON::Object::Ptr loJsonObject = loParsedJsonResult.extract<Poco::JSON::Object::Ptr>();

			// validate object
			if (loJsonObject.isNull()) {
				this->opdid->logVerbose(this->nodeID + ": Error: File format mismatch for weather data provider " + this->provider, this->logVerbosity);
				return;
			}
			std::string time = GetJSONStringValue(loJsonObject, "time");
			this->opdid->logDebug(this->nodeID + ": Found weather data; time: " + time, this->logVerbosity);

			// parse time
			int tzd = 0;
			// parse JSON date; assume MM-DD-YYYY format (possibly locale dependent?)
			Poco::DateTime jsonLocalTime = Poco::DateTimeParser::parse("%m/%d/%Y %h:%M:%S %A", time, tzd);
			Poco::DateTime jsonTime = jsonLocalTime;
			// convert to UTC for comparison
			jsonTime.makeUTC(Poco::Timezone::tzd());
			// check whether the data is considered valid (or too old); data validity in microseconds
			Poco::DateTime now;
//			this->opdid->log(this->nodeID + ": TimeDiff is: " + opdid->to_string((now - jsonTime).totalSeconds()), this->logVerbosity);
			if (now - jsonTime > this->dataValiditySeconds * 1000 * 1000) {
				this->opdid->logNormal(this->nodeID + ": Error: Weewx JSON weather data is too old; timestamp is: "
						+ Poco::DateTimeFormatter::format(jsonLocalTime, "%m/%d/%Y %h:%M:%S %A"), this->logVerbosity);
				return;
			}
			// get stats/current object
			Poco::JSON::Object::Ptr stats = GetJSONObject(loJsonObject, "stats");
			if (stats.isNull()) {
				this->opdid->logNormal(this->nodeID + ": Error: File format mismatch for weather data provider " + this->provider + "; expected element 'stats' not found", this->logVerbosity);
				return;
			}
			Poco::JSON::Object::Ptr current = GetJSONObject(stats, "current");
			if (current.isNull()) {
				this->opdid->logNormal(this->nodeID + ": Error: File format mismatch for weather data provider " + this->provider + "; expected element 'current' not found", this->logVerbosity);
				return;
			}

			// get data keys
			std::vector<std::string> keys;
			current->getNames(keys);

			for (size_t i = 0; i < keys.size(); i++) {
				std::string dataElement = keys[i];
				std::string data = GetJSONStringValue(current, dataElement);

				// find weather port for the data element
				auto it = this->weatherPorts.begin();
				auto ite = this->weatherPorts.end();
				while (it != ite) {
					if ((*it)->getDataElement() == dataElement) {
						this->opdid->logDebug(this->nodeID + ": Evaluating JSON weather data element: " + dataElement + " with data: " + data, this->logVerbosity);
						(*it)->extract(data);
					}
					++it;
				}
			}
		}

	} catch (Poco::FileNotFoundException &fnfe) {
		this->opdid->logVerbose(this->nodeID + ": The file was not found: " + fnfe.message(), this->logVerbosity);
	} catch (Poco::Net::NetException &ne) {
		this->opdid->logVerbose(this->nodeID + ": Network problem: " + ne.className() + ": " + ne.message(), this->logVerbosity);
	} catch (Poco::UnknownURISchemeException &uuse) {
		this->opdid->logVerbose(this->nodeID + ": Unknown URI scheme: " + uuse.message(), this->logVerbosity);
	} catch (Poco::Exception &e) {
		this->opdid->logVerbose(this->nodeID + ": " + e.className() + ": " + e.message(), this->logVerbosity);
	}
}

void WeatherPlugin::run(void) {
	this->opdid->logDebug(this->nodeID + ": WeatherPlugin worker thread started", this->logVerbosity);

	Poco::Net::HTTPStreamFactory::registerFactory();
	// Poco::Net::HTTPSStreamFactory::registerFactory();
	Poco::Net::FTPStreamFactory::registerFactory();

	while (!this->opdid->shutdownRequested) {
		try {
			this->refreshData();

		} catch (Poco::Exception &e) {
			this->opdid->logNormal(this->nodeID + ": Unhandled exception in worker thread: " + e.message(), this->logVerbosity);
		}

		// sleep for the specified refresh time (milliseconds)
		Poco::Thread::sleep(this->refreshTime * 1000);
	}

	this->opdid->logDebug(this->nodeID + ": WeatherPlugin worker thread terminated", this->logVerbosity);
}

// plugin instance factory function

#ifdef _WINDOWS

extern "C" __declspec(dllexport) IOPDIDPlugin* __cdecl GetOPDIDPluginInstance(int majorVersion, int minorVersion, int patchVersion)

#elif linux

extern "C" IOPDIDPlugin* GetOPDIDPluginInstance(int majorVersion, int minorVersion, int /*patchVersion*/)

#else
#error "Unable to compile plugin instance factory function: Compiler not supported"
#endif
{
	// check whether the version is supported
	if ((majorVersion > OPDID_MAJOR_VERSION) || (minorVersion > OPDID_MINOR_VERSION))
		throw Poco::Exception("This plugin supports only OPDID versions up to "
			OPDI_QUOTE(OPDID_MAJOR_VERSION) "." OPDI_QUOTE(OPDID_MINOR_VERSION));

	// return a new instance of this plugin
	return new WeatherPlugin();
}
