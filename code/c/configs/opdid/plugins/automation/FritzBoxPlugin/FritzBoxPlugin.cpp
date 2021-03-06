#include <sstream>

#include "Poco/Tuple.h"
#include "Poco/Runnable.h"
#include <Poco/Net/HTTPClientSession.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/StreamCopier.h>
#include <Poco/Path.h>
#include <Poco/URI.h>
#include <Poco/Exception.h>
#include "Poco/DOM/DOMParser.h"
#include "Poco/DOM/Document.h"
#include "Poco/DOM/NodeIterator.h"
#include "Poco/DOM/NodeFilter.h"
#include "Poco/DOM/AutoPtr.h"
#include "Poco/SAX/InputSource.h"
#include "Poco/DigestStream.h"
#include "Poco/MD5Engine.h"
#include "Poco/UTF16Encoding.h"
#include "Poco/UnicodeConverter.h"
#include "Poco/Notification.h"
#include "Poco/NotificationQueue.h"
#include "Poco/AutoPtr.h"
#include "Poco/NumberParser.h"

#include "opdi_constants.h"
#include "opdi_platformfuncs.h"

#include "AbstractOPDID.h"

#define INVALID_SID		"0000000000000000"

namespace {

class FritzBoxPlugin;

class FritzPort  {
	std::string id;
public:
	FritzPort(std::string id) : id(id) {};

	virtual void query() = 0;
};

class ActionNotification : public Poco::Notification {
public:
	typedef Poco::AutoPtr<ActionNotification> Ptr;

	enum ActionType {
		LOGIN,
		LOGOUT,
		GETSWITCHSTATE,
		SETSWITCHSTATEHIGH,
		SETSWITCHSTATELOW,
		GETSWITCHPOWER,
		GETSWITCHENERGY
	};

	ActionType type;
	FritzPort* port;

	ActionNotification(ActionType type, FritzPort* port);
};

ActionNotification::ActionNotification(ActionType type, FritzPort* port) {
	this->type = type;
	this->port = port;
}

////////////////////////////////////////////////////////////////////////
// Plugin main class
////////////////////////////////////////////////////////////////////////

class FritzBoxPlugin : public IOPDIDPlugin, public opdid::IOPDIDConnectionListener, public Poco::Runnable {
	friend class FritzDECT200Switch;
	friend class FritzDECT200Power;
	friend class FritzDECT200Energy;

protected:
	std::string nodeID;

	std::string host;
	int port;
	std::string user;
	std::string password;
	int timeoutSeconds;

	std::string sid;
	int errorCount;
	std::string lastErrorMessage;

	opdi::LogVerbosity logVerbosity;

	typedef std::vector<FritzPort*> FritzPorts;
	FritzPorts fritzPorts;

	Poco::NotificationQueue queue;

	Poco::Thread workThread;

	void errorOccurred(const std::string& message);

public:
	opdid::AbstractOPDID* opdid;

	virtual std::string httpGet(const std::string& url);

	virtual std::string getResponse(const std::string& challenge, const std::string& password);

	virtual std::string getSessionID(const std::string& user, const std::string& password);

	virtual std::string getXMLValue(const std::string& xml, const std::string& node);

	virtual void login(void);

	virtual void checkLogin(void);

	virtual void logout(void);

	virtual void getSwitchState(FritzPort* port);

	virtual void setSwitchState(FritzPort* port, uint8_t line);

	virtual void getSwitchEnergy(FritzPort* port);

	virtual void getSwitchPower(FritzPort* port);

	virtual void masterConnected(void) override;
	virtual void masterDisconnected(void) override;

	virtual void run(void);

	virtual void setupPlugin(opdid::AbstractOPDID* abstractOPDID, const std::string& node, Poco::Util::AbstractConfiguration* nodeConfig) override;
};

////////////////////////////////////////////////////////////////////////
// Plugin ports
////////////////////////////////////////////////////////////////////////

class FritzDECT200Switch : public opdi::DigitalPort, public FritzPort {
	friend class FritzBoxPlugin;
protected:

