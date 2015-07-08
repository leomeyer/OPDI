
#include "OPDID_TimerPort.h"

#include "Poco/Tuple.h"
#include "Poco/Timezone.h"
#include "Poco/DateTimeFormatter.h"
#include "Poco/NumberParser.h"
#include "Poco/Format.h"

#include "opdi_constants.h"

#include "SunRiseSet.h"

///////////////////////////////////////////////////////////////////////////////
// Timer Port
///////////////////////////////////////////////////////////////////////////////

OPDID_TimerPort::ScheduleComponent* OPDID_TimerPort::ScheduleComponent::Parse(Type type, std::string def) {
	ScheduleComponent* result = new ScheduleComponent();
	result->type = type;

	std::string compName;
	switch (type) {
	case MONTH: result->values.resize(13); compName = "Month"; break;
	case DAY: result->values.resize(32); compName = "Day"; break;
	case HOUR: result->values.resize(24); compName = "Hour"; break;
	case MINUTE: result->values.resize(60); compName = "Minute"; break;
	case SECOND: result->values.resize(60); compName = "Second"; break;
	case WEEKDAY:
		throw Poco::ApplicationException("Weekday is currently not supported");
	}

	// split definition at blanks
	std::stringstream ss(def);
	std::string item;
	while (std::getline(ss, item, ' ')) {
		bool val = true;
		// inversion?
		if (item[0] == '!') {
			val = false;
			item = item.substr(1);
		}
		if (item == "*") {
			// set all values
			for (size_t i = 0; i < result->values.size(); i++)
				result->values[i] = val;
		} else {
			// parse as integer
			int number = Poco::NumberParser::parse(item);
			bool valid = true;
			switch (type) {
			case MONTH: valid = (number >= 1) && (number <= 12); break;
			case DAY: valid = (number >= 1) && (number <= 31); break;
			case HOUR: valid = (number >= 0) && (number <= 23); break;
			case MINUTE: valid = (number >= 0) && (number <= 59); break;
			case SECOND: valid = (number >= 0) && (number <= 59); break;
			case WEEKDAY:
				throw Poco::ApplicationException("Weekday is currently not supported");
			}
			if (!valid)
				throw Poco::DataException("The specification '" + item + "' is not valid for the date/time component " + compName);
			result->values[number] = val;
		}
	}

	return result;
}

bool OPDID_TimerPort::ScheduleComponent::getNextPossibleValue(int* currentValue, bool* rollover, bool* changed, int month, int year) {
	*rollover = false;
	*changed = false;
	// find a match
	int i = *currentValue;
	for (; i < (int)this->values.size(); i++) {
		if (this->values[i]) {
			// for days, make a special check whether the month has as many days
			if ((this->type != DAY) || (i < Poco::DateTime::daysOfMonth(year, month))) {
				*changed = *currentValue != i;
				*currentValue = i;
				return true;
			}
			// day is out of range; need to rollover
			break;
		}
	}
	// not found? rollover; return first possible value
	*rollover = true;
	return this->getFirstPossibleValue(currentValue, month, year);
}

bool OPDID_TimerPort::ScheduleComponent::getFirstPossibleValue(int* currentValue, int month, int year) {
	// find a match
	int i = 0;
	switch (type) {
	case MONTH: i = 1; break;
	case DAY: i = 1; break;
	case HOUR: break;
	case MINUTE: break;
	case SECOND: break;
	case WEEKDAY:
		throw Poco::ApplicationException("Weekday is currently not supported");
	}
	for (; i < (int)this->values.size(); i++) {
		if (this->values[i]) {
			*currentValue = i;
			return true;
		}
	}
	// nothing found
	return false;
}

OPDID_TimerPort::OPDID_TimerPort(AbstractOPDID *opdid, const char *id) : OPDI_DigitalPort(id, id, OPDI_PORTDIRCAP_OUTPUT, 0) {
	this->opdid = opdid;

	OPDI_DigitalPort::setMode(OPDI_DIGITAL_MODE_OUTPUT);

	// default: enabled
	this->line = 1;
	this->masterLoggedIn = false;

	// set default icon
	this->icon = "alarmclock";

	// set default texts
	this->deactivatedText = "Deactivated";
	this->notScheduledText = "Not scheduled";
	this->timestampFormat = "Next event: " + opdid->timestampFormat;
}

OPDID_TimerPort::~OPDID_TimerPort() {
}

