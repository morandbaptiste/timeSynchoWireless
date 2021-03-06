/*
 * conf_timeProtocol.h
 *
 * Created: 18/05/2016 19:42:33
 *  Author: MORAND Baptiste
 */ 


#ifndef CONF_TIMEPROTOCOL_H_
#define CONF_TIMEPROTOCOL_H_

/************************************************************************/
/* MACRO                                                                */
/************************************************************************/
//#define MASTERMODE 0
#define SLAVEMODE 2
#define MAX_SLAVE_CLOCK 10
#define TIMESYNC 10
#define TIMOUTSTATE 5000
//#define TIMEDELAYREQUEST 10*TIMESYNC
#define TIME_CORRECTION TIMESYNC*10
#define MOYCORLENGTH 10
#define Coef_Software_Correction 1
#define VALUE_TIMER 1000


#define RTC_FREQ 32000
#endif /* CONF_TIMEPROTOCOL_H_ */