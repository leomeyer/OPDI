
#include "OPDID_ExpressionPort.h"

#include "opdi_constants.h"

#ifdef OPDID_USE_EXPRTK

///////////////////////////////////////////////////////////////////////////////
// Expression Port
///////////////////////////////////////////////////////////////////////////////

OPDID_ExpressionPort::OPDID_ExpressionPort(AbstractOPDID *opdid, const char *id) : OPDI_DigitalPort(id, id, OPDI_PORTDIRCAP_OUTPUT, 0), OPDID_PortFunctions(id) {
	this->opdid = opdid;

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
}

void OPDID_ExpressionPort::setDirCaps(const char *dirCaps) {
	throw PortError(this->ID() + ": The direction capabilities of an ExpressionPort cannot be changed");
}

void OPDID_ExpressionPort::setMode(uint8_t mode) {
	throw PortError(this->ID() + ": The mode of an ExpressionPort cannot be changed");
}

bool OPDID_ExpressionPort::prepareVariables(bool duringSetup) {
	// clear symbol table and values
	this->symbol_table.clear();
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
//	this->logDebug(this->ID() + ": Preparing port");
	OPDI_DigitalPort::prepare();

	// find ports; throws errors if something required is missing
	this->findPorts(this->getID(), "OutputPorts", this->outputPortStr, this->outputPorts);

	// prepare the expression
	symbol_table.add_constants();
	expression.register_symbol_table(symbol_table);

	parser_t parser;
	parser.enable_unknown_symbol_resolver();
	// collect only variables as symbol names
	parser.dec().collect_variables() = true;

	// compile to detect variables
	if (!parser.compile(this->expressionStr, expression))
		throw Poco::Exception(this->ID() + ": Error in expression: " + parser.error());

	// store symbol list (input variables)
	parser.dec().symbols(this->symbol_list);

	if (!this->prepareVariables(true)) {
		throw Poco::Exception(this->ID() + ": Unable to resolve variables");
	}
	parser.disable_unknown_symbol_resolver();

	if (!parser.compile(this->expressionStr, expression))
		throw Poco::Exception(this->ID() + ": Error in expression: " + parser.error());
}

uint8_t OPDID_ExpressionPort::doWork(uint8_t canSend)  {
	OPDI_DigitalPort::doWork(canSend);

	if (this->line == 1) {
		// prepareVariables will return false in case of errors
		if (this->prepareVariables(false)) {

			double value = expression.value();

			this->logExtreme(this->ID() + ": Expression result: " + to_string(value));

			// go through list of output ports
			OPDI::PortList::iterator it = this->outputPorts.begin();
			while (it != this->outputPorts.end()) {
				try {
					if ((*it)->getType()[0] == OPDI_PORTTYPE_DIGITAL[0]) {
						if (value == 0)
							((OPDI_DigitalPort *)(*it))->setLine(0);
						else
							((OPDI_DigitalPort *)(*it))->setLine(1);
					} else 
					if ((*it)->getType()[0] == OPDI_PORTTYPE_ANALOG[0]) {
						// analog port: relative value (0..1)
						((OPDI_AnalogPort *)(*it))->setRelativeValue(value);
					} else 
					if ((*it)->getType()[0] == OPDI_PORTTYPE_DIAL[0]) {
						// dial port: absolute value
						((OPDI_DialPort *)(*it))->setPosition((int64_t)value);
					} else 
					if ((*it)->getType()[0] == OPDI_PORTTYPE_SELECT[0]) {
						// select port: current position number
						((OPDI_SelectPort *)(*it))->setPosition((uint16_t)value);
					} else
						throw PortError("");
				} catch (Poco::Exception &e) {
					this->opdid->logNormal(this->ID() + ": Error setting output port value of port " + (*it)->getID() + ": " + e.message());
				}

				++it;
			}
		}
	}

	return OPDI_STATUS_OK;
}

#endif
