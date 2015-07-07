
#include "opdi_constants.h"
#include "opdi_platformfuncs.h"

#ifdef _WINDOWS
#include "WindowsOPDID.h"
#endif

#ifdef linux
#include "LinuxOPDID.h"
#endif

#include "OPDID_PortFunctions.h"

class WindowPort : public OPDI_SelectPort, OPDID_PortFunctions {
friend class WindowPlugin;
protected:

#define POSITION_OFF	0
#define POSITION_CLOSED	1
#define POSITION_OPEN	2
#define POSITION_AUTO	3

	enum WindowMode {
		H_BRIDGE,
		SERIAL_RELAY
	};

	enum WindowState {
		UNKNOWN,
		UNKNOWN_WAITING,
		CLOSED,
		OPEN,
		WAITING_AFTER_ENABLE,
		WAITING_BEFORE_DISABLE_OPEN,
		WAITING_BEFORE_DISABLE_CLOSED,
		WAITING_BEFORE_DISABLE_ERROR,
		WAITING_BEFORE_ENABLE_OPENING,
		WAITING_BEFORE_ENABLE_CLOSING,
		WAITING_AFTER_DISABLE,
		CLOSING,
		OPENING,
		ERR
	};

	enum ResetTo {
		RESET_NONE,
		RESET_TO_CLOSED,
		RESET_TO_OPEN
	};

	// configuration
	std::string sensor;
	uint8_t sensorClosed;
	std::string motorA;
	std::string motorB;
	std::string direction;
	uint8_t motorActive;
	uint64_t motorDelay;
	std::string enable;
	uint64_t enableDelay;
	uint8_t enableActive;
	uint64_t openingTime;
	uint64_t closingTime;
	std::string autoOpen;
	std::string autoClose;
	std::string forceOpen;
	std::string forceClose;
	std::string statusPortStr;
	std::string errorPortStr;
	std::string resetPortStr;
	ResetTo resetTo;
	int16_t positionAfterClose;
	int16_t positionAfterOpen;

	// processed configuration
	OPDI_DigitalPort *sensorPort;
	OPDI_DigitalPort *motorAPort;
	OPDI_DigitalPort *motorBPort;
	OPDI_DigitalPort *directionPort;
	OPDI_DigitalPort *enablePort;
	OPDI_SelectPort *statusPort;

	typedef std::vector<OPDI_DigitalPort *> DigitalPortList;

	DigitalPortList autoOpenPorts;
	DigitalPortList autoClosePorts;
	DigitalPortList forceOpenPorts;
	DigitalPortList forceClosePorts;
	DigitalPortList errorPorts;
	DigitalPortList resetPorts;

	WindowMode mode;
	// state
	WindowState targetState;
	WindowState currentState;
	uint64_t delayTimer;
	uint64_t openTimer;
	// set to true when the position has been just changed by the master
	bool positionNewlySet;
	bool isMotorEnabled;
	bool isMotorOn;

	OPDI_SelectPort *findSelectPort(std::string setting, std::string portID, bool required);

	OPDI_DigitalPort *findDigitalPort(std::string setting, std::string portID, bool required);

	void findDigitalPorts(std::string setting, std::string portIDs, DigitalPortList &portList);

	void prepare() override;

	// gets the line status from the digital port
	uint8_t getPortLine(OPDI_DigitalPort *port);

	// sets the line status of the digital port
	void setPortLine(OPDI_DigitalPort *port, uint8_t line);

	bool isSensorClosed(void);

	void enableMotor(void);

	void disableMotor(void);

	void setMotorOpening(void);
	
	void setMotorClosing(void);

	void setMotorOff(void);

	std::string getMotorStateText(void);

	std::string getStateText(WindowState state);

	void setCurrentState(WindowState state);

	void setTargetState(WindowState state);

	void checkOpenHBridge(void);

	void checkCloseHBridge(void);


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
	this->positionNewlySet = false;
	this->isMotorEnabled = false;
	this->isMotorOn = false;
	this->refreshMode = REFRESH_NOT_SET;
	this->sensorPort = NULL;
	this->enablePort = NULL;
	this->directionPort = NULL;
	this->statusPort = NULL;
	this->resetTo = RESET_NONE;
	this->positionAfterClose = -1;
	this->positionAfterOpen = -1;
}