	FritzBoxPlugin* plugin;
	std::string ain;

	int8_t switchState;

	virtual void setSwitchState(int8_t line);

public:
	FritzDECT200Switch(FritzBoxPlugin* plugin, const char* id);

	virtual void configure(Poco::Util::AbstractConfiguration* portConfig);

	virtual void query() override;

	virtual void setLine(uint8_t line, ChangeSource changeSource = opdi::Port::ChangeSource::CHANGESOURCE_INT) override;

	virtual void getState(uint8_t* mode, uint8_t* line) const override;
};

class FritzDECT200Power : public opdi::DialPort, public FritzPort {
	friend class FritzBoxPlugin;
protected:

	FritzBoxPlugin* plugin;
	std::string ain;

	int32_t power;

	virtual void setPower(int32_t power);

public:
	FritzDECT200Power(FritzBoxPlugin* plugin, const char* id);

	virtual void configure(Poco::Util::AbstractConfiguration* portConfig);

	virtual void query() override;

	virtual void getState(int64_t* position) const override;

	virtual void doRefresh(void) override;
};


class FritzDECT200Energy : public opdi::DialPort, public FritzPort {
	friend class FritzBoxPlugin;
protected:

	FritzBoxPlugin* plugin;
	std::string ain;

	int32_t energy;

	virtual void setEnergy(int32_t energy);

public:
	FritzDECT200Energy(FritzBoxPlugin* plugin, const char* id);

	virtual void configure(Poco::Util::AbstractConfiguration* portConfig);

	virtual void query() override;

	virtual void getState(int64_t* position) const override;

	virtual void doRefresh(void) override;
};

}	// end anonymous namespace

FritzDECT200Switch::FritzDECT200Switch(FritzBoxPlugin* plugin, const char* id) : opdi::DigitalPort(id), FritzPort(id) {
	this->plugin = plugin;
	this->switchState = -1;	// unknown
	this->refreshMode =RefreshMode::REFRESH_PERIODIC;

	// output only
	this->setDirCaps(OPDI_PORTDIRCAP_OUTPUT);
	this->setIcon("powersocket");
}

void FritzDECT200Switch::configure(Poco::Util::AbstractConfiguration* portConfig) {
	this->plugin->opdid->configureDigitalPort(portConfig, this);

	// get actor identification number (required)
	this->ain = plugin->opdid->getConfigString(portConfig, this->ID(), "AIN", "", true);
	std::string group = plugin->opdid->getConfigString(portConfig, this->ID(), "Group", "", false);
	if (group != "") {
		this->setGroup(group);
	}
}

void FritzDECT200Switch::query() {
	this->plugin->queue.enqueueNotification(new ActionNotification(ActionNotification::GETSWITCHSTATE, this));
}

void FritzDECT200Switch::setLine(uint8_t line, ChangeSource /*changeSource*/) {
	if (this->line == line)
		return;

	if (line == 0)
		this->plugin->queue.enqueueNotification(new ActionNotification(ActionNotification::SETSWITCHSTATELOW, this));
	else
	if (line == 1)
		this->plugin->queue.enqueueNotification(new ActionNotification(ActionNotification::SETSWITCHSTATEHIGH, this));
}

void FritzDECT200Switch::getState(uint8_t* mode, uint8_t* line) const {
	if (this->switchState < 0) {
		// this->plugin->nodeID + "/" + this->id + " (" + this->ain + ") : 
		throw PortError(this->ID() + ": The switch state is unknown");
	}
	opdi::DigitalPort::getState(mode, line);
}

void FritzDECT200Switch::setSwitchState(int8_t line) {
	this->setError(Error::VALUE_OK);

	if (this->switchState == line)
		return;

	this->switchState = line;
	if ((line == 0) || (line == 1))
		opdi::DigitalPort::setLine(line);
	else
		this->refreshRequired = true;
}

