#pragma once

// need to guard against security check warnings
#define _SCL_SECURE_NO_WARNINGS	1

#include <sstream>
#include <fstream>
#include <list>

// expression evaluation library
#include <exprtk.hpp>

// serial port library
#include "ctb-0.16/ctb.h"

#include "Poco/Util/AbstractConfiguration.h"
#include "Poco/TimedNotificationQueue.h"

#include "opdi_constants.h"

#include "AbstractOPDID.h"


class OPDID_PortFunctions {
friend class AbstractOPDID;

protected:
	typedef std::vector<OPDI_Port *> PortList;
	typedef std::vector<OPDI_DigitalPort *> DigitalPortList;
	typedef std::vector<OPDI_AnalogPort *> AnalogPortList;

	AbstractOPDID *opdid;
	AbstractOPDID::LogVerbosity logVerbosity;

	OPDID_PortFunctions();

	virtual OPDI_Port *findPort(std::string configPort, std::string setting, std::string portID, bool required);

	virtual void findPorts(std::string configPort, std::string setting, std::string portIDs, PortList &portList);

	virtual OPDI_DigitalPort *findDigitalPort(std::string configPort, std::string setting, std::string portID, bool required);

	virtual void findDigitalPorts(std::string configPort, std::string setting, std::string portIDs, DigitalPortList &portList);

	virtual OPDI_AnalogPort *findAnalogPort(std::string configPort, std::string setting, std::string portID, bool required);

	virtual void findAnalogPorts(std::string configPort, std::string setting, std::string portIDs, AnalogPortList &portList);

	virtual OPDI_SelectPort *findSelectPort(std::string configPort, std::string setting, std::string portID, bool required);

	virtual void configureVerbosity(Poco::Util::AbstractConfiguration* config);

	virtual void logWarning(std::string message);

	virtual void logNormal(std::string message);

	virtual void logVerbose(std::string message);

	virtual void logDebug(std::string message);

	virtual void logExtreme(std::string message);
};

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
* Processing occurs in the OPDI waiting event loop, i. e. about once a ms.
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

	virtual uint8_t doWork(uint8_t canSend);

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
	int32_t period;
	double dutyCycle;
	int8_t disabledState;
	std::string periodPortStr;
	std::string dutyCyclePortStr;
	std::string enablePortStr;
	std::string outputPortStr;
	std::string inverseOutputPortStr;
	typedef std::vector<OPDI_DigitalPort *> PortList;
	DigitalPortList enablePorts;
	DigitalPortList outputPorts;
	DigitalPortList inverseOutputPorts;
	OPDI_AnalogPort *periodPort;
	OPDI_AnalogPort *dutyCyclePort;

	// state
	uint8_t pulseState;
	uint64_t lastStateChangeTime;

	virtual uint8_t doWork(uint8_t canSend);

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

	virtual uint8_t doWork(uint8_t canSend);

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
// Expression Port
///////////////////////////////////////////////////////////////////////////////

/** An ExpressionPort is a DigitalPort that sets the value of other ports
*   depending on the result of a calculation expression.
*   The expression is evaluated in each doWork iteration if the ExpressionPort is enabled,
*   i. e. its digital state is High (default).
*   The expression can refer to port IDs (input variables). Although these port IDs are case-
*   insensitive (a restriction of the underlying library), it is recommended to use the correct case.
*   The rules for the different port types are:
*    - A digital port is evaluated as 1 or 0 (High or Low).
*    - An analog port's relative value is evaluated in the range 0..1.
*    - A dial port's value is evaluated to its 64-bit value.
*    - A select port's value is its current item position.
*   The expression is evaluated using the ExprTk library. It queries the current state
*   of all ports to make their values available to the expression formula.
*   If a port state cannot be queried (e. g. due to an exception) the expression is not evaluated.
*   The resulting value can be assigned to a number of output ports.
*   The rules for the different port types are:
*    - A digital port is set to Low if the value is 0 and to High otherwise.
*    - An analog port's relative value is set to the range 0..1.
*      If the value is < 0, it is assumed as 0. If the value is > 1, it is assumed as 1.
*    - A dial port's value is set by casting the value to a 64-bit signed integer.
*    - A select port's position is set by casting the value to a 16-bit unsigned int.
*/
#ifdef OPDID_USE_EXPRTK

class OPDID_ExpressionPort : public OPDI_DigitalPort, protected OPDID_PortFunctions {
protected:
	std::string expressionStr;

	std::vector<double> portValues;	// holds the values of the ports for the expression evaluation

	std::string outputPortStr;
	PortList outputPorts;

	typedef exprtk::symbol_table<double> symbol_table_t;
	typedef exprtk::expression<double> expression_t;
	typedef exprtk::parser<double> parser_t;
	typedef parser_t::dependent_entity_collector::symbol_t symbol_t;
	std::deque<symbol_t> symbol_list;

	symbol_table_t symbol_table;
	expression_t expression;

	virtual bool prepareVariables(void);

	virtual uint8_t doWork(uint8_t canSend);

public:
	OPDID_ExpressionPort(AbstractOPDID *opdid, const char *id);

	virtual ~OPDID_ExpressionPort();

	virtual void configure(Poco::Util::AbstractConfiguration *config);

	virtual void setDirCaps(const char *dirCaps) override;

	virtual void setMode(uint8_t mode) override;

	virtual void prepare() override;
};

#endif // def OPDID_USE_EXPRTK

///////////////////////////////////////////////////////////////////////////////
// Timer Port
///////////////////////////////////////////////////////////////////////////////

