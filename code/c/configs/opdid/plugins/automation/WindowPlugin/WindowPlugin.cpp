
#include "opdi_constants.h"
#include "opdi_platformfuncs.h"

#ifdef _WINDOWS
#include "WindowsOPDID.h"
#endif

#ifdef linux
#include "LinuxOPDID.h"
#endif

#define LOCAL_DEBUG	1

class WindowPort : public OPDI_SelectPort {
friend class WindowPlugin;
protected:

	enum WindowState {
		UNKNOWN,
		UNKNOWN_WAITING,
		CLOSED,
		OPEN,
		WAITING_AFTER_ENABLE,
		WAITING_BEFORE_DISABLE_OPEN,
		WAITING_BEFORE_DISABLE_CLOSED,
		WAITING_BEFORE_DISABLE_ERROR,
		CLOSING,
		OPENING,
		ERR
	};

	AbstractOPDID *opdid;

	// configuration
	std::string sensor;
	uint8_t sensorClosed;
	std::string motorA;
	std::string motorB;
	uint8_t motorActive;
	uint64_t motorDelay;
	std::string enable;
	uint64_t enableDelay;
	uint8_t enableActive;
	uint64_t openingTime;
	uint64_t closingTime;
	std::string autoOpen;
	std::string autoClose;
	std::string forceClose;

	// processed configuration
	OPDI_DigitalPort *sensorPort;
	OPDI_DigitalPort *motorAPort;
	OPDI_DigitalPort *motorBPort;
	OPDI_DigitalPort *enablePort;
	
	typedef std::vector<OPDI_DigitalPort *> PortList;
	PortList autoOpenPorts;
	PortList autoClosePorts;
	PortList forceClosePorts;

	// state
	int targetState;
	int currentState;
	uint64_t delayTimer;
	uint64_t openTimer;
	// set to true when the state changes; next time canSend is true the port will self-refresh
	bool requiresRefresh;
	// set to true when the position has been just changed by the master
	bool positionNewlySet;
	bool isMotorEnabled;
	bool isMotorOn;

	OPDI_DigitalPort *findDigitalPort(std::string setting, std::string portID, bool required);

	void findDigitalPorts(std::string setting, std::string portIDs, PortList &portList);

	void prepare() override;

	// checks whether the port is an input port and throws an exception if not
	uint8_t getInputPortLine(OPDI_DigitalPort *port);

	// checks whether the port is an output port and throws an exception if not
	void setOutputPortLine(OPDI_DigitalPort *port, uint8_t line);

	bool isSensorClosed(void);

	void enableMotor(void);

	void disableMotor(void);

	void setMotorOpening(void);
	
	void setMotorClosing(void);

	void setMotorOff(void);

	std::string getMotorStateText(void);

	std::string getStateText(int state);

	void setCurrentState(int state);

	void setTargetState(int state);

	virtual uint8_t doWork(uint8_t canSend) override;

public:
	WindowPort(AbstractOPDID *opdid, const char *id);

	virtual void setPosition(uint16_t position) override;
	
	virtual void getState(uint16_t *position) override;
};

WindowPort::WindowPort(AbstractOPDID *opdid, const char *id) : OPDI_SelectPort(id) {
	this->opdid = opdid;

	this->targetState = UNKNOWN;
	this->currentState = UNKNOWN;
	this->requiresRefresh = false;
	this->positionNewlySet = false;
	this->isMotorEnabled = false;
	this->isMotorOn = false;
}

void WindowPort::setPosition(uint16_t position) {
	// recovery from error state is always possible
	if ((this->currentState == ERR) || this->position != position) {
		OPDI_SelectPort::setPosition(position);
		this->positionNewlySet = true;

		if (opdid->logVerbosity >= AbstractOPDID::VERBOSE)
			opdid->log(std::string(this->id) + ": Selected position " + to_string(position) + 
				(position == 0 ? " (CLOSED)" : (position == 1 ? " (OPEN)" : " (AUTOMATIC)")) +
				"; current state is: " + this->getStateText(this->currentState) + this->getMotorStateText());
	}
}

