
#include "Poco/Timestamp.h"

#include "OPDID_ExpressionPort.h"

#include "opdi_constants.h"

#ifdef OPDID_USE_EXPRTK

///////////////////////////////////////////////////////////////////////////////
// Expression Port
///////////////////////////////////////////////////////////////////////////////

OPDID_ExpressionPort::OPDID_ExpressionPort(AbstractOPDID *opdid, const char *id) : OPDI_DigitalPort(id, id, OPDI_PORTDIRCAP_OUTPUT, 0), OPDID_PortFunctions(id) {
	this->opdid = opdid;
	this->numIterations = 0;
	this->fallbackSpecified = false;
	this->fallbackValue = 0;
	this->deactivationSpecified = false;
	this->deactivationValue = 0;

	OPDI_DigitalPort::setMode(OPDI_DIGITAL_MODE_OUTPUT);

	// default: enabled
	this->line = 1;
}

OPDID_ExpressionPort::~OPDID_ExpressionPort() {
}

void OPDID_ExpressionPort::configure(Poco::Util::AbstractConfiguration *config) {
	this->opdid->configurePort(config, this, 0);
	this->logVerbosity = this->opdid->getConfigLogVerbosity(config, AbstractOPDID::UNKNOWN);

	this->expressionStr = config->getString("Expression", "");
	if (this->expressionStr == "")
		throw Poco::DataException("You have to specify an Expression");

	this->outputPortStr = config->getString("OutputPorts", "");
	if (this->outputPortStr == "")
		throw Poco::DataException("You have to specify at least one output port in the OutputPorts setting");

	this->numIterations = config->getInt64("Iterations", 0);

	if (config->hasProperty("FallbackValue")) {
		this->fallbackValue = config->getDouble("FallbackValue");
		this->fallbackSpecified = true;
	}

	if (config->hasProperty("DeactivationValue")) {
		this->deactivationValue = config->getDouble("DeactivationValue");
		this->deactivationSpecified = true;
	}
}

void OPDID_ExpressionPort::setDirCaps(const char * /*dirCaps*/) {
	throw PortError(this->ID() + ": The direction capabilities of an ExpressionPort cannot be changed");
}

void OPDID_ExpressionPort::setMode(uint8_t /*mode*/, ChangeSource /*changeSource*/) {
	throw PortError(this->ID() + ": The mode of an ExpressionPort cannot be changed");
}

void OPDID_ExpressionPort::setLine(uint8_t line, ChangeSource /*changeSource*/) {
	OPDI_DigitalPort::setLine(line);

	// if the line has been set to High, start the iterations
	if (line == 1) {
		this->logDebug(this->ID() + ": Expression activated, number of iterations: " + this->to_string(this->numIterations));
		this->iterations = this->numIterations;
	}
	// set to 0; check whether to set a deactivation value
	else {
		this->logDebug(this->ID() + ": Expression deactivated");
		if (this->deactivationSpecified) {
			this->logDebug(this->ID() + ": Applying deactivation value: " + this->to_string(this->deactivationValue));
			this->setOutputPorts(this->deactivationValue);
		}
	}
}

bool OPDID_ExpressionPort::prepareSymbols(bool duringSetup) {
	this->symbol_table.add_function("timestamp", this->timestampFunc);

	// Adding constants leads to a memory leak; furthermore, constants are not detected as known symbols.
	// As there are only three constants (pi, epsilon, infinity) we can well do without this function.
	//	this->symbol_table.add_constants();

	return true;
}

bool OPDID_ExpressionPort::prepareVariables(bool duringSetup) {
	this->portValues.clear();
	this->portValues.reserve(this->symbol_list.size());

	// go through dependent entities (variables) of the expression
	for (std::size_t i = 0; i < this->symbol_list.size(); ++i)
	{
		symbol_t& symbol = this->symbol_list[i];

		if (symbol.second != parser_t::e_st_variable)
			continue;

		// find port (variable name is the port ID)
		OPDI_Port *port = this->opdid->findPortByID(symbol.first.c_str(), true);

		// port not found?
		if (port == nullptr) {
			throw PortError(this->ID() + ": Expression variable did not resolve to an available port ID: " + symbol.first);
		}

		// calculate port value
		try {
			double value = opdid->getPortValue(port);
			this->logExtreme(this->ID() + ": Resolved value of port " + port->ID() + " to: " + to_string(value));
			this->portValues.push_back(value);
		} catch (Poco::Exception& e) {
			// error handling during setup is different; to avoid too many warnings (in the doWork method)
			// we make a difference here
			if (duringSetup) {
				// emit a warning
				this->logWarning(this->ID() + ": Failed to resolve value of port " + port->ID() + ": " + e.message());
				this->portValues.push_back(0.0f);
			} else {
				// warn in extreme logging mode only
				this->logExtreme(this->ID() + ": Failed to resolve value of port " + port->ID() + ": " + e.message());
				// the expression cannot be evaluated if there is an error
				return false;
			}
		}

		// add reference to the port value (by port ID)
		if (!this->symbol_table.add_variable(port->getID(), this->portValues[i]))
			return false;
	}

	return true;
}

