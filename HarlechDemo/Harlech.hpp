#ifndef _HARLECH_HPP_
#define _HARLECH_HPP_


#if defined(ARDUINO_ARCH_ESP32)
//#include "analogWrite.h"
#include "ESP32PWM.h"

#else

#include <ESP8266WiFi.h> // This allows the modem to be powered off, saving 40mA!

#endif

#include <map>
#include <vector>
#include <memory>

#ifdef HC_ENABLE_DEBUG_LOG

#define HC_LOG1( A ) Serial.println( A )
#define HC_LOG2( B, A ) Serial.print( B ); Serial.print( ' ' ); HC_LOG1( A )
#define HC_LOG3( C, B, A ) Serial.print( C ); Serial.print( ' ' ); HC_LOG2( B, A )
#define HC_LOG4( D, C, B, A ) Serial.print( D ); Serial.print( ' ' ); HC_LOG3( C, B, A )
#define HC_LOG5( E, D, C, B, A ) Serial.print( E ); Serial.print( ' ' ); HC_LOG4( D, C, B, A )

#else

#define HC_LOG1( A )
#define HC_LOG2( B, A )
#define HC_LOG3( C, B, A )
#define HC_LOG4( D, C, B, A )
#define HC_LOG5( E, D, C, B, A )

#endif


#endif //  _HARLECH_HPP_
