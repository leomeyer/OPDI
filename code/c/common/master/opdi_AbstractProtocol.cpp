
/** This class implements generic functions of OPDI communication protocols.
 * 
 * @author Leo
 *
 */
#include "Poco/NumberParser.h"

#include "opdi_AbstractProtocol.h"

#include "opdi_strings.h"
#include "opdi_platformfuncs.h"
#include "opdi_MessageQueueDevice.h"
#include "opdi_StringTools.h"


POCO_IMPLEMENT_EXCEPTION(ProtocolException, Poco::ApplicationException, "Protocol Error")

POCO_IMPLEMENT_EXCEPTION(DisconnectedException, Poco::ApplicationException, "Disconnected")

POCO_IMPLEMENT_EXCEPTION(DisagreementException, Poco::ApplicationException, "Disagreement")

POCO_IMPLEMENT_EXCEPTION(TimeoutException, Poco::ApplicationException, "Timeout")

POCO_IMPLEMENT_EXCEPTION(PortAccessDeniedException, Poco::ApplicationException, "Port Access Denied")

POCO_IMPLEMENT_EXCEPTION(PortErrorException, Poco::ApplicationException, "Port Error")


const double AbstractProtocol::HANDSHAKE_VERSION_DOUBLE = parseDouble(OPDI_Handshake_version, "Handshake version", 0, 1000);
long AbstractProtocol::HANDSHAKE_TIMEOUT = 5000L;

int AbstractProtocol::parseInt(std::string s, std::string paramName, int min, int max)
{
	try {
		int value = stoi(s);
		if (value < min)
			throw ProtocolException("Parameter " + paramName + ": value too small: " + s);
		if (value > max)
			throw ProtocolException("Parameter " + paramName + ": value too large: " + s);
		return value;
	} catch (std::invalid_argument iae) {
		throw ProtocolException("Parameter " + paramName + ": number expected instead of '" + s + "'");
	} catch (std::out_of_range oore) {
		throw ProtocolException("Parameter " + paramName + ": number too large for int: '" + s + "'");
	}
}

double AbstractProtocol::parseDouble(std::string s, std::string paramName, double min, double max)
{
	try {
		double value = Poco::NumberParser::parseFloat(s);
		if (value < min)
			throw ProtocolException("Parameter " + paramName + ": value too small: " + s);
		if (value > max)
			throw ProtocolException("Parameter " + paramName + ": value too large: " + s);
		return value;
	} catch (Poco::SyntaxException se) {
		throw ProtocolException("Parameter " + paramName + ": number expected instead of '" + s + "'");
	}
}

AbstractProtocol::AbstractProtocol(IDevice* device) {
	this->device = device;
}

IDevice* AbstractProtocol::getDevice() {
	return device;
}

int AbstractProtocol::send(Message* message)
{
	if (!device->isConnected()) 
		throw DisconnectedException();
	device->sendMessage(message);
	return message->getChannel();
}
	
void AbstractProtocol::sendError(std::string message)
{
	if (!device->isConnected()) return;
	Message* msg = new Message(0, OPDI_Error); //StringTools::join(SEPARATOR, std::string() + ERR, message));
	device->sendMessage(msg);
}
	
Message* AbstractProtocol::expect(long channel, unsigned int timeout /*, IAbortable abortable */)
{
	if (channel < 0)
		throw DisconnectedException();

	Poco::NotificationQueue* queue = device->getInputMessages();

	long startTime = opdi_get_time_ms();
	while (opdi_get_time_ms() - startTime < timeout && 
			//(abortable == null || !abortable.isAborted()) &&
			device->isConnected()) {

		Poco::AutoPtr<Poco::Notification> pNf(queue->waitDequeueNotification(1));
		// message received?
		if (pNf) {
			MessageNotification* mn = dynamic_cast<MessageNotification*>(pNf.get());
			if (mn)
			{
				// check whether a message with this channel is in the queue
				if (mn->message->getChannel() == channel) {

					// analyze message for port status
					std::vector<std::string> parts;
					StringTools::split(mn->message->getPayload(), SEPARATOR, parts);
					if (parts.size() < 1)
						throw ProtocolException("invalid number of message parts");

					if (parts[0] == OPDI_Disagreement)
					{
						if (parts.size() > 1)
							throw PortAccessDeniedException(parts[1]);
						else
							throw PortAccessDeniedException();
					}
					else
					if (parts[0] == OPDI_Error)
					{
						if (parts.size() > 1)
							throw PortErrorException(parts[1]);
						else
							throw PortErrorException();
					}
					else
						return mn->message;
				}
				else {
					// the message is for a different channel; let other thread handle it
					queue->enqueueUrgentNotification(mn);
				}
			}
		}

		/*
	if (abortable != null && abortable.isAborted())
		throw new InterruptedException("The operation was interrupted");
		*/
	}
		
	// the device may have disconnected (due to an error or planned action)
	if (!device->isConnected())
		throw DisconnectedException();
		
	// the operation timed out
	device->setError(-1, "Timeout waiting for a message");
	throw TimeoutException();
}

/*
Message expect(long channel, int timeout) throws TimeoutException, InterruptedException, DisconnectedException, DeviceException {
		return expect(channel, timeout, null);
	}
	*/

void AbstractProtocol::disconnect() {
	// send disconnect message
	try {
		send(new Message(0, OPDI_Disconnect));
	} catch (DisconnectedException e) {
		// ignore DisconnectedExceptions
	}
	// do not expect an answer
}