void OPDID_ExpressionPort::prepare() {
	OPDI_DigitalPort::prepare();

	// find ports; throws errors if something required is missing
	this->findPorts(this->getID(), "OutputPorts", this->outputPortStr, this->outputPorts);

	// clear symbol table and values
	this->symbol_table.clear();

	expression.register_symbol_table(symbol_table);
	this->prepareSymbols(true);

	parser_t parser;
	parser.enable_unknown_symbol_resolver();
	// collect only variables as symbol names
	parser.dec().collect_variables() = true;

	// compile to detect variables
	if (!parser.compile(this->expressionStr, expression))
		throw Poco::Exception(this->ID() + ": Error in expression: " + parser.error());

	// store symbol list (input variables)
	parser.dec().symbols(this->symbol_list);

	this->symbol_table.clear();
	this->prepareSymbols(true);
	if (!this->prepareVariables(true)) {
		throw Poco::Exception(this->ID() + ": Unable to resolve variables");
	}
	parser.disable_unknown_symbol_resolver();

	if (!parser.compile(this->expressionStr, expression))
		throw Poco::Exception(this->ID() + ": Error in expression: " + parser.error());

	// initialize the number of iterations
	// if the port is Low this has no effect; if it is High it will disable after the evaluation
	this->iterations = this->numIterations;
}

void OPDID_ExpressionPort::setOutputPorts(double value) {
	// go through list of output ports
	OPDI::PortList::iterator it = this->outputPorts.begin();
	while (it != this->outputPorts.end()) {
		try {
			if ((*it)->getType()[0] == OPDI_PORTTYPE_DIGITAL[0]) {
				if (value == 0)
					((OPDI_DigitalPort *)(*it))->setLine(0);
				else
					((OPDI_DigitalPort *)(*it))->setLine(1);
			}
			else
				if ((*it)->getType()[0] == OPDI_PORTTYPE_ANALOG[0]) {
					// analog port: relative value (0..1)
					((OPDI_AnalogPort *)(*it))->setRelativeValue(value);
				}
				else
					if ((*it)->getType()[0] == OPDI_PORTTYPE_DIAL[0]) {
						// dial port: absolute value
						((OPDI_DialPort *)(*it))->setPosition((int64_t)value);
					}
					else
						if ((*it)->getType()[0] == OPDI_PORTTYPE_SELECT[0]) {
							// select port: current position number
							((OPDI_SelectPort *)(*it))->setPosition((uint16_t)value);
						}
						else
							throw PortError("");
		}
		catch (Poco::Exception &e) {
			this->opdid->logNormal(this->ID() + ": Error setting output port value of port " + (*it)->getID() + ": " + e.message());
		}

		++it;
	}
}

uint8_t OPDID_ExpressionPort::doWork(uint8_t canSend)  {
	OPDI_DigitalPort::doWork(canSend);

	if (this->line == 1) {
		// clear symbol table and values
		this->symbol_table.clear();

		this->prepareSymbols(false);

		// prepareVariables will return false in case of errors
		if (this->prepareVariables(false)) {

			double value = expression.value();

			this->logExtreme(this->ID() + ": Expression result: " + to_string(value));

			this->setOutputPorts(value);
		}
		else {
			// the variables could not be prepared, due to some error

			// fallback value specified?
			if (this->fallbackSpecified) {
				double value = this->fallbackValue;

				this->logExtreme(this->ID() + ": An error occurred, applying fallback value of: " + to_string(value));

				this->setOutputPorts(value);
			}
		}

		// maximum number of iterations specified and reached?
		if ((this->numIterations > 0) && (--this->iterations <= 0)) {
			// disable this expression
			this->setLine(0);
		}
	}

	return OPDI_STATUS_OK;
}

#endif