FritzDECT200Power::FritzDECT200Power(FritzBoxPlugin* plugin, const char* id) : opdi::DialPort(id), FritzPort(id) {
	this->plugin = plugin;
	this->power = -1;	// unknown
	this->refreshMode =RefreshMode::REFRESH_PERIODIC;

	this->minValue = 0;
	this->maxValue = 2300000;	// measured in mW; 2300 W is maximum power load for the DECT200
	this->step = 1;
	this->position = 0;

	// port is readonly
	this->setReadonly(true);
}

void FritzDECT200Power::configure(Poco::Util::AbstractConfiguration* portConfig) {
	// get actor identification number (required)
	this->ain = plugin->opdid->getConfigString(portConfig, this->ID(), "AIN", "", true);

	// setting PowerHidden takes precedende
	if (portConfig->has("PowerHidden"))
		this->setHidden(portConfig->getBool("PowerHidden", false));
	else
		this->setHidden(portConfig->getBool("Hidden", false));

	// the default label is the port ID
	std::string portLabel = portConfig->getString("PowerLabel", this->getID());
	this->setLabel(portLabel.c_str());

	int flags = portConfig->getInt("PowerFlags", -1);
	if (flags >= 0) {
		this->setFlags(flags);
	}

	int time = portConfig->getInt("PowerRefreshTime", portConfig->getInt("RefreshTime", 30000));
	if (time >= 0) {
		this->setPeriodicRefreshTime(time);
	} else {
		throw Poco::DataException(this->ID() + ": A PowerRefreshTime > 0 must be specified: " + to_string(time));
	}

	// extended properties
	std::string unit = portConfig->getString("PowerUnit", "");
	if (unit != "") {
		this->setUnit(unit);
	}
	std::string group = plugin->opdid->getConfigString(portConfig, this->ID(), "PowerGroup", portConfig->getString("Group", ""), false);
	if (group != "") {
		this->setGroup(group);
	}
}

void FritzDECT200Power::query() {
	this->plugin->queue.enqueueNotification(new ActionNotification(ActionNotification::GETSWITCHPOWER, this));
}

void FritzDECT200Power::getState(int64_t* position) const {
	if (this->power < 0) {
		throw PortError(this->ID() + ": The power state is unknown");
	}
	opdi::DialPort::getState(position);
}

void FritzDECT200Power::setPower(int32_t power) {
	this->setError(Error::VALUE_OK);

	if (this->power == power)
		return;

	this->power = power;
	if (power > -1)
		opdi::DialPort::setPosition(power);

	this->refreshRequired = true;
}

void FritzDECT200Power::doRefresh(void) {
	// cause a query to be performed before actually refreshing
	this->query();
}


FritzDECT200Energy::FritzDECT200Energy(FritzBoxPlugin* plugin, const char* id) : opdi::DialPort(id), FritzPort(id) {
	this->plugin = plugin;
	this->energy = -1;	// unknown
	this->refreshMode =RefreshMode::REFRESH_PERIODIC;

	this->minValue = 0;
	this->maxValue = 2147483647;	// measured in Wh
	this->step = 1;
	this->position = 0;

	// port is readonly
	this->setReadonly(true);
}