/** A TimerPort is a DigitalPort that switches other ports according to one or more
*   timing schedules. A TimerPort is output only.
*   In its doWork method the TimerPort checks whether a timestamp defined
*   by a schedule has been reached. If a schedule is due the line
*   of the output port(s) is set according to the schedule specification.
*   The TimerPort supports the following scheduling types:
*    - Once: Executes only at the specified time.
*    - Interval: Executes with the specifed interval.
*    - Periodic: Executes at matching points in time specified by a pattern.
*    - Random: Executes randomly.
*   In schedules of type Interval, Periodic and Random you can specify how often the schedule
*   should trigger. When the maximum number of occurrences has been reached the schedule
*   will become inactive.
*   The TimerPort will act on the output ports only if it is enabled (line = High).
*   When setting a TimerPort to Low, all future events (including deactivations) will be
*   ignored.
*   When setting the TimerPort to High after it has been Low, all schedules start to run again.
*   Each schedule can act on the output in three ways: SetHigh, SetLow and Toggle.
*   If the action is SetHigh all output ports will be set to High when the schedule triggers.
*   If the schedule specifies a deactivation delay the output ports will be set to Low when
*   the deactivation time has passed.
*   Correspondingly, if the action is SetLow all output ports will be set to Low when the schedule triggers.
*   If the schedule specifies a deactivation delay the output ports will be set to High when
*   the deactivation time has passed.
*   If the action is Toggle, the port state is queried and inverted. The same happens after the optional
*   deactivation time.
*   If the deactivation time specified is 0 (default) no deactivation event will take place.
*   To specify points in time or intervals you can use the following configuration properties:
*    - Year
*    - Month
*    - Day
*    - Weekday (only Periodic)
*    - Hour
*    - Minute
*    - Second
*
*   The schedule types Periodic and Random are not yet implemented.
*/
class OPDID_TimerPort : public OPDI_DigitalPort, protected OPDID_PortFunctions {
protected:

	// helper class
	class ScheduleComponent {
	private:
		std::vector<bool> values;

	public:
		enum Type {
			MONTH,
			DAY,
			WEEKDAY,
			HOUR,
			MINUTE,
			SECOND
		};
		Type type;

		static ScheduleComponent* Parse(Type type, std::string def);

		bool getNextPossibleValue(int* currentValue, bool* rollover, bool* changed, int month, int year);

		bool getFirstPossibleValue(int* value, int month, int year);
	};

	enum ScheduleType {
		ONCE,
		INTERVAL,
		PERIODIC,
		ASTRONOMICAL,
		RANDOM,
		_DEACTIVATE
	};

	enum AstroEvent {
		SUNRISE,
		SUNSET
	};

	enum Action {
		SET_HIGH,
		SET_LOW,
		TOGGLE
	};

	struct Schedule {
		std::string nodeName;
		ScheduleType type;
		Action action;
		union data {
			struct time {		// for time-based schedules
				int16_t year;
				int16_t month;
				int16_t day;
				int16_t weekday;
				int16_t hour;
				int16_t minute;
				int16_t second;
			} time;
			struct random {		// for random-based schedules
				int jitter;

			} random;
		} data;
		// schedule components for PERIODIC
		ScheduleComponent* monthComponent;
		ScheduleComponent* dayComponent;
		ScheduleComponent* hourComponent;
		ScheduleComponent* minuteComponent;
		ScheduleComponent* secondComponent;
		// parameters for ASTRONOMICAL
		AstroEvent astroEvent;
		int64_t astroOffset;
		double astroLon;
		double astroLat;

		int occurrences;		// occurrence counter
		int maxOccurrences;		// maximum number of occurrences that this schedule is active
		long delayMs;			// deactivation time in milliseconds
	};

	class ScheduleNotification : public Poco::Notification {
	public:
		typedef Poco::AutoPtr<ScheduleNotification> Ptr;
		Poco::Timestamp timestamp;
		Schedule schedule;

		inline ScheduleNotification(Schedule schedule) {
			this->schedule = schedule;
		};
	};

	typedef std::vector<Schedule> ScheduleList;
	ScheduleList schedules;

	std::string outputPortStr;
	DigitalPortList outputPorts;

	Poco::TimedNotificationQueue queue;

	Poco::Timestamp calculateNextOccurrence(Schedule *schedule);

	void addNotification(ScheduleNotification::Ptr notification, Poco::Timestamp timestamp);

	virtual uint8_t doWork(uint8_t canSend) override;

public:
	OPDID_TimerPort(AbstractOPDID *opdid, const char *id);

	virtual ~OPDID_TimerPort();

	virtual void configure(Poco::Util::AbstractConfiguration *config);

	virtual void setDirCaps(const char *dirCaps) override;

	virtual void setMode(uint8_t mode) override;

	virtual void setLine(uint8_t line) override;

	virtual void prepare() override;
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
	PortList inputPorts;

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
// Logging Streaming Port
///////////////////////////////////////////////////////////////////////////////

/** Defines a streaming port that can log port states and optionally write them to a log file.
 */
class OPDID_LoggingPort : public OPDI_StreamingPort, protected OPDID_PortFunctions {

friend class OPDI;

protected:

	enum Format {
		CSV
	};

	uint32_t logPeriod;
	Format format;
	std::string separator;
	std::string portsToLogStr;
	PortList portsToLog;

	uint64_t lastEntryTime;
	std::ofstream outFile;

	std::string getPortStateStr(OPDI_Port* port);

	virtual uint8_t doWork(uint8_t canSend) override;

public:
	OPDID_LoggingPort(AbstractOPDID *opdid, const char *id);

	virtual ~OPDID_LoggingPort();

	virtual void configure(Poco::Util::AbstractConfiguration *config);

	virtual void prepare() override;

	virtual int write(char *bytes, size_t length) override;

	virtual int available(size_t count) override;

	virtual int read(char *result) override;

	virtual bool hasError(void) override;
};
