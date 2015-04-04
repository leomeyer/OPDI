#include <iostream>
#include <sstream>

#ifdef linux
#include <memory>
#endif

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
#include "Poco/RegularExpression.h"

#include "opdi_constants.h"
#include "opdi_platformfuncs.h"

#include "AbstractOPDID.h"

/** Interface for weather ports */
class WeatherPort {
public:
	virtual std::string getDataElement(void) = 0;

	virtual void invalidate(void) = 0;

	virtual void extract(std::string rawValue) = 0;
};

/** A WeatherGaugePort extracts a data value from the weather station's data source.
* It is represented as a DialPort internally.
* Its extract() method is fed the raw string data value. It applies the specifed regex
* to match the value into a string that is then parsed as a double.
* The resulting double is multiplied with the numerator and divided by the denominator.
* This allows to scale the value into a meaningful integer value. The resulting integer
* value is set as the position of the dial port.
*/
class WeatherGaugePort : public OPDI_DialPort, public WeatherPort {

protected:
	AbstractOPDID *opdid;

	bool isValid;
	bool lastRequestedValidState;
	std::string dataElement;
	std::string regexMatch;
	std::string regexReplace;
	std::string replaceBy;
	int numerator;
	int denominator;

	std::string rawValue;	// stores the value for in-thread processing

	Poco::FastMutex mutex;	// mutex for thread-safe accessing

public:

	WeatherGaugePort(AbstractOPDID *opdid, const char *id);

	virtual std::string getDataElement(void);

	virtual void configure(Poco::Util::AbstractConfiguration *nodeConfig);

	virtual void invalidate(void);

	virtual void extract(std::string rawValue);

	virtual uint8_t doWork(uint8_t canSend) override;

	virtual void prepare(void) override;

	virtual void getState(int32_t *position) override;
};

WeatherGaugePort::WeatherGaugePort(AbstractOPDID *opdid, const char *id) : OPDI_DialPort(id) {
	this->opdid = opdid;
	this->numerator = 1;
	this->denominator = 1;
	this->isValid = false;
	this->lastRequestedValidState = false;
}

std::string WeatherGaugePort::getDataElement(void){
	return this->dataElement;
}

void WeatherGaugePort::configure(Poco::Util::AbstractConfiguration *nodeConfig) {
	opdid->configureDialPort(nodeConfig, this);

	this->dataElement = opdid->getConfigString(nodeConfig, "DataElement", "", true);
	this->regexMatch = opdid->getConfigString(nodeConfig, "RegexMatch", "", false);
	this->regexReplace = opdid->getConfigString(nodeConfig, "RegexReplace", "", false);
	this->replaceBy = opdid->getConfigString(nodeConfig, "ReplaceBy", "", false);
	this->numerator = nodeConfig->getInt("Numerator", this->numerator);
	this->denominator = nodeConfig->getInt("Denominator", this->denominator);
	if (this->denominator == 0) 
		throw Poco::InvalidArgumentException(std::string(this->getID()) + ": The Denominator may not be 0");
}

void WeatherGaugePort::prepare(void) {
	// cannot change weather parameters on the master; so better set this to readonly
	this->setReadonly(true);
	if (this->refreshMode == OPDI_Port::REFRESH_NOT_SET)
		// automatically refresh when the value changes
		this->setRefreshMode(OPDI_Port::REFRESH_AUTO);
	OPDI_DialPort::prepare();
}

void WeatherGaugePort::invalidate(void) {
	this->mutex.lock();
	this->isValid = false;
	this->mutex.unlock();
}

void WeatherGaugePort::extract(std::string rawValue) {
	// important! Do not process on the weather plugin's thread;
	// instead, store the value for processing on the OPDID thread
	this->mutex.lock();
	this->rawValue = rawValue;
	this->mutex.unlock();
}

