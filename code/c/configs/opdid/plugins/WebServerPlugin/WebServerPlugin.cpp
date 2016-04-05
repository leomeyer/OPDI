#include <iostream>
#include <sstream>

#include <Poco/File.h>
#include <Poco/Path.h>
#include <Poco/Exception.h>
#include <Poco/JSON/JSON.h>
#include <Poco/JSON/Parser.h>
#include <Poco/JSON/Object.h>

#include <mongoose.h>

#include "opdi_constants.h"
#include "opdi_platformfuncs.h"

#include "AbstractOPDID.h"
#include "OPDID_PortFunctions.h"

////////////////////////////////////////////////////////////////////////
// Plugin main class
////////////////////////////////////////////////////////////////////////

class WebServerPlugin : public IOPDIDPlugin, public IOPDIDConnectionListener, public OPDI_DigitalPort, protected OPDID_PortFunctions {
	// web server management structures
	struct mg_mgr mgr;
	struct mg_connection *nc;
	struct mg_serve_http_opts s_http_server_opts;
	
	std::string httpPort;
	std::string documentRoot;
	std::string enableDirListing;
	std::string indexFiles;
	
	std::string jsonRpcUrl;

public:
	WebServerPlugin(): OPDI_DigitalPort("WebServerPlugin"), OPDID_PortFunctions("WebServerPlugin") {
		this->httpPort = "8080";
		this->documentRoot = ".";
		this->enableDirListing = "no";
		this->indexFiles = "index.html";
		this->jsonRpcUrl = "/api/jsonrpc";
	};

	virtual void setupPlugin(AbstractOPDID *abstractOPDID, std::string node, Poco::Util::AbstractConfiguration *nodeConfig) override;

	void handleEvent(struct mg_connection *nc, int ev, void *p);

	virtual uint8_t doWork(uint8_t canSend) override;

	virtual void masterConnected(void) override;
	virtual void masterDisconnected(void) override;
	
	// JSON-RPC functions
	Poco::JSON::Object jsonRpcList();
};

static WebServerPlugin* instance;

// Mongoose event handler function
static void ev_handler(struct mg_connection *nc, int ev, void *p) {
	// delegate to class instance
	instance->handleEvent(nc, ev, p);
}

Poco::JSON::Object WebServerPlugin::jsonRpcList() {
	// list all ports, expect no parameter
	
	// return an array of port objects
	
	return "test";
}

void WebServerPlugin::handleEvent(struct mg_connection *nc, int ev, void *p) {
	struct http_message *hm = (struct http_message*)p;
	
  	switch (ev) {
		case MG_EV_HTTP_REQUEST:
			this->logVerbose(this->ID() + ": Request received for: " + std::string(hm->uri.p, hm->uri.len));
			// JSON-RPC url received?
			if (mg_vcmp(&hm->uri, jsonRpcUrl.c_str()) == 0) {
				std::string json(hm->body.p, hm->body.len);
				this->logDebug(this->ID() + ": Received JSON request: " + json);
				// parse JSON
				try {
					Poco::JSON::Parser parser;
					Poco::Dynamic::Var request = parser.parse(json);
					Poco::JSON::Object::Ptr object = request.extract<Poco::JSON::Object::Ptr>();
					Poco::Dynamic::Var method = object->get("method");
					std::string methodStr = method.convert<std::string>();
					Poco::Dynamic::Var params = object->get("params");
					Poco::Dynamic::Var id = object->get("id");
					
					Poco::JSON::Object result;
					if (methodStr == "list") {
						result = this->jsonRpcList();
					}

					// create JSON-RPC response object
					
					Poco::JSON::Object response;
					response.set("id", id);
					response.set("error", nullptr);
					response.set("result", result);
					
					std::stringstream sOut;
					response.stringify(sOut);
					std::string strOut = sOut.str();

					// send result
					mg_printf(nc, "HTTP/1.0 200 OK\r\nContent-Length: %d\r\n"
						"Content-Type: application/json\r\n\r\n%s",
						strOut.size(), strOut.c_str());

				} catch (Poco::Exception& e) {
					std::string err("Error processing request: ");
					err.append(e.what());
					err.append(": ");
					err.append(e.message());
					
					this->logVerbose(this->ID() + ": " + err);
					// send error response
					mg_printf(nc, "HTTP/1.0 200 OK\r\nContent-Length: %d\r\n"
						"Content-Type: application/json\r\n\r\n%s",
						err.size(), err.c_str());
				} catch (std::exception& e) {
					std::string err("Error processing request: ");
					err.append(e.what());
					
					this->logVerbose(this->ID() + ": " + err);
					// send error response
					mg_printf(nc, "HTTP/1.0 200 OK\r\nContent-Length: %d\r\n"
						"Content-Type: application/json\r\n\r\n%s",
						err.size(), err.c_str());
				}
				// check result for JSON-RPC call
				//mg_rpc_dispatch(hm->body.p, hm->body.len, buf, sizeof(buf), json_methods, json_handlers);
				nc->flags |= MG_F_SEND_AND_CLOSE;
				break;
			} else
				// serve static content
				mg_serve_http(nc, hm, this->s_http_server_opts);
			break;
		default:
			break;
	  }
}