void OPDID_TimerPort::configure(Poco::Util::AbstractConfiguration *config, Poco::Util::AbstractConfiguration *parentConfig) {
	this->opdid->configureDigitalPort(config, this);
	this->logVerbosity = this->opdid->getConfigLogVerbosity(config, AbstractOPDID::UNKNOWN);

	this->outputPortStr = this->opdid->getConfigString(config, "OutputPorts", "", true);

	this->deactivatedText = config->getString("DeactivatedText", this->deactivatedText);
	this->notScheduledText = config->getString("NotScheduledText", this->notScheduledText);
	this->timestampFormat = config->getString("TimestampFormat", this->timestampFormat);

	// enumerate schedules of the <timer>.Schedules node
	this->logVerbose(std::string("Enumerating Timer schedules: ") + this->getID() + ".Schedules");

	Poco::Util::AbstractConfiguration *nodes = config->createView("Schedules");

	// get ordered list of schedules
	Poco::Util::AbstractConfiguration::Keys scheduleKeys;
	nodes->keys("", scheduleKeys);

	typedef Poco::Tuple<int, std::string> Item;
	typedef std::vector<Item> ItemList;
	ItemList orderedItems;

	// create ordered list of schedule keys (by priority)
	for (Poco::Util::AbstractConfiguration::Keys::const_iterator it = scheduleKeys.begin(); it != scheduleKeys.end(); ++it) {

		int itemNumber = nodes->getInt(*it, 0);
		// check whether the item is active
		if (itemNumber < 0)
			continue;

		// insert at the correct position to create a sorted list of items
		ItemList::iterator nli = orderedItems.begin();
		while (nli != orderedItems.end()) {
			if (nli->get<0>() > itemNumber)
				break;
			nli++;
		}
		Item item(itemNumber, *it);
		orderedItems.insert(nli, item);
	}

	if (orderedItems.size() == 0) {
		this->logNormal(std::string("Warning: No schedules configured in node ") + this->getID() + ".Schedules; is this intended?");
	}

	// go through items, create schedules in specified order
	ItemList::const_iterator nli = orderedItems.begin();
	while (nli != orderedItems.end()) {

		std::string nodeName = nli->get<1>();

		this->logVerbose("Setting up timer schedule for node: " + nodeName);

		// get schedule section from the configuration
		Poco::Util::AbstractConfiguration *scheduleConfig = parentConfig->createView(nodeName);

		Schedule schedule;
		schedule.nodeName = nodeName;

		schedule.maxOccurrences = scheduleConfig->getInt("MaxOccurrences", -1);
		schedule.duration = scheduleConfig->getInt("Duration", 1000);	// default duration: 1 second
		schedule.action = SET_HIGH;										// default action

		std::string action = this->opdid->getConfigString(scheduleConfig, "Action", "", false);
		if (action == "SetHigh") {
			schedule.action = SET_HIGH;
		} else
		if (action == "SetLow") {
			schedule.action = SET_LOW;
		} else
		if (action == "Toggle") {
			schedule.action = TOGGLE;
		} else
			if (action != "")
				throw Poco::DataException(nodeName + ": Unknown schedule action; expected: 'SetHigh', 'SetLow' or 'Toggle': " + action);

		// get schedule type (required)
		std::string scheduleType = this->opdid->getConfigString(scheduleConfig, "Type", "", true);

		if (scheduleType == "Once") {
			schedule.type = ONCE;
			schedule.data.time.year = scheduleConfig->getInt("Year", -1);
			schedule.data.time.month = scheduleConfig->getInt("Month", -1);
			schedule.data.time.day = scheduleConfig->getInt("Day", -1);
			schedule.data.time.weekday = scheduleConfig->getInt("Weekday", -1);
			schedule.data.time.hour = scheduleConfig->getInt("Hour", -1);
			schedule.data.time.minute = scheduleConfig->getInt("Minute", -1);
			schedule.data.time.second = scheduleConfig->getInt("Second", -1);

			if (schedule.data.time.weekday > -1)
				throw Poco::DataException(nodeName + ": You cannot use the Weekday setting with schedule type Once");
			if ((schedule.data.time.year < 0) || (schedule.data.time.month < 0)
				|| (schedule.data.time.day < 0)
				|| (schedule.data.time.hour < 0) || (schedule.data.time.minute < 0)
				|| (schedule.data.time.second < 0))
				throw Poco::DataException(nodeName + ": You have to specify all time components (except Weekday) for schedule type Once");
		} else
		if (scheduleType == "Interval") {
			schedule.type = INTERVAL;
			schedule.data.time.year = scheduleConfig->getInt("Year", -1);
			schedule.data.time.month = scheduleConfig->getInt("Month", -1);
			schedule.data.time.day = scheduleConfig->getInt("Day", -1);
			schedule.data.time.weekday = scheduleConfig->getInt("Weekday", -1);
			schedule.data.time.hour = scheduleConfig->getInt("Hour", -1);
			schedule.data.time.minute = scheduleConfig->getInt("Minute", -1);
			schedule.data.time.second = scheduleConfig->getInt("Second", -1);

			if ((schedule.data.time.day > -1) && (schedule.data.time.weekday > -1))
				throw Poco::DataException(nodeName + ": Please specify either Day or Weekday but not both");
			if (schedule.data.time.year > -1)
				throw Poco::DataException(nodeName + ": You cannot use the Year setting with schedule type Interval");
			if (schedule.data.time.month > -1)
				throw Poco::DataException(nodeName + ": You cannot use the Month setting with schedule type Interval");
			if (schedule.data.time.weekday > -1)
				throw Poco::DataException(nodeName + ": You cannot use the Weekday setting with schedule type Interval");

			if ((schedule.data.time.day < 0)
				&& (schedule.data.time.hour < 0) && (schedule.data.time.minute < 0)
				&& (schedule.data.time.second < 0))
				throw Poco::DataException(nodeName + ": You have to specify at least one of Day, Hour, Minute or Second for schedule type Interval");
		} else
		if (scheduleType == "Periodic") {
			schedule.type = PERIODIC;
			if (scheduleConfig->getString("Year", "") != "")
				throw Poco::DataException(nodeName + ": You cannot use the Year setting with schedule type Periodic");
			schedule.monthComponent = ScheduleComponent::Parse(ScheduleComponent::MONTH, scheduleConfig->getString("Month", "*"));
			schedule.dayComponent = ScheduleComponent::Parse(ScheduleComponent::DAY, scheduleConfig->getString("Day", "*"));
			schedule.hourComponent = ScheduleComponent::Parse(ScheduleComponent::HOUR, scheduleConfig->getString("Hour", "*"));
			schedule.minuteComponent = ScheduleComponent::Parse(ScheduleComponent::MINUTE, scheduleConfig->getString("Minute", "*"));
			schedule.secondComponent = ScheduleComponent::Parse(ScheduleComponent::SECOND, scheduleConfig->getString("Second", "*"));
		} else
		if (scheduleType == "Astronomical") {
			schedule.type = ASTRONOMICAL;
			std::string astroEventStr = this->opdid->getConfigString(scheduleConfig, "AstroEvent", "", true);
			if (astroEventStr == "Sunrise") {
				schedule.astroEvent = SUNRISE;
			} else
			if (astroEventStr == "Sunset") {
				schedule.astroEvent = SUNSET;
			} else
				throw Poco::DataException(nodeName + ": Parameter AstroEvent must be specified; use either 'Sunrise' or 'Sunset'");
			schedule.astroOffset = scheduleConfig->getInt("AstroOffset", 0);
			schedule.astroLon = scheduleConfig->getDouble("Longitude", -999);
			schedule.astroLat = scheduleConfig->getDouble("Latitude", -999);
			if ((schedule.astroLon < -180) || (schedule.astroLon > 180))
				throw Poco::DataException(nodeName + ": Parameter Longitude must be specified and within -180..180");
			if ((schedule.astroLat < -90) || (schedule.astroLat > 90))
				throw Poco::DataException(nodeName + ": Parameter Latitude must be specified and within -90..90");
			if ((schedule.astroLat < -65) || (schedule.astroLat > 65))
				throw Poco::DataException(nodeName + ": Sorry. Latitudes outside -65..65 are currently not supported (library crash). You can either relocate or try and fix the bug.");
		} else
		/*
		if (scheduleType == "Random") {
			schedule.type = RANDOM;
		} else
		*/
		if (scheduleType == "OnLogin") {
			schedule.type = ONLOGIN;
		} else
		if (scheduleType == "OnLogout") {
			schedule.type = ONLOGOUT;
		} else
			throw Poco::DataException(nodeName + ": Schedule type is not supported: " + scheduleType);

		this->schedules.push_back(schedule);

		nli++;
	}
}