uint8_t WeatherGaugePort::doWork(uint8_t canSend) {
	OPDI_DialPort::doWork(canSend);

	// ensure thread safety for this block
	Poco::FastMutex::ScopedLock lock(mutex);

	std::string rawValue = this->rawValue;

	// no value to process?
	if (rawValue == "")
		return OPDI_STATUS_OK;

	// mark as invalid
	this->isValid = false;

	std::string value = rawValue;
	if (this->regexMatch != "") {
		Poco::RegularExpression regex(this->regexMatch, 0, true);
		if (this->opdid->logVerbosity >= AbstractOPDID::VERBOSE)
			this->opdid->log(std::string(this->getID()) + ": WeatherGaugePort for element " + this->dataElement + ": Matching regex against weather data: " + rawValue);
		if (regex.extract(rawValue, value, 0) == 0) {
			this->opdid->log(std::string(this->getID()) + ": WeatherGaugePort for element " + this->dataElement + ": Warning: Matching regex returned no result; weather data: " + rawValue);
		}
	}
	if (this->regexReplace != "") {
		Poco::RegularExpression regex(this->regexReplace, 0, true);
		if (regex.subst(value, this->replaceBy, Poco::RegularExpression::RE_GLOBAL) == 0) {
			if (this->opdid->logVerbosity >= AbstractOPDID::VERBOSE)
				this->opdid->log(std::string(this->getID()) + ": WeatherGaugePort for element " + this->dataElement + ": Warning: Replacement regex did not match in value: " + value);
		}
	}

	// clear the raw value
	this->rawValue = "";

	// try to parse the value as double
	double result = 0;
	try {
		result = Poco::NumberParser::parseFloat(value);
	} catch (Poco::Exception e) {
		this->opdid->log(std::string(this->getID()) + ": WeatherGaugePort for element " + this->dataElement + ": Warning: Unable to parse weather data: " + value);
		return OPDI_STATUS_OK;
	}

	// scale the result
	int newPos = (int)(result * this->numerator / this->denominator * 1.0);
	if (this->opdid->logVerbosity >= AbstractOPDID::VERBOSE)
		this->opdid->log(std::string(this->getID()) + ": WeatherGaugePort for element " + this->dataElement + ": Extracted value is: " + to_string(newPos));

	this->setPosition(newPos);

	// mark as valid
	this->isValid = true;

	// if the master's last state is invalid, and we're now valid, we should self-refresh
	// to update the master (except it's disabled)
	if (!this->lastRequestedValidState && (this->refreshMode != OPDI_Port::REFRESH_OFF))
		this->refreshRequired = true;

	return OPDI_STATUS_OK;
}

// function that fills in the current port state
void WeatherGaugePort::getState(int32_t *position) {

	// ensure thread safety for this block
	// also ensures that the doWork method and getState do not cross
	Poco::FastMutex::ScopedLock lock(mutex);

	// remember whether the master has a valid state or not
	this->lastRequestedValidState = this->isValid;

	if (!this->isValid) {
		throw PortError("Reading is not valid");
	}

	OPDI_DialPort::getState(position);
}

////////////////////////////////////////////////////////////////////////
// Plugin main class
////////////////////////////////////////////////////////////////////////

class WeatherPlugin : public IOPDIDPlugin, public IOPDIDConnectionListener, public Poco::Runnable {

protected:
	std::string nodeID;

	std::string url;
	int timeoutSeconds;

	// weather data provider; at the moment only "weewx" is supported
	std::string provider;

	std::string xpath;

	int refreshTimeMs;

	std::string sid;

	Poco::Thread workThread;

	typedef std::vector<WeatherPort *> WeatherPortList;
	WeatherPortList weatherPorts;

public:
	AbstractOPDID *opdid;

	virtual void masterConnected(void) override;
	virtual void masterDisconnected(void) override;

	virtual void refreshData(void);

	virtual void run(void);

	virtual void setupPlugin(AbstractOPDID *abstractOPDID, std::string node, Poco::Util::AbstractConfiguration *nodeConfig) override;
};


