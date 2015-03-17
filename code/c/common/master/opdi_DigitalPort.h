#ifndef __DIGITALPORT_H
#define __DIGITALPORT_H

#include "opdi_OPDIPort.h"


/** Represents a digital port on a device. This port may be of type input, output, or bidirectional.
 * 
 * @author Leo
 *
 */
class DigitalPort : public OPDIPort {
	
protected:
	int flags;

	// State section
	int8_t mode;
	int8_t line;
	
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
	
	void setPortState(IBasicProtocol& protocol, int8_t mode);

	void setPortLine(IBasicProtocol& protocol, int8_t line);
	
	void checkMode(int8_t portMode);

	/** Configures the port in the given mode. Throws an IllegalArgumentException if the mode
	 * is not supported.
	 * @param portMode
	 */
	void setMode(int8_t portMode);
	
	int8_t getMode();

	std::string getModeText();
	
	/** Sets the port to the given state. Throws an IllegalArgumentException if the state
	 * is not supported.
	 * @param portState
	 */
	void setLine(int8_t portLine);
	
	int8_t getLine();

	std::string getLineText();
	
	std::string toString() override;

	void refresh() override;
};

#endif
