#pragma once

#include <string>
#include <vector>
#include <ostream>
#include <sstream>

#include "Poco/Exception.h"

#include "opdi_platformtypes.h"
#include "opdi_configspecs.h"

namespace opdi {

class OPDI;
class Port;
class DigitalPort;
class AnalogPort;
class SelectPort;
class DialPort;

typedef std::vector<Port*> PortList;
typedef std::vector<DigitalPort*> DigitalPortList;
typedef std::vector<AnalogPort*> AnalogPortList;


/** Base class for OPDI port wrappers.
	*
	*/
class Port {

	friend class OPDI;

public:
	enum class ChangeSource {
		CHANGESOURCE_INT,
		CHANGESOURCE_USER
	};

	enum class RefreshMode : unsigned int {
		REFRESH_NOT_SET,
		// no automatic refresh
		REFRESH_OFF,
		// time based refresh
		REFRESH_PERIODIC,
		// automatic refresh on state changes
		REFRESH_AUTO
	};

	enum class Error {
		VALUE_OK,
		VALUE_EXPIRED,
		VALUE_NOT_AVAILABLE
	};

	// disable copy constructor
	Port(const Port& that) = delete;

protected:
	// protected constructor - for use by friend classes only
	Port(const char* id, const char* type);

	// protected constructor: This class can't be instantiated directly
	Port(const char* id, const char* label, const char* type, const char* dircaps, int32_t flags, void* ptr);

	char* id;
	char* label;
	char type[2];	// type constants are one character (see opdi_port.h)
	char caps[2];	// caps constants are one character (port direction constants)
	int32_t flags;
	void* ptr;

	// If a port is hidden it is not included in the device capabilities as queried by the master.
	bool hidden;

	// If a port is readonly its state cannot be changed by the master.
	bool readonly;

	Error error;

	// extended info variables
	std::string unit;
	std::string icon;
	std::string group;

	// total extended info string
	std::string extendedInfo;

	// extended state historic values
	std::string history;

	// pointer to OPDI class instance
	OPDI* opdi;

	// OPDI implementation management structure
	// this pointer is managed by the OPDI class
	void* data;

	// Specifies when this port sends refresh messages to a connected master.
	// Default is off (REFRESH_NOT_SET).
	RefreshMode refreshMode;

	// Is set to true if the need for a refresh is detected (due to a state change or the timer).
	// The port will be refreshed in the doWork method.
	bool refreshRequired;

	// for refresh mode Periodic, the time in milliseconds between self-refresh messages
	uint32_t periodicRefreshTime;
	uint64_t lastRefreshTime;

	// indicates whether port state should be written to a persistent storage
	bool persistent;

	// utility function for string conversion 
	template <class T> std::string to_string(const T& t) const;

	/** Called regularly by the OPDI system. Enables the port to do work.
	* Override this method in subclasses to implement more complex functionality.
	* In this method, an implementation may send asynchronous messages to the master ONLY if canSend is true.
	* This includes messages like Resync, Refresh, Debug etc.
	* If canSend is 0 (= false), it means that there is no master is connected or the system is in the middle
	* of sending a message of its own. It is not safe to send messages if canSend = 0!
	* Returning any other value than OPDI_STATUS_OK causes the message processing to exit.
	* This will usually signal a device error to the master or cause the master to time out.
	* This base class uses doWork to implement the refresh timer. It calls doRefresh when the
	* periodic refresh time has been reached. Implementations can decide in doRefresh whether
	* they want to cause a refresh (set refreshRequired = true) or not.
	*/
	virtual uint8_t doWork(uint8_t canSend);

	/** Performs actions necessary before shutting down. The default implementation calls persist() if 
	* persistent is true and ignores any errors. */
	virtual void shutdown(void);

	/** Override this method to implement specific persistence mechanisms. The default implementation 
	* does nothing. */
	virtual void persist(void);

	/** Causes the port to be refreshed by sending a refresh message to a connected master.
	* Only if the port is not hidden. */
	virtual uint8_t refresh();

