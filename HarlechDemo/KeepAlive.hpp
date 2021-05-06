#ifndef _KEEPALIVE_HPP_
#define _KEEPALIVE_HPP_

#include "Harlech.hpp"

// This sets up the USBBank Keep Alive Functionality. By default, this is set to 100mA of additional current, for 1S every 10.
#include <Ticker.h>  //Ticker Library

class KeepAlivePinger {
public:
  Ticker USBBank;
  int USBBankState;
  const int kaTimeOn = 1; // Time to switch the keep alive circuit on.
  const int kaTimeOff = 8; // Time to switch the keep alive circuit off for.


  KeepAlivePinger() : USBBankState(0) {}

  void scheduleNext( int stateTime = 1 ) {
    USBBank.attach(stateTime,std::bind(&KeepAlivePinger::keepAliveChangeState, this) );
  }
      
  void keepAliveChangeState()
  {
    //=======================================================================
    // This is a subroutine that switches the keepAlive pin on/off to switch on a transistor that pulls current to ground, through a 47Ohm resistor
    // PLEASE NOTE: You cannot leave this on for a long long time, as the resistor will begin to heat up beyond it's rating!
    int pinState = USBBankState ? HIGH : LOW;
    int stateTime = USBBankState ? kaTimeOff : kaTimeOn;
    USBBankState = !USBBankState;

    HC_LOG3( "keep alive :", pinState, stateTime );
    digitalWrite(HPC_KEEP_ALIVE, pinState);
    #ifdef HPC_KEEP_ALIVE2
    digitalWrite(HPC_KEEP_ALIVE2, pinState);
    #endif
    scheduleNext(stateTime);
  }
};

#endif //  _KEEPALIVE_HPP_
