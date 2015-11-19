#pragma once

// need to guard against security check warnings
#define _SCL_SECURE_NO_WARNINGS	1

#include "Poco/Util/AbstractConfiguration.h"
#include "Poco/TimedNotificationQueue.h"

#include "OPDID_PortFunctions.h"

///////////////////////////////////////////////////////////////////////////////
// Timer Port
///////////////////////////////////////////////////////////////////////////////

/** A TimerPort is a DigitalPort that switches other DigitalPorts on or off 
*   according to one or more scheduled events. A TimerPort is output only.
*   In its doWork method the TimerPort checks whether a timestamp defined
*   by a schedule has been reached. If a schedule is due the line
*   of the output port(s) is set according to the schedule specification.
*   The TimerPort supports the following scheduling types:
*    - Once: Executes only at the specified time.
*    - Interval: Executes with the specifed interval.
*    - Periodic: Executes at matching points in time specified by a pattern.
*    - Random: Executes randomly.
*   In schedules of type Interval, Periodic and Random you can specify how often the schedule
*   should trigger. When the maximum number of occurrences has been reached the schedule
*   will become inactive.
*   The TimerPort will act on the output ports only if it is enabled (line = High).
*   When setting a TimerPort to Low, all future events (including deactivations) will be
*   ignored.
*   When setting the TimerPort to High after it has been Low, all schedules start to run again.
*   Each schedule can act on the output in three ways: SetHigh, SetLow and Toggle.
*   If the action is SetHigh all output ports will be set to High when the schedule triggers.
*   If the schedule specifies a deactivation delay the output ports will be set to Low when
*   the deactivation time has passed.
*   Correspondingly, if the action is SetLow all output ports will be set to Low when the schedule triggers.
*   If the schedule specifies a deactivation delay the output ports will be set to High when
*   the deactivation time has passed.
*   If the action is Toggle, the port state is queried and inverted. The same happens after the optional
*   deactivation time.
*   If the deactivation time specified is 0 (default) no deactivation event will take place.
*   To specify points in time or intervals you can use the following configuration properties:
*    - Year
*    - Month
*    - Day
*    - Weekday (only Periodic)
*    - Hour
*    - Minute
*    - Second
*
*   The schedule types Periodic and Random are not yet implemented.
*/
class OPDID_TimerPort : public OPDI_DigitalPort, protected OPDID_PortFunctions {

protected:
	// helper class
	class ScheduleComponent {
	private:
		std::vector<bool> values;

	public:
		enum Type {
			MONTH,
			DAY,
			WEEKDAY,
			HOUR,
			MINUTE,
			SECOND
		};
		Type type;

		static int ParseValue(Type type, std::string val);

		static ScheduleComponent Parse(Type type, std::string def);

		bool getNextPossibleValue(int* currentValue, bool* rollover, bool* changed, int month, int year);

		bool getFirstPossibleValue(int* value, int month, int year);

		bool hasValue(int value);

		int getMinimum(void);

		int getMaximum(void);
	};

	enum ScheduleType {
		ONCE,
		INTERVAL,
		PERIODIC,
		ASTRONOMICAL,
		RANDOM,
		ONLOGIN,
		ONLOGOUT
	};

	enum AstroEvent {
		SUNRISE,
		SUNSET
	};

	enum Action {
		SET_HIGH,
		SET_LOW,
		TOGGLE
	};

	struct Schedule {
		std::string nodeName;
		ScheduleType type;
		Action action;
		union data {
			struct time {		// for time-based schedules
				int16_t year;
				int16_t month;
				int16_t day;
				int16_t weekday;
				int16_t hour;
				int16_t minute;
				int16_t second;
			} time;
			struct random {		// for random-based schedules
				int jitter;
				// TODO
			} random;
		} data;

		// schedule components for PERIODIC
		ScheduleComponent monthComponent;
		ScheduleComponent dayComponent;
		ScheduleComponent hourComponent;
		ScheduleComponent minuteComponent;
		ScheduleComponent secondComponent;
		ScheduleComponent weekdayComponent;

		// parameters for ASTRONOMICAL
		AstroEvent astroEvent;
		int64_t astroOffset;
		double astroLon;
		double astroLat;

		int occurrences;		// occurrence counter
		int maxOccurrences;		// maximum number of occurrences that this schedule is active (counted from application start)
		uint64_t duration;		// duration in milliseconds until the timer is deactivated (0 = no deactivation)

		Poco::Timestamp nextEvent;
	};

	class ScheduleNotification : public Poco::Notification {
	public:
		typedef Poco::AutoPtr<ScheduleNotification> Ptr;
		Poco::Timestamp timestamp;
		Schedule *schedule;
		bool deactivate;

		inline ScheduleNotification(Schedule *schedule, bool deactivate) {
			this->schedule = schedule;
			this->deactivate = deactivate;
		};
	};

	typedef std::vector<Schedule> ScheduleList;
	ScheduleList schedules;

	std::string outputPortStr;
	DigitalPortList outputPorts;

	Poco::TimedNotificationQueue queue;

	Poco::Timestamp calculateNextOccurrence(Schedule *schedule);
	std::string deactivatedText;
	std::string notScheduledText;
	std::string timestampFormat;
	std::string nextOccurrenceStr;

	bool masterLoggedIn;

	Poco::Timestamp lastWorkTimestamp;

	void addNotification(ScheduleNotification::Ptr notification, Poco::Timestamp timestamp);

	bool matchWeekday(int day, int month, int year, ScheduleComponent *weekdayScheduleComponent);

	void recalculateSchedules(void);

	virtual uint8_t doWork(uint8_t canSend) override;

public:
	OPDID_TimerPort(AbstractOPDID *opdid, const char *id);

	virtual ~OPDID_TimerPort();

	virtual void configure(Poco::Util::AbstractConfiguration *config, Poco::Util::AbstractConfiguration *parentConfig);

	virtual void setDirCaps(const char *dirCaps) override;

	virtual void setMode(uint8_t mode) override;

	virtual void setLine(uint8_t line) override;

	virtual void prepare() override;

	virtual std::string getExtendedState(void) override;
};
