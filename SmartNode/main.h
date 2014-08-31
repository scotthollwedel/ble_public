#ifndef MAIN_H
#define MAIN_H
#include <stdbool.h>
void setReportStatus();
char * getWifiMac();
bool parseJSONObject(char * buffer, int size);
#endif