void WebServerPlugin::setupPlugin(AbstractOPDID *abstractOPDID, std::string node, Poco::Util::AbstractConfiguration *config) {
	this->opdid = abstractOPDID;
	this->setID(node.c_str());
	this->portFunctionID = node;
	
	Poco::AutoPtr<Poco::Util::AbstractConfiguration> nodeConfig = config->createView(node);

	abstractOPDID->configureDigitalPort(nodeConfig, this, true);

	// get web server configuration
	this->httpPort = nodeConfig->getString("Port", this->httpPort);
	// determine document root; default is the configuration's directory
	this->documentRoot = nodeConfig->getString("DocumentRoot", this->documentRoot);
	// determine application-relative path depending on location of config file
	std::string configFilePath = config->getString(OPDID_CONFIG_FILE_SETTING, "");
	if (configFilePath == "")
		throw Poco::DataException("Programming error: Configuration file path not specified in config settings");
	Poco::Path filePath(configFilePath);
	Poco::Path absPath(filePath.absolute());
	Poco::Path parentPath = absPath.parent();
	// append document root to path of previous config file (or replace it)
	Poco::Path finalPath = parentPath.resolve(this->documentRoot);
	this->documentRoot = finalPath.toString();
	this->logDebug(this->ID() + ": Resolved document root to: " + this->documentRoot);
	if (!Poco::File(finalPath).isDirectory())
		throw Poco::DataException("Document root folder does not exist or is not a folder: " + documentRoot);
		
	// expose JSON-RPC API via special URL (can be disabled by setting the URL to "")
	this->jsonRpcUrl = nodeConfig->getString("JsonRpcUrl", this->jsonRpcUrl);
		
	this->s_http_server_opts.document_root = this->documentRoot.c_str();
	this->enableDirListing = nodeConfig->getBool("EnableDirListing", false) ? "yes" : "";
	this->s_http_server_opts.enable_directory_listing = this->enableDirListing.c_str();
	this->indexFiles = nodeConfig->getString("IndexFiles", this->indexFiles);
	this->s_http_server_opts.index_files = this->indexFiles.c_str();

	this->logVerbose(this->ID() + ": Setting up web server at: " + this->httpPort);
		
	// setup web server
	mg_mgr_init(&this->mgr, nullptr);
	this->nc = mg_bind(&this->mgr, this->httpPort.c_str(), ev_handler);
	
	if (this->nc == nullptr)		
		throw Poco::ApplicationException(this->ID() + ": Unable to setup web server: " + strerror(errno));
	
	// set HTTP server parameters
	mg_set_protocol_http_websocket(this->nc);

	this->logVerbose(this->ID() + ": WebServerPlugin setup completed successfully at: " + this->httpPort);
	
	// register port (necessary for doWork to be called regularly)
	this->opdid->addPort(this);
}

uint8_t WebServerPlugin::doWork(uint8_t canSend) {
	// call Mongoose work function
	mg_mgr_poll(&this->mgr, 1);
	
	return OPDI_STATUS_OK;
}

void WebServerPlugin::masterConnected() {
}

void WebServerPlugin::masterDisconnected() {
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
	instance = new WebServerPlugin();
	
	return instance;
}
