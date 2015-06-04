#include "opdi_platformfuncs.h"

#include "opdi_MessageQueueDevice.h"
#include "opdi_IODevice.h"

/** This class defines the functions of a device that communicates via messages that are put into
 * input and output queues. It provides an IBasicProtocol object that is connected to these queues.
 * 
 * @author Leo
 *
 */

using Poco::Mutex;

#define BUFFER_SIZE			32

MessageProcessor::MessageProcessor(MessageQueueDevice* device, IBasicProtocol* protocol)
{
	this->device = device;
	this->protocol = protocol;
	this->stop = false;
	this->hasMessagesToSend = false;
}

/**
    * This method runs during a connection with a remote device.
    * It handles all incoming and outgoing transmissions.
	*/
void MessageProcessor::run()
{
	// Read and write messages while not disconnected

	std::vector<char> message;
	char buffer[BUFFER_SIZE];

	int bytesProcessed = 0;

	while (!stop || hasMessagesToSend) {
        try {
			if ((device->getEncryption() == 0 ? device->hasBytes() > 0 : device->has_block())) {
        		int bytes = (device->getEncryption() == 0 ? device->read(buffer, BUFFER_SIZE) : device->read_block(buffer));
				// append received bytes to message until terminator character
				int terminatorPos = -1;
				int bufferEnd = 0;
				for (; bufferEnd < bytes; bytesProcessed++, bufferEnd++) {
        			if (buffer[bufferEnd] == OPDIMessage::TERMINATOR) {
        				terminatorPos = bytesProcessed;
						// signal end of message string
						message.push_back('\0');
						break;
        			}
					else
						message.push_back(buffer[bufferEnd]);
				}
        		
        		// at least one complete message?
        		while (terminatorPos > -1) {
        			int msgLen = terminatorPos;
        			// copy current part
					std::vector<char> part(message.begin(), message.begin() + msgLen + 1);
					// try to decode the message
					try {
						OPDIMessage* msg = OPDIMessage::decode(&part[0]); //, getEncoding());
                		device->logDebug("Message received: " + msg->toString());
						// message is valid
						// let the protocol dispatch the message in case it contains streaming data
						if (!protocol->dispatch(msg))
							// add the message to the input queue
							device->enqueueIn(msg);
					} catch (MessageException e) {
						device->logDebug("Invalid message: " + e.displayText());
                		device->logDebug("Invalid message content: " + (bytesProcessed == 0 ? std::string("<empty>") : std::string(&part[0])));
					}
					// start over with a new message
					bytesProcessed = 0;
					terminatorPos = -1;
					message.clear();
					// there may be remaining characters in buffer after the terminator
					if (bytes > ++bufferEnd) {
						// only if encryption is off - in this case we discard the remaining buffer because
						// we deal with a certain block size
						if (device->getEncryption() == 0) {
							for (; bufferEnd < bytes; bytesProcessed++, bufferEnd++) {
        						if (buffer[bufferEnd] == OPDIMessage::TERMINATOR) {
        							terminatorPos = bytesProcessed;
									// signal end of message string
									message.push_back('\0');
									break;
        						}
								else
									message.push_back(buffer[bufferEnd]);
							}
						}
					}        	
				}
			}
			// are there messages to send?
			hasMessagesToSend = device->processOutQueue();
			Poco::Thread::sleep(1);
        } catch (Poco::IOException e) {
            if (!stop) {
	            device->setDeviceError("IO error: " + e.displayText());
            }
            return;
        } catch (MessageException e) {
            if (!stop) {
				device->setDeviceError("Invalid message: " + e.displayText());
            }
            return;
		} catch (DeviceException e) {
            if (!stop) {
				device->setDeviceError("Device error: " + e.displayText());
            }
		}
		done = true;
    }
	/*
	byte[] message = new byte[getBufferSize()];
	byte[] buffer = new byte[getBufferSize()];
	int bytesProcessed = 0;
    while (!stop || hasMessagesToSend) {
        try {
            // Read from the InputStream
            synchronized(inQueue) {
                if ((encryption == null ? hasBytes() > 0 : has_block())) {
                    // fill buffer with available bytes
                    int bytes = (encryption == null ? read(buffer) : read_block(buffer));
                    // need to increase message?
                    if (bytesProcessed + bytes > message.length) {
                    	byte[] msg = message;
                    	message = new byte[message.length + getBufferSize()];
                    	System.arraycopy(msg, 0, message, 0, bytesProcessed);
                    }
                    // append received bytes to message until terminator character
                    int terminatorPos = -1;
                    int bufferEnd = 0;
                    for (; bufferEnd < bytes; bytesProcessed++, bufferEnd++) {
                    	message[bytesProcessed] = buffer[bufferEnd];
                    	if (buffer[bufferEnd] == Message.TERMINATOR) {
                    		terminatorPos = bytesProcessed;
                    		break;
                    	}
                    }
                    		
                    // at least one complete message?
                    while (terminatorPos > -1) {
                    	int msgLen = terminatorPos;
                    	// copy current part
                    	byte[] part = new byte[msgLen];
                    	System.arraycopy(message, 0, part, 0, msgLen);
                        // try to decode the message
                        Message msg;
						try {
							msg = Message.decode(part, getEncoding());
                            logDebug("Message received: " + msg.toString());
                            // message is valid
	                        // let the protocol dispatch the message in case it contains streaming data
	                        if (!protocol.dispatch(msg))
	                        	// add the message to the input queue
	                        	inQueue.add(msg);
						} catch (MessageException e) {
                            logDebug("Invalid message: " + e.getMessage());
                            logDebug("Invalid message content: " + (bytesProcessed == 0 ? "<empty>" : new String(part, getEncoding())));
                        }
                            	
                        // start over with a new message
                        bytesProcessed = 0;
                        terminatorPos = -1;
                        message = new byte[getBufferSize()];
                        // there may be remaining characters in buffer after the terminator
                        if (bytes > ++bufferEnd) {
                            // only if encryption is off - in this case we discard the remaining buffer because
                            // we deal with a certain block size
                            if (encryption == null) {
	                            for (; bufferEnd < bytes; bytesProcessed++, bufferEnd++) {
	                            	message[bytesProcessed] = buffer[bufferEnd];
	                            	if (buffer[bufferEnd] == Message.TERMINATOR) {
	                            		terminatorPos = bytesProcessed;
	                            		break;
	                            	}
	                            }
                            }
                        }
                    }
                }                		
            }
            // are there messages to send?
            synchronized(outQueue) {
                if (!outQueue.isEmpty()) {
                	Message m = outQueue.remove();
                			
                	sendSynchronous(m);
                        	
                    hasMessagesToSend = !outQueue.isEmpty();
                } else
                	hasMessagesToSend = false;
            }
            Thread.sleep(1);
        } catch (IOException e) {
            if (!stop) {
	            String msg = MessageFormat.format(ResourceFactory.instance.getString(ResourceFactory.CONNECTION_FAILURE), e.getMessage());
	            setDeviceError(msg);
            }
            return;
        } catch (MessageException e) {
            if (!stop) {
	            String msg = MessageFormat.format(ResourceFactory.instance.getString(ResourceFactory.INVALID_MESSAGE), e.getMessage());
	            setDeviceError(msg);
            }
            return;
        } catch (InterruptedException e) {
            if (!stop) {
	            String msg = "Thread interrupted";
	            setDeviceError(msg);
            }
            Thread.currentThread().interrupt();
            return;
		} catch (DeviceException e) {
            if (!stop) {
                String msg = MessageFormat.format(ResourceFactory.instance.getString(ResourceFactory.DEVICE_ERROR), e.getMessage());
	            setDeviceError(msg);
            }
		} finally {
			done = true;
		}                
    }
*/
}