void OPDID_TimerPort::setDirCaps(const char *dirCaps) {
	throw PortError(std::string(this->getID()) + ": The direction capabilities of a TimerPort cannot be changed");
}

void OPDID_TimerPort::setMode(uint8_t mode) {
	if (mode != OPDI_DIGITAL_MODE_OUTPUT)
		throw PortError(std::string(this->getID()) + ": The mode of a TimerPort cannot be set to anything other than 'Output'");
}

void OPDID_TimerPort::prepare() {
	OPDI_DigitalPort::prepare();

	// find ports; throws errors if something required is missing
	this->findDigitalPorts(this->getID(), "OutputPorts", this->outputPortStr, this->outputPorts);

	if (this->line == 1) {
		// calculate all schedules
		for (ScheduleList::iterator it = this->schedules.begin(); it != this->schedules.end(); it++) {
			Schedule *schedule = &*it;
/*
			// test cases
			if (schedule.type == ASTRONOMICAL) {
				CSunRiseSet sunRiseSet;
				Poco::DateTime now;
				Poco::DateTime today(now.julianDay());
				for (size_t i = 0; i < 365; i++) {
					Poco::DateTime result = sunRiseSet.GetSunrise(schedule.astroLat, schedule.astroLon, Poco::DateTime(now.julianDay() + i));
					this->opdid->println("Sunrise: " + Poco::DateTimeFormatter::format(result, this->opdid->timestampFormat));
					result = sunRiseSet.GetSunset(schedule.astroLat, schedule.astroLon, Poco::DateTime(now.julianDay() + i));
					this->opdid->println("Sunset: " + Poco::DateTimeFormatter::format(result, this->opdid->timestampFormat));
				}
			}
*/
			// calculate
			Poco::Timestamp nextOccurrence = this->calculateNextOccurrence(schedule);
			if (nextOccurrence > Poco::Timestamp()) {
				// add with the specified occurrence time
				this->addNotification(new ScheduleNotification(schedule, false), nextOccurrence);
			} else {
				this->logVerbose(this->ID() + ": Next scheduled time for " + schedule->nodeName + " could not be determined");
			}
		}
	}
}

