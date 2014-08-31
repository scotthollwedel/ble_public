#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "schedule.h"
#include "ble.h"
#include "serial.h"
#include "time.h"

typedef struct {
    int transitionTime;
    bool beaconEnabled;
    int beaconPower;
    int beaconPeriod;
} ScheduleRecord;


ScheduleRecord schedule[64];
int scheduleLength = 0;

ScheduleRecord currentSchedule;

void addScheduleRecord(const int transitionTime, const bool beaconEnabled, const int beaconPower, const int beaconPeriod) {
    schedule[scheduleLength].transitionTime = transitionTime;
    schedule[scheduleLength].beaconEnabled = beaconEnabled;
    schedule[scheduleLength].beaconPower = beaconPower;
    schedule[scheduleLength].beaconPeriod = beaconPeriod;
    scheduleLength++;
}

void resetSchedule() {
    scheduleLength = 0;
}

bool isBeaconEnabled() {
    if(scheduleLength == 0)
        return false;
    else
        return currentSchedule.beaconEnabled;
}

void checkSchedule()
{
    debug_print("Check schedule");
    /* Handle schedule changes here */
    int scheduleTime = getSystemTime();
    int tempTransitionTime = 0;
    ScheduleRecord tempCurrentSchedule;
    /* Find max transition time of records that are less than the schedule time */
    for(int i = 0; i < scheduleLength; i++) {
        if(schedule[i].transitionTime <= scheduleTime && schedule[i].transitionTime > tempTransitionTime) {
            tempTransitionTime = schedule[i].transitionTime;
            memcpy(&tempCurrentSchedule, &schedule[i], sizeof(ScheduleRecord));
        }
    }
    /* Only change record if we need to */
    if(scheduleLength > 0 && memcmp(&currentSchedule, &tempCurrentSchedule, sizeof(ScheduleRecord)) != 0) {
        sprintf(serialBuffer, "Switch to new schedule: %d %d %d %d", tempCurrentSchedule.transitionTime, 
                                                                  tempCurrentSchedule.beaconEnabled,
                                                                  tempCurrentSchedule.beaconPower, 
                                                                  tempCurrentSchedule.beaconPeriod);
        debug_print(serialBuffer);
        memcpy(&currentSchedule, &tempCurrentSchedule, sizeof(ScheduleRecord));
        if(true == currentSchedule.beaconEnabled) {
            setBLEBeaconTransmitPeriod(currentSchedule.beaconPeriod);
            setBLEBeaconTransmitPower(currentSchedule.beaconPower);
        }
    }
}