void MessageProcessor::stopProcessing()
{
	stop = true;
}

MessageQueueDevice::MessageQueueDevice(std::string id): IDevice(id)
{
	status = DS_DISCONNECTED;
}

void MessageQueueDevice::sendMessage(OPDIMessage* message)
{
	// synchronized on outQueue
	Mutex::ScopedLock lockOut(outMutex);

	outQueue.enqueueNotification(new MessageNotification(message));
	msgProcessor->hasMessagesToSend = true;
}

void MessageQueueDevice::clearQueues() {
	// synchronized on inQueue
	Mutex::ScopedLock lockIn(inMutex);
	inQueue.clear();

	// synchronized on outQueue
	Mutex::ScopedLock lockOut(outMutex);
	outQueue.clear();
}
	
uint64_t MessageQueueDevice::getLastSendTimeMS()
{
	return lastSendTimeMS;
}

DeviceStatus MessageQueueDevice::getStatus()
{
	return status;
}
	
void MessageQueueDevice::setStatus(DeviceStatus status, std::string message)
{
	this->status = status;
	statusInfo = message;
}

void MessageQueueDevice::setStatus(DeviceStatus status)
{
	this->status = status;
	statusInfo = "";
}

std::string MessageQueueDevice::getStatusText()
{
	switch (status) {
	case DS_CONNECTED: return "Connected";
	case DS_DISCONNECTED : return "Disconnected";
	case DS_CONNECTING : return "Connecting";
	case DS_DISCONNECTING: return "Disconnecting";
	case DS_ERR : return "Error";
	}
	return "Unknown Status";
}		

