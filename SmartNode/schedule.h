#ifndef SCHEDULE_H
#define SCHEDULE_H
void checkSchedule();
bool isBeaconEnabled();
void addScheduleRecord(const int transitionTime, const bool beaconEnabled, const int beaconPower, const int beaconPeriod);
void resetSchedule();
#endif
