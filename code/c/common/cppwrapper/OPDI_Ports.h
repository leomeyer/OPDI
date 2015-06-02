#pragma once

#include <string>
#include <vector>
#include <sstream>

#include "Poco/Exception.h"

#include "opdi_platformtypes.h"
#include "opdi_configspecs.h"

class OPDI;

/** Base class for OPDI port wrappers.
 *
 */
class OPDI_Port {

friend class OPDI;

public:
	enum RefreshMode
#ifdef __GNUG__
// required by GCC for the forward declaration of the enum
		: unsigned int
#endif
		;

protected:

	// protected constructor - for use by friend classes only
	OPDI_Port(const char *id, const char *type);

	// Protected constructor: This class can't be instantiated directly
	OPDI_Port(const char *id, const char *label, const char *type, const char* dircaps, int32_t flags, void* ptr);

	char *id;
	char *label;
	char type[2];	// type constants are one character (see opdi_port.h)
	char caps[2];	// caps constants are one character (port direction constants)
	int32_t flags;
	void* ptr;
	char *extendedInfo;

	// If a port is hidden it is not included in the device capabilities as queried by the master.
	bool hidden;

	// If a port is readonly its state cannot be changed from the master.
	bool readonly;

	// extended info variables
	std::string unit;
	std::string icon;
	std::string group;

	// Utility function for string conversion 
	template <class T> std::string to_string(const T& t);

	/** Called regularly by the OPDI system. Enables the port to do work.
	 * Override this in subclasses to implement more complex functionality.
	 * In this method, an implementation may send asynchronous messages to the master ONLY if canSend is true.
	 * This includes messages like Resync, Refresh, Debug etc.
	 * If canSend is 0 (= false), it means that there is no master is connected or the system is in the middle
	 * of sending a message of its own. It is not safe to send messages if canSend = 0!
	 * Returning any other value than OPDI_STATUS_OK causes the message processing to exit.
	 * This will usually signal a device error to the master or cause the master to time out.
	 * This base class uses doWork to implement the self refresh timer. It calls doSelfRefresh when the 
	 * periodic self refresh time has been reached. Implementations can decide in doSelfRefresh whether
	 * they want to cause a self refresh (set refreshRequired = true) or not.
	 */
	virtual uint8_t doWork(uint8_t canSend);

	// pointer to OPDI class instance
	OPDI *opdi;

	// OPDI implementation management structure
	void* data;

	// A list of ports that should automatically refresh when the port state changes.
	// How this state change is handled depends on the port implementation.
	//std::vector<std::string> autoRefreshPorts;

	// Specifies when this port sends refresh messages to a connected master.
	// Default is OFF.
	RefreshMode refreshMode;

	// Can be set to true if a refresh is necessary (due to a state change or the timer).
	// The port will be refreshed in the doWork method as soon as canSend is true.
	// If the port is hidden it will not be refreshed.
	bool refreshRequired;

	// the minimum time in milliseconds between self-refresh messages
	uint32_t refreshTime;
	uint64_t lastRefreshTime;

	// Called when a periodic self-refresh is due. If a refresh is necessary, implementations
	// should set this->refreshRequired = true.
	virtual void doSelfRefresh(void) = 0;

	virtual void updateExtendedInfo(void);

public:

	enum RefreshMode : unsigned int {
		REFRESH_NOT_SET,
		// no automatic refresh
		REFRESH_OFF,
		// time based refresh
		REFRESH_PERIODIC,
		// automatic refresh on state changes
		REFRESH_AUTO
	};

	/** This exception can be used by implementations to indicate an error during a port operation.
	 *  Its message will be transferred to the master. */
	class PortError : public Poco::Exception
	{
	public:
		PortError(std::string message): Poco::Exception(message) {};
	};

	/** This exception can be used by implementations to indicate that a port operation is not allowed.
	 *  Its message will be transferred to the master. */
	class AccessDenied : public Poco::Exception
	{
	public:
		AccessDenied(std::string message): Poco::Exception(message) {};
	};

	// linked list of ports - pointer to next port
	OPDI_Port *next;

	/** Pointer for help structures. Not used internally; may be used by the application. */
	void* tag;

	/** Virtual destructor for the port. */
	virtual ~OPDI_Port();

