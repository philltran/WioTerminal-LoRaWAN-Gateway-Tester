#include "config.h"
#include "gps.h"
#include "testeur.h"
#include "SoftwareSerial1.h"
#include <TinyGPS++.h>

String N_date, N_time,N_satellites= "0";
String P_date, P_time,P_lat,P_lng,P_satellites,P_meters;
String N_lat = "0:0:0.00";
String N_lng = "0:0:0.00";
String N_meters = "0.00";
static double Lat,Lng,Meters,Satellites;
uint32_t hdop;
SoftwareSerial softSerial1(3,2);

TinyGPSPlus gps;
TinyGPSCustom ExtLat(gps, "GPGGA", 3);  //N for Latitude
TinyGPSCustom ExtLng(gps, "GPGGA", 5);  //E for Longitude


void GpsSerialInit()
{
    softSerial1.begin(9600);
    NVIC_DisableIRQ(EIC_4_IRQn);
    NVIC_ClearPendingIRQ(EIC_4_IRQn);
    NVIC_SetPriority(EIC_4_IRQn, 1);
    NVIC_EnableIRQ(EIC_4_IRQn);    
    NVIC_DisableIRQ(EIC_7_IRQn);
    NVIC_ClearPendingIRQ(EIC_7_IRQn);
    NVIC_SetPriority(EIC_7_IRQn, 1);
    NVIC_EnableIRQ(EIC_7_IRQn);
    softSerial1.listen();
}

void GetGpsInfoPolling(){
  while (softSerial1.available() > 0) {
    char c = softSerial1.read();
    gps.encode(c);
  } 
}
void GpsListening(){
  softSerial1.listen();
}
void GpsstopListening(){
  softSerial1.stopListening();
}

void UpdateGpsInfo(){
  if (gps.location.isUpdated() && gps.location.isValid()) 
  {
    double lat0 = gps.location.lat();
    double lat1 = (lat0 -int(lat0))*60;
    double lat2 = (lat1 - int(lat1))*60;

    N_lat = String(int(lat0))+':' + String(int(lat1))+':'+String(lat2) + ' ' + String(ExtLat.value());
    
    double lng0 = gps.location.lng();
    double lng1 = (lng0 -int(lng0))*60;
    double lng2 = (lng1 - int(lng1))*60;

    N_lng = String(int(lng0))+':' + String(int(lng1))+':'+String(lng2) + ' ' + String(ExtLng.value());
    Lat = lat0*10000000;
    Lng = lng0*10000000;
  }
  if(gps.satellites.isUpdated() && gps.satellites.isValid())
  {
    N_satellites = String(gps.satellites.value());
    Satellites = gps.satellites.value();
  }
  if(gps.altitude.isUpdated() && gps.altitude.isValid())
  {
    N_meters = String(gps.altitude.meters());
    Meters = gps.altitude.meters();
  }
  if (gps.date.isUpdated() && gps.date.isValid()) 
  {
    int y = gps.date.year();
    int m = gps.date.month();
    int d = gps.date.day(); 
    N_date = String(y)+'/'+('0'+String(m)).substring(('0'+String(m)).length()-2)+'/'+('0'+String(d)).substring(('0'+String(d)).length()-2);    
  }
  if (gps.time.isUpdated() && gps.time.isValid()) 
  {
    int h = gps.time.hour();
    int m = gps.time.minute();
    int s = gps.time.second(); 
    N_time = ('0'+String(h)).substring(('0'+String(h)).length()-2)+':'+('0'+String(m)).substring(('0'+String(m)).length()-2)+':'+('0'+String(s)).substring(('0'+String(s)).length()-2);    
  }
  if (gps.hdop.isUpdated())
  {
    hdop = gps.hdop.hdop()*100;
  }
}
int UpdateGpsData(char* destination){
  char GpsData[10];
  uint64_t pos = gpsEncodePosition48b();
  GpsData[0] = (pos >> 40) & 0xFF;
  GpsData[1] = (pos >> 32) & 0xFF;
  GpsData[2] = (pos >> 24) & 0xFF;
  GpsData[3] = (pos >> 16) & 0xFF;
  GpsData[4] = (pos >>  8) & 0xFF;
  GpsData[5] = (pos      ) & 0xFF;
  GpsData[6] = ((int)(Meters+1000) >> 8) & 0xFF;
  GpsData[7] = ((int)(Meters+1000)     ) & 0xFF;
  GpsData[8] = (uint8_t)(hdop / 10);
  GpsData[9] = Satellites;
  sprintf(destination, "\"%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\"\r\n", GpsData[0],GpsData[1],GpsData[2],GpsData[3],GpsData[4],GpsData[5],GpsData[6],GpsData[7],GpsData[8],GpsData[9]);
}

/**
 * Compact encoding of the current position
 * The result is stored in the **output** uint64_t variable
 * the result is stored in 0x0000_FFFF_FFFF_FFFF
 * See https://www.disk91.com/2015/technology/sigfox/telecom-design-sdk-decode-gps-frame/
 *  for encoding detail
 * Basically
 * 444444443333333333222222222211111111110000000000
 * 765432109876543210987654321098765432109876543210
 * X                                                - lng Sign 1=-
 *  X                                               - lat Sign 1=-
 *   XXXXXXXXXXXXXXXXXXXXXXX                        - 23b Latitude
 *                          XXXXXXXXXXXXXXXXXXXXXXX - 23b Longitude
 *
 *  division by 215 for longitude is to get 180*10M to fit in 2^23b
 *  substraction of 107 is 0.5 * 215 to round the value and not always be floored.
 */
uint64_t gpsEncodePosition48b() {

  uint64_t t = 0;
  uint64_t l = 0;
  if ( Lng < 0 ) {
    t |= 0x800000000000L;
    l = -Lng;
  } else {
    l = Lng;
  }
  if ( l/10000000 >= 180  ) {
    l = 8372093;
  } else {
    if ( l < 107 ) {
      l = 0;
    } else {
      l = (l - 107) / 215;
    }
  }
  t |= (l & 0x7FFFFF );

  if ( Lat < 0 ) {
    t |= 0x400000000000L;
    l = -Lat;
  } else {
    l = Lat;
  }
  if ( l/10000000 >= 90  ) {
    l = 8333333;
  } else {
    if ( l < 53 ) {
      l = 0;
    } else {
      l = (l - 53) / 108;
    }
  }
  t |= (l << 23) & 0x3FFFFF800000;

  return t;
}