void WeatherPlugin::setupPlugin(AbstractOPDID *abstractOPDID, std::string node, Poco::Util::AbstractConfiguration *nodeConfig) {
	this->opdid = abstractOPDID;
	this->nodeID = node;
	this->timeoutSeconds = 10;

	this->url = abstractOPDID->getConfigString(nodeConfig, "Url", "", true);
	this->provider = abstractOPDID->getConfigString(nodeConfig, "Provider", "", true);

	if (this->provider == "Weewx") {
		// configure xpath for default skin
		this->xpath = nodeConfig->getString("XPath", "html/body/div/div[@id='stats_group']/div/table/tbody");
	} else 
		throw Poco::DataException(node + ": Provider not supported: " + this->provider);

	this->timeoutSeconds = nodeConfig->getInt("Timeout", this->timeoutSeconds);

	// store main node's group (will become the default of ports)
	std::string group = nodeConfig->getString("Group", "");
	this->refreshTimeMs = nodeConfig->getInt("RefreshTime", 10000);
	if (this->refreshTimeMs < 0)
		throw Poco::DataException(node + "Please specify a non-negative RefreshTime in milliseconds");

	// enumerate keys of the plugin's nodes (in specified order)
	if (this->opdid->logVerbosity >= AbstractOPDID::VERBOSE)
		this->opdid->log("Enumerating Weather nodes: " + node + ".Nodes");

	Poco::Util::AbstractConfiguration *nodes = nodeConfig->createView("Nodes");

	// get ordered list of ports
	Poco::Util::AbstractConfiguration::Keys portKeys;
	nodes->keys("", portKeys);

	typedef Poco::Tuple<int, std::string> Item;
	typedef std::vector<Item> ItemList;
	ItemList orderedItems;

	// create ordered list of port keys (by priority)
	for (Poco::Util::AbstractConfiguration::Keys::const_iterator it = portKeys.begin(); it != portKeys.end(); ++it) {

		int itemNumber = nodes->getInt(*it, 0);
		// check whether the item is active
		if (itemNumber < 0)
			continue;

		// insert at the correct position to create a sorted list of items
		ItemList::iterator nli = orderedItems.begin();
		while (nli != orderedItems.end()) {
			if (nli->get<0>() > itemNumber)
				break;
			nli++;
		}
		Item item(itemNumber, *it);
		orderedItems.insert(nli, item);
	}

	if (orderedItems.size() == 0) {
		this->opdid->log("Warning: No ports configured in node " + node + ".Nodes; is this intended?");
	}

	// go through items, create ports in specified order
	ItemList::const_iterator nli = orderedItems.begin();
	while (nli != orderedItems.end()) {

		std::string nodeName = nli->get<1>();

		if (this->opdid->logVerbosity >= AbstractOPDID::VERBOSE)
			this->opdid->log("Setting up Weather port(s) for node: " + nodeName);

		// get port section from the configuration
		Poco::Util::AbstractConfiguration *portConfig = abstractOPDID->getConfiguration()->createView(nodeName);

		// get port type (required)
		std::string portType = abstractOPDID->getConfigString(portConfig, "Type", "", true);

		if (portType == "WeatherGaugePort") {

			WeatherGaugePort *port = new WeatherGaugePort(this->opdid, nodeName.c_str());
			port->setGroup(group);
			port->configure(portConfig);
			opdid->addPort(port);
			// add port to internal list
			this->weatherPorts.push_back(port);
		} else
			throw Poco::DataException("This plugin does not support the port type", portType);

		nli++;
	}

	this->opdid->addConnectionListener(this);

	this->workThread.setName(node + " work thread");
	this->workThread.start(*this);

	if (this->opdid->logVerbosity >= AbstractOPDID::VERBOSE)
		this->opdid->log(this->nodeID + ": FritzBoxPlugin setup completed successfully");
}

void WeatherPlugin::masterConnected() {
	// when the master connects, login to the FritzBox
	//this->queue.enqueueNotification(new ActionNotification(ActionNotification::LOGIN, NULL));
}

