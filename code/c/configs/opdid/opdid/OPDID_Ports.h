#pragma once

// need to guard against security check warnings
#define _SCL_SECURE_NO_WARNINGS	1

#include <sstream>
#include <fstream>
#include <list>

#include "Poco/Util/AbstractConfiguration.h"
#include "Poco/TimedNotificationQueue.h"
#include "Poco/DirectoryWatcher.h"

// serial port library
#include "ctb-0.16/ctb.h"

#include "opdi_constants.h"

#include "OPDID_PortFunctions.h"

///////////////////////////////////////////////////////////////////////////////
// Logic Port
///////////////////////////////////////////////////////////////////////////////

/** A LogicPort implements logic functions for digital ports. It supports
* the following operations:
* - OR (default): The line is High if at least one of its inputs is High
* - AND: The line is High if all of its inputs are High
* - XOR: The line is High if an odd number of its inputs is High
* - ATLEAST (n): The line is High if at least n inputs are High
* - ATMOST (n): The line is High if at most n inputs are High
* - EXACT (n): The line is High if exactly n inputs are High
* Additionally you can specify whether the output should be negated.
* The LogicPort requires at least one digital port as input. The output
* can optionally be distributed to an arbitrary number of digital ports.
* Processing occurs in the OPDI waiting event loop.
* All input ports' state is queried. If the logic function results in a change
* of this port's state the new state is set on the output ports. This means that
* there is no unnecessary continuous state propagation.
* If the port is not hidden it will perform a self-refresh when the state changes.
* Non-hidden output ports whose state is changed will be refreshed.
* You can also specify inverted output ports who will be updated with the negated
* state of this port.
*/
class OPDID_LogicPort : public OPDI_DigitalPort, protected OPDID_PortFunctions {
protected:
	enum LogicFunction {
		UNKNOWN,
		OR,
		AND,
		XOR,
		ATLEAST,
		ATMOST,
		EXACT
	};

	LogicFunction function;
	size_t funcN;
	bool negate;
	std::string inputPortStr;
	std::string outputPortStr;
	std::string inverseOutputPortStr;
	DigitalPortList inputPorts;
	DigitalPortList outputPorts;
	DigitalPortList inverseOutputPorts;

	virtual uint8_t doWork(uint8_t canSend) override;

public:
	OPDID_LogicPort(AbstractOPDID *opdid, const char *id);

	virtual ~OPDID_LogicPort();

	virtual void configure(Poco::Util::AbstractConfiguration *config);

	virtual void setDirCaps(const char *dirCaps) override;

	virtual void setMode(uint8_t mode) override;

	virtual void setLine(uint8_t line) override;

	virtual void prepare() override;
};

///////////////////////////////////////////////////////////////////////////////
// Pulse Port
///////////////////////////////////////////////////////////////////////////////

/** A pulse port generates a digital pulse with a defined period (measured
* in milliseconds) and a duty cycle in percent. The period and duty cycle
* can optionally be set by analog ports. The period is in this case
* calculated as the percentage of Period. The pulse is active if the line
* of this port is set to High. If enable digital ports are specified the
* pulse is also being generated if at least one of the enable ports is High.
* The output can be normal or inverted. There are two lists of output digital
* ports which receive the normal or inverted output respectively.
*/
class OPDID_PulsePort : public OPDI_DigitalPort, protected OPDID_PortFunctions {
protected:
	bool negate;
	ValueResolver period;
	ValueResolver dutyCycle;
	int8_t disabledState;
	std::string enablePortStr;
	std::string outputPortStr;
	std::string inverseOutputPortStr;
	typedef std::vector<OPDI_DigitalPort *> PortList;
	DigitalPortList enablePorts;
	DigitalPortList outputPorts;
	DigitalPortList inverseOutputPorts;

	// state
	uint8_t pulseState;
	uint64_t lastStateChangeTime;

	virtual uint8_t doWork(uint8_t canSend) override;

public:
	OPDID_PulsePort(AbstractOPDID *opdid, const char *id);

	virtual ~OPDID_PulsePort();

	virtual void configure(Poco::Util::AbstractConfiguration *config);

	virtual void setDirCaps(const char *dirCaps) override;

	virtual void setMode(uint8_t mode) override;

	virtual void prepare() override;
};

///////////////////////////////////////////////////////////////////////////////
// Selector Port
///////////////////////////////////////////////////////////////////////////////

/** A SelectorPort is a DigitalPort that is High when the specified select port
*   is in the specified position and Low otherwise. If set to High it will switch
*   the select port to the specified position. If set to Low, it will do nothing.
*/
class OPDID_SelectorPort : public OPDI_DigitalPort, protected OPDID_PortFunctions {
protected:
	std::string selectPortStr;
	OPDI_SelectPort *selectPort;
	uint16_t position;

