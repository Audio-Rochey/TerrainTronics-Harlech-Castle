#ifndef _HARLECH_PIN_CONFIG_HPP_
#define _HARLECH_PIN_CONFIG_HPP_

#define HPC_ESP8266_DUAL 1
#define HPC_ESP8266_PG10 2
#define HPC_ESP8266_PG11 3

#define HPC_ESP326_DUAL 4
#define HPC_ESP32_PG10 5
#define HPC_ESP32_PG11 6


#ifndef HARLECH_PIN_CONFIG
#define HARLECH_PIN_CONFIG HPC_ESP8266_DUAL
#endif

// *****************************************************************************************
#if HARLECH_PIN_CONFIG == HPC_ESP8266_PG10
#define HPC_LATCH           D8
#define HPC_CLOCK           D5
#define HPC_DATA            D6
#define HPC_OE              D4
#define HPC_ADC_IN          A0
#define HPC_KEEP_ALIVE      D0

// *****************************************************************************************
#elif HARLECH_PIN_CONFIG == HPC_ESP8266_PG11
#define HPC_LATCH           D8
#define HPC_CLOCK           D5
#define HPC_DATA            D6
#define HPC_OE              D0
#define HPC_ADC_IN          A0
#define HPC_KEEP_ALIVE      D7

// *****************************************************************************************
#elif HARLECH_PIN_CONFIG == HPC_ESP8266_DUAL
#define HPC_LATCH           D8
#define HPC_CLOCK           D5
#define HPC_DATA            D6
#define HPC_OE              D4
#define HPC_OE2             D7
#define HPC_ADC_IN          A0
#define HPC_KEEP_ALIVE      D3
#define HPC_KEEP_ALIVE2     D0


// *****************************************************************************************
#elif HARLECH_PIN_CONFIG == HPC_ESP32_PG10
#define HPC_LATCH           5
#define HPC_CLOCK           18
#define HPC_DATA            19
#define HPC_OE              16
#define HPC_ADC_IN          ADC2
#define HPC_KEEP_ALIVE      17

// *****************************************************************************************
#elif HARLECH_PIN_CONFIG == HPC_ESP32_PG11
#define HPC_LATCH           5
#define HPC_CLOCK           18
#define HPC_DATA            19
#define HPC_OE              23
#define HPC_ADC_IN          ADC2
#define HPC_KEEP_ALIVE      26

// *****************************************************************************************
#elif HARLECH_PIN_CONFIG == HPC_ESP32_DUAL
#define HPC_LATCH           5
#define HPC_CLOCK           18
#define HPC_DATA            19
#define HPC_OE              23
#define HPC_OE2             16
#define HPC_ADC_IN          ADC2
#define HPC_KEEP_ALIVE      26
#define HPC_KEEP_ALIVE2     17

// *****************************************************************************************
#else
#endif



#endif _HARLECH_PIN_CONFIG_HPP_