void WeatherPlugin::masterDisconnected() {
	// when the master connects, logout from the FritzBox
	//this->queue.enqueueNotification(new ActionNotification(ActionNotification::LOGOUT, NULL));
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

void WeatherPlugin::refreshData(void) {
	try {
		// invalidate all ports
		WeatherPortList::iterator it = this->weatherPorts.begin();
		while (it != this->weatherPorts.end()) {
			(*it)->invalidate();
			it++;
		}

		if (this->opdid->logVerbosity >= AbstractOPDID::VERBOSE)
			this->opdid->log(this->nodeID + ": Fetching content of URL: " + this->url);

		Poco::URI uri(this->url);
		std::auto_ptr<std::istream> pStr(Poco::URIStreamOpener::defaultOpener().open(uri));

		// save stream content
		std::string content;
		Poco::StreamCopier::copyToString(*pStr.get(), content); 

		if (this->provider == "Weewx") {

			// parse content as XML
			// cleanup structure errors in Weewx default skin
			replaceAll(content, "select NAME=noaaselect", "select name=\"noaaselect\"");
			replaceAll(content, "option selected", "option selected=\"1\"");

			if (this->opdid->logVerbosity >= AbstractOPDID::VERBOSE)
				this->opdid->log(this->nodeID + ": Parsing weather data content as XML");

			std::stringstream in(content);
			Poco::XML::InputSource src(in);
			Poco::XML::DOMParser parser;
			Poco::AutoPtr<Poco::XML::Document> pDoc = parser.parse(&src);

			Poco::XML::Node *node;
			
			node = pDoc->getNodeByPath(this->xpath);
			if (node == NULL) {
				this->opdid->log(this->nodeID + ": Error: File format mismatch for weather data provider " + this->provider);
				this->opdid->log(this->nodeID + ": Check XPath expression: " + this->xpath);
				return;
			}
			if (this->opdid->logVerbosity >= AbstractOPDID::VERBOSE)
				this->opdid->log(this->nodeID + ": Found weather data XML node: " + node->localName());
			Poco::XML::NodeList *children = node->childNodes();
			for (size_t i = 0; i < children->length(); i++) {
				if (children->item(i)->localName() != "tr")
					continue;

				Poco::XML::Node *labelNode = children->item(i)->getNodeByPath("td[@class='stats_label']");
				Poco::XML::Node *dataNode = children->item(i)->getNodeByPath("td[@class='stats_data']");

				if ((labelNode == NULL) || (dataNode == NULL) || (labelNode == dataNode))
					continue;

				std::string dataElement = labelNode->innerText();

				// find weather port for the data element
				WeatherPortList::iterator it = this->weatherPorts.begin();
				while (it != this->weatherPorts.end()) {
					if ((*it)->getDataElement() == dataElement) {
						if (this->opdid->logVerbosity >= AbstractOPDID::VERBOSE)
							this->opdid->log(this->nodeID + ": Evaluating XML weather data element: " + dataElement);
						(*it)->extract(dataNode->innerText());
					}
					it++;
				}
			}
			children->release();
		}

	} catch (Poco::FileNotFoundException &fnfe) {
		this->opdid->log(this->nodeID + ": The file was not found: " + fnfe.message());
	} catch (Poco::Net::NetException &ne) {
		this->opdid->log(this->nodeID + ": Network problem: " + ne.className() + ": " + ne.message());
	} catch (Poco::UnknownURISchemeException &uuse) {
		this->opdid->log(this->nodeID + ": Unknown URI scheme: " + uuse.message());
	} catch (Poco::Exception &e) {
		this->opdid->log(this->nodeID + ": " + e.className() + ": " + e.message());
	}
}

void WeatherPlugin::run(void) {
	if (this->opdid->logVerbosity >= AbstractOPDID::VERBOSE)
		this->opdid->log(this->nodeID + ": WeatherPlugin worker thread started");

	Poco::Net::HTTPStreamFactory::registerFactory();  
	// Poco::Net::HTTPSStreamFactory::registerFactory();  
	Poco::Net::FTPStreamFactory::registerFactory();

	while (!this->opdid->shutdownRequested) {
		try {
			this->refreshData();

		} catch (Poco::Exception &e) {
			this->opdid->log(this->nodeID + ": Unhandled exception in worker thread: " + e.message());
		}

		// sleep for the specified refresh time
		Poco::Thread::sleep(this->refreshTimeMs);
	}

	if (this->opdid->logVerbosity >= AbstractOPDID::VERBOSE)
		this->opdid->log(this->nodeID + ": WeatherPlugin worker thread terminated");
}

// plugin instance factory function

#ifdef _WINDOWS

extern "C" __declspec(dllexport) IOPDIDPlugin* __cdecl GetOPDIDPluginInstance(int majorVersion, int minorVersion, int patchVersion) {

#elif linux

extern "C" IOPDIDPlugin* GetOPDIDPluginInstance(int majorVersion, int minorVersion, int patchVersion) {

#endif

	// check whether the version is supported
	if ((majorVersion > 0) || (minorVersion > 1))
		throw Poco::Exception("This version of the WeatherPlugin supports only OPDID versions up to 0.1");

	// return a new instance of this plugin
	return new WeatherPlugin();
}
