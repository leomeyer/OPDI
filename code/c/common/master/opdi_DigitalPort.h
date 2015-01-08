#ifndef __DIGITALPORT_H
#define __DIGITALPORT_H

#include "opdi_Port.h"

/** The mode a digital port may be in.*/
enum DigitalPortMode {
	DIGITAL_MODE_UNKNOWN = -1,
	/** The port is in input mode and floating. */
	DIGITAL_INPUT_FLOATING,
	/** The port is in input mode and the pull down resistor is active. */
	DIGITAL_INPUT_PULLUP,
	/** The port is in input mode and the pull down resistor is active. */
	DIGITAL_INPUT_PULLDOWN,
	/** The port is in output mode. */
	DIGITAL_OUTPUT
};	

/** The states a digital line may be in. */
enum DigitalPortLine {	
	DIGITAL_LINE_UNKNOWN = -1,
	/** Indicates that the port is low. */
	DIGITAL_LOW,
	/** Indicates that the port is high. */
	DIGITAL_HIGH
};	

/** Represents a digital port on a device. This port may be of type input, output, or bidirectional.
 * 
 * @author Leo
 *
 */
class DigitalPort : public Port {
	
protected:
	int flags;

	// State section
	DigitalPortMode mode;
	DigitalPortLine line;
	
public:

	/** Constructor for deserializing from wire form
	 * 
	 * @param protocol
	 * @param parts
	 * @throws ProtocolException
	 */
	DigitalPort(IBasicProtocol& protocol, std::vector<std::string> parts);
	
	std::string serialize() override;
	
	bool hasPullup();

	bool hasPulldown();
	
	// Retrieve all port state from the device
	void getPortState() override;
	
	void setPortState(IBasicProtocol& protocol, DigitalPortMode mode);

	void setPortLine(IBasicProtocol& protocol, DigitalPortLine line);
	
	void checkMode(DigitalPortMode portMode);

	/** Configures the port in the given mode. Throws an IllegalArgumentException if the mode
	 * is not supported.
	 * @param portMode
	 */
	void setMode(DigitalPortMode portMode);
	
	DigitalPortMode getMode();

	std::string getModeText();
	
	/** Sets the port to the given state. Throws an IllegalArgumentException if the state
	 * is not supported.
	 * @param portState
	 */
	void setLine(DigitalPortLine portLine);
	
	DigitalPortLine getLine();

	std::string getLineText();
	
	std::string toString() override;

	void refresh() override;
};

#endif