Poco::Timestamp OPDID_TimerPort::calculateNextOccurrence(Schedule *schedule) {
	if (schedule->type == ONCE) {
		// validate
		if ((schedule->data.time.month < 1) || (schedule->data.time.month > 12))
			throw Poco::DataException(std::string(this->getID()) + ": Invalid Month specification in schedule " + schedule->nodeName);
		if ((schedule->data.time.day < 1) || (schedule->data.time.day > Poco::DateTime::daysOfMonth(schedule->data.time.year, schedule->data.time.month)))
			throw Poco::DataException(std::string(this->getID()) + ": Invalid Day specification in schedule " + schedule->nodeName);
		if (schedule->data.time.hour > 23)
			throw Poco::DataException(std::string(this->getID()) + ": Invalid Hour specification in schedule " + schedule->nodeName);
		if (schedule->data.time.minute > 59)
			throw Poco::DataException(std::string(this->getID()) + ": Invalid Minute specification in schedule " + schedule->nodeName);
		if (schedule->data.time.second > 59)
			throw Poco::DataException(std::string(this->getID()) + ": Invalid Second specification in schedule " + schedule->nodeName);
		// create timestamp via datetime
		Poco::DateTime result = Poco::DateTime(schedule->data.time.year, schedule->data.time.month, schedule->data.time.day, 
			schedule->data.time.hour, schedule->data.time.minute, schedule->data.time.second);
		// ONCE values are specified in local time; convert to UTC
		result.makeUTC(Poco::Timezone::tzd());
		return result.timestamp();
	} else
	if (schedule->type == INTERVAL) {
		Poco::Timestamp result;	// now
		// add interval values (ignore year and month because those are not well defined in seconds)
		if (schedule->data.time.second > 0)
			result += schedule->data.time.second * result.resolution();
		if (schedule->data.time.minute > 0)
			result += schedule->data.time.minute * 60 * result.resolution();
		if (schedule->data.time.hour > 0)
			result += schedule->data.time.hour * 60 * 60 * result.resolution();
		if (schedule->data.time.day > 0)
			result += schedule->data.time.day * 24 * 60 * 60 * result.resolution();
		return result;
	} else
	if (schedule->type == PERIODIC) {

#define correctValues	if (second > 59) { second = 0; minute++; if (minute > 59) { minute = 0; hour++; if (hour > 23) { hour = 0; day++; if (day > Poco::DateTime::daysOfMonth(year, month)) { day = 1; month++; if (month > 12) { month = 1; year++; }}}}}

		Poco::LocalDateTime now;
		// start from the next second
		int second = now.second() + 1;
		int minute = now.minute();
		int hour = now.hour();
		int day = now.day();
		int month = now.month();
		int year = now.year();
		correctValues;

		bool rollover = false;
		bool changed = false;
		// calculate next possible seconds
		if (!schedule->secondComponent->getNextPossibleValue(&second, &rollover, &changed, month, year))
			return Poco::Timestamp();
		// rolled over into next minute?
		if (rollover) {
			minute++;
			correctValues;
		}
		// get next possible minute
		if (!schedule->minuteComponent->getNextPossibleValue(&minute, &rollover, &changed, month, year))
			return Poco::Timestamp();
		// rolled over into next hour?
		if (rollover) {
			hour++;
			correctValues;
		}
		// if the minute was changed the second value is now invalid; set it to the first
		// possible value (because the sub-component range starts again with every new value)
		if (rollover || changed) {
			schedule->secondComponent->getFirstPossibleValue(&second, month, year);
		}
		// get next possible hour
		if (!schedule->hourComponent->getNextPossibleValue(&hour, &rollover, &changed, month, year))
			return Poco::Timestamp();
		// rolled over into next day?
		if (rollover) {
			day++;
			correctValues;
		}
		if (rollover || changed) {
			schedule->secondComponent->getFirstPossibleValue(&second, month, year);
			schedule->minuteComponent->getFirstPossibleValue(&minute, month, year);
		}
		// get next possible day
		if (!schedule->dayComponent->getNextPossibleValue(&day, &rollover, &changed, month, year))
			return Poco::Timestamp();
		// rolled over into next month?
		if (rollover) {
			month++;
			correctValues;
		}
		if (rollover || changed) {
			schedule->secondComponent->getFirstPossibleValue(&second, month, year);
			schedule->minuteComponent->getFirstPossibleValue(&minute, month, year);
			schedule->hourComponent->getFirstPossibleValue(&hour, month, year);
		}
		// get next possible month
		if (!schedule->monthComponent->getNextPossibleValue(&month, &rollover, &changed, month, year))
			return Poco::Timestamp();
		// rolled over into next year?
		if (rollover) {
			year++;
			correctValues;
		}
		if (rollover || changed) {
			schedule->secondComponent->getFirstPossibleValue(&second, month, year);
			schedule->minuteComponent->getFirstPossibleValue(&minute, month, year);
			schedule->hourComponent->getFirstPossibleValue(&hour, month, year);
			schedule->dayComponent->getFirstPossibleValue(&day, month, year);
		}
		Poco::DateTime result = Poco::DateTime(year, month, day, hour, minute, second);
		// values are specified in local time; convert to UTC
		result.makeUTC(Poco::Timezone::tzd());
		return result.timestamp();
	} else
	if (schedule->type == ASTRONOMICAL) {
		CSunRiseSet sunRiseSet;
		Poco::DateTime now;
		now.makeLocal(Poco::Timezone::tzd());
		Poco::DateTime today(now.julianDay());
		switch (schedule->astroEvent) {
		case SUNRISE: {
			// find today's sunrise
			Poco::DateTime result = sunRiseSet.GetSunrise(schedule->astroLat, schedule->astroLon, today);
			// sun already risen?
			if (result < now)
				// find tomorrow's sunrise
				result = sunRiseSet.GetSunrise(schedule->astroLat, schedule->astroLon, Poco::DateTime(now.julianDay() + 1));
			// values are specified in local time; convert to UTC
			result.makeUTC(Poco::Timezone::tzd());
			return result.timestamp() + schedule->astroOffset * 1000000;		// add offset in nanoseconds
		}
		case SUNSET: {
			// find today's sunset
			Poco::DateTime result = sunRiseSet.GetSunset(schedule->astroLat, schedule->astroLon, today);
			// sun already set?
			if (result < now)
				// find tomorrow's sunset
				result = sunRiseSet.GetSunset(schedule->astroLat, schedule->astroLon, Poco::DateTime(now.julianDay() + 1));
			// values are specified in local time; convert to UTC
			result.makeUTC(Poco::Timezone::tzd());
			return result.timestamp() + schedule->astroOffset * 1000000;		// add offset in nanoseconds
		}
		}
		return Poco::Timestamp();
	} else
	if (schedule->type == RANDOM) {
		// determine next point in type randomly
		return Poco::Timestamp();
	} else
		// return default value (now; must not be enqueued)
		return Poco::Timestamp();
}

