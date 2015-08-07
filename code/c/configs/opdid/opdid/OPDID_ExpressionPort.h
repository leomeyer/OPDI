#pragma once

// need to guard against security check warnings
#define _SCL_SECURE_NO_WARNINGS	1

#include "Poco/Util/AbstractConfiguration.h"

// expression evaluation library
#include <exprtk.hpp>

#include "OPDID_PortFunctions.h"

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
	OPDI::PortList outputPorts;

	typedef exprtk::symbol_table<double> symbol_table_t;
	typedef exprtk::expression<double> expression_t;
	typedef exprtk::parser<double> parser_t;
	typedef parser_t::dependent_entity_collector::symbol_t symbol_t;
	std::deque<symbol_t> symbol_list;

	symbol_table_t symbol_table;
	expression_t expression;

	virtual bool prepareVariables(bool duringSetup);

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

