
#include "Poco/Process.h"

#include "OPDID_ExecPort.h"

#include "opdi_constants.h"

///////////////////////////////////////////////////////////////////////////////
// Exec Port
///////////////////////////////////////////////////////////////////////////////

OPDID_ExecPort::OPDID_ExecPort(AbstractOPDID *opdid, const char *id) : OPDI_DigitalPort(id, id, OPDI_PORTDIRCAP_OUTPUT, 0), OPDID_PortFunctions(id) {
	this->opdid = opdid;

	OPDI_DigitalPort::setMode(OPDI_DIGITAL_MODE_OUTPUT);

	// default: Low
	this->line = 0;
	this->lastTriggerTime = 0;
}

OPDID_ExecPort::~OPDID_ExecPort() {
}

void OPDID_ExecPort::configure(Poco::Util::AbstractConfiguration *config) {
	this->opdid->configurePort(config, this, 0);
	this->logVerbosity = this->opdid->getConfigLogVerbosity(config, AbstractOPDID::UNKNOWN);

	this->programName = config->getString("Program", "");
	if (this->programName == "")
		throw Poco::DataException("You have to specify a Program parameter");

	this->parameters = config->getString("Parameters", "");

	this->waitTimeMs = config->getInt64("WaitTime", 0);
	if (this->waitTimeMs < 0)
		throw Poco::DataException("Please specify a positive value for WaitTime", this->waitTimeMs);

	this->resetTimeMs = config->getInt64("ResetTime", 0);
	if (this->resetTimeMs < 0)
		throw Poco::DataException("Please specify a positive value for ResetTime", this->resetTimeMs);

	this->forceKill = config->getBool("ForceKill", false);
}

void OPDID_ExecPort::setDirCaps(const char *dirCaps) {
	throw PortError(std::string(this->getID()) + ": The direction capabilities of an ExecPort cannot be changed");
}

void OPDID_ExecPort::setMode(uint8_t mode) {
	throw PortError(std::string(this->getID()) + ": The mode of an ExecPort cannot be changed");
}

void OPDID_ExecPort::prepare() {
	OPDI_DigitalPort::prepare();
}

uint8_t OPDID_ExecPort::doWork(uint8_t canSend)  {
	OPDI_DigitalPort::doWork(canSend);

	if (this->line == 1) {
		// port state switched to High? wait time must be up if specified
		Poco::Timestamp::TimeDiff timeDiff = (Poco::Timestamp() - this->lastTriggerTime);
		if (this->wasLow && ((this->waitTimeMs == 0) || (timeDiff / 1000 > this->waitTimeMs))) { // Poco TimeDiff is in microseconds
			// trigger detected
			this->wasLow = false;

			// program still running?
			if (Poco::Process::isRunning(this->processPID) && forceKill) {
				this->logDebug(this->ID() + ": Trying to kill previously started process with PID " + this->to_string(this->processPID));

				// kill process
				Poco::Process::kill(this->processPID);
			}

			// build parameter string
			std::string params(this->parameters);

			typedef std::map<std::string, std::string> PortValues;
			PortValues portValues;
			std::string allPorts;
			// go through all ports
			OPDI::PortList pl = this->opdid->getPorts();
			OPDI::PortList::const_iterator pli = pl.begin();
			while (pli != pl.end()) {
				std::string val = this->opdid->getPortStateStr(*pli);
				if (val == "")
					val = "<error>";
				std::string id = "$" + (*pli)->ID();
				portValues[id] = val;
				allPorts += (*pli)->ID() + "=" + val + " ";
				pli++;
			}
			portValues["$ALL_PORTS"] = allPorts;

			// replace parameters in content
			for (PortValues::iterator iterator = portValues.begin(); iterator != portValues.end(); iterator++) {
				std::string key = iterator->first;
				std::string value = iterator->second;

				size_t start = 0;
				while ((start = params.find(key, start)) != std::string::npos) {
					params.replace(start, key.length(), value);
				}
			}

			this->logDebug(this->ID() + ": Preparing start of program '" + this->programName + "'");
			this->logDebug(this->ID() + ": Parameters: " + params);

			// split parameters
			std::vector<std::string> argList;
			std::stringstream ss(params);
			std::string item;
			while (std::getline(ss, item, ' ')) {
				if (item != "")
					argList.push_back(item);
			}

			// execute program
			this->lastTriggerTime = Poco::Timestamp();

			try {
				Poco::ProcessHandle ph(Poco::Process::launch(this->programName, argList));
				this->processPID = ph.id();
				this->logVerbose(this->ID() + ": Started program '" + this->programName + "' with PID " + this->to_string(this->processPID));
			} catch (Poco::Exception &e) {
				this->opdid->logError(this->ID() + ": Unable to start program '" + this->programName + "': " + e.message());
			}
		}

		// reset time up?
		if ((this->resetTimeMs > 0) && ((Poco::Timestamp() - this->lastTriggerTime) / 1000 > this->resetTimeMs)) {
			// set back to Low
			this->setLine(0);
		}
	} else {
		// state is Low
		this->wasLow = true;
	}

	return OPDI_STATUS_OK;
}