void WindowPort::getState(uint16_t *position) {
	if (this->currentState == ERR)
		throw PortError("Sensor or motor failure or misconfiguration");
	OPDI_SelectPort::getState(position);
}

OPDI_DigitalPort *WindowPort::findDigitalPort(std::string setting, std::string portID, bool required) {
	// locate port by ID
	OPDI_Port *port = this->opdid->findPortByID(portID.c_str());
	// no found but required?
	if (port == NULL) { 
		if (required)
			throw Poco::DataException(std::string(this->getID()) + ": Port required by Setting " + setting + " not found: " + portID);
		return NULL;
	}

	// port type must be Digital
	if (port->getType()[0] != OPDI_PORTTYPE_DIGITAL[0])
		throw Poco::DataException(std::string(this->getID()) + ": Port required by Setting " + setting + " is not a digital port: " + portID);

	return (OPDI_DigitalPort *)port;
}

void WindowPort::findDigitalPorts(std::string setting, std::string portIDs, PortList &portList) {
	// split list at blanks
	std::stringstream ss(portIDs);
	std::string item;
	while (std::getline(ss, item, ' ')) {
		// ignore empty items
		if (item != "") {
			OPDI_DigitalPort *port = this->findDigitalPort(setting, item, true);
			if (port != NULL)
				portList.push_back(port);
		}
	}
}

void WindowPort::prepare() {
	// find ports; throws errors if something required is missing
	this->sensorPort = this->findDigitalPort("Sensor", this->sensor, false);
	this->motorAPort = this->findDigitalPort("MotorA", this->motorA, true);
	this->motorBPort = this->findDigitalPort("MotorB", this->motorB, true);
	this->enablePort = this->findDigitalPort("Enable", this->enable, false);

	this->findDigitalPorts("AutoOpen", this->autoOpen, autoOpenPorts);
	this->findDigitalPorts("AutoClose", this->autoClose, autoClosePorts);
	this->findDigitalPorts("ForceClose", this->forceClose, forceClosePorts);
	
	// no enable port? assume always enabled
	if (this->enablePort == NULL)
		this->isMotorEnabled = true;
}

uint8_t WindowPort::getInputPortLine(OPDI_DigitalPort *port) {
	uint8_t mode;
	uint8_t line;
	port->getState(&mode, &line);
#ifndef LOCAL_DEBUG
	if ((mode < OPDI_DIGITAL_MODE_INPUT_FLOATING) || (mode > OPDI_DIGITAL_MODE_INPUT_PULLDOWN))
		throw Poco::AssertionViolationException(std::string("Window port ") + this->id + ": Port " + port->getID() + " is not in input mode, cannot query");
#endif
	return line;
}

// checks whether the port is an output port and throws an exception if not
void WindowPort::setOutputPortLine(OPDI_DigitalPort *port, uint8_t newLine) {
#ifndef LOCAL_DEBUG
	uint8_t mode;
	uint8_t line;
	port->getState(&mode, &line);
	if (mode != OPDI_DIGITAL_MODE_OUTPUT)
		throw Poco::AssertionViolationException(std::string("Window port ") + this->id + ": Port " + port->getID() + " is not in output mode, cannot set");
#endif
	port->setLine(newLine);
}

bool WindowPort::isSensorClosed(void) {
	// sensor not present?
	if (this->sensorPort == NULL)
		return false;
	// query the sensor
	if (this->getInputPortLine(this->sensorPort) == this->sensorClosed)
		return true;
	else
		return false;
}

void WindowPort::enableMotor(void) {
	if (this->enablePort == NULL)
		return;
	if (opdid->logVerbosity >= AbstractOPDID::VERBOSE)
		opdid->log(std::string(this->id) + ": Enabling motor");
	this->setOutputPortLine(this->enablePort, (this->enableActive == 1 ? 1 : 0));
	this->isMotorEnabled = true;
}

