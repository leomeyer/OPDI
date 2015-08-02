#pragma once

#include "Poco/Util/AbstractConfiguration.h"

#include "OPDID_PortFunctions.h"

///////////////////////////////////////////////////////////////////////////////
// Exec Port
///////////////////////////////////////////////////////////////////////////////

/** An ExecPort is a DigitalPort that starts an operating system process when set from Low to High.
*   It has a certain WaitTime (in milliseconds) to avoid starting the process too often.
*   If a ResetTime is specified (in milliseconds) the port will automatically reset to Low
*   after this time.
*   The process is started with the supplied parameters. The parameter string can contain
*   placeholders which are the names of ports; the syntax is $<port_id>. This expression will be
*   substituted with the value of the port when the process is being started. A DigitalPort
*   will be evaluated to 0 or 1. An AnalogPort will be evaluated as its relative value. A
*   DialPort will be evaluated as its absolute value, as will the position of a SelectPort.
*   If an error occurs during port evaluation the value will be represented as "<error>".
*   A port ID that cannot be found will not be substituted.
*   The special value $ALL_PORTS in the parameter string will be replaced by a space-separated
*   list of <port_id>=<value> specifiers.
*   If the parameter ForceKill is a running process is killed if it is still running when the
*   same process is to be started again.
*/
class OPDID_ExecPort : public OPDI_DigitalPort, protected OPDID_PortFunctions {
protected:
	std::string programName;
	std::string parameters;
	uint64_t waitTimeMs;
	uint64_t resetTimeMs;
	bool forceKill;

	bool wasLow;
	Poco::Timestamp lastTriggerTime;
	Poco::Process::PID processPID;

	virtual uint8_t doWork(uint8_t canSend);

public:
	OPDID_ExecPort(AbstractOPDID *opdid, const char *id);

	virtual ~OPDID_ExecPort();

	virtual void configure(Poco::Util::AbstractConfiguration *config);

	virtual void setDirCaps(const char *dirCaps) override;

	virtual void setMode(uint8_t mode) override;

	virtual void prepare() override;
};