bool MessageQueueDevice::usesEncryption()
{
	return isConnected() && getEncryption() != 0;
}

/** Start processing the messages.
	*/
void MessageQueueDevice::startMessageProcessing(IBasicProtocol* protocol) {		
	// Start the thread that handles message IO with the connected device
	msgProcessor = new MessageProcessor(this, protocol);
	msgThread.start(*msgProcessor);
	notifyOpened();
}

void MessageQueueDevice::stopProcessing()
{
	// synchronized on inQueue
	Mutex::ScopedLock lockIn(inMutex);
	// synchronized on outQueue
	Mutex::ScopedLock lockOut(outMutex);

	if (msgThread.isRunning()) {
		msgProcessor->stopProcessing();
	}
}
	
bool MessageQueueDevice::isConnected()
{
	return getStatus() == DS_CONNECTED;
}
	
bool MessageQueueDevice::isConnecting()
{
	return getStatus() == DS_CONNECTING;
}
	
void MessageQueueDevice::disconnect(bool onError)
{
	if (!onError && !isConnected()) return;
	setStatus(DS_DISCONNECTING);
	notifyTerminating();

	close();

	setStatus(DS_DISCONNECTED);
	notifyClosed();

	// free device listeners
	for (std::vector<IDeviceListener*>::iterator listener = listeners.begin(); listener != end(listeners); ++listener)
		delete (*listener);

	listeners.clear();
}

void MessageQueueDevice::notifyOpened() {
	// synchronized on listeners
	Mutex::ScopedLock lock(listenerMutex);

	for (std::vector<IDeviceListener*>::iterator listener = listeners.begin(); listener != end(listeners); ++listener)
		(*listener)->connectionOpened(this);
}

void MessageQueueDevice::notifyTerminating() {
	// synchronized on listeners
	Mutex::ScopedLock lock(listenerMutex);

	for (std::vector<IDeviceListener*>::iterator listener = listeners.begin(); listener != end(listeners); ++listener)
		(*listener)->connectionTerminating(this);
}	

void MessageQueueDevice::notifyClosed() {
	// synchronized on listeners
	Mutex::ScopedLock lock(listenerMutex);

	for (std::vector<IDeviceListener*>::iterator listener = listeners.begin(); listener != end(listeners); ++listener)
		(*listener)->connectionClosed(this);
}