void WindowPort::setPosition(uint16_t position) {
	// recovery from error state is always possible
	if ((this->currentState == ERR) || this->position != position) {
		// prohibit disabling the automatic mode by setting the position to the current state
		if ((this->positionAfterClose >= 0) && (position == POSITION_CLOSED ) && (this->currentState == CLOSED))
			position = POSITION_AUTO;
		if ((this->positionAfterOpen >= 0) && (position == POSITION_OPEN ) && (this->currentState == OPEN))
			position = POSITION_AUTO;

		OPDI_SelectPort::setPosition(position);
		this->positionNewlySet = true;

		std::string info = std::string(this->id) + ": Setting position to " + to_string(position) + " ";
		switch (position) {
		case POSITION_OFF:
			info += "(OFF)";
			break;
		case POSITION_CLOSED:
			info += "(CLOSED)";
			break;
		case POSITION_OPEN:
			info += "(OPEN)";
			break;
		default:
			info += "(AUTOMATIC)";
			break;
		}
		this->logVerbose(info + "; current state is: " + this->getStateText(this->currentState) + this->getMotorStateText());
	}
}

void WindowPort::getState(uint16_t *position) {
	if (this->currentState == ERR && !this->positionNewlySet)
		throw PortError(std::string(this->getID()) + ": Sensor or motor failure or misconfiguration");
	OPDI_SelectPort::getState(position);
}

