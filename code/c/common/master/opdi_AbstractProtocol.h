#ifndef __OPDI_ABSTRACTPROTOCOL_H
#define __OPDI_ABSTRACTPROTOCOL_H

#include <sstream>

#include <Poco/Exception.h>

#include "opdi_protocol_constants.h"

#include <opdi_IDevice.h>

#define EXCEPTION_API

POCO_DECLARE_EXCEPTION(EXCEPTION_API, ProtocolException, Poco::ApplicationException)

POCO_DECLARE_EXCEPTION(EXCEPTION_API, DisconnectedException, Poco::ApplicationException)

POCO_DECLARE_EXCEPTION(EXCEPTION_API, DisagreementException, Poco::ApplicationException)

POCO_DECLARE_EXCEPTION(EXCEPTION_API, TimeoutException, Poco::ApplicationException)

POCO_DECLARE_EXCEPTION(EXCEPTION_API, PortErrorException, Poco::ApplicationException)

POCO_DECLARE_EXCEPTION(EXCEPTION_API, PortAccessDeniedException, Poco::ApplicationException)

/** This class implements generic functions of OPDI communication protocols.
 * 
 * @author Leo
 *
 */
class AbstractProtocol : public IBasicProtocol {

	/*
	public interface IAbortable {
		public boolean isAborted();
	}
	*/
public:
	static const char SEPARATOR = ':';
/*
	static const char *DISCONNECT;
	static const char *HANDSHAKE;
	static const char *HANDSHAKE_VERSION;
	static const char *AGREEMENT;
	static const char *DISAGREEMENT;
	static const char *AUTHENTICATE;
*/
	static const double HANDSHAKE_VERSION_DOUBLE;
	static long HANDSHAKE_TIMEOUT;

	/** Parses a parameter as integer and throws exceptions if error conditions are met.
	 * 
	 * @param s
	 * @param paramName
	 * @param min
	 * @param max
	 * @return
	 * @throws ProtocolException
	 */
	static int parseInt(std::string s, std::string paramName, int min, int max);

	/** Parses a parameter as double and throws exceptions if error conditions are met.
	 * 
	 * @param s
	 * @param paramName
	 * @param min
	 * @param max
	 * @return
	 * @throws ProtocolException
	 */
	static double parseDouble(std::string s, std::string paramName, double min, double max);

protected:
	IDevice* device;
	
	AbstractProtocol(IDevice* device);

	virtual IDevice* getDevice();
	/** Tries to send a message to the device. If the device is not connected, throws a DisconnectedException 
	 * otherwise returns the channel of the message.
	 * 
	 * @param message
	 * @return
	 * @throws DisconnectedException 
	 */
	virtual int send(OPDIMessage* message);
	
	/** Sends the error message to the device. This will cause the connection to be terminated.
	 * 
	 * @param message
	 */
	virtual void sendError(std::string message);
	
	/** This method expects an input message with the specific channel within the specified timeout
	 * (in milliseconds). If such a message does not arrive a TimeoutException is raised. Otherwise
	 * the message is returned. This method blocks the calling thread. It should not be called
	 * from the UI thread. If the device is disconnected during waiting a DisconnectedException is raised.
	 * If the thread is interrupted an InterruptedException is thrown.
	 * Time calculation is not absolutely exact but it's guaranteed to not be less than timeout milliseconds.
	 * 
	 */
	virtual OPDIMessage* expect(long channel, unsigned int timeout /*, IAbortable abortable */);
	
	// A convenience method without an abortable.
	//virtual Message expect(long channel, int timeout);

	/** Initiates the protocol. Must be provided by subclasses.
	 * 
	 */
	virtual void initiate() = 0;

	/** Sends the disconnect message to the connected device.
	 * 
	 */
	virtual void disconnect();

	
	template <class T> std::string to_string(const T& t);
};


template <class T> inline std::string AbstractProtocol::to_string(const T& t) {
	std::stringstream ss;
	ss << t;
	return ss.str();
}

#endif

