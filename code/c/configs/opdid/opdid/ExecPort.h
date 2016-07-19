#pragma once

#include "Poco/Process.h"
#include "Poco/Thread.h"
#include "Poco/Util/AbstractConfiguration.h"
#include "Poco/PipeStream.h"
#include "Poco/StreamCopier.h"

#include <memory>

#include "PortFunctions.h"

namespace opdid {

///////////////////////////////////////////////////////////////////////////////
// Exec Port
///////////////////////////////////////////////////////////////////////////////

/** An ExecPort is a DigitalPort that starts an operating system process when its state is changed.
*   It has a certain WaitTime (in milliseconds) to avoid starting the process too often.
*   If set High and a ResetTime is specified (in milliseconds) the port will automatically reset to Low
*   after this time which may again trigger the process execution.
*   The process is started with the supplied parameters. The parameter string can contain
*   placeholders which are the names of ports; the syntax is $<port_id>. This expression will be
*   substituted with the value of the port when the process is being started. A DigitalPort
*   will be evaluated to 0 or 1. An AnalogPort will be evaluated as its relative value. A
*   DialPort will be evaluated as its absolute value, as will the position of a SelectPort.
*   If an error occurs during port evaluation the value will be represented as "<error>".
*   A port ID that cannot be found will not be substituted.
*   The special value $ALL_PORTS in the parameter string will be replaced by a space-separated
*   list of <port_id>=<value> specifiers.
*   If the parameter ForceKill is true a running process is killed if it is still running when the
*   same process is to be started again.
*/
class ExecPort : public opdi::DigitalPort, protected PortFunctions {
protected:

    enum ChangeType {
        CHANGED_TO_HIGH,
        CHANGED_TO_LOW,
        ANY_CHANGE
    };

	class ProcessIO {
		std::unique_ptr<Poco::Pipe> outPipe;
		std::unique_ptr<Poco::Pipe> errPipe;
		std::unique_ptr<Poco::Pipe> inPipe;

		Poco::PipeInputStream pis;	// stdout
		Poco::PipeInputStream pes;	// stderr
		Poco::PipeOutputStream pos;	// stdin

		ExecPort* execPort;

	public:
		ProcessIO(ExecPort* execPort, std::unique_ptr<Poco::Pipe> outPipe, std::unique_ptr<Poco::Pipe> errPipe, std::unique_ptr<Poco::Pipe> inPipe)
			: pis(*outPipe.get()), pes(*errPipe.get()), pos(*inPipe.get()) {
			this->execPort = execPort;
		}

		void process() {
			// does the stdin stream request data?
			if (this->pos.eof()) {
				// data cannot be served, the process must be killed
				this->execPort->logWarning("Process " + this->execPort->to_string(this->execPort->processPID) + " requests data that cannot be provided, killing the process");
				Poco::Process::kill(this->execPort->processPID);
			}
			// check whether stream data is available
			int ces = (this->pes.good() ? this->pes.peek() : EOF);
			if (ces != EOF) {
				// read data from the stream
				std::string data;
				Poco::StreamCopier::copyToString(this->pes, data);
				this->execPort->logNormal("stderr: " + data);
			}
			int cis = (this->pis.good() ? this->pis.peek() : EOF);
			if (cis != EOF) {
				// read data from the stream
				std::string data;
				Poco::StreamCopier::copyToString(this->pis, data);
				this->execPort->logVerbose("stdout: " + data);
			}
		}
	};

	std::string programName;
	std::string parameters;
	ChangeType changeType;
	int64_t waitTimeMs;
	int64_t resetTimeMs;
	bool forceKill;

	uint8_t lastState;
	Poco::Timestamp lastTriggerTime;
	Poco::Process::PID processPID;
	ProcessIO* processIO;

	virtual uint8_t doWork(uint8_t canSend);

public:
	ExecPort(AbstractOPDID* opdid, const char* id);

	virtual ~ExecPort();

	virtual void configure(Poco::Util::AbstractConfiguration* config);

	virtual void setDirCaps(const char* dirCaps) override;

	virtual void setMode(uint8_t mode, ChangeSource changeSource = opdi::Port::ChangeSource::CHANGESOURCE_INT) override;

	virtual void prepare() override;
};

}		// namespace opdid