void WindowPort::disableMotor(void) {
	if (this->enablePort == NULL)
		return;
	if (opdid->logVerbosity >= AbstractOPDID::VERBOSE)
		opdid->log(std::string(this->id) + ": Disabling motor");
	this->setOutputPortLine(this->enablePort, (this->enableActive == 1 ? 0 : 1));
	this->isMotorEnabled = false;
}

void WindowPort::setMotorOpening(void) {
	if (opdid->logVerbosity >= AbstractOPDID::VERBOSE)
		opdid->log(std::string(this->id) + ": Setting motor to 'opening'");
	this->setOutputPortLine(this->motorAPort, (this->motorActive == 1 ? 1 : 0));
	this->setOutputPortLine(this->motorBPort, (this->motorActive == 1 ? 0 : 1));
	this->isMotorOn = true;
}

void WindowPort::setMotorClosing(void) {
	if (opdid->logVerbosity >= AbstractOPDID::VERBOSE)
		opdid->log(std::string(this->id) + ": Setting motor to 'closing'");
	this->setOutputPortLine(this->motorAPort, (this->motorActive == 1 ? 0 : 1));
	this->setOutputPortLine(this->motorBPort, (this->motorActive == 1 ? 1 : 0));
	this->isMotorOn = true;
}

void WindowPort::setMotorOff(void) {
	if (opdid->logVerbosity >= AbstractOPDID::VERBOSE)
		opdid->log(std::string(this->id) + ": Stopping motor");
	this->setOutputPortLine(this->motorAPort, (this->motorActive == 1 ? 0 : 1));
	this->setOutputPortLine(this->motorBPort, (this->motorActive == 1 ? 0 : 1));
	this->isMotorOn = false;
}

std::string WindowPort::getMotorStateText(void) {
	if (this->isMotorOn)
		return " (Motor is on)";
	else
		return std::string(" (Motor is off and ") + (this->isMotorEnabled ? "enabled" : "disabled") + ")";
}

std::string WindowPort::getStateText(int state) {
	switch (state) {
		case UNKNOWN:                        return std::string("UNKNOWN");
		case UNKNOWN_WAITING:				 return std::string("UNKNOWN_WAITING");
		case CLOSED:						 return std::string("CLOSED");
		case OPEN:							 return std::string("OPEN");
		case WAITING_AFTER_ENABLE:			 return std::string("WAITING_AFTER_ENABLE");
		case WAITING_BEFORE_DISABLE_OPEN:	 return std::string("WAITING_BEFORE_DISABLE_OPEN");
		case WAITING_BEFORE_DISABLE_CLOSED:	 return std::string("WAITING_BEFORE_DISABLE_CLOSED");
		case WAITING_BEFORE_DISABLE_ERROR:	 return std::string("WAITING_BEFORE_DISABLE_ERROR");
		case CLOSING:						 return std::string("CLOSING");
		case OPENING:						 return std::string("OPENING");
		case ERR:							 return std::string("ERR");
	};
	return std::string("unknown state ") + to_string(state);
}

void WindowPort::setCurrentState(int state) {
	if (this->currentState != state) {
		if (opdid->logVerbosity >= AbstractOPDID::VERBOSE)
			opdid->log(std::string(this->id) + ": Changing current state to: " + this->getStateText(state) + this->getMotorStateText());
		this->currentState = state;

		// undefined states reset the target
		if ((state == UNKNOWN) || (state == ERR))
			this->setTargetState(UNKNOWN);
		
		if ((state != UNKNOWN_WAITING) && 
			(state != WAITING_BEFORE_DISABLE_OPEN) && 
			(state != WAITING_BEFORE_DISABLE_CLOSED) && 
			(state != WAITING_BEFORE_DISABLE_ERROR))
			this->requiresRefresh = true;
			
		// if the window has been closed or opened, reflect it in the UI
		// except if the position is set to automatic
		if (this->position < 2) {
			if (state == CLOSED)
				position = 0;
			else if (state == OPEN)
				position = 1;
		}
	}
}