void OPDID_TimerPort::addNotification(ScheduleNotification::Ptr notification, Poco::Timestamp timestamp) {
	Poco::Timestamp now;

	// for debug output: convert UTC timestamp to local time
	Poco::LocalDateTime ldt(timestamp);
	if (timestamp > now) {
		std::string timeText = Poco::DateTimeFormatter::format(ldt, this->opdid->timestampFormat);
		if (!notification->deactivate)
			this->logVerbose(this->ID() + ": Next scheduled time for node " + 
					notification->schedule->nodeName + " is: " + timeText);
		// add with the specified activation time
		this->queue.enqueueNotification(notification, timestamp);
		if (!notification->deactivate)
			notification->schedule->nextEvent = timestamp;
	} else {
		this->logNormal(this->ID() + ": Warning: Scheduled time for node " + 
				notification->schedule->nodeName + " lies in the past, ignoring: " + Poco::DateTimeFormatter::format(ldt, this->opdid->timestampFormat));
/*
		this->opdid->log(std::string(this->getID()) + ": Timestamp is: " + Poco::DateTimeFormatter::format(timestamp, "%Y-%m-%d %H:%M:%S"));
		this->opdid->log(std::string(this->getID()) + ": Now is      : " + Poco::DateTimeFormatter::format(now, "%Y-%m-%d %H:%M:%S"));
*/
	}
}