void FritzDECT200Energy::configure(Poco::Util::AbstractConfiguration* portConfig) {
	// get actor identification number (required)
	this->ain = plugin->opdid->getConfigString(portConfig, this->ID(), "AIN", "", true);

	// setting EnergyHidden takes precedende
	if (portConfig->has("EnergyHidden"))
		this->setHidden(portConfig->getBool("EnergyHidden", false));
	else
		this->setHidden(portConfig->getBool("Hidden", false));

	// the default label is the port ID
	std::string portLabel = portConfig->getString("EnergyLabel", this->getID());
	this->setLabel(portLabel.c_str());

	int flags = portConfig->getInt("EnergyFlags", -1);
	if (flags >= 0) {
		this->setFlags(flags);
	}

	int time = portConfig->getInt("EnergyRefreshTime", portConfig->getInt("RefreshTime", 30000));
	if (time >= 0) {
		this->setPeriodicRefreshTime(time);
	} else {
		throw Poco::DataException(this->ID() + ": An EnergyRefreshTime > 0 must be specified: " + to_string(time));
	}

	// extended properties
	std::string unit = portConfig->getString("EnergyUnit", "");
	if (unit != "") {
		this->setUnit(unit);
	}
	std::string group = plugin->opdid->getConfigString(portConfig, this->ID(), "EnergyGroup", portConfig->getString("Group", ""), false);
	if (group != "") {
		this->setGroup(group);
	}
}

void FritzDECT200Energy::query() {
	this->plugin->queue.enqueueNotification(new ActionNotification(ActionNotification::GETSWITCHENERGY, this));
}

void FritzDECT200Energy::getState(int64_t* position) const {
	if (this->energy < 0) {
		throw PortError(this->ID() + ": The energy state is unknown");
	}
	opdi::DialPort::getState(position);
}

void FritzDECT200Energy::setEnergy(int32_t energy) {
	this->setError(Error::VALUE_OK);

	if (this->energy == energy)
		return;

	this->energy = energy;
	if (energy > -1)
		opdi::DialPort::setPosition(energy);

	this->refreshRequired = true;
}

void FritzDECT200Energy::doRefresh(void) {
	// cause a query to be performed before actually refreshing
	this->query();
}

////////////////////////////////////////////////////////
// Plugin implementation
////////////////////////////////////////////////////////

// Credits: Sir Slick, https://stackoverflow.com/questions/2589096/find-most-significant-bit-left-most-that-is-set-in-a-bit-array
unsigned int msb32(unsigned int x)
{
	static const unsigned int bval[] =
	{ 0,1,2,2,3,3,3,3,4,4,4,4,4,4,4,4 };

	unsigned int r = 0;
	if (x & 0xFFFF0000) { r += 16 / 1; x >>= 16 / 1; }
	if (x & 0x0000FF00) { r += 16 / 2; x >>= 16 / 2; }
	if (x & 0x000000F0) { r += 16 / 4; x >>= 16 / 4; }
	return r + bval[x];
}

void FritzBoxPlugin::errorOccurred(const std::string& message) {
	// this method is for errors that are usually logged in verbosity Normal
	// identical error message?
	if (this->lastErrorMessage == message) {
		this->errorCount++;
	} else {
		// different error, reset counter
		this->errorCount = 1;
		this->lastErrorMessage = message;
	}
	// the error message is output if only the highest bit is set causing exponentially decreasing output frequency
	// get highest bit
	unsigned int msb = msb32(this->errorCount);
	// are the lower bits zero?
	if ((this->errorCount & ((1 << (msb - 1)) - 1)) == 0) {
		// add occurrence count if larger than 1
		this->opdid->logNormal((this->errorCount > 1 ? "(" + this->opdid->to_string(this->errorCount) + ") " : "") + message, this->logVerbosity);
	}
}

