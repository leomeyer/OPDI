#include "ExecPort.h"

#include "opdi_constants.h"

namespace opdid {

///////////////////////////////////////////////////////////////////////////////
// Exec Port
///////////////////////////////////////////////////////////////////////////////

ExecPort::ExecPort(AbstractOPDID *opdid, const char *id) : opdi::DigitalPort(id, id, OPDI_PORTDIRCAP_OUTPUT, 0), PortFunctions(id) {
	this->opdid = opdid;

	opdi::DigitalPort::setMode(OPDI_DIGITAL_MODE_OUTPUT);

	// default: Low
	this->line = 0;
	this->lastTriggerTime = 0;
	this->changeType = CHANGED_TO_HIGH;
}

ExecPort::~ExecPort() {
}

void ExecPort::configure(Poco::Util::AbstractConfiguration *config) {
	this->opdid->configurePort(config, this, 0);
	this->logVerbosity = this->opdid->getConfigLogVerbosity(config, AbstractOPDID::UNKNOWN);

	std::string changeTypeStr = config->getString("ChangeType", "");
	if (changeTypeStr == "ChangedToHigh") {
        this->changeType = CHANGED_TO_HIGH;
	} else
	if (changeTypeStr == "ChangedToLow") {
        this->changeType = CHANGED_TO_LOW;
	} else
	if (changeTypeStr == "AnyChange") {
        this->changeType = ANY_CHANGE;
	} else
	if (!changeTypeStr.empty())
        throw Poco::DataException(this->ID() + ": Illegal value for 'ChangeType', expected: 'ChangedToHigh', 'ChangedToLow', or 'AnyChange': " + changeTypeStr);

	this->programName = config->getString("Program", "");
	if (this->programName == "")
		throw Poco::DataException(this->ID() + ": You have to specify a Program parameter");

	this->parameters = config->getString("Parameters", "");

	this->waitTimeMs = config->getInt64("WaitTime", 0);
	if (this->waitTimeMs < 0)
		throw Poco::DataException(this->ID() + ": Please specify a positive value for WaitTime: ", this->to_string(this->waitTimeMs));

	this->resetTimeMs = config->getInt64("ResetTime", 0);
	if (this->resetTimeMs < 0)
		throw Poco::DataException(this->ID() + ": Please specify a positive value for ResetTime: ", this->to_string(this->resetTimeMs));

	this->forceKill = config->getBool("ForceKill", false);
	
	if ((this->waitTimeMs > 0) && (this->resetTimeMs > 0) && (this->waitTimeMs > this->resetTimeMs))
		this->logWarning(std::string() + "The specified wait time is larger than the reset time; reset will not execute!");
}

void ExecPort::setDirCaps(const char * /*dirCaps*/) {
	throw PortError(this->ID() + ": The direction capabilities of an ExecPort cannot be changed");
}

void ExecPort::setMode(uint8_t /*mode*/, ChangeSource /*changeSource*/) {
	throw PortError(this->ID() + ": The mode of an ExecPort cannot be changed");
}

void ExecPort::prepare() {
	this->logDebug(std::string() + "Preparing port");
	opdi::DigitalPort::prepare();
}

uint8_t ExecPort::doWork(uint8_t canSend)  {
	opdi::DigitalPort::doWork(canSend);

	uint8_t lastLine = this->lastState;
	this->lastState = this->line;

    // state changed since last check?
	if (this->line != lastLine) {
        // check state change condition
        bool stateChanged = true;
        if ((this->line == 0) && (this->changeType == CHANGED_TO_HIGH))
            stateChanged = false;
        if ((this->line == 1) && (this->changeType == CHANGED_TO_LOW))
            stateChanged = false;

		if (stateChanged) {
			// relevant change detected - wait time must be up if specified
			Poco::Timestamp::TimeDiff timeDiff = (Poco::Timestamp() - this->lastTriggerTime);
			if ((this->waitTimeMs == 0) || (timeDiff / 1000 > this->waitTimeMs)) { // Poco TimeDiff is in microseconds
				// trigger detected

				// program still running?
				if (Poco::Process::isRunning(this->processPID) && forceKill) {
					this->logDebug(std::string() + "Trying to kill previously started process with PID " + this->to_string(this->processPID));

					// kill process
					Poco::Process::kill(this->processPID);
				}

				// build parameter string
				std::string params(this->parameters);

				typedef std::map<std::string, std::string> PortValues;
				PortValues portValues;
				std::string allPorts;
				// go through all ports
				opdi::OPDI::PortList pl = this->opdid->getPorts();
				auto pli = pl.begin();
				auto plie = pl.end();
				while (pli != plie) {
					std::string val = this->opdid->getPortStateStr(*pli);
					if (val.empty())
						val = "<error>";
					// store value
					portValues[std::string("$") + (*pli)->ID()] = val;
					allPorts += (*pli)->ID() + "=" + val + " ";
					++pli;
				}
				portValues["$ALL_PORTS"] = allPorts;

				// replace parameters in content
				for (auto iterator = portValues.begin(), iteratorEnd = portValues.end(); iterator != iteratorEnd; ++iterator) {
					std::string key = iterator->first;
					std::string value = iterator->second;

					size_t start = 0;
					while ((start = params.find(key, start)) != std::string::npos) {
						params.replace(start, key.length(), value);
					}
				}

				this->logDebug(std::string() + "Preparing start of program '" + this->programName + "'");
				this->logDebug(std::string() + "Parameters: " + params);

				// split parameters
				std::vector<std::string> argList;
				std::stringstream ss(params);
				std::string item;
				while (std::getline(ss, item, ' ')) {
					if (!item.empty())
						argList.push_back(item);
				}

				// for automatic reset logic, if set high...
				if (this->line == 1)
					// ... remember start time
					this->lastTriggerTime = Poco::Timestamp();

				// execute program
				try {
					Poco::ProcessHandle ph(Poco::Process::launch(this->programName, argList));
					this->processPID = ph.id();
					this->logVerbose(std::string() + "Started program '" + this->programName + "' with PID " + this->to_string(this->processPID));
				} catch (Poco::Exception &e) {
					this->logNormal(std::string() + "ERROR: Unable to start program '" + this->programName + "': " + e.message());
				}
			}		// not blocked by wait time
		}		// stateChanged
	}		// line != lastLine

    // set High and reset time up?
    if ((this->line == 1) && (this->resetTimeMs > 0) && ((Poco::Timestamp() - this->lastTriggerTime) / 1000 > this->resetTimeMs)) {
        // set back to Low; this may trigger an execution as well
        this->setLine(0);
    }

	return OPDI_STATUS_OK;
}

}		// namespace opdid