	virtual const char *getID(void);

	virtual const char *getType(void);

	virtual void setHidden(bool hidden);

	virtual bool isHidden(void);

	virtual void setReadonly(bool readonly);

	virtual bool isReadonly(void);

	/** Sets the label of the port. */
	virtual void setLabel(const char *label);

	virtual const char *getLabel(void);

	/** Sets the direction capabilities of the port. */
	virtual void setDirCaps(const char *dirCaps);

	/** Sets the flags of the port. */
	virtual void setFlags(int32_t flags);

	virtual void setUnit(std::string unit);

	virtual void setIcon(std::string icon);

	virtual void setGroup(std::string group);

	/** Causes the port to be refreshed by sending a refresh message to a connected master.
	*   Only if the port is not hidden. */
	virtual uint8_t refresh(void);

	virtual RefreshMode getRefreshMode(void);

	virtual void setRefreshMode(RefreshMode refreshMode);

	/** Sets the list of ports that should be auto-refreshed, as a space-delimited string. 
	virtual void setAutoRefreshPorts(std::string portList);
	*/

	/** Sets the minimum time in milliseconds between self-refresh messages. If this time is 0 (default),
	* the self-refresh is disabled. */
	virtual void setRefreshTime(uint32_t timeInMs);

	/** This method should be called just before the OPDI system is ready to start.
	* It gives the port the chance to do necessary initializations. */
	virtual void prepare(void);

	/** This method returns true if the port is in an error state. This will likely be the case
	*   when the getState() method of the port throws an exception.
	*/
	virtual bool hasError(void) = 0;
};

template <class T> inline std::string OPDI_Port::to_string(const T& t) {
	std::stringstream ss;
	ss << t;
	return ss.str();
}

class OPDI_PortGroup {

friend class OPDI;

protected:

	char *id;
	char *label;
	char *parent;
	int32_t flags;
	char *extendedInfo;

	// pointer to OPDI class instance
	OPDI *opdi;

	// OPDI implementation management structure
	void* data;

	// linked list of port groups - pointer to next port group
	OPDI_PortGroup *next;

	std::string icon;

	virtual void updateExtendedInfo(void);

public:
	OPDI_PortGroup(const char *id);

	/** Virtual destructor for the port. */
	virtual ~OPDI_PortGroup();

	virtual const char *getID(void);

	/** Sets the label of the port group. */
	virtual void setLabel(const char *label);

	virtual const char *getLabel(void);

	/** Sets the parent of the port group. */
	virtual void setParent(const char *parent);

	virtual const char *getParent(void);

	/** Sets the flags of the port group. */
	virtual void setFlags(int32_t flags);

	virtual void setIcon(std::string icon);
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Port definitions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/** Defines a digital port.
 */
class OPDI_DigitalPort : public OPDI_Port {

friend class OPDI;

protected:
	uint8_t mode;
	uint8_t line;

	/** A digital port performs a self refresh only if it is in input mode.	*/
	virtual void doSelfRefresh(void) override;

public:
	OPDI_DigitalPort(const char *id);

	// Initialize a digital port. Specify one of the OPDI_PORTDIRCAPS_* values for dircaps.
	// Specify one or more of the OPDI_DIGITAL_PORT_* values for flags, or'ed together, to specify pullup/pulldown resistors.
	OPDI_DigitalPort(const char *id, const char *label, const char * dircaps, const int32_t flags);

	virtual ~OPDI_DigitalPort();

	virtual void setDirCaps(const char *dirCaps);

	// function that handles the set mode command (opdi_set_digital_port_mode)
	// mode = 0: floating input
	// mode = 1: input with pullup on
	// mode = 2: not supported
	// mode = 3: output
	virtual void setMode(uint8_t mode);

	// function that handles the set line command (opdi_set_digital_port_line)
	// line = 0: state low
	// line = 1: state high
	virtual void setLine(uint8_t line);

	// function that fills in the current port state
	virtual void getState(uint8_t *mode, uint8_t *line);

	virtual bool hasError(void) override;
};

/** Defines an analog port.
 */
class OPDI_AnalogPort : public OPDI_Port {

friend class OPDI;

protected:
	uint8_t mode;
	uint8_t reference;
	uint8_t resolution;
	int32_t value;