std::string FritzBoxPlugin::httpGet(const std::string& url) {
	Poco::URI uri(std::string("http://") + this->host + ":" + this->opdid->to_string(this->port) + url);
	try {
		// prepare session
		Poco::Net::HTTPClientSession session(uri.getHost(), uri.getPort());
		session.setTimeout(Poco::Timespan(this->timeoutSeconds, 0));

		// prepare path
		std::string path(uri.getPathAndQuery());
		if (path.empty()) path = "/";

		this->opdid->logDebug(this->nodeID + ": HTTP GET: " + uri.toString(), this->logVerbosity);

		// send request
		Poco::Net::HTTPRequest req(Poco::Net::HTTPRequest::HTTP_GET, path, Poco::Net::HTTPMessage::HTTP_1_1);
		session.sendRequest(req);

		// get response
		Poco::Net::HTTPResponse res;

		// print response
		std::istream &is = session.receiveResponse(res);

		if (res.getStatus() != 200) {
			this->errorOccurred(this->nodeID + ": HTTP GET: " + uri.toString() + ": The server returned an error: " + this->opdid->to_string(res.getStatus()) + " " + res.getReason());
			return "";
		} else
			this->opdid->logDebug(this->nodeID + ": HTTP Response: " + this->opdid->to_string(res.getStatus()) + " " + res.getReason(), this->logVerbosity);

		std::stringstream ss;

		Poco::StreamCopier::copyStream(is, ss);

		if (this->errorCount > 0) {
			this->opdid->logNormal(this->nodeID + ": HTTP GET successful: " + uri.toString(), this->logVerbosity);
			this->errorCount = 0;
		} else
			this->opdid->logDebug(this->nodeID + ": HTTP GET successful: " + uri.toString(), this->logVerbosity);

		return ss.str().erase(ss.str().find_last_not_of("\n") + 1);
	} catch (Poco::Exception &e) {
		this->errorOccurred(this->nodeID + ": Error during HTTP GET for " + uri.toString() + ": " + e.message());
	}

	return "";
}

std::string FritzBoxPlugin::getXMLValue(const std::string& xml, const std::string& node) {
	try {
		std::stringstream in(xml);
		Poco::XML::InputSource src(in);
		Poco::XML::DOMParser parser;
		Poco::AutoPtr<Poco::XML::Document> pDoc = parser.parse(&src);
		Poco::XML::NodeIterator it(pDoc, Poco::XML::NodeFilter::SHOW_ALL);
		Poco::XML::Node* pNode = it.nextNode();
		while (pNode)
		{
			if ((pNode->nodeName() == node) && (pNode->firstChild() != nullptr))
				return pNode->firstChild()->nodeValue();
			pNode = it.nextNode();
		}
	} catch (Poco::Exception &e) {
		throw Poco::Exception("Error parsing XML", e);
	}

	throw Poco::NotFoundException("Node or value not found in XML: " + node);
}

std::string FritzBoxPlugin::getResponse(const std::string& challenge, const std::string& password) {
	Poco::MD5Engine md5;
	// password is UTF8 internally; need to convert to UTF16LE
	std::string toHash = challenge + "-" + password;
	std::wstring toHashUTF16;
	Poco::UnicodeConverter::toUTF16(toHash, toHashUTF16);
	// replace chars > 255 with a dot (compatibility reasons)
	// convert to 16 bit types (can't use wstring directly because on Linux it's more than 16 bit wide)
	std::vector<uint16_t> charVec;
	wchar_t* c = (wchar_t*)&toHashUTF16.c_str()[0];
	while (*c) {
		if (*c > 255)
			*c = '.';
		charVec.push_back((uint16_t)*c);
		c++;
	}
	// twice the string length (each char has 2 bytes)
	md5.update(charVec.data(), charVec.size() * 2);
	const Poco::DigestEngine::Digest& digest = md5.digest(); // obtain result
	return challenge + "-" + Poco::DigestEngine::digestToHex(digest);
}

std::string FritzBoxPlugin::getSessionID(const std::string& user, const std::string& password) {

	std::string loginPage = this->httpGet("/login_sid.lua?sid=" + this->sid);
	if (loginPage.empty())
		return INVALID_SID;
	std::string sid = getXMLValue(loginPage, "SID");
	if (sid == INVALID_SID) {
		std::string challenge = getXMLValue(loginPage, "Challenge");
		std::string uri = "/login_sid.lua?username=" + user + "&response=" + this->getResponse(challenge, password);
		loginPage = this->httpGet(uri);
		if (loginPage.empty())
			return INVALID_SID;
		sid = getXMLValue(loginPage,  "SID");
	}
	return sid;
}