OPDI_SelectPort *WindowPort::findSelectPort(std::string setting, std::string portID, bool required) {
	// locate port by ID
	OPDI_Port *port = this->opdid->findPortByID(portID.c_str());
	// no found but required?
	if (port == NULL) { 
		if (required)
			throw Poco::DataException(std::string(this->getID()) + ": Port required by Setting " + setting + " not found: " + portID);
		return NULL;
	}

	// port type must be Digital
	if (port->getType()[0] != OPDI_PORTTYPE_SELECT[0])
		throw Poco::DataException(std::string(this->getID()) + ": Port required by Setting " + setting + " is not a select port: " + portID);

	return (OPDI_SelectPort *)port;
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

void WindowPort::findDigitalPorts(std::string setting, std::string portIDs, DigitalPortList &portList) {
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
	OPDI_Port::prepare();

	// find ports; throws errors if something required is missing
	if (this->sensor != "")
		this->sensorPort = this->findDigitalPort("Sensor", this->sensor, true);
	if (this->mode == H_BRIDGE) {
		this->motorAPort = this->findDigitalPort("MotorA", this->motorA, true);
		this->motorBPort = this->findDigitalPort("MotorB", this->motorB, true);
		if (this->enable != "")
			this->enablePort = this->findDigitalPort("Enable", this->enable, true);
		// no enable port? assume always enabled
		if (this->enablePort == NULL)
			this->isMotorEnabled = true;

	} else if (this->mode == SERIAL_RELAY) {
		this->directionPort = this->findDigitalPort("Direction", this->direction, true);
		this->enablePort = this->findDigitalPort("Enable", this->enable, true);
	} else
		throw Poco::DataException(std::string(this->getID()) + ": Unknown window mode; only H-Bridge and SerialRelay are supported");

	if (this->statusPortStr != "")
		this->statusPort = this->findSelectPort("StatusPort", this->statusPortStr, true);

	this->findDigitalPorts("AutoOpen", this->autoOpen, this->autoOpenPorts);
	this->findDigitalPorts("AutoClose", this->autoClose, this->autoClosePorts);
	this->findDigitalPorts("ForceOpen", this->forceOpen, this->forceOpenPorts);
	this->findDigitalPorts("ForceClose", this->forceClose, this->forceClosePorts);
	this->findDigitalPorts("ErrorPorts", this->errorPortStr, this->errorPorts);
	this->findDigitalPorts("ResetPorts", this->resetPortStr, this->resetPorts);
	
	// a window port normally refreshes itself automatically unless specified otherwise
	if (this->refreshMode == REFRESH_NOT_SET)
		this->refreshMode = REFRESH_AUTO;
}

uint8_t WindowPort::getPortLine(OPDI_DigitalPort *port) {
	uint8_t mode;
	uint8_t line;
	port->getState(&mode, &line);
	return line;
}

void WindowPort::setPortLine(OPDI_DigitalPort *port, uint8_t newLine) {
	uint8_t mode;
	uint8_t line;
	port->getState(&mode, &line);
	port->setLine(newLine);
}

bool WindowPort::isSensorClosed(void) {
	// sensor not present?
	if (this->sensorPort == NULL)
		return false;
	// query the sensor
	if (this->getPortLine(this->sensorPort) == this->sensorClosed)
		return true;
	else
		return false;
}

void WindowPort::enableMotor(void) {
	if (this->enablePort == NULL)
		return;
	this->logDebug(std::string(this->id) + ": Enabling motor");
	this->setPortLine(this->enablePort, (this->enableActive == 1 ? 1 : 0));
	this->isMotorEnabled = true;
}

void WindowPort::disableMotor(void) {
	if (this->enablePort == NULL)
		return;
	this->logDebug(std::string(this->id) + ": Disabling motor");
	this->setPortLine(this->enablePort, (this->enableActive == 1 ? 0 : 1));
	// relay mode?
	if (this->mode == SERIAL_RELAY) {
		// disable direction (setting the relay to the OFF position in order not to waste energy)
		this->setPortLine(this->directionPort, (this->motorActive == 1 ? 0 : 1));
	}	
	this->isMotorEnabled = false;
}

void WindowPort::setMotorOpening(void) {
	this->logDebug(std::string(this->id) + ": Setting motor to 'opening'");
	if (this->mode == H_BRIDGE) {
		this->setPortLine(this->motorAPort, (this->motorActive == 1 ? 1 : 0));
		this->setPortLine(this->motorBPort, (this->motorActive == 1 ? 0 : 1));
	} else if (this->mode == SERIAL_RELAY) {
		this->setPortLine(this->directionPort, (this->motorActive == 1 ? 1 : 0));
	}
	this->isMotorOn = true;
}

void WindowPort::setMotorClosing(void) {
	this->logDebug(std::string(this->id) + ": Setting motor to 'closing'");
	if (this->mode == H_BRIDGE) {
		this->setPortLine(this->motorAPort, (this->motorActive == 1 ? 0 : 1));
		this->setPortLine(this->motorBPort, (this->motorActive == 1 ? 1 : 0));
	} else if (this->mode == SERIAL_RELAY) {
		this->setPortLine(this->directionPort, (this->motorActive == 1 ? 0 : 1));
	}
	this->isMotorOn = true;
}

void WindowPort::setMotorOff(void) {
	this->logDebug(std::string(this->id) + ": Stopping motor");
	if (this->mode == H_BRIDGE) {
		this->setPortLine(this->motorAPort, (this->motorActive == 1 ? 0 : 1));
		this->setPortLine(this->motorBPort, (this->motorActive == 1 ? 0 : 1));
	} else if (this->mode == SERIAL_RELAY)
		throw Poco::ApplicationException(std::string(this->getID()) + ": Cannot set motor off in Serial Relay Mode");
	this->isMotorOn = false;
}

std::string WindowPort::getMotorStateText(void) {
	if (this->mode == H_BRIDGE) {
		if (this->isMotorOn)
			return " (Motor is on)";
		else
			return std::string(" (Motor is off and ") + (this->isMotorEnabled ? "enabled" : "disabled") + ")";
	} else if (this->mode == SERIAL_RELAY) {
		return std::string(" (Motor is ") + (this->isMotorEnabled ? "enabled" : "disabled") + ")";
	} else
		return std::string(" (Mode is unknown)");
}

std::string WindowPort::getStateText(WindowState state) {
	switch (state) {
		case UNKNOWN:                        return std::string("UNKNOWN");
		case UNKNOWN_WAITING:				 return std::string("UNKNOWN_WAITING");
		case CLOSED:						 return std::string("CLOSED");
		case OPEN:							 return std::string("OPEN");
		case WAITING_AFTER_ENABLE:			 return std::string("WAITING_AFTER_ENABLE");
		case WAITING_BEFORE_DISABLE_OPEN:	 return std::string("WAITING_BEFORE_DISABLE_OPEN");
		case WAITING_BEFORE_DISABLE_CLOSED:	 return std::string("WAITING_BEFORE_DISABLE_CLOSED");
		case WAITING_BEFORE_DISABLE_ERROR:	 return std::string("WAITING_BEFORE_DISABLE_ERROR");
		case WAITING_BEFORE_ENABLE_OPENING:	 return std::string("WAITING_BEFORE_ENABLE_OPENING");
		case WAITING_BEFORE_ENABLE_CLOSING:	 return std::string("WAITING_BEFORE_ENABLE_CLOSING");
		case WAITING_AFTER_DISABLE:			 return std::string("WAITING_AFTER_DISABLE");
		case CLOSING:						 return std::string("CLOSING");
		case OPENING:						 return std::string("OPENING");
		case ERR:							 return std::string("ERR");
	};
	return std::string("unknown state ") + to_string(state);
}

void WindowPort::setCurrentState(WindowState state) {
	if (this->currentState != state) {
		this->logVerbose(std::string(this->id) + ": Changing current state to: " + this->getStateText(state) + this->getMotorStateText());

		// if set to ERR or ERR is cleared, notify the ErrorPorts
		bool notifyErrorPorts = (this->currentState == ERR) || (state == ERR);
		this->currentState = state;

		if (notifyErrorPorts) {
			// go through error ports
			DigitalPortList::iterator pi = this->errorPorts.begin();
			while (pi != this->errorPorts.end()) {
				this->logDebug(std::string(this->id) + ": Notifying error port: " + (*pi)->getID() + ": " + (state == ERR ? "Entering" : "Leaving") + " error state");
				this->setPortLine((*pi), (state == ERR ? 1 : 0));
				pi++;
			}
		}

		// undefined states reset the target
		if ((state == UNKNOWN) || (state == ERR))
			this->setTargetState(UNKNOWN);
		
		if ((state == UNKNOWN) ||
			(state == CLOSED) || 
			(state == OPEN) || 
			(state == ERR)) {

			if ((this->targetState != UNKNOWN) && (state == CLOSED) && (this->positionAfterClose >= 0) && (this->position != POSITION_OFF))
				this->setPosition(this->positionAfterClose);
			if ((this->targetState != UNKNOWN) && (state == OPEN) && (this->positionAfterOpen >= 0) && (this->position != POSITION_OFF))
				this->setPosition(this->positionAfterOpen);

			this->refreshRequired = (this->refreshMode == REFRESH_AUTO);
		}
		
		// update status port?
		if ((this->statusPort != NULL) &&
			((state == UNKNOWN) ||
			(state == CLOSED) || 
			(state == OPEN) || 
			(state == CLOSING) || 
			(state == OPENING) || 
			(state == ERR))) {

			// translate to select port's position
			uint16_t selPortPos = 0;
			switch (state) {
			case UNKNOWN: selPortPos = 0; break;
			case CLOSED: selPortPos = 1; break;
			case OPEN: selPortPos = 2; break;
			case CLOSING: selPortPos = 3; break;
			case OPENING: selPortPos = 4; break;
			case ERR: selPortPos = 5; break;
			default: break;
			}

			this->logDebug(std::string(this->id) + ": Notifying status port: " + this->statusPort->getID() + ": new position = " + to_string((int)selPortPos));

			this->statusPort->setPosition(selPortPos);
		}
	}
}

void WindowPort::setTargetState(WindowState state) {
	if (this->targetState != state) {
		this->logVerbose(std::string(this->id) + ": Changing target state to: " + this->getStateText(state));
		this->targetState = state;
	}
}

void WindowPort::checkOpenHBridge(void) {
	if (this->targetState == OPEN) {					
		if (this->enableDelay > 0) {					
			this->delayTimer = opdi_get_time_ms();		
			this->enableMotor();						
			this->setCurrentState(WAITING_AFTER_ENABLE);
		} else {										
			this->openTimer = opdi_get_time_ms();		
			this->setMotorOpening();					
			this->setCurrentState(OPENING);				
		}												
	}
}

void WindowPort::checkCloseHBridge(void) {
	if (this->targetState == CLOSED) {					
		if (this->enableDelay > 0) {					
			this->delayTimer = opdi_get_time_ms();		
			this->enableMotor();						
			this->setCurrentState(WAITING_AFTER_ENABLE);
		} else {										
			this->openTimer = opdi_get_time_ms();		
			this->setMotorClosing();					
			this->setCurrentState(CLOSING);				
		}												
	}
}

uint8_t WindowPort::doWork(uint8_t canSend)  {
	OPDI_SelectPort::doWork(canSend);

	// state machine implementations

	if (this->mode == H_BRIDGE) {
		// state machine for H-Bridge mode

		// unknown state (first call or off or transition not yet implemented)? initialize
		if (this->currentState == UNKNOWN) {
			// disable the motor
			this->setMotorOff();
			this->disableMotor();
		
			// do we know that we're closed?
			if (this->isSensorClosed()) {
				this->logDebug(std::string(this->id) + ": Closing sensor signal detected");
				this->setCurrentState(CLOSED);
			} else
				// we don't know
				this->setCurrentState(UNKNOWN_WAITING);

			// set target state according to selected position (== mode)
			if (this->position == POSITION_OPEN) {
				this->setTargetState(OPEN);
			} else
			if (this->position == POSITION_CLOSED) {
				this->setTargetState(CLOSED);
			}
		} else
		// unknown; waiting for command
		if ((this->currentState == UNKNOWN_WAITING)) {
			if (this->isSensorClosed()) {
				this->logDebug(std::string(this->id) + ": Closing sensor signal detected");
				this->setCurrentState(CLOSED);
			} else {
				// do not change current state

				// opening required?
				this->checkOpenHBridge();

				// closing required?
				this->checkCloseHBridge();
			}
		} else
		// open?
		if (this->currentState == OPEN) {
			// closing required?
			this->checkCloseHBridge();
		} else
		// closed?
		if (this->currentState == CLOSED) {
			// opening required?
			this->checkOpenHBridge();
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
					this->logNormal(std::string(this->id) + ": Warning: Closing sensor signal still present after opening");
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
				this->logDebug(std::string(this->id) + ": Closing sensor signal detected");

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
					this->logNormal(std::string(this->id) + ": Warning: Closing sensor signal not detected while closing");

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
	}	// end of H-Bridge Mode state machine

	else if (this->mode == SERIAL_RELAY) {
		// state machine for serial relay mode

		// unknown state (first call or off or transition not yet implemented)? initialize
		if (this->currentState == UNKNOWN) {
			// disable the motor
			this->delayTimer = opdi_get_time_ms();
			this->disableMotor();
			this->setCurrentState(WAITING_AFTER_DISABLE);

			// set target state according to selected position
			if (this->position == POSITION_OPEN) {
				this->setTargetState(OPEN);
			} else
			if (this->position == POSITION_CLOSED) {
				this->setTargetState(CLOSED);
			}
		} else
		// waiting after disable?
		if (this->currentState == WAITING_AFTER_DISABLE) {
			// waiting time up?
			if (opdi_get_time_ms() - this->delayTimer > this->enableDelay) {
				// disable delay is up
			
				// target state is unknown; wait
				this->setCurrentState(UNKNOWN_WAITING);
			}
		} else
		// unknown; waiting for command
		if ((this->currentState == UNKNOWN_WAITING)) {
			if (this->isSensorClosed()) {
				this->logDebug(std::string(this->id) + ": Closing sensor signal detected");
				this->setCurrentState(CLOSED);
			} else {
				// do not change current state

				// opening required?
				if (this->targetState == OPEN) {
					this->delayTimer = opdi_get_time_ms();
					this->setCurrentState(WAITING_BEFORE_ENABLE_OPENING);
				}

				// closing required?
				if (this->targetState == CLOSED) {
					this->delayTimer = opdi_get_time_ms();
					this->setCurrentState(WAITING_BEFORE_ENABLE_CLOSING);
				}
			}
		} else
		// open?
		if (this->currentState == OPEN) {
			// closing required?
			if (this->targetState == CLOSED) {
				// disable the motor
				this->delayTimer = opdi_get_time_ms();
				this->disableMotor();
				this->setCurrentState(WAITING_BEFORE_ENABLE_CLOSING);
			}
		} else
		// closed?
		if (this->currentState == CLOSED) {
			// opening required?
			if (this->targetState == OPEN) {
				// disable the motor
				this->delayTimer = opdi_get_time_ms();
				this->disableMotor();
				this->setCurrentState(WAITING_BEFORE_ENABLE_OPENING);
			}
		} else
		// waiting before enable before opening?
		if (this->currentState == WAITING_BEFORE_ENABLE_OPENING) {
			if (this->targetState == CLOSED) {
				// need to close
				this->delayTimer = opdi_get_time_ms();
				this->setCurrentState(WAITING_BEFORE_ENABLE_CLOSING);
			} else
			// waiting time up?
			if (opdi_get_time_ms() - this->delayTimer > this->enableDelay) {
				this->openTimer = opdi_get_time_ms();
				// set direction
				this->setMotorOpening();
				this->enableMotor();
			
				this->setCurrentState(OPENING);
			}
		} else
		// waiting before enable before closing?
		if (this->currentState == WAITING_BEFORE_ENABLE_CLOSING) {
			if (this->targetState == OPEN) {
				// need to open
				this->delayTimer = opdi_get_time_ms();
				this->setCurrentState(WAITING_BEFORE_ENABLE_OPENING);
			} else
			// waiting time up?
			if (opdi_get_time_ms() - this->delayTimer > this->enableDelay) {
				this->openTimer = opdi_get_time_ms();
				// set direction
				this->setMotorClosing();
				this->enableMotor();
			
				this->setCurrentState(CLOSING);
			}
		} else
		// opening?
		if (this->currentState == OPENING) {
			if (this->targetState == CLOSED) {
				// need to close
				this->delayTimer = opdi_get_time_ms();
				this->disableMotor();
				this->setCurrentState(WAITING_BEFORE_ENABLE_CLOSING);
			} else
			// opening time up?
			if (opdi_get_time_ms() - this->openTimer > this->openingTime) {
				// stop motor
				this->disableMotor();
				// sensor still closed? (error condition)
				if (this->isSensorClosed()) {
					this->logNormal(std::string(this->id) + ": Warning: Closing sensor signal still present after opening");
					this->setCurrentState(ERR);
				} else
					// assume that the window is open now
					this->setCurrentState(OPEN);
			}
		} else
		// closing?
		if (this->currentState == CLOSING) {
			if (this->targetState == OPEN) {
				// need to open
				this->delayTimer = opdi_get_time_ms();
				this->disableMotor();
				this->setCurrentState(WAITING_BEFORE_ENABLE_OPENING);
			} else
			// sensor close detected?
			if (this->isSensorClosed()) {
				this->logDebug(std::string(this->id) + ": Closing sensor signal detected");
				// stop motor immediately
				this->disableMotor();
				this->setCurrentState(CLOSED);
			}
			else
			// closing time up?
			if (opdi_get_time_ms() - this->openTimer > this->closingTime) {
				// stop motor immediately
				this->disableMotor();

				// without detecting sensor? error condition
				if (this->sensorPort != NULL) {
					this->logNormal(std::string(this->id) + ": Warning: Closing sensor signal not detected while closing");

					this->setCurrentState(ERR);
				} else {
					// sensor not present - assume closed
					this->setCurrentState(CLOSED);
				}
			}
		}
	}

	DigitalPortList::const_iterator pi;
	bool forceOpen = false;
	bool forceClose = false;

	// if the window has detected an error, do not automatically open or close
	if (this->currentState != ERR) {
		// if one of the ForceOpen ports is High, the window must be opened
		pi = this->forceOpenPorts.begin();
		while (pi != this->forceOpenPorts.end()) {
			if (this->getPortLine(*pi) == 1) {
				this->logExtreme(std::string(this->id) + ": ForceOpen detected from port: " + (*pi)->getID());
				forceOpen = true;
				break;
			}
			pi++;
		}
		// if one of the ForceClose ports is High, the window must be closed
		pi = this->forceClosePorts.begin();
		while (pi != this->forceClosePorts.end()) {
			if (this->getPortLine(*pi) == 1) {
				this->logExtreme(std::string(this->id) + ": ForceClose detected from port: " + (*pi)->getID());
				forceClose = true;
				break;
			}
			pi++;
		}
	} else {
		// if one of the Reset ports is High, the window should be reset to the defined state
		pi = this->resetPorts.begin();
		while (pi != this->resetPorts.end()) {
			if (this->getPortLine(*pi) == 1) {
				this->logExtreme(std::string(this->id) + ": Reset detected from port: " + (*pi)->getID());
				this->setCurrentState(UNKNOWN);
				if (this->resetTo == RESET_TO_CLOSED)
					forceClose = true;
				else if (this->resetTo == RESET_TO_OPEN)
					forceOpen = true;
				break;
			}
			pi++;
		}
	}

	if (forceOpen) {
		this->setTargetState(OPEN);
	} else if (forceClose) {
		this->setTargetState(CLOSED);
	} else {
		WindowState target = UNKNOWN;
		// if the window has detected an error, do not automatically open or close
		// if the position is set to automatic, evaluate auto ports
		if ((this->currentState != ERR) && (this->position >= POSITION_AUTO)) {
			// if one of the AutoClose ports is High, the window should be closed (takes precedence)
			pi = this->autoClosePorts.begin();
			while (pi != this->autoClosePorts.end()) {
				if (this->getPortLine(*pi) == 1) {
					// avoid repeating messages
					if (this->targetState != CLOSED) {
						this->logExtreme(std::string(this->id) + ": AutoClose detected from port: " + (*pi)->getID());
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
					if (this->getPortLine(*pi) == 1) {
						// avoid repeating messages
						if (this->targetState != OPEN) {
							this->logExtreme(std::string(this->id) + ": AutoOpen detected from port: " + (*pi)->getID());
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
		if (this->positionNewlySet && (this->position == POSITION_OFF)) {
			// setting the window "UNKNOWN" disables the motor and re-initializes
			this->setCurrentState(UNKNOWN);
		} else
		if (this->positionNewlySet && (this->position == POSITION_OPEN)) {
			target = OPEN;
		} else
		if (this->positionNewlySet && (this->position == POSITION_CLOSED)) {
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

	return OPDI_STATUS_OK;
}


////////////////////////////////////////////////////////////////////////
// Plugin main class
////////////////////////////////////////////////////////////////////////

class WindowPlugin : public IOPDIDPlugin, public IOPDIDConnectionListener {

protected:
	AbstractOPDID *opdid;

public:
	virtual void setupPlugin(AbstractOPDID *abstractOPDID, std::string node, Poco::Util::AbstractConfiguration *config) override;

	virtual void masterConnected(void) override;
	virtual void masterDisconnected(void) override;
};

void WindowPlugin::setupPlugin(AbstractOPDID *abstractOPDID, std::string node, Poco::Util::AbstractConfiguration *config) {
	this->opdid = abstractOPDID;

	Poco::Util::AbstractConfiguration *nodeConfig = config->createView(node);

	// get port type
	std::string portType = nodeConfig->getString("Type", "");

	if (portType == "SelectPort") {
		// create window port
		WindowPort *port = new WindowPort(abstractOPDID, node.c_str());
		abstractOPDID->configureSelectPort(nodeConfig, config, port);

		port->logVerbosity = this->opdid->getConfigLogVerbosity(nodeConfig, AbstractOPDID::UNKNOWN);

		port->enableDelay = nodeConfig->getInt("EnableDelay", 0);
		if (port->enableDelay < 0)
			throw Poco::DataException("EnableDelay may not be negative: " + abstractOPDID->to_string(port->enableDelay));

		// read control mode
		std::string controlMode = abstractOPDID->getConfigString(nodeConfig, "ControlMode", "", true);
		if (controlMode == "H-Bridge") {
			if ((port->logVerbosity == AbstractOPDID::UNKNOWN) || (port->logVerbosity >= AbstractOPDID::VERBOSE))
				this->opdid->logVerbose("Configuring WindowPlugin port " + node + " in H-Bridge Mode");
			port->mode = WindowPort::H_BRIDGE;
			// motorA and motorB are required
			port->motorA = abstractOPDID->getConfigString(nodeConfig, "MotorA", "", true);
			port->motorB = abstractOPDID->getConfigString(nodeConfig, "MotorB", "", true);
			port->motorDelay = nodeConfig->getInt("MotorDelay", 0);
			if (port->motorDelay < 0)
				throw Poco::DataException("MotorDelay may not be negative: " + abstractOPDID->to_string(port->motorDelay));
			port->enable = nodeConfig->getString("Enable", "");
			if (port->enableDelay < port->motorDelay)
				throw Poco::DataException("If using MotorDelay, EnableDelay must be greater or equal: " + abstractOPDID->to_string(port->enableDelay));
		} else if (controlMode == "SerialRelay") {
			if ((port->logVerbosity == AbstractOPDID::UNKNOWN) || (port->logVerbosity >= AbstractOPDID::VERBOSE))
				this->opdid->logVerbose("Configuring WindowPlugin port " + node + " in Serial Relay Mode");
			port->mode = WindowPort::SERIAL_RELAY;
			// direction and enable ports are required
			port->direction = abstractOPDID->getConfigString(nodeConfig, "Direction", "", true);
			port->enable = abstractOPDID->getConfigString(nodeConfig, "Enable", "", true);
		} else
			throw Poco::DataException("ControlMode setting not supported; expected 'H-Bridge' or 'SerialRelay'", controlMode);

		// read additional configuration parameters
		port->sensor = abstractOPDID->getConfigString(nodeConfig, "Sensor", "", false);
		port->sensorClosed = (uint8_t)nodeConfig->getInt("SensorClosed", 1);
		if (port->sensorClosed > 1)
			throw Poco::DataException("SensorClosed must be either 0 or 1: " + abstractOPDID->to_string(port->sensorClosed));
		port->motorActive = (uint8_t)nodeConfig->getInt("MotorActive", 1);
		if (port->motorActive > 1)
			throw Poco::DataException("MotorActive must be either 0 or 1: " + abstractOPDID->to_string(port->motorActive));
		port->enableActive = (uint8_t)nodeConfig->getInt("EnableActive", 1);
		if (port->enableActive > 1)
			throw Poco::DataException("EnableActive must be either 0 or 1: " + abstractOPDID->to_string(port->enableActive));
		port->openingTime = nodeConfig->getInt("OpeningTime", 0);
		if (port->openingTime <= 0)
			throw Poco::DataException("OpeningTime must be specified and greater than 0: " + abstractOPDID->to_string(port->openingTime));
		port->closingTime = nodeConfig->getInt64("ClosingTime", port->openingTime);
		if (port->closingTime <= 0)
			throw Poco::DataException("ClosingTime must be greater than 0: " + abstractOPDID->to_string(port->closingTime));
		port->autoOpen = abstractOPDID->getConfigString(nodeConfig, "AutoOpen", "", false);
		port->autoClose = abstractOPDID->getConfigString(nodeConfig, "AutoClose", "", false);
		port->forceOpen = abstractOPDID->getConfigString(nodeConfig, "ForceOpen", "", false);
		port->forceClose = abstractOPDID->getConfigString(nodeConfig, "ForceClose", "", false);
		if ((port->forceOpen != "") && (port->forceClose != ""))
			throw Poco::DataException("You cannot use ForceOpen and ForceClose at the same time");
		port->statusPortStr = abstractOPDID->getConfigString(nodeConfig, "StatusPort", "", false);
		port->errorPortStr = abstractOPDID->getConfigString(nodeConfig, "ErrorPorts", "", false);
		port->resetPortStr = abstractOPDID->getConfigString(nodeConfig, "ResetPorts", "", false);
		std::string resetTo = abstractOPDID->getConfigString(nodeConfig, "ResetTo", "Off", false);
		if (resetTo == "Closed") {
			port->resetTo = WindowPort::RESET_TO_CLOSED;
		} else if (resetTo == "Open") {
			port->resetTo = WindowPort::RESET_TO_OPEN;
		} else if (resetTo != "Off")
			throw Poco::DataException("Invalid value for the ResetTo setting; expected 'Off', 'Closed' or 'Open'", resetTo);
		port->positionAfterClose = nodeConfig->getInt("PositionAfterClose", -1);
		port->positionAfterOpen = nodeConfig->getInt("PositionAfterOpen", -1);

		abstractOPDID->addPort(port);
	} else
		throw Poco::DataException("Node type 'SelectPort' expected", portType);

	this->opdid->addConnectionListener(this);

	if (this->opdid->logVerbosity >= AbstractOPDID::VERBOSE)
		this->opdid->logVerbose("WindowPlugin setup completed successfully as node " + node);
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
	if ((majorVersion > OPDID_MAJOR_VERSION) || (minorVersion > OPDID_MINOR_VERSION))
		throw Poco::Exception("This plugin supports only OPDID versions up to "
			OPDI_QUOTE(OPDID_MAJOR_VERSION) "." OPDI_QUOTE(OPDID_MINOR_VERSION));

	// return a new instance of this plugin
	return new WindowPlugin();
}