void WindowPort::setTargetState(int state) {
	if (this->targetState != state) {
		if (opdid->logVerbosity >= AbstractOPDID::VERBOSE)
			opdid->log(std::string(this->id) + ": Changing target state to: " + this->getStateText(state));
		this->targetState = state;
	}
}

uint8_t WindowPort::doWork(uint8_t canSend)  {
	OPDI_SelectPort::doWork(canSend);

#define CHECK_OPEN															\
	if (this->targetState == OPEN) {										\
		if (this->enableDelay > 0) {										\
			this->delayTimer = opdi_get_time_ms();							\
			this->enableMotor();											\
			this->setCurrentState(WAITING_AFTER_ENABLE);					\
		} else {															\
			this->openTimer = opdi_get_time_ms();							\
			this->setMotorOpening();										\
			this->setCurrentState(OPENING);									\
		}																	\
	}

#define CHECK_CLOSE															\
	if (this->targetState == CLOSED) {										\
		if (this->enableDelay > 0) {										\
			this->delayTimer = opdi_get_time_ms();							\
			this->enableMotor();											\
			this->setCurrentState(WAITING_AFTER_ENABLE);					\
		} else {															\
			this->openTimer = opdi_get_time_ms();							\
			this->setMotorClosing();										\
			this->setCurrentState(CLOSING);									\
		}																	\
	}

	// state machine for current window state

	// unknown state (first call or transition not yet implemented)? initialize
	if (this->currentState == UNKNOWN) {
		// do we know that we're closed?
		if (this->isSensorClosed()) {
			this->setCurrentState(CLOSED);
		} else
			// we don't know
			this->setCurrentState(UNKNOWN_WAITING);

		// disable the motor
		this->setMotorOff();
		this->disableMotor();
		
		// set target state according to initial position
		if (this->position == 1) {
			this->setTargetState(OPEN);
		} else
		if (this->position == 0) {
			this->setTargetState(CLOSED);
		}
	} else
	// unknown; waiting for command
	if ((this->currentState == UNKNOWN_WAITING)) {
		if (this->isSensorClosed()) {
			if (opdid->logVerbosity >= AbstractOPDID::VERBOSE)
				opdid->log(std::string(this->id) + ": Closing sensor signal detected");
			this->setCurrentState(CLOSED);
		} else {
			// do not change current state

			// opening required?
			CHECK_OPEN

			// closing required?
			CHECK_CLOSE
		}
	} else
	// open?
	if (this->currentState == OPEN) {
		// closing required?
		CHECK_CLOSE
	} else
	// closed?
	if (this->currentState == CLOSED) {
		// opening required?
		CHECK_OPEN
	} else
	// waiting after enable?
	if (this->currentState == WAITING_AFTER_ENABLE) {
		// waiting time up?
		if (opdi_get_time_ms() - this->delayTimer > this->enableDelay) {
			// enable delay is up
			
			// target may have changed; check again
			if (this->targetState == OPEN) {
				// start opening immediately
				this->openTimer = opdi_get_time_ms();
				this->setMotorOpening();
				this->setCurrentState(OPENING);
			} else
			if (this->targetState == CLOSED) {
				// start closing immediately
				this->openTimer = opdi_get_time_ms();
				this->setMotorClosing();
				this->setCurrentState(CLOSING);
			} else {
				// target state is unknown; wait
				this->setCurrentState(UNKNOWN_WAITING);
			}
		}
	} else
	// waiting before disable after opening?
	if (this->currentState == WAITING_BEFORE_DISABLE_OPEN) {
		if (this->targetState == CLOSED) {
			// need to close
			this->openTimer = opdi_get_time_ms();	// reset timer; TODO set to delta value?
			this->setMotorClosing();
			this->setCurrentState(CLOSING);
		} else
		// waiting time up?
		if (opdi_get_time_ms() - this->delayTimer > this->enableDelay) {
			// disable
			this->disableMotor();
			
			this->setCurrentState(OPEN);
		}
	} else
	// waiting before disable after closing?
	if (this->currentState == WAITING_BEFORE_DISABLE_CLOSED) {
		if (this->targetState == OPEN) {
			// need to open
			this->openTimer = opdi_get_time_ms();	// reset timer; TODO set to delta value?
			this->setMotorOpening();
			this->setCurrentState(OPENING);
		} else {
			// motor delay time up?
			if (this->isMotorOn && (opdi_get_time_ms() - this->delayTimer > this->motorDelay)) {
				// stop motor immediately
				this->setMotorOff();
			}
			// waiting time up?
			if (opdi_get_time_ms() - this->delayTimer > this->enableDelay) {
				// disable
				this->disableMotor();
			
				this->setCurrentState(CLOSED);
			}
		}
	} else
	// waiting before disable after error while closing?
	if (this->currentState == WAITING_BEFORE_DISABLE_ERROR) {
		// waiting time up?
		if (opdi_get_time_ms() - this->delayTimer > this->enableDelay) {
			// disable
			this->disableMotor();
			
			this->setCurrentState(ERR);
		}
	} else
	// opening?
	if (this->currentState == OPENING) {
		if (this->targetState == CLOSED) {
			// need to close
			this->openTimer = opdi_get_time_ms();	// reset timer; TODO set to delta value?
			this->setMotorClosing();
			this->setCurrentState(CLOSING);
		} else
		// opening time up?
		if (opdi_get_time_ms() - this->openTimer > this->openingTime) {
			// stop motor
			this->setMotorOff();

			// sensor still closed? (error condition)
			if (this->isSensorClosed()) {
				if (opdid->logVerbosity > AbstractOPDID::QUIET)
					opdid->log(std::string(this->id) + ": Warning: Closing sensor signal still present after opening");
				// enable delay specified?
				if (this->enableDelay > 0) {
					// need to disable first
					this->delayTimer = opdi_get_time_ms();
					this->setCurrentState(WAITING_BEFORE_DISABLE_ERROR);
				} else {
					// set error directly
					this->setCurrentState(ERR);
				}
			} else {
				// enable delay specified?
				if (this->enableDelay > 0) {
					// need to disable first
					this->delayTimer = opdi_get_time_ms();
					this->setCurrentState(WAITING_BEFORE_DISABLE_OPEN);
				} else {
					// assume that the window is open now
					this->setCurrentState(OPEN);
				}
			}
		}
	} else
	// closing?
	if (this->currentState == CLOSING) {
		if (this->targetState == OPEN) {
			// need to open
			this->openTimer = opdi_get_time_ms();	// reset timer; TODO set to delta value?
			this->setMotorOpening();
			this->setCurrentState(OPENING);
		} else
		// sensor close detected?
		if (this->isSensorClosed()) {
			if (opdid->logVerbosity >= AbstractOPDID::VERBOSE)
				opdid->log(std::string(this->id) + ": Closing sensor signal detected");

			// enable delay specified?
			if (this->enableDelay > 0) {
				// need to disable first
				this->delayTimer = opdi_get_time_ms();
				this->setCurrentState(WAITING_BEFORE_DISABLE_CLOSED);
			} else {
				// the window is closed now
				this->setCurrentState(CLOSED);
			}
		} else
		// closing time up?
		if (opdi_get_time_ms() - this->openTimer > this->closingTime) {

			// stop motor immediately
			this->setMotorOff();

			// without detecting sensor? error condition
			if (this->sensorPort != NULL) {
				if (opdid->logVerbosity > AbstractOPDID::QUIET)
					opdid->log(std::string(this->id) + ": Warning: Closing sensor signal not detected while closing");

				// enable delay specified?
				if (this->enableDelay > 0) {
					// need to disable first
					this->delayTimer = opdi_get_time_ms();
					this->setCurrentState(WAITING_BEFORE_DISABLE_ERROR);
				} else {
					// go to error condition directly
					this->setCurrentState(ERR);
				}
			} else {
				// sensor not present - assume closed
				// enable delay specified?
				if (this->enableDelay > 0) {
					// need to disable first
					this->delayTimer = opdi_get_time_ms();
					this->setCurrentState(WAITING_BEFORE_DISABLE_CLOSED);
				} else {
					// go to closed condition directly
					this->setCurrentState(CLOSED);
				}
			}
		}
	}

	PortList::const_iterator pi;
	bool forceClose = false;

	// if the window has detected an error, do not automatically close
	if (this->currentState != ERR) {
		// if one of the ForceClose ports is High, the window must be closed
		pi = this->forceClosePorts.begin();
		while (pi != this->forceClosePorts.end()) {
			if (this->getInputPortLine(*pi) == 1) {
				forceClose = true;
				break;
			}
			pi++;
		}
	}

	if (forceClose) {
		this->setTargetState(CLOSED);
	} else {
		int target = UNKNOWN;
		// if the window has detected an error, do not automatically open or close
		// if the position is set to automatic, evaluate auto ports
		if ((this->currentState != ERR) && (this->position > 1)) {
			// if one of the AutoClose ports is High, the window should be closed (takes precedence)
			pi = this->autoClosePorts.begin();
			while (pi != this->autoClosePorts.end()) {
				if (this->getInputPortLine(*pi) == 1) {
					// avoid repeating messages
					if (this->targetState != CLOSED) {
						if (opdid->logVerbosity >= AbstractOPDID::VERBOSE)
							opdid->log(std::string(this->id) + ": AutoClose detected from port: " + (*pi)->getID());
					}
					target = CLOSED;
					break;
				}
				pi++;
			}
			if (target == UNKNOWN) {
				// if one of the AutoOpen ports is High, the window should be opened
				pi = this->autoOpenPorts.begin();
				while (pi != this->autoOpenPorts.end()) {
					if (this->getInputPortLine(*pi) == 1) {
						// avoid repeating messages
						if (this->targetState != OPEN) {
							if (opdid->logVerbosity >= AbstractOPDID::VERBOSE)
								opdid->log(std::string(this->id) + ": AutoOpen detected from port: " + (*pi)->getID());
						}
						target = OPEN;
						break;
					}
					pi++;
				}
			}
		} else
		// check for position changes only - not if the previous setting is still active
		// this helps to recover from an error state - the user must manually change the position
		if (this->positionNewlySet && (this->position == 1)) {
			target = OPEN;
		} else
		if (this->positionNewlySet && (this->position == 0)) {
			target = CLOSED;
		}
		this->positionNewlySet = false;

		if (target != UNKNOWN) {
			this->setTargetState(target);
			// resetting from error state
			if (this->currentState == ERR)
				this->setCurrentState(UNKNOWN_WAITING);
		}
	}

	if (this->requiresRefresh && canSend) {
		this->refresh();

		this->requiresRefresh = false;
	}

	return OPDI_STATUS_OK;
}