void FritzBoxPlugin::login(void) {
	this->opdid->logDebug(this->nodeID + ": Attempting to login to FritzBox " + this->host + " with user " + this->user, this->logVerbosity);

	this->sid = this->getSessionID(this->user, this->password);

	if (sid == INVALID_SID) {
		this->errorOccurred(this->nodeID + ": Login to FritzBox " + this->host + " with user " + this->user + " failed");
		return;
	}
	if (this->errorCount > 0) {
		this->opdid->logNormal(this->nodeID + ": Login to FritzBox " + this->host + " with user " + this->user + " successful; sid = " + this->sid, this->logVerbosity);
		this->errorCount = 0;
	} else
		this->opdid->logDebug(this->nodeID + ": Login to FritzBox " + this->host + " with user " + this->user + " successful; sid = " + this->sid, this->logVerbosity);

	// query ports (post notifications to query)
	auto it = this->fritzPorts.begin();
	auto ite = this->fritzPorts.end();
	while (it != ite) {
		(*it)->query();
		++it;
	}
}

void FritzBoxPlugin::checkLogin(void) {
	// check whether we're currently logged in
	std::string loginPage = this->httpGet("/login_sid.lua?sid=" + this->sid);
	// error case
	if (loginPage.empty()) {
		this->sid = INVALID_SID;
		return;
	}
	this->sid = getXMLValue(loginPage, "SID");
	if (sid == INVALID_SID) {
		// try to log in
		this->login();
	}
}

void FritzBoxPlugin::logout(void) {
	// not connected?
	if (this->sid == INVALID_SID)
		return;

	this->opdid->logDebug(this->nodeID + ": Logout from FritzBox " + this->host, this->logVerbosity);

	this->httpGet("/login_sid.lua?logout=true&sid=" + this->sid);

	this->sid = INVALID_SID;
}

void FritzBoxPlugin::getSwitchState(FritzPort* port) {
	// port must be a DECT 200 switch port
	FritzDECT200Switch* switchPort = (FritzDECT200Switch*)port;

	this->checkLogin();
	// problem?
	if (this->sid == INVALID_SID) {
		switchPort->setError(opdi::Port::Error::VALUE_NOT_AVAILABLE);
		return;
	}

	// query the FritzBox
	std::string result = httpGet("/webservices/homeautoswitch.lua?ain=" + switchPort->ain + "&switchcmd=getswitchstate&sid=" + this->sid);
	// problem?
	if (result.empty()) {
		switchPort->setError(opdi::Port::Error::VALUE_NOT_AVAILABLE);
		return;
	}

	if (result == "1") {
		switchPort->setSwitchState(1);
	} else
	if (result == "0") {
		switchPort->setSwitchState(0);
	} else
		switchPort->setSwitchState(-1);
}

void FritzBoxPlugin::setSwitchState(FritzPort* port, uint8_t line) {
	// port must be a DECT 200 switch port
	FritzDECT200Switch* switchPort = (FritzDECT200Switch*)port;

	this->checkLogin();
	// problem?
	if (this->sid == INVALID_SID) {
		switchPort->setError(opdi::Port::Error::VALUE_NOT_AVAILABLE);
		return;
	}

	std::string cmd = (line == 1 ? "setswitchon" : "setswitchoff");
	std::string result = this->httpGet("/webservices/homeautoswitch.lua?ain=" + switchPort->ain + "&switchcmd=" + cmd + "&sid=" + this->sid);
	// problem?
	if (result.empty()) {
		switchPort->setError(opdi::Port::Error::VALUE_NOT_AVAILABLE);
		return;
	}

	if (result == "1") {
		switchPort->setSwitchState(1);
	} else
	if (result == "0") {
		switchPort->setSwitchState(0);
	} else
		switchPort->setSwitchState(-1);
}