	/** An analog port performs a self refresh only if it is in input mode.	*/
	virtual void doSelfRefresh(void) override;

	virtual int32_t validateValue(int32_t value);

public:
	OPDI_AnalogPort(const char *id);

	OPDI_AnalogPort(const char *id, const char *label, const char * dircaps, const int32_t flags);

	virtual ~OPDI_AnalogPort();

	// mode = 0: input
	// mode = 1: output
	virtual void setMode(uint8_t mode);

	virtual void setResolution(uint8_t resolution);

	// reference = 0: internal voltage reference
	// reference = 1: external voltage reference
	virtual void setReference(uint8_t reference);

	// value: an integer value ranging from 0 to 2^resolution - 1
	virtual void setValue(int32_t value);

	virtual void getState(uint8_t *mode, uint8_t *resolution, uint8_t *reference, int32_t *value);

	// returns the value as a factor between 0 and 1 of the maximum resolution
	virtual double getRelativeValue(void);

	// sets the value from a factor between 0 and 1
	virtual void setRelativeValue(double value);

	virtual bool hasError(void) override;
};

/** Defines a select port.
 *
 */
class OPDI_SelectPort : public OPDI_Port {

friend class OPDI;

protected:
	int itemCount;
	char **items;
	uint16_t count;
	uint16_t position;

	// frees the internal items memory
	void freeItems();

	/** A select port does not support self refreshing. */
	virtual void doSelfRefresh(void) override;

public:
	OPDI_SelectPort(const char *id);

	// Initialize a select port. The direction of a select port is output only.
	// You have to specify a list of items that are the labels of the different select positions. The last element must be NULL.
	// The items are copied into the privately managed data structure of this class.
	OPDI_SelectPort(const char *id, const char *label, const char **items);

	virtual ~OPDI_SelectPort();

	// Copies the items into the privately managed data structure of this class.
	virtual void setItems(const char **items);

	// function that handles position setting; position may be in the range of 0..(items.length - 1)
	virtual void setPosition(uint16_t position);

	// function that fills in the current port state
	virtual void getState(uint16_t *position);

	virtual uint16_t getMaxPosition(void);

	virtual bool hasError(void) override;
};

/** Defines a dial port.
 *
 */
class OPDI_DialPort : public OPDI_Port {

friend class OPDI;

protected:
	int64_t minValue;
	int64_t maxValue;
	uint64_t step;
	int64_t position;

	/** A select port does not support self refreshing. */
	virtual void doSelfRefresh(void) override;

public:
	OPDI_DialPort(const char *id);

	// Initialize a dial port. The direction of a dial port is output only.
	// You have to specify boundary values and a step size.
	OPDI_DialPort(const char *id, const char *label, int64_t minValue, int64_t maxValue, uint64_t step);
	virtual ~OPDI_DialPort();

	virtual void setMin(int64_t min);
	virtual void setMax(int64_t max);
	virtual void setStep(uint64_t step);

	// function that handles position setting; position may be in the range of minValue..maxValue
	virtual void setPosition(int64_t position);

	// function that fills in the current port state
	virtual void getState(int64_t *position);

	virtual bool hasError(void) override;
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
class OPDI_StreamingPort : public OPDI_Port {

friend class OPDI;

protected:
	/** A streaming port does not support self refreshing. */
	virtual void doSelfRefresh(void) override;

public:
	// Initialize a streaming port. A streaming port is always bidirectional.
	OPDI_StreamingPort(const char *id);

	virtual ~OPDI_StreamingPort();

	// Writes the specified bytes to the data sink. Returns the number of bytes written.
	// If the returned value is less than 0 it is considered an (implementation specific) error.
	virtual int write(char *bytes, size_t length) = 0;

	// Checks how many bytes are available from the data source. If count is > 0
	// it is used as a value to request the number of bytes if the underlying system
	// supports this type of request. Otherwise it has no meaning.
	// If no bytes are available the result is 0. If the returned
	// value is less than 0 it is considered an (implementation specific) error.
	virtual int available(size_t count) = 0;

	// Reads one byte from the data source and places it in result. If the returned
	// value is less than 1 it is considered an (implementation specific) error.
	virtual int read(char *result) = 0;
};
