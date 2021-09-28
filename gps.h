#include <Arduino.h>

#ifndef __GPS_H__
#define __GPS_H__

extern String N_date, N_time,N_lat,N_lng,N_satellites,N_meters;
extern String P_date, P_time,P_lat,P_lng,P_satellites,P_meters;

void GpsSerialInit();
void GetGpsInfoPolling();
void UpdateGpsInfo();
int UpdateGpsData(char* destination);
uint64_t gpsEncodePosition48b();
void GpsListening();
void GpsstopListening();
#endif