////////////////////////////////////////////////////////////////////////
// Plugin main class
////////////////////////////////////////////////////////////////////////

class WindowPlugin : public IOPDIDPlugin, public IOPDIDConnectionListener {

protected:
	AbstractOPDID *opdid;

public:
	virtual void setupPlugin(AbstractOPDID *abstractOPDID, std::string node, Poco::Util::AbstractConfiguration *nodeConfig) override;

	virtual void masterConnected(void) override;
	virtual void masterDisconnected(void) override;
};

void WindowPlugin::setupPlugin(AbstractOPDID *abstractOPDID, std::string node, Poco::Util::AbstractConfiguration *nodeConfig) {
	this->opdid = abstractOPDID;

	// get port type
	std::string portType = nodeConfig->getString("Type", "");

	if (portType == "SelectPort") {
		// create window port
		WindowPort *port = new WindowPort(abstractOPDID, node.c_str());
		abstractOPDID->configureSelectPort(nodeConfig, port);

		// read additional configuration parameters
		port->sensor = abstractOPDID->getConfigString(nodeConfig, "Sensor", "", false);
		port->sensorClosed = (uint8_t)nodeConfig->getInt("SensorClosed", 1);
		if (port->sensorClosed > 1)
			throw Poco::DataException("SensorClosed must be either 0 or 1: " + abstractOPDID->to_string(port->sensorClosed));
		port->motorA = abstractOPDID->getConfigString(nodeConfig, "MotorA", "", true);
		port->motorB = abstractOPDID->getConfigString(nodeConfig, "MotorB", "", true);
		port->motorActive = (uint8_t)nodeConfig->getInt("MotorActive", 1);
		if (port->motorActive > 1)
			throw Poco::DataException("MotorActive must be either 0 or 1: " + abstractOPDID->to_string(port->motorActive));
		port->motorDelay = nodeConfig->getInt("MotorDelay", 0);
		if (port->motorDelay < 0)
			throw Poco::DataException("MotorDelay may not be negative: " + abstractOPDID->to_string(port->motorDelay));
		port->enable = nodeConfig->getString("Enable", "");
		port->enableDelay = nodeConfig->getInt("EnableDelay", 0);
		if (port->enableDelay < 0)
			throw Poco::DataException("EnableDelay may not be negative: " + abstractOPDID->to_string(port->enableDelay));
		if (port->enableDelay < port->motorDelay)
			throw Poco::DataException("If using MotorDelay, EnableDelay must be greater or equal: " + abstractOPDID->to_string(port->enableDelay));
		port->enableActive = (uint8_t)nodeConfig->getInt("EnableActive", 1);
		if (port->enableActive > 1)
			throw Poco::DataException("EnableActive must be either 0 or 1: " + abstractOPDID->to_string(port->enableActive));
		port->openingTime = nodeConfig->getInt("OpeningTime", 0);
		if (port->openingTime <= 0)
			throw Poco::DataException("OpeningTime must be specified and greater than 0: " + abstractOPDID->to_string(port->openingTime));
		port->closingTime = nodeConfig->getInt("OpeningTime", port->openingTime);
		if (port->closingTime <= 0)
			throw Poco::DataException("ClosingTime must be greater than 0: " + abstractOPDID->to_string(port->closingTime));
		port->autoOpen = abstractOPDID->getConfigString(nodeConfig, "AutoOpen", "", false);
		port->autoClose = abstractOPDID->getConfigString(nodeConfig, "AutoClose", "", false);
		port->forceClose = abstractOPDID->getConfigString(nodeConfig, "ForceClose", "", false);

		abstractOPDID->addPort(port);
	} else
		throw Poco::DataException("Node type 'SelectPort' expected", portType);

	this->opdid->addConnectionListener(this);

	if (this->opdid->logVerbosity >= AbstractOPDID::VERBOSE)
		this->opdid->log("WindowPlugin setup completed successfully as node " + node);
}

void WindowPlugin::masterConnected() {
}

void WindowPlugin::masterDisconnected() {
}

// plugin instance factory function

#ifdef _WINDOWS

extern "C" __declspec(dllexport) IOPDIDPlugin* __cdecl GetOPDIDPluginInstance(int majorVersion, int minorVersion, int patchVersion) {

#elif linux

extern "C" IOPDIDPlugin* GetOPDIDPluginInstance(int majorVersion, int minorVersion, int patchVersion) {

#endif

	// check whether the version is supported
	if ((majorVersion > 0) || (minorVersion > 1))
		throw Poco::Exception("This version of the WindowPlugin supports only OPDID versions up to 0.1");

	// return a new instance of this plugin
	return new WindowPlugin();
}