	/** Called when the port should send a refresh message to the master.
	* This implementation sets this->refreshRequired = true. */
	virtual void doRefresh(void);

	virtual void updateExtendedInfo(void);

	std::string escapeKeyValueText(const std::string& str) const;

	/** Checks the error state and throws an exception.
	* Should be used by subclasses in getState() methods. */
	virtual void checkError(void) const;

	virtual void setID(const char* newID);

	/** This method must be called when the state changes in order to
	* handle the onChange* functionality. */
	virtual void handleStateChange(ChangeSource changeSource);

public:

	/** Virtual destructor for the port. */
	virtual ~Port();

	/** This exception can be used by implementations to indicate an error during a port operation.
	*  Its message will be transferred to the master. */
	class PortError : public Poco::Exception
	{
	public:
		explicit PortError(std::string message) : Poco::Exception(message) {};
	};

	/** This exception can be used by implementations to indicate that the value has expired. */
	class ValueExpired : public PortError
	{
	public:
		ValueExpired() : PortError("The value has expired") {};
	};

	/** This exception can be used by implementations to indicate that no value is available. */
	class ValueUnavailable : public PortError
	{
	public:
		ValueUnavailable() : PortError("The value is unavailable") {};
	};

	/** This exception can be used by implementations to indicate that a port operation is not allowed.
		*  Its message will be transferred to the master. */
	class AccessDenied : public Poco::Exception
	{
	public:
		explicit AccessDenied(std::string message) : Poco::Exception(message) {};
	};

	// used to provide display ordering on ports
	int orderID;

	// The tag is additional information that is not used anywhere
	std::string tag;

	// Lists of digital ports that are to be set to High if a change occurs
	// The lists are to be set from external code but handled internally (this->handleStateChange).
	std::string onChangeIntPortsStr;
	std::string onChangeUserPortsStr;
	DigitalPortList onChangeIntPorts;
	DigitalPortList onChangeUserPorts;

	virtual const char* getID(void) const;

	std::string ID() const;

	virtual const char* getType(void) const;

	virtual void setHidden(bool hidden);

	virtual bool isHidden(void) const;

	virtual void setReadonly(bool readonly);

	virtual bool isReadonly(void) const;

	virtual void setPersistent(bool persistent);

	virtual bool isPersistent(void) const;

	/** Sets the label of the port. */
	virtual void setLabel(const char* label);

	virtual const char* getLabel(void) const;

	/** Sets the direction capabilities of the port. */
	virtual void setDirCaps(const char* dirCaps);

	virtual const char* getDirCaps(void) const;

	/** Sets the flags of the port. */
	virtual void setFlags(int32_t flags);

	virtual int32_t getFlags(void) const;

	virtual void setUnit(const std::string& unit);

	virtual void setIcon(const std::string& icon);

	virtual void setGroup(const std::string& group);

	virtual void setHistory(uint64_t intervalSeconds, int maxCount, const std::vector<int64_t>& values);

	virtual void clearHistory(void);

	virtual std::string getExtendedState(void) const;

	virtual std::string getExtendedInfo(void) const;

	virtual RefreshMode getRefreshMode(void);

	virtual void setRefreshMode(RefreshMode refreshMode);

	/** Sets the list of ports that should be auto-refreshed, as a space-delimited string.
	virtual void setAutoRefreshPorts(std::string portList);
	*/

	/** Sets the minimum time in milliseconds between self-refresh messages. If this time is 0 (default),
	* the self-refresh is disabled. */
	virtual void setPeriodicRefreshTime(uint32_t timeInMs);

	/** This method should be called just before the OPDI system is ready to start.
	* It gives the port the chance to do necessary initializations. */
	virtual void prepare(void);

	/** Sets the error state of this port. */
	virtual void setError(Error error);

	/** Gets the error state of this port. */
	virtual Port::Error getError(void) const;