void MessageQueueDevice::notifyError(std::string message) {
	// synchronized on listeners
	Mutex::ScopedLock lock(listenerMutex);

	for (std::vector<IDeviceListener*>::iterator listener = listeners.begin(); listener != end(listeners); ++listener)
		(*listener)->connectionError(this, message);
}
	
void MessageQueueDevice::addConnectionListener(IDeviceListener* aListener) {
	// synchronized on listeners
	Mutex::ScopedLock lock(listenerMutex);

	// listener already contained?
	for (std::vector<IDeviceListener*>::iterator listener = listeners.begin(); listener != end(listeners); ++listener)
		if ((*listener) == aListener)
			return;

	listeners.push_back(aListener);
}

void MessageQueueDevice::removeConnectionListener(IDeviceListener* aListener) {
	// synchronized on listeners
	Mutex::ScopedLock lock(listenerMutex);
	// find listener in list
	for (std::vector<IDeviceListener*>::iterator listener = listeners.begin(); listener != end(listeners); ++listener)
		if ((*listener) == aListener) {
			listeners.erase(listener);
			return;
		}
}

int MessageQueueDevice::getBufferSize() {
	return bufferSize;
}

void MessageQueueDevice::setBufferSize(int bufferSize) {
	this->bufferSize = bufferSize;
}

int MessageQueueDevice::getEncoding() {
		return encoding;
	}

void MessageQueueDevice::setEncoding(int encoding) {
/*
		if (!getMastersSupportedCharsets().contains(encoding))
			throw new IllegalArgumentException(ResourceFactory.instance.getString(ResourceFactory.ENCODING_NOT_SUPPORTED) + encoding);
			*/
	this->encoding = encoding;
}
	
void MessageQueueDevice::clearEncryption() 
{
	this->encryption = NO_ENCRYPTION;
}

MessageQueueDevice::Encryption MessageQueueDevice::getEncryption()
{
	return encryption;
}

void MessageQueueDevice::setEncryption(Encryption encryption) 
{
	/*
		random = new Random();
		if (encryption == null)
			clearEncryption();
		else
			switch (encryption) {
			case AES:
				if (getEncryptionKey() == null || getEncryptionKey().isEmpty())
					throw new DeviceException(ResourceFactory.instance.getString(ResourceFactory.ENCRYPTION_KEY_INVALID));
				
				skeySpec = new SecretKeySpec(getEncryptionKey().getBytes(), "AES");
	
				// Instantiate the cipher
				try {
					cipher = Cipher.getInstance("AES/ECB/NoPadding");
				} catch (Exception e) {
					throw new DeviceException(ResourceFactory.instance.getString(ResourceFactory.ENCRYPTION_ERROR_INITIALIZING) + e);
				}
				this.encryption = encryption;
				break;
			default: throw new IllegalArgumentException(ResourceFactory.instance.getString(ResourceFactory.ENCRYPTION_NOT_SUPPORTED) + encryption);
			}		
	*/
}


Poco::NotificationQueue* MessageQueueDevice::getInputMessages()
{
	return &inQueue;
}

void MessageQueueDevice::enqueueIn(OPDIMessage* message)
{
	// synchronize on inQueue
	Mutex::ScopedLock lock(inMutex);

	inQueue.enqueueNotification(new MessageNotification(message));
}

bool MessageQueueDevice::processOutQueue()
{
	// synchronize on outQueue
	Mutex::ScopedLock lock(inMutex);

	if (outQueue.size() > 0) {
		Poco::AutoPtr<Poco::Notification> pNf(outQueue.dequeueNotification());
		// message received?
		if (pNf) {
			MessageNotification* mn = dynamic_cast<MessageNotification*>(pNf.get());
                			
			sendSynchronous(mn->message);

//			delete mn->message;
		}
	}

	return outQueue.size() > 0;
}

// Returns true if there are enough bytes for a block to be read and decoded
bool MessageQueueDevice::has_block()
{
	/*
	switch (encryption) {
		case AES: return hasBytes() >= AES_BLOCKSIZE; 
		default: throw new IllegalArgumentException(ResourceFactory.instance.getString(ResourceFactory.ENCRYPTION_NOT_SUPPORTED) + encryption);
	}
	*/
	return false;
}
	
