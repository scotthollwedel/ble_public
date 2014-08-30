#ifndef TIME_H
#define TIME_H

#define LF_CLOCK_PERIOD         		 32768
#define HF_CLOCK_PERIOD                  16000000
#define LOOP_PERIOD_IN_MS                100

struct tm
{
    unsigned long sec;
    unsigned long usec;
};

void rtc_init(void);
void timer0_init(void);
void getTime(struct tm * time);
long getUptime();
#endif