	/** This method returns true if the port is in an error state. This will likely be the case
	*   when the getState() method of the port throws an exception.
	*/
	virtual bool hasError(void) const = 0;
};

inline std::ostream& operator<<(std::ostream& oStream, const Port::Error error) {
	switch (error) {
	case Port::Error::VALUE_OK: oStream << "VALUE_OK";
	case Port::Error::VALUE_EXPIRED: oStream << "VALUE_EXPIRED";
	case Port::Error::VALUE_NOT_AVAILABLE: oStream << "VALUE_NOT_AVAILABLE";
	default: oStream << "<unknown error>";
	}
	return oStream;
};


template <class T> inline std::string Port::to_string(const T& t) const {
	std::stringstream ss;
	ss << t;
	return ss.str();
}

class PortGroup {
	friend class OPDI;

	/*
	private:
		// disable copy constructor
		PortGroup(const PortGroup& that);
	*/
protected:
	char* id;
	char* label;
	char* parent;
	int32_t flags;
	char* extendedInfo;

	// pointer to OPDI class instance
	OPDI* opdi;

	// OPDI implementation management structure
	void* data;

	// linked list of port groups - pointer to next port group
	PortGroup* next;

	std::string icon;

	virtual void updateExtendedInfo(void);

public:
	explicit PortGroup(const char* id);

	/** Virtual destructor for the port. */
	virtual ~PortGroup();

	virtual const char* getID(void);

	/** Sets the label of the port group. */
	virtual void setLabel(const char* label);

	virtual const char* getLabel(void);

	/** Sets the parent of the port group. */
	virtual void setParent(const char* parent);

	virtual const char* getParent(void);

	/** Sets the flags of the port group. */
	virtual void setFlags(int32_t flags);

	virtual void setIcon(const std::string& icon);
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Port definitions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/** Defines a digital port.
	*/
class DigitalPort : public Port {
	friend class OPDI;

protected:
	uint8_t mode;
	uint8_t line;

public:
	explicit DigitalPort(const char* id);

	// Initialize a digital port. Specify one of the OPDI_PORTDIRCAPS_* values for dircaps.
	// Specify one or more of the OPDI_DIGITAL_PORT_* values for flags, or'ed together, to specify pullup/pulldown resistors.
	DigitalPort(const char* id, const char* label, const char*  dircaps, const int32_t flags);

	virtual ~DigitalPort();

	virtual void setDirCaps(const char* dirCaps);

	// function that handles the set mode command (opdi_set_digital_port_mode)
	// mode = 0: floating input
	// mode = 1: input with pullup on
	// mode = 2: not supported
	// mode = 3: output
	virtual void setMode(uint8_t mode, ChangeSource changeSource = Port::ChangeSource::CHANGESOURCE_INT);

	// function that handles the set line command (opdi_set_digital_port_line)
	// line = 0: state low
	// line = 1: state high
	virtual void setLine(uint8_t line, ChangeSource changeSource = Port::ChangeSource::CHANGESOURCE_INT);

	// function that fills in the current port state
	virtual void getState(uint8_t* mode, uint8_t* line) const;

	virtual uint8_t getMode(void);

	virtual bool hasError(void) const override;
};

/** Defines an analog port.
	*/
class AnalogPort : public Port {
	friend class OPDI;

protected:
	uint8_t mode;
	uint8_t reference;
	uint8_t resolution;
	int32_t value;

	virtual int32_t validateValue(int32_t value) const;

public:
	explicit AnalogPort(const char* id);

	AnalogPort(const char* id, const char* label, const char*  dircaps, const int32_t flags);

	virtual ~AnalogPort();

	// mode = 0: input
	// mode = 1: output
	virtual void setMode(uint8_t mode, ChangeSource changeSource = Port::ChangeSource::CHANGESOURCE_INT);

	virtual void setResolution(uint8_t resolution, ChangeSource changeSource = Port::ChangeSource::CHANGESOURCE_INT);

	// reference = 0: internal voltage reference
	// reference = 1: external voltage reference
	virtual void setReference(uint8_t reference, ChangeSource changeSource = Port::ChangeSource::CHANGESOURCE_INT);

	// value: an integer value ranging from 0 to 2^resolution - 1
	virtual void setValue(int32_t value, ChangeSource changeSource = Port::ChangeSource::CHANGESOURCE_INT);