	virtual uint8_t doWork(uint8_t canSend) override;

public:
	OPDID_SelectorPort(AbstractOPDID *opdid, const char *id);

	virtual ~OPDID_SelectorPort();

	virtual void configure(Poco::Util::AbstractConfiguration *config);

	virtual void setDirCaps(const char *dirCaps);

	virtual void setMode(uint8_t mode);

	virtual void setLine(uint8_t line);

	virtual void prepare();
};

///////////////////////////////////////////////////////////////////////////////
// Error Detector Port
///////////////////////////////////////////////////////////////////////////////

/** An ErrorDetectorPort is a DigitalPort whose state is determined by one or more 
*   specified ports. If any of these ports will have an error, i. e. their hasError()
*   method returns true, the state of this port will be High and Low otherwise.
*   The logic level can be negated.
*/
class OPDID_ErrorDetectorPort : public OPDI_DigitalPort, protected OPDID_PortFunctions {
protected:
	bool negate;
	std::string inputPortStr;
	OPDI::PortList inputPorts;

	virtual uint8_t doWork(uint8_t canSend) override;

public:
	OPDID_ErrorDetectorPort(AbstractOPDID *opdid, const char *id);

	virtual ~OPDID_ErrorDetectorPort();

	virtual void configure(Poco::Util::AbstractConfiguration *config);

	virtual void setDirCaps(const char *dirCaps) override;

	virtual void setMode(uint8_t mode) override;

	virtual void prepare() override;
};

///////////////////////////////////////////////////////////////////////////////
// Serial Streaming Port
///////////////////////////////////////////////////////////////////////////////

/** Defines a serial streaming port that supports streaming from and to a serial port device.
 */
class OPDID_SerialStreamingPort : public OPDI_StreamingPort, protected OPDID_PortFunctions {
friend class OPDI;

protected:
	// a serial streaming port may pass the bytes through or return them in the doWork method (loopback)
	enum Mode {
		PASS_THROUGH,
		LOOPBACK
	};

	Mode mode;

	ctb::IOBase* device;
	ctb::SerialPort* serialPort;

	virtual uint8_t doWork(uint8_t canSend) override;

public:
	OPDID_SerialStreamingPort(AbstractOPDID *opdid, const char *id);

	virtual ~OPDID_SerialStreamingPort();

	virtual void configure(Poco::Util::AbstractConfiguration *config);

	virtual int write(char *bytes, size_t length) override;

	virtual int available(size_t count) override;

	virtual int read(char *result) override;

	virtual bool hasError(void) override;
};

///////////////////////////////////////////////////////////////////////////////
// Logger Streaming Port
///////////////////////////////////////////////////////////////////////////////

/** Defines a streaming port that can log port states and optionally write them to a log file.
 */
class OPDID_LoggerPort : public OPDI_StreamingPort, protected OPDID_PortFunctions {
friend class OPDI;

protected:
	enum Format {
		CSV
	};

	uint32_t logPeriod;
	Format format;
	std::string separator;
	std::string portsToLogStr;
	OPDI::PortList portsToLog;
	bool writeHeader;
	uint64_t lastEntryTime;
	
	std::ofstream outFile;

	std::string getPortStateStr(OPDI_Port* port);

	virtual uint8_t doWork(uint8_t canSend) override;

public:
	OPDID_LoggerPort(AbstractOPDID *opdid, const char *id);

	virtual ~OPDID_LoggerPort();

	virtual void configure(Poco::Util::AbstractConfiguration *config);

	virtual void prepare() override;

	virtual int write(char *bytes, size_t length) override;

	virtual int available(size_t count) override;

	virtual int read(char *result) override;

	virtual bool hasError(void) override;
};

///////////////////////////////////////////////////////////////////////////////
// Fader Port
///////////////////////////////////////////////////////////////////////////////

/** A FaderPort can fade AnalogPorts or DialPorts in or out. You can set a left and right value
* as well as a duration time in milliseconds. If the FaderPort is inactive (line = Low),
* it will not act on the output ports. If it is set to High it will start at the
* left value and count to the right value within the specified time. The values have 
* to be specified as percentages. Once the duration is up the port sets itself to 
* inactive (line = Low). When the fader is done it can set optionally specified DigitalPorts
* (end switches) to High.
* If ReturnToLeft is specified as true, the output port is set to the current value of Left
* when switched off.
*/
class OPDID_FaderPort : public OPDI_DigitalPort, protected OPDID_PortFunctions {
protected:
	enum FaderMode {
		LINEAR,
		EXPONENTIAL
	};

	enum SwitchOffAction {
		NONE,
		SET_TO_LEFT,
		SET_TO_RIGHT
	};

	FaderMode mode;
	ValueResolver leftValue;
	ValueResolver rightValue;
	ValueResolver durationMsValue;
	double left;
	double right;
	double durationMs;
	double expA;
	double expB;
	double expMax;
	bool invert;
	SwitchOffAction switchOffAction;

	std::string outputPortStr;
	PortList outputPorts;

	std::string endSwitchesStr;
	DigitalPortList endSwitches;

	Poco::Timestamp startTime;
	double lastValue;
	SwitchOffAction actionToPerform;

	virtual uint8_t doWork(uint8_t canSend) override;

public:
	OPDID_FaderPort(AbstractOPDID *opdid, const char *id);

	virtual ~OPDID_FaderPort();

	virtual void configure(Poco::Util::AbstractConfiguration *config);

	virtual void setDirCaps(const char *dirCaps) override;

	virtual void setMode(uint8_t mode) override;

	virtual void setLine(uint8_t line) override;

	virtual void prepare() override;
};

///////////////////////////////////////////////////////////////////////////////
// Scene Select Port
///////////////////////////////////////////////////////////////////////////////

/** A SceneSelectPort is a select port with n scene settings. Each scene setting corresponds
* with a settings file. If a scene is selected the port opens the settings file and sets all
* ports defined in the settings file to the specified values.
* The SceneSelectPort automatically sends a "Refresh all" message to a connected master when
* a scene has been selected.
*/
class OPDID_SceneSelectPort : public OPDI_SelectPort, protected OPDID_PortFunctions {
protected:
	typedef std::vector<std::string> FileList;
	FileList fileList;
	std::string configFilePath;

	bool positionSet;

	virtual uint8_t doWork(uint8_t canSend) override;

public:
	OPDID_SceneSelectPort(AbstractOPDID *opdid, const char *id);

	virtual ~OPDID_SceneSelectPort();

	virtual void configure(Poco::Util::AbstractConfiguration *config, Poco::Util::AbstractConfiguration *parentConfig);

	virtual void setPosition(uint16_t position) override;

	virtual void prepare() override;
};

///////////////////////////////////////////////////////////////////////////////
// File Input Port
///////////////////////////////////////////////////////////////////////////////

/** A FileInputPort is a port that reads its state from a file. It is represented by
* a DigitalPort; as such it can be either active (line = High) or inactive (line = Low).
* The FileInputPort actually consists of two ports: 1.) one DigitalPort which handles
* the actual file monitoring; it can be enabled and disabled, and 2.) one more port with
* a specified type that reflects the file content.
* You have to specify a PortNode setting which provides information about the actual
* input port that should reflect the file content. This node's configuration is
* evaluated just as if it was a standard port node, except that you cannot set
* the Mode setting to anything other than Input (if available).
* The port monitors the file and reads its contents when it changes. The content is
* parsed according to the port type:
*  - DigitalPort: content must be either 0 or 1
*  - AnalogPort: content must be a decimal number (separator '.') in range [0..1]
*  - DialPort: content must be a number in range [min..max] (it's automatically adjusted
*    to fit the step setting)
*  - SelectPort: content must be a positive integer or 0 specifying the select port position
*  - StreamingPort: content is injected into the streaming port as raw bytes
* Additionally you can specify a delay that signals how long to wait until the file is
* being re-read. This can help avoid too many refreshes.
* For analog and dial ports the value can be scaled using a numerator and a denominator.
*/
class OPDID_FileInputPort : public OPDI_DigitalPort, protected OPDID_PortFunctions {
protected:

	enum PortType {
		DIGITAL_PORT,
		ANALOG_PORT,
		DIAL_PORT,
		SELECT_PORT,
		STREAMING_PORT
	};

	std::string filePath;
	Poco::File directory;
	OPDI_Port *port;
	PortType portType;
	int reloadDelayMs;
	int expiryMs;
	int numerator;
	int denominator;

	Poco::DirectoryWatcher *directoryWatcher;
	uint64_t lastReloadTime;
	Poco::Mutex mutex;
	bool needsReload;

	virtual uint8_t doWork(uint8_t canSend) override;

	void fileChangedEvent(const void*, const Poco::DirectoryWatcher::DirectoryEvent&);

public:
	OPDID_FileInputPort(AbstractOPDID *opdid, const char *id);

	virtual ~OPDID_FileInputPort();

	virtual void configure(Poco::Util::AbstractConfiguration *config, Poco::Util::AbstractConfiguration *parentConfig);
};

