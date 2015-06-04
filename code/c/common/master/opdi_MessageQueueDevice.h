#ifndef __OPDI_MESSAGEQUEUEDEVICE_H
#define __OPDI_MESSAGEQUEUEDEVICE_H

#include "Poco/Runnable.h"
#include "Poco/Thread.h"

#include "opdi_IDevice.h"

#define AES_BLOCKSIZE 16

/** This class encapsulates a message ready for notification.
* It takes ownership of the passed-in message. The message is destroyed when the notification is destroyed.
*/
class MessageNotification : public Poco::Notification
{
public:
	OPDIMessage *message;

	MessageNotification(OPDIMessage *message);
	~MessageNotification();
};

class MessageQueueDevice;

class MessageProcessor : public Poco::Runnable
{
private:
	MessageQueueDevice* device;
	IBasicProtocol *protocol;
    bool stop;
    bool done;

public:
	MessageProcessor(MessageQueueDevice* device, IBasicProtocol* protocol);

	virtual void run();

	void stopProcessing();

    // flag to ensure that remaining messages are sent before stopping
    volatile bool hasMessagesToSend;
};


/** This class defines the functions of a device that communicates via messages that are put into
 * input and output queues. It provides an IBasicProtocol object that is connected to these queues.
 * Supports encryption.
 *
 * @author Leo
 *
 */
class MessageQueueDevice : public IDevice {

//private:
//	std::ostream debug;

protected:
	/** Defines the possible encryption methods.
	 * 
	 * @author Leo
	 *
	 */
	enum Encryption {
		NO_ENCRYPTION,
		AES
	};

	// time in ms of the last message send as measured by System.currentTimeMillis()
	volatile uint64_t lastSendTimeMS;

	DeviceStatus status; // = DeviceStatus.DISCONNECTED;
	std::string statusInfo;

	Poco::NotificationQueue inQueue;	
	Poco::NotificationQueue outQueue;

	Poco::Mutex outMutex;
	Poco::Mutex inMutex;

	MessageProcessor* msgProcessor;
	Poco::Thread msgThread;

	std::vector<IDeviceListener*> listeners;
	Poco::Mutex listenerMutex;
	int bufferSize;
	int encoding;
	Encryption encryption;

	/*

	
	SecretKeySpec skeySpec; 
	Cipher cipher;	
	Random random;
*/
	MessageQueueDevice(std::string id);

public:

void sendMessage(OPDIMessage* message) override;

void clearQueues();
	
uint64_t getLastSendTimeMS() override;

DeviceStatus getStatus() override;
	
void setStatus(DeviceStatus status, std::string message);

void setStatus(DeviceStatus status);

std::string getStatusText() override;

bool usesEncryption() override;

/** Start processing the messages.
	*/
void startMessageProcessing(IBasicProtocol* protocol);

void stopProcessing();

bool isConnected() override;
	
bool isConnecting() override;
	
void disconnect(bool onError) override;

void notifyOpened();

void notifyTerminating();

void notifyClosed();

void notifyError(std::string message);
	
void addConnectionListener(IDeviceListener* listener) override;

void removeConnectionListener(IDeviceListener* listener) override;

int getBufferSize();

void setBufferSize(int bufferSize);

int getEncoding();

void setEncoding(int encoding);
	
void clearEncryption();

virtual Encryption getEncryption();

void setEncryption(Encryption encryption);

virtual std::string getEncryptionKey() = 0;

Poco::NotificationQueue* getInputMessages() override;

void enqueueIn(OPDIMessage* message);

// Process messages on the output queue. Returns true if there are more messages to be sent
bool processOutQueue();

// Returns true if there are enough bytes for a block to be read and decoded
virtual bool has_block();
	
// read and decrypt an encrypted block from the wire
virtual int read_block(char buffer[]);

// writes the bytes as blocks. count specifies the number of bytes total
virtual void write_blocks(char bytes[], int count);

/** Reads the next byte.
* 
* @param buffer
* @return
* @throws IOException
*/
virtual char read() = 0;

// reads the maximum number of bytes into the buffer. Returns the number of bytes read
virtual int read_bytes(char buffer[], int maxlength) = 0;

/** Returns the number of bytes that could be read by read().
* 
* @return
*/
virtual int hasBytes() = 0;

/** Reads the received bytes into the buffer.
* 
* @param buffer
* @return
* @throws IOException
*/
virtual int read(char buffer[], int length) = 0;

/** Writes the bytes out.
	* 
	* @param buffer
	* @throws IOException
	*/
virtual void write(char buffer[], int length) = 0;


/** Closes allocated resources. May fail silently.
	*/
virtual void close() = 0;

/** Sends a message immediately.
	* 
	* @param message
	* @throws IOException
	* @throws DeviceException 
	*/
void sendSynchronous(OPDIMessage* message);

/** Waits for a message until the timeout expires, the operation is aborted or a valid message
	* is actually received.
	* @param timeout
	* @param abortable
	* @return
	* @throws IOException 
	* @throws InterruptedException 
	* @throws AbortedException 
	* @throws TimeoutException 
	* @throws DeviceException 
	*/
OPDIMessage* receiveHandshakeMessage(int timeout /*, IAbortable abortable*/);

bool hasCredentials() override;

void setDeviceError(std::string msg);

void receivedDebug(std::string message) override;

void receivedReconfigure() override;

void receivedRefresh(std::vector<std::string> portIDs) override;
	
void setError(int error, std::string text) override;

};

#endif