	virtual void getState(uint8_t* mode, uint8_t* resolution, uint8_t* reference, int32_t* value) const;

	// returns the value as a factor between 0 and 1 of the maximum resolution
	virtual double getRelativeValue(void);

	// sets the value from a factor between 0 and 1
	virtual void setRelativeValue(double value);

	virtual uint8_t getMode(void);

	virtual bool hasError(void) const override;
};

/** Defines a select port.
	*
	*/
class SelectPort : public Port {
	friend class OPDI;

protected:
	char** items;
	uint16_t count;
	uint16_t position;

	// frees the internal items memory
	void freeItems();

public:
	explicit SelectPort(const char* id);

	// Initialize a select port. The direction of a select port is output only.
	// You have to specify a list of items that are the labels of the different select positions. The last element must be NULL.
	// The items are copied into the privately managed data structure of this class.
	SelectPort(const char* id, const char* label, const char** items);

	virtual ~SelectPort();

	// Copies the items into the privately managed data structure of this class.
	virtual void setItems(const char* *items);

	// function that handles position setting; position may be in the range of 0..(items.length - 1)
	virtual void setPosition(uint16_t position, ChangeSource changeSource = Port::ChangeSource::CHANGESOURCE_INT);

	// function that fills in the current port state
	virtual void getState(uint16_t* position) const;

	virtual const char* getPositionLabel(uint16_t position);

	virtual uint16_t getMaxPosition(void);

	virtual bool hasError(void) const override;
};

/** Defines a dial port.
	*
	*/
class DialPort : public Port {
	friend class OPDI;

protected:
	int64_t minValue;
	int64_t maxValue;
	uint64_t step;
	int64_t position;

public:
	explicit DialPort(const char* id);

	// Initialize a dial port. The direction of a dial port is output only.
	// You have to specify boundary values and a step size.
	DialPort(const char* id, const char* label, int64_t minValue, int64_t maxValue, uint64_t step);
	virtual ~DialPort();

	virtual int64_t getMin(void);
	virtual int64_t getMax(void);
	virtual int64_t getStep(void);

	virtual void setMin(int64_t min);
	virtual void setMax(int64_t max);
	virtual void setStep(uint64_t step);

	// function that handles position setting; position may be in the range of minValue..maxValue
	virtual void setPosition(int64_t position, ChangeSource changeSource = Port::ChangeSource::CHANGESOURCE_INT);

	// function that fills in the current port state
	virtual void getState(int64_t* position) const;

	virtual bool hasError(void) const override;
};

/** Defines a streaming port.
	* A streaming port represents a serial data connection on the device. It can send and receive bytes.
	* Examples are RS232 or I2C ports.
	* If a master is connected it can bind to a streaming port. Binding means that received bytes are
	* transferred to the master and bytes received from the master are sent to the connection.
	* The streaming port can thus act as an internal connection data source and sink, as well as a
	* transparent connection which directly connects the master and the connection partner.
	* Data sources can be read-only or write-only. Data may be also transformed before it is transmitted
	* to the master. How exactly this is done depends on the concrete implementation.
	*/
class StreamingPort : public Port {
	friend class OPDI;

public:
	// Initialize a streaming port. A streaming port is always bidirectional.
	explicit StreamingPort(const char* id);

	virtual ~StreamingPort();

	// Writes the specified bytes to the data sink. Returns the number of bytes written.
	// If the returned value is less than 0 it is considered an (implementation specific) error.
	virtual int write(char* bytes, size_t length) = 0;

	// Checks how many bytes are available from the data source. If count is > 0
	// it is used as a value to request the number of bytes if the underlying system
	// supports this type of request. Otherwise it has no meaning.
	// If no bytes are available the result is 0. If the returned
	// value is less than 0 it is considered an (implementation specific) error.
	virtual int available(size_t count) = 0;

	// Reads one byte from the data source and places it in result. If the returned
	// value is less than 1 it is considered an (implementation specific) error.
	virtual int read(char* result) = 0;
};

}	// namespace opdi