// read and decrypt an encrypted block from the wire
int MessageQueueDevice::read_block(char buffer[])
{
	/*
	try {
		switch (encryption) {
		case AES:
			byte[] bytes = new byte[AES_BLOCKSIZE];
			// read the block
			for (int i = 0; i < AES_BLOCKSIZE; i++)
				bytes[i] = read();

			// decrypt the block
			cipher.init(Cipher.DECRYPT_MODE, skeySpec);
			byte[] out = cipher.doFinal(bytes);
			System.arraycopy(out, 0, buffer, 0, AES_BLOCKSIZE);
			return AES_BLOCKSIZE;
		default: throw new IllegalArgumentException(ResourceFactory.instance.getString(ResourceFactory.ENCRYPTION_NOT_SUPPORTED) + encryption);
		}
	} catch (BadPaddingException e) {
		throw new DeviceException("Error decrypting block: " + e);
	} catch (InvalidKeyException e) {
		throw new DeviceException("Error decrypting block: " + e);
	} catch (IllegalBlockSizeException e) {
		throw new DeviceException("Error decrypting block: " + e);
	}
	*/
	return 0;
}
	
void MessageQueueDevice::write_blocks(char bytes[], int length)
{
	/*
		try {
			switch (encryption) {
			case AES:
				cipher.init(Cipher.ENCRYPT_MODE, skeySpec);
				byte[] block = new byte[AES_BLOCKSIZE];
				
				int b = 0;
				while (bytes.length > b * AES_BLOCKSIZE) {
					for (int i = 0; i < AES_BLOCKSIZE; i++) {
						// source position
						int pos = b * AES_BLOCKSIZE + i;
						if (pos < bytes.length)
							block[i] = bytes[pos];
						else
							// pad with random byte; terminator may not occur
							do { 
								block[i] = (byte)random.nextInt(); 
							} while (block[i] == Message.TERMINATOR);
							
					}
					
					// encrypt the block
					byte[] toSend = cipher.doFinal(block);
					write(toSend);
					// next block
					b++;
				}
				break;
			default: throw new IllegalArgumentException(ResourceFactory.instance.getString(ResourceFactory.ENCRYPTION_NOT_SUPPORTED) + encryption);
			}
		} catch (BadPaddingException e) {
			throw new DeviceException(ResourceFactory.instance.getString(ResourceFactory.ENCRYPTION_ERROR_BLOCK) + e);
		} catch (InvalidKeyException e) {
			throw new DeviceException(ResourceFactory.instance.getString(ResourceFactory.ENCRYPTION_ERROR_BLOCK) + e);
		} catch (IllegalBlockSizeException e) {
			throw new DeviceException(ResourceFactory.instance.getString(ResourceFactory.ENCRYPTION_ERROR_BLOCK) + e);
		}
		*/
}
	
	
/** Closes allocated resources. May fail silently.
	*/
void MessageQueueDevice::close()
{
	// signal to stop
	stopProcessing();
}

/** Sends a message immediately.
	* 
	* @param message
	* @throws IOException
	* @throws DeviceException 
	*/
void MessageQueueDevice::sendSynchronous(OPDIMessage* message) {
#define MESSAGE_MAXLENGTH		1024
	char bytes[MESSAGE_MAXLENGTH];
	int length = message->encode(0, bytes, MESSAGE_MAXLENGTH); 

    // write the bytes
	if (encryption == NO_ENCRYPTION)
		write(bytes, length);
	else
		write_blocks(bytes, length);
		
	lastSendTimeMS = opdi_get_time_ms();
	
    logDebug("Message sent: " + message->toString());
}	

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
OPDIMessage* MessageQueueDevice::receiveHandshakeMessage(int timeout /* , IAbortable abortable */)
{
	std::vector<char> message;
	char buffer[BUFFER_SIZE];

    int bytesReceived = 0;
	int counter = 0;

	while (counter++ < timeout /* && (abortable == null || !abortable.isAborted()) */) {
        if ((encryption == 0 ? hasBytes() > 0 : has_block())) {
        	int bytes = (encryption == 0 ? read(buffer, BUFFER_SIZE) : read_block(buffer));
        	// append received bytes to buffer
        	int terminatorPos = -1;
        	for (int i = 0; i < bytes; bytesReceived++, i++) {
        		if (buffer[i] == OPDIMessage::TERMINATOR) {
        			terminatorPos = bytesReceived;
					// signal end of message string
					message.push_back('\0');
        		}
				else
					message.push_back(buffer[i]);
        	}
        		
        	// at least one complete message?
        	while (terminatorPos > -1) {
        		int msgLen = terminatorPos;
        		// copy current part
				std::vector<char> part(message.begin(), message.begin() + msgLen + 1);
                // try to decode the message
                
				try {
					OPDIMessage* msg = OPDIMessage::decode(&part[0]); //, getEncoding());
                	logDebug("Message received: " + msg->toString());
                	// this discards remaining bytes in buffer but this shouldn't be a problem 
                	// because we're expecting handshake messages which are always synchronous
                	return msg;
				} catch (MessageException e) {
					logDebug("Invalid message: " + e.displayText());
                	logDebug("Invalid message content: " + (bytesReceived == 0 ? std::string("<empty>") : std::string(&part[0])));
                }
                // there may be remaining characters in buffer after the terminator
                if (bytes > terminatorPos + 1) {
                	// start over with the remainder that's in the buffer
					message.clear();
                	bytesReceived = 0;
                	int i = terminatorPos + 1;
                	terminatorPos = -1;
                	for (; i < bytes; bytesReceived++, i++) {
                		message.push_back(buffer[i]);
                		if (buffer[i] == OPDIMessage::TERMINATOR)
                			terminatorPos = i;
                	}
                }
        	}
        }                		
        Poco::Thread::sleep(1);
    }
	/*
	if (abortable != null && abortable.isAborted())
		throw new InterruptedException();
		*/
	throw Poco::TimeoutException("Timeout waiting for handshake message");
}
	
bool MessageQueueDevice::hasCredentials()
{
	return false;
}

void MessageQueueDevice::setDeviceError(std::string msg) {
    // close the connection
	close();
	setStatus(DS_ERR, msg);
	notifyError(msg);
}

void MessageQueueDevice::receivedDebug(std::string message) {
	// synchronized on listeners
	Mutex::ScopedLock lock(listenerMutex);
	// find listener in list
	for (std::vector<IDeviceListener*>::iterator listener = listeners.begin(); listener != end(listeners); ++listener)
		(*listener)->receivedDebug(this, message);
}

void MessageQueueDevice::receivedReconfigure() {
	// synchronized on listeners
	Mutex::ScopedLock lock(listenerMutex);
	// find listener in list
	for (std::vector<IDeviceListener*>::iterator listener = listeners.begin(); listener != end(listeners); ++listener)
		(*listener)->receivedReconfigure(this);
}

void MessageQueueDevice::receivedRefresh(std::vector<std::string> portIDs) {
	// synchronized on listeners
	Mutex::ScopedLock lock(listenerMutex);
	// find listener in list
	for (std::vector<IDeviceListener*>::iterator listener = listeners.begin(); listener != end(listeners); ++listener)
		(*listener)->receivedRefresh(this, portIDs);
}
	
void MessageQueueDevice::setError(int error, std::string text) {
		// map error message from int to text
//		String message = ResourceFactory.instance.getString(DeviceErrors.getErrorText(error));
//		if (text != null)
//			message += (message.equals("") ? "" : ": ") + text;
	setDeviceError(text);
}

MessageNotification::MessageNotification(OPDIMessage* message)
{
	this->message = message;
}

MessageNotification::~MessageNotification()
{
	// delete message;
}