void FritzBoxPlugin::getSwitchEnergy(FritzPort* port) {
	// port must be a DECT 200 energy port
	FritzDECT200Energy* energyPort = (FritzDECT200Energy*)port;

	this->checkLogin();

	// problem?
	if (this->sid == INVALID_SID) {
		energyPort->setError(opdi::Port::Error::VALUE_NOT_AVAILABLE);
		return;
	}

	// query the FritzBox
	std::string result = httpGet("/webservices/homeautoswitch.lua?ain=" + energyPort->ain + "&switchcmd=getswitchenergy&sid=" + this->sid);
	// problem?
	if (result.empty()) {
		energyPort->setError(opdi::Port::Error::VALUE_NOT_AVAILABLE);
		return;
	}

	// parse result
	int energy = -1;
	Poco::NumberParser::tryParse(result, energy);

	energyPort->setEnergy(energy);
}

void FritzBoxPlugin::getSwitchPower(FritzPort* port) {
	// port must be a DECT 200 power port
	FritzDECT200Power* powerPort = (FritzDECT200Power*)port;

	this->checkLogin();
	// problem?
	if (this->sid == INVALID_SID) {
		powerPort->setError(opdi::Port::Error::VALUE_NOT_AVAILABLE);
		return;
	}

	// query the FritzBox
	std::string result = httpGet("/webservices/homeautoswitch.lua?ain=" + powerPort->ain + "&switchcmd=getswitchpower&sid=" + this->sid);
	// problem?
	if (result.empty()) {
		powerPort->setError(opdi::Port::Error::VALUE_NOT_AVAILABLE);
		return;
	}

	// parse result
	int power = -1;
	Poco::NumberParser::tryParse(result, power);

	powerPort->setPower(power);
}

