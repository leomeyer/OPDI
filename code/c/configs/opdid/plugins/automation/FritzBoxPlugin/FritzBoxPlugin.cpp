#include <iostream>
#include <sstream>

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

#include "opdi_constants.h"
#include "opdi_platformfuncs.h"

#ifdef _WINDOWS
#include "WindowsOPDID.h"
#endif

#ifdef linux
#include "LinuxOPDID.h"
#endif

/*
class FritzDECT200 : public OPDI_DigitalPort {
friend class FritzBoxPlugin;
protected:

	AbstractOPDID *opdid;


	virtual uint8_t doWork(uint8_t canSend) override;

public:
	FritzDECT200(AbstractOPDID *opdid, const char *id);

	virtual void setLine(uint8_t line) override;
	
	virtual void getState(uint8_t mode, uint16_t *position) override;
};
*/


////////////////////////////////////////////////////////////////////////
// Plugin main class
////////////////////////////////////////////////////////////////////////

class FritzBoxPlugin : public IOPDIDPlugin, public IOPDIDConnectionListener {

protected:
	AbstractOPDID *opdid;

	virtual std::string httpGet(std::string url);

	virtual std::string getResponse(std::string challenge, std::string password);

	virtual std::string getSessionID(std::string host, std::string user, std::string password);

	virtual std::string getXMLValue(std::string xml, std::string node);

public:
	virtual void setupPlugin(AbstractOPDID *abstractOPDID, std::string node, Poco::Util::AbstractConfiguration *nodeConfig) override;

	virtual void masterConnected(void) override;
	virtual void masterDisconnected(void) override;

};

std::string FritzBoxPlugin::httpGet(std::string url) {
	// prepare session
	Poco::URI uri(url);
	Poco::Net::HTTPClientSession session(uri.getHost(), uri.getPort());

	// prepare path
	std::string path(uri.getPathAndQuery());
	if (path.empty()) path = "/";

	if (this->opdid->logVerbosity >= AbstractOPDID::VERBOSE)
		this->opdid->log("HTTP GET: " + url);

	// send request
	Poco::Net::HTTPRequest req(Poco::Net::HTTPRequest::HTTP_GET, path, Poco::Net::HTTPMessage::HTTP_1_1);
	session.sendRequest(req);

	// get response
	Poco::Net::HTTPResponse res;

	// print response
	std::istream &is = session.receiveResponse(res);

	if (this->opdid->logVerbosity >= AbstractOPDID::VERBOSE)
	this->opdid->log("HTTP Response Code: " + this->opdid->to_string(res.getStatus()) + " " + res.getReason());

	std::stringstream ss;

	Poco::StreamCopier::copyStream(is, ss);

	return ss.str();
}

std::string FritzBoxPlugin::getXMLValue(std::string xml, std::string node) {
	std::stringstream in(xml);
	Poco::XML::InputSource src(in);
	Poco::XML::DOMParser parser;
	Poco::AutoPtr<Poco::XML::Document> pDoc = parser.parse(&src);
	Poco::XML::NodeIterator it(pDoc, Poco::XML::NodeFilter::SHOW_ALL);
	Poco::XML::Node* pNode = it.nextNode();
	while (pNode)
	{
		if ((pNode->nodeName() == node) && (pNode->firstChild() != NULL))
			return pNode->firstChild()->nodeValue();
		pNode = it.nextNode();
	}

	throw Poco::NotFoundException("Node or value not found in XML: " + node);
}

std::string FritzBoxPlugin::getResponse(std::string challenge, std::string password) {
	Poco::MD5Engine md5;
	// password is UTF8 internally; need to convert to UTF16LE
	std::string toHash = challenge + "-" + password;
	std::wstring toHashUTF16;
	Poco::UnicodeConverter::toUTF16(toHash, toHashUTF16);
	// replace chars > 255 with a dot (compatibility reasons)
	wchar_t *c = (wchar_t *)&toHashUTF16.c_str()[0];
	while (*c) {
		if (*c > 255)
			*c = '.';
		c++;
	}
	// twice the string length (each char has 2 bytes)
	md5.update(toHashUTF16.c_str(), toHashUTF16.length() * 2);
	const Poco::DigestEngine::Digest& digest = md5.digest(); // obtain result
	return challenge + "-" + Poco::DigestEngine::digestToHex(digest);
}

std::string FritzBoxPlugin::getSessionID(std::string host, std::string user, std::string password) {

	std::string loginPage = this->httpGet(std::string("http://") +host + "/login_sid.lua");
	std::string sid = getXMLValue(loginPage, "SID");
	if (sid == "0000000000000000") {
		std::string challenge = getXMLValue(loginPage, "Challenge");
		std::string uri = "http://" + host + "/login_sid.lua?username=" + user + "&response=" + this->getResponse(challenge, password);
		std::string loginPage = this->httpGet(uri);
		sid = getXMLValue(loginPage,  "SID");
	}
	return sid;
}

void FritzBoxPlugin::setupPlugin(AbstractOPDID *abstractOPDID, std::string node, Poco::Util::AbstractConfiguration *nodeConfig) {
	this->opdid = abstractOPDID;

	// test case for response calculation (see documentation: http://avm.de/fileadmin/user_upload/Global/Service/Schnittstellen/AVM_Technical_Note_-_Session_ID.pdf)
	//	abstractOPDID->log("Test Response: " + this->getResponse("1234567z", "äbc"));

	// get host and credentials
	std::string host = abstractOPDID->getConfigString(nodeConfig, "Host", "", true);
	std::string user = abstractOPDID->getConfigString(nodeConfig, "User", "", true);
	std::string password = abstractOPDID->getConfigString(nodeConfig, "Password", "", true);

	abstractOPDID->log("Session ID: " + this->getSessionID(host, user, password));

	this->opdid->addConnectionListener(this);

	if (this->opdid->logVerbosity >= AbstractOPDID::VERBOSE)
		this->opdid->log("FritzBoxPlugin setup completed successfully as node " + node);
}

void FritzBoxPlugin::masterConnected() {
}

void FritzBoxPlugin::masterDisconnected() {
}

// plugin instance factory function

#ifdef _WINDOWS

extern "C" __declspec(dllexport) IOPDIDPlugin* __cdecl GetOPDIDPluginInstance(int majorVersion, int minorVersion, int patchVersion) {

#elif linux

extern "C" IOPDIDPlugin* GetOPDIDPluginInstance(int majorVersion, int minorVersion, int patchVersion) {

#endif

	// check whether the version is supported
	if ((majorVersion > 0) || (minorVersion > 1))
		throw Poco::Exception("This version of the FritzBoxPlugin supports only OPDID versions up to 0.1");

	// return a new instance of this plugin
	return new FritzBoxPlugin();
}