uint8_t OPDID_TimerPort::doWork(uint8_t canSend)  {
	OPDI_DigitalPort::doWork(canSend);

	// detect connection status change
	bool connected = this->opdid->isConnected() != 0;
	bool connectionStateChanged = connected != this->masterLoggedIn;
	this->masterLoggedIn = connected;

	// timer not active?
	if (this->line != 1)
		return OPDI_STATUS_OK;

	Poco::Notification::Ptr notification = NULL;

	if (connectionStateChanged) {
		// check whether a schedule is specified for this event
		ScheduleList::iterator it = this->schedules.begin();
		while (it != this->schedules.end()) {
			if ((*it).type == (connected ? ScheduleType::ONLOGIN : ScheduleType::ONLOGOUT)) {
				logDebug(this->ID() + ": Connection status change detected; executing schedule " + (*it).nodeName + ((*it).type == ONLOGIN ? " (OnLogin)" : " (OnLogout)"));
				// schedule found; create event notification
				notification = new ScheduleNotification(&*it, false);
				break;
			}
			it++;
		}
	}
	
	// notification created due to status change?
	if (notification.isNull())
		// no, get next object from the priority queue
		notification = this->queue.dequeueNotification();

	// notification will only be a valid object if a schedule is due
	if (notification) {
		try {
			ScheduleNotification::Ptr workNf = notification.cast<ScheduleNotification>();

			this->logVerbose(this->ID() + ": Timer reached scheduled " + (workNf->deactivate ? "deactivation " : "") 
				+ "time for node: " + workNf->schedule->nodeName);

			workNf->schedule->occurrences++;

			// cause master's UI state refresh if no deactivate
			this->refreshRequired = !workNf->deactivate;

			// calculate next occurrence depending on type; maximum ocurrences must not have been reached
			if ((!workNf->deactivate) && (workNf->schedule->type != ONCE) 
				&& ((workNf->schedule->maxOccurrences < 0) || (workNf->schedule->occurrences < workNf->schedule->maxOccurrences))) {

				Poco::Timestamp nextOccurrence = this->calculateNextOccurrence(workNf->schedule);
				if (nextOccurrence > Poco::Timestamp()) {
					// add with the specified occurrence time
					this->addNotification(workNf, nextOccurrence);
				} else {
					// warn if unable to calculate next ocucurrence; except if login or logout event
					if ((workNf->schedule->type != ONLOGIN) && (workNf->schedule->type != ONLOGOUT))
						this->logNormal(this->ID() + ": Warning: Next scheduled time for " + workNf->schedule->nodeName + " could not be determined");
				}
			}

			// need to deactivate?
			if ((!workNf->deactivate) && (workNf->schedule->duration > 0)) {
				// enqueue the notification for the deactivation
				Schedule *deacSchedule = workNf->schedule;
				ScheduleNotification *notification = new ScheduleNotification(deacSchedule, true);
				Poco::Timestamp deacTime;
				Poco::Timestamp::TimeDiff timediff = workNf->schedule->duration * Poco::Timestamp::resolution() / 1000;
				deacTime += timediff;
				Poco::DateTime deacLocal(deacTime);
				deacLocal.makeLocal(Poco::Timezone::tzd());
				this->logVerbose(this->ID() + ": Scheduled deactivation time for node " + deacSchedule->nodeName + " is at: " + 
						Poco::DateTimeFormatter::format(deacLocal, this->opdid->timestampFormat)
						+ "; in " + this->to_string(timediff / 1000000) + " second(s)");
				// add with the specified deactivation time
				this->addNotification(notification, deacTime);
			}

			// set the output ports' state
			int8_t outputLine = -1;	// assume: toggle
			if (workNf->schedule->action == SET_HIGH)
				outputLine = (workNf->deactivate ? 0 : 1);
			if (workNf->schedule->action == SET_LOW)
				outputLine = (workNf->deactivate ? 1 : 0);

			DigitalPortList::iterator it = this->outputPorts.begin();
			while (it != this->outputPorts.end()) {
				try {
					// toggle?
					if (outputLine < 0) {
						// get current output port state
						uint8_t mode;
						uint8_t line;
						(*it)->getState(&mode, &line);
						// set new state
						outputLine = (line == 1 ? 0 : 1);
					}

					// set output line
					(*it)->setLine(outputLine);
				} catch (Poco::Exception &e) {
					this->opdid->logNormal(std::string(this->getID()) + ": Error setting output port state: " + (*it)->getID() + ": " + e.message());
				}
				it++;
			}
		} catch (Poco::Exception &e) {
			this->opdid->logNormal(std::string(this->getID()) + ": Error processing timer schedule: " + e.message());
		}
	}

	// determine next scheduled time text
	this->nextOccurrenceStr = "";

	if (this->line == 1) {
		// go through schedules
		Poco::Timestamp ts = Poco::Timestamp::TIMEVAL_MAX;
		Poco::Timestamp now;
		ScheduleList::iterator it = this->schedules.begin();
		// select schedule with the earliest nextEvent timestamp
		while (it != this->schedules.end()) {
			if (((*it).nextEvent > now) && ((*it).nextEvent < ts)) {
				ts = (*it).nextEvent;
			}
			it++;
		}

		if (ts < Poco::Timestamp::TIMEVAL_MAX) {
			// calculate extended port info text
			Poco::LocalDateTime ldt(ts);
			this->nextOccurrenceStr = Poco::DateTimeFormatter::format(ldt, this->timestampFormat);
		}
	}

	return OPDI_STATUS_OK;
}