void FritzBoxPlugin::setupPlugin(opdid::AbstractOPDID* abstractOPDID, const std::string& node, Poco::Util::AbstractConfiguration* config) {
	this->opdid = abstractOPDID;
	this->nodeID = node;
	this->sid = INVALID_SID;			// default; means not connected
	this->timeoutSeconds = 2;			// short timeout (assume local network)

	this->errorCount = 0;

	Poco::AutoPtr<Poco::Util::AbstractConfiguration> nodeConfig = config->createView(node);

	// test case for response calculation (see documentation: http://avm.de/fileadmin/user_upload/Global/Service/Schnittstellen/AVM_Technical_Note_-_Session_ID.pdf)
	// abstractOPDID->log("Test Response: " + this->getResponse("1234567z", "äbc"));

	// get host and credentials
	this->host = abstractOPDID->getConfigString(nodeConfig, node, "Host", "", true);
	this->port = nodeConfig->getInt("Port", 80);
	this->user = abstractOPDID->getConfigString(nodeConfig, node, "User", "", true);
	this->password = abstractOPDID->getConfigString(nodeConfig, node, "Password", "", true);
	this->timeoutSeconds = nodeConfig->getInt("Timeout", this->timeoutSeconds);

	// store main node's group (will become the default of ports)
	std::string group = nodeConfig->getString("Group", "");

	// store main node's verbosity level (will become the default of ports)
	this->logVerbosity = this->opdid->getConfigLogVerbosity(config, opdi::LogVerbosity::UNKNOWN);

	// enumerate keys of the plugin's nodes (in specified order)
	this->opdid->logVerbose("Enumerating FritzBox devices: " + node + ".Devices", this->logVerbosity);

	Poco::AutoPtr<Poco::Util::AbstractConfiguration> nodes = config->createView(node + ".Devices");

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
		this->opdid->logWarning("No devices configured in " + node + ".Devices; is this intended?");
	}

	// go through items, create ports in specified order
	auto nli = orderedItems.begin();
	auto nlie = orderedItems.end();
	while (nli != nlie) {

		std::string nodeName = nli->get<1>();

		this->opdid->logVerbose("Setting up FritzBox port(s) for device: " + nodeName, this->logVerbosity);

		// get port section from the configuration
		Poco::Util::AbstractConfiguration* portConfig = config->createView(nodeName);

		// get port type (required)
		std::string portType = abstractOPDID->getConfigString(portConfig, nodeName, "Type", "", true);

		if (portType == "FritzDECT200") {
			// setup the switch port instance and add it
			FritzDECT200Switch* switchPort = new FritzDECT200Switch(this, nodeName.c_str());
			// set default group: FritzBox's node's group
			switchPort->setGroup(group);
			switchPort->configure(portConfig);
			switchPort->logVerbosity = this->opdid->getConfigLogVerbosity(config, this->logVerbosity);
			abstractOPDID->addPort(switchPort);
			// remember port in plugin
			this->fritzPorts.push_back(switchPort);

			// setup the energy port instance and add it
			FritzDECT200Energy* energyPort = new FritzDECT200Energy(this, (nodeName + "Energy").c_str());
			// set default group: FritzBox's node's group
			energyPort->setGroup(group);
			energyPort->configure(portConfig);
			energyPort->logVerbosity = this->opdid->getConfigLogVerbosity(config, this->logVerbosity);
			abstractOPDID->addPort(energyPort);
			// remember port in plugin
			this->fritzPorts.push_back(energyPort);

			// setup the power port instance and add it
			FritzDECT200Power* powerPort = new FritzDECT200Power(this, (nodeName + "Power").c_str());
			// set default group: FritzBox's node's group
			powerPort->setGroup(group);
			powerPort->configure(portConfig);
			powerPort->logVerbosity = this->opdid->getConfigLogVerbosity(config, this->logVerbosity);
			abstractOPDID->addPort(powerPort);
			// remember port in plugin
			this->fritzPorts.push_back(powerPort);

		} else
			throw Poco::DataException("This plugin does not support the port type", portType);

		++nli;
	}

	// this->opdid->addConnectionListener(this);

	this->workThread.setName(node + " work thread");
	this->workThread.start(*this);

	this->opdid->logVerbose(this->nodeID + ": FritzBoxPlugin setup completed successfully", this->logVerbosity);
}

void FritzBoxPlugin::masterConnected() {
}

void FritzBoxPlugin::masterDisconnected() {
}

void FritzBoxPlugin::run(void) {

	this->opdid->logDebug(this->nodeID + ": FritzBoxPlugin worker thread started", this->logVerbosity);

	while (!this->opdid->shutdownRequested) {
		try {

			Poco::Notification::Ptr notification = this->queue.waitDequeueNotification(100);
			if (notification) {
				ActionNotification::Ptr workNf = notification.cast<ActionNotification>();
				if (workNf) {
					// inspect action and decide what to do
					switch (workNf->type) {
					case ActionNotification::LOGIN: this->login(); break;
					case ActionNotification::LOGOUT: this->logout(); break;
					case ActionNotification::GETSWITCHSTATE: this->getSwitchState(workNf->port); break;
					case ActionNotification::SETSWITCHSTATELOW: this->setSwitchState(workNf->port, 0); break;
					case ActionNotification::SETSWITCHSTATEHIGH: this->setSwitchState(workNf->port, 1); break;
					case ActionNotification::GETSWITCHENERGY: this->getSwitchEnergy(workNf->port); break;
					case ActionNotification::GETSWITCHPOWER: this->getSwitchPower(workNf->port); break;
					}
				}
			}
		} catch (Poco::Exception &e) {
			this->opdid->logNormal(this->nodeID + ": Unhandled exception in worker thread: " + e.message(), this->logVerbosity);
		}
	}

	this->opdid->logDebug(this->nodeID + ": FritzBoxPlugin worker thread terminated", this->logVerbosity);
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
	return new FritzBoxPlugin();
}