///////////////////////////////////////////////////////////////////////////////
// Aggregator Port
///////////////////////////////////////////////////////////////////////////////

/** An AggregatorPort is a DigitalPort that collects the values of another port
* (the source port) and calculates with these historic values using specified
* calculations. The values can be multiplied by a specified factor to account
* for fractions that might otherwise be lost.
* The query interval and the number of values to collect must be specified.
* Additionally, a minimum and maximum delta value that must not be exceeded
* can be specified to validate the incoming values. A new value is compared against
* the last collected value. If the difference exceeds the specified bounds the
* whole collection is invalidated as a safeguard against implausible results.
* Each calculation presents its result as a DialPort. At startup all such ports
* will signal an error (invalid value). The port values will also become invalid 
* if delta values are exceeded or if the source port signals an error.
* A typical example for using this port is with a gas counter. Such a counter
* emits impulses corresponding to a certain consumed gas value. If consumption
* is low, the impulses will typically arrive very slowly, perhaps once every few
* minutes. It is interesting to calculate the average gas consumption per hour
* or per day. To achieve this values are collected over the specified period in
* regular intervals. The Delta algorithm causes the value of the AggregatorPort
* to be set to the difference of the last and the first value (moving window).
* Another example is the calculation of averages, for example, temperature.
* In this case you should can use Average or ArithmeticMean for the algorithm.
* Using AllowIncomplete you can specify that the result should also be computed
* if not all necessary values have been collected. For an averaging algorithm
* the default is True, while for the Delta algorithm the default is false.
* In the case of temperature it might be interesting to determine the minimum
* and maximum in the specified range. 
*/
class OPDID_AggregatorPort : public OPDI_DigitalPort, public OPDID_PortFunctions {
protected:
	enum Algorithm {
		DELTA,
		ARITHMETIC_MEAN,
		MINIMUM,
		MAXIMUM
	};

	class Calculation : public OPDI_DialPort {
	public:		
		Algorithm algorithm;
		bool allowIncomplete;

		Calculation(std::string id);

		void calculate(OPDID_AggregatorPort* aggregator);
	};

	std::string sourcePortID;
	OPDI_Port *sourcePort;
	uint64_t queryInterval;
	uint16_t totalValues;
	int32_t multiplier;
	int64_t minDelta;
	int64_t maxDelta;

	std::vector<int64_t> values;
	uint64_t lastQueryTime;

	std::vector<Calculation*> calculations;

	virtual uint8_t doWork(uint8_t canSend) override;

	void resetValues(void);

public:
	OPDID_AggregatorPort(AbstractOPDID *opdid, const char *id);

	virtual void configure(Poco::Util::AbstractConfiguration *portConfig, Poco::Util::AbstractConfiguration *parentConfig);

	virtual void prepare() override;

	virtual void setLine(uint8_t newLine) override;
};

///////////////////////////////////////////////////////////////////////////////
// Trigger Port
///////////////////////////////////////////////////////////////////////////////

/** A TriggerPort is a DigitalPort that continuously monitors one or more 
* digital ports for changes of their state. If a state change is detected it
* can change the state of other DigitalPorts.
* Triggering can happen on a change from Low to High (rising edge), from
* High to Low (falling edge), or on both changes.
* The effected state change can either be to set the output DigitalPorts
* High, Low, or toggle them. If you specify inverse output ports the state change
* is inverted, except for the Toggle specification.
* Disabling the TriggerPort sets all previously recorded port states to "unknown".
* No change is performed the first time a DigitalPort is read when its current
* state is unknown. A port that returns an error will also be set to "unknown".
*/
class OPDID_TriggerPort : public OPDI_DigitalPort, public OPDID_PortFunctions {
protected:

	enum TriggerType {
		RISING_EDGE,
		FALLING_EDGE,
		BOTH
	};

	enum ChangeType {
		SET_HIGH,
		SET_LOW,
		TOGGLE
	};

	enum PortState {
		UNKNOWN,
		LOW,
		HIGH
	};

	typedef Poco::Tuple<OPDI_DigitalPort*, PortState> PortData;
	typedef std::vector<PortData> PortDataList;

	std::string inputPortStr;
	std::string outputPortStr;
	std::string inverseOutputPortStr;
	DigitalPortList outputPorts;
	DigitalPortList inverseOutputPorts;
	TriggerType triggerType;
	ChangeType changeType;

	PortDataList portDataList;

	virtual uint8_t doWork(uint8_t canSend) override;

public:
	OPDID_TriggerPort(AbstractOPDID *opdid, const char *id);

	virtual void configure(Poco::Util::AbstractConfiguration *portConfig);

	virtual void prepare() override;

	virtual void setLine(uint8_t newLine) override;
};