void OPDID_TimerPort::setLine(uint8_t line) {
	bool wasLow = (this->line == 0);

	OPDI_DigitalPort::setLine(line);

	// set to Low?
	if (this->line == 0) {
		if (!wasLow) {
			// clear all schedules
			this->queue.clear();
		}
	}

	// set to High?
	if (this->line == 1) {
		if (wasLow) {
			// recalculate all schedules
			for (ScheduleList::iterator it = this->schedules.begin(); it != this->schedules.end(); it++) {
				Schedule *schedule = &*it;
				// calculate
				Poco::Timestamp nextOccurrence = this->calculateNextOccurrence(schedule);
				if (nextOccurrence > Poco::Timestamp()) {
					// add with the specified occurrence time
					this->addNotification(new ScheduleNotification(schedule, false), nextOccurrence);
				} else {
					this->logVerbose(this->ID() + ": Next scheduled time for " + schedule->nodeName + " could not be determined");
				}
			}
		}
	}

	this->refreshRequired = true;
}

std::string OPDID_TimerPort::getExtendedState(void) {
	std::string result;
	if (this->line != 1) {
		result = this->deactivatedText;
	} else {
		if (this->nextOccurrenceStr == "")
			result = this->notScheduledText;
		else
			result = this->nextOccurrenceStr;
	}
	return "text=" + this->escapeKeyValueText(result);
}
