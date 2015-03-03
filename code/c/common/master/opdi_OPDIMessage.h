#ifndef __OPDI_OPDIMESSAGE_H
#define __OPDI_OPDIMESSAGE_H

#include "Poco/Exception.h"

class MessageException : public Poco::Exception
{
public:
	MessageException(std::string message): Poco::Exception(message, 0) {}
};

/** Represents a message that is sent to or from a device.
 *
 * @author Leo
 *
 */
class OPDIMessage {
	
protected:
	int channel;
	std::string payload;
	int checksum;

public:
	static const char SEPARATOR = ':';
	static const char TERMINATOR = '\n';

	/** Creates a message.
	 * 
	 * @param payload
	 */
	OPDIMessage(int channel, std::string payload);
	
	OPDIMessage(int channel, std::string payload, int checksum);

	static OPDIMessage* decode(char *serialForm/*, Charset encoding*/);

	/** Returns the serial form of a message that contains payload and checksum.
	 * The parameter maxlength specifies the maximum length of the buffer.
	 * Throws a MessageException if the buffer is not large enough to contain 
	 * the message content.
	 * If everything is ok, returns the length of the message in bytes.
	 * @throws MessageException 
	 */
	int encode(int encoding, char buffer[], int maxlength);

	std::string getPayload();

	int getChannel();

	std::string toString();
};

#endif
