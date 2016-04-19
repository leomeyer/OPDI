#pragma once

#include <iostream>

#include "Poco/NumberParser.h"
#include "Poco/RegularExpression.h"

#include "AbstractOPDID.h"

class ValueResolver;

class OPDID_PortFunctions {
friend class AbstractOPDID;
friend class ValueResolver;
friend std::ostream& operator<<(std::ostream& os, const ValueResolver& vr);

protected:
	typedef std::vector<OPDI_Port*> PortList;
	typedef std::vector<OPDI_DigitalPort*> DigitalPortList;
	typedef std::vector<OPDI_AnalogPort*> AnalogPortList;

	AbstractOPDID *opdid;
	AbstractOPDID::LogVerbosity logVerbosity;
	std::string portFunctionID;

	OPDID_PortFunctions(std::string id);

	virtual OPDI_Port* findPort(const std::string& configPort, const std::string& setting, const std::string& portID, bool required);

	virtual void findPorts(const std::string& configPort, const std::string& setting, const std::string& portIDs, OPDI::PortList& portList);

	virtual OPDI_DigitalPort* findDigitalPort(const std::string& configPort, const std::string& setting, const std::string& portID, bool required);

	virtual void findDigitalPorts(const std::string& configPort, const std::string& setting, const std::string& portIDs, DigitalPortList& portList);

	virtual OPDI_AnalogPort* findAnalogPort(const std::string& configPort, const std::string& setting, const std::string& portID, bool required);

	virtual void findAnalogPorts(const std::string& configPort, const std::string& setting, const std::string& portIDs, AnalogPortList& portList);

	virtual OPDI_SelectPort* findSelectPort(const std::string& configPort, const std::string& setting, const std::string& portID, bool required);

	virtual void logWarning(const std::string& message);

	virtual void logNormal(const std::string& message);

	virtual void logVerbose(const std::string& message);

	virtual void logDebug(const std::string& message);

	virtual void logExtreme(const std::string& message);
};


/** This class wraps either a fixed value or a port name. It is used when port configuration parameters
*   can be either fixed or dynamic values.
*   A port name can be followed by a double value in brackets. This indicates that the port's value
*   should be scaled, i. e. multiplied, with the given factor. This is especially useful when the value
*   of an analog port, which is by definition always between 0 and 1 inclusive, should be mapped to 
*   a different range of numbers.
*   The port name can be optionally followed by a slash (/) plus a double value that specifies the value
*   in case the port returns an error. This allows for a reasonable reaction in case port errors are to
*   be expected. If this value is omitted the error will be propagated via an exception to the caller
*   which must then be react to the error condition. The error default can be prohibited.
*   Syntax examples:
*   10               - fixed value 10
*   MyPort1          - dynamic value of MyPort1
*   MyPort1(100)     - dynamic value of MyPort1, multiplied by 100
*   MyPort1/0        - dynamic value of MyPort1, 0 in case of an error
*   MyPort1(100)/0   - dynamic value of MyPort1, multiplied by 100, 0 in case of an error
**/
class ValueResolver {
friend std::ostream& operator<<(std::ostream& os, const ValueResolver& vr);

	OPDID_PortFunctions* origin;
	std::string paramName;
	std::string portID;
	bool useScaleValue;
	double scaleValue;
	bool useErrorDefault;
	double errorDefault;
	OPDI_Port* port;
	bool isFixed;
	double fixedValue;

public:
	ValueResolver();

	void initialize(OPDID_PortFunctions* origin, const std::string& paramName, const std::string& value, bool allowErrorDefault = true);

	bool validate(double min, double max);

	bool validateAsInt(int min, int max);

	operator double();
};

inline std::ostream& operator<<(std::ostream& os, const ValueResolver& vr)
{
	double d = (ValueResolver&)vr;
	os << vr.origin->opdid->to_string(d);
	return os;
}

