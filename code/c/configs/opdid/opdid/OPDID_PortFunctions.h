#pragma once

#include "Poco/NumberParser.h"
#include "Poco/RegularExpression.h"

#include "AbstractOPDID.h"

template<typename T>
class ValueResolver;

class OPDID_PortFunctions {
friend class AbstractOPDID;
friend class ValueResolver<double>;
friend class ValueResolver<int32_t>;
friend class ValueResolver<int64_t>;

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
template<typename T>
class ValueResolver {

	OPDID_PortFunctions* origin;
	std::string paramName;
	std::string portID;
	bool useScaleValue;
	double scaleValue;
	bool useErrorDefault;
	T errorDefault;
	mutable OPDI_Port* port;
	bool isFixed;
	T fixedValue;

public:
	ValueResolver() {
		this->fixedValue = 0;
		this->isFixed = false;
		this->useScaleValue = false;
		this->scaleValue = 0;
		this->useErrorDefault = false;
		this->errorDefault = 0;
	}

	ValueResolver(T initialValue): ValueResolver() {
		this->isFixed = true;
		this->fixedValue = initialValue;
	}

	void initialize(OPDID_PortFunctions* origin, const std::string& paramName, const std::string& value, bool allowErrorDefault = true) {
		this->origin = origin;
		this->useScaleValue = false;
		this->useErrorDefault = false;
		this->port = nullptr;

		this->origin->logDebug(origin->portFunctionID + ": Parsing ValueResolver expression of parameter '" + paramName + "': " + value);
		// try to convert the value to a double
		double d;
		if (Poco::NumberParser::tryParseFloat(value, d)) {
			this->fixedValue = (T)d;
			this->isFixed = true;
			this->origin->logDebug(origin->portFunctionID + ": ValueResolver expression resolved to fixed value: " + this->origin->opdid->to_string(value));
		}
		else {
			this->isFixed = false;
			Poco::RegularExpression::MatchVec matches;
			std::string portName;
			std::string scaleStr;
			std::string defaultStr;
			// try to match full syntax
			Poco::RegularExpression reFull("(.*)\\((.*)\\)\\/(.*)");
			if (reFull.match(value, 0, matches) == 4) {
				if (!allowErrorDefault)
					throw Poco::ApplicationException(origin->portFunctionID + ": Parameter " + paramName + ": Specifying an error default value is not allowed: " + value);
				portName = value.substr(matches[1].offset, matches[1].length);
				scaleStr = value.substr(matches[2].offset, matches[2].length);
				defaultStr = value.substr(matches[3].offset, matches[3].length);
			}
			else {
				// try to match default value syntax
				Poco::RegularExpression reDefault("(.*)\\/(.*)");
				if (reDefault.match(value, 0, matches) == 3) {
					if (!allowErrorDefault)
						throw Poco::ApplicationException(origin->portFunctionID + ": Parameter " + paramName + ": Specifying an error default value is not allowed: " + value);
					portName = value.substr(matches[1].offset, matches[1].length);
					defaultStr = value.substr(matches[2].offset, matches[2].length);
				}
				else {
					// try to match scale value syntax
					Poco::RegularExpression reScale("(.*)\\((.*)\\)");
					if (reScale.match(value, 0, matches) == 3) {
						portName = value.substr(matches[1].offset, matches[1].length);
						scaleStr = value.substr(matches[2].offset, matches[2].length);
					}
					else {
						// could not match a pattern - use value as port name
						portName = value;
					}
				}
			}
			// parse values if specified
			if (scaleStr != "") {
				if (Poco::NumberParser::tryParseFloat(scaleStr, this->scaleValue)) {
					this->useScaleValue = true;
				}
				else
					throw Poco::ApplicationException(origin->portFunctionID + ": Parameter " + paramName + ": Invalid scale value specified; must be numeric: " + scaleStr);
			}
			if (defaultStr != "") {
				double e;
				if (Poco::NumberParser::tryParseFloat(defaultStr, e)) {
					this->errorDefault = (T)e;
					this->useErrorDefault = true;
				}
				else
					throw Poco::ApplicationException(origin->portFunctionID + ": Parameter " + paramName + ": Invalid error default value specified; must be numeric: " + defaultStr);
			}

			this->portID = portName;

			this->origin->logDebug(origin->portFunctionID + ": ValueResolver expression resolved to port ID: " + this->portID
				+ (this->useScaleValue ? ", scale by " + this->origin->opdid->to_string(this->scaleValue) : "")
				+ (this->useErrorDefault ? ", error default is " + this->origin->opdid->to_string(this->errorDefault) : ""));
		}
	}

	bool validate(T min, T max) const {
		// no fixed value? assume it's valid
		if (!this->isFixed)
			return true;

		return ((this->fixedValue >= min) && (this->fixedValue <= max));
	}

	T value() const {
		if (isFixed)
			return fixedValue;
		else {
			// port not yet resolved?
			if (this->port == nullptr) {
				if (this->portID == "")
					throw Poco::ApplicationException(this->origin->portFunctionID + ": Parameter " + paramName + ": ValueResolver not initialized (programming error)");
				// try to resolve the port
				this->port = this->origin->findPort(this->origin->portFunctionID, this->paramName, this->portID, true);
			}
			// resolve port value to a double
			double result = 0;

			try {
				result = origin->opdid->getPortValue(this->port);
			}
			catch (Poco::Exception &pe) {
				origin->logExtreme(this->origin->portFunctionID + ": Unable to get the value of the port " + port->ID() + ": " + pe.message());
				if (this->useErrorDefault) {
					return this->errorDefault;
				}
				else
					// propagate exception
					throw Poco::ApplicationException(this->origin->portFunctionID + ": Unable to get the value of the port " + port->ID() + ": " + pe.message());
			}

			// scale?
			if (this->useScaleValue) {
				result *= this->scaleValue;
			}

			return (T)result;
		}
	}
};

