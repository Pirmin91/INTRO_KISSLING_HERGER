/**
 * \file
 * \brief Event driver implementation.
 * \author Erich Styger, erich.styger@hslu.ch
 *
 * This module implements a generic event driver. We are using numbered events starting with zero.
 * EVNT_HandleEvent() can be used to process the pending events. Note that the event with the number zero
 * has the highest priority and will be handled first.
 * \todo Make this module reentrant and thread safe!
 */

#include "Platform.h"
#if PL_CONFIG_HAS_EVENTS
#include "Event.h" /* our own interface */
#include "CS1.h"

typedef uint8_t EVNT_MemUnit; /*!< memory unit used to store events flags */
#define EVNT_MEM_UNIT_NOF_BITS  (sizeof(EVNT_MemUnit)*8u)		//sizeof() gibt Byte zur�ck
  /*!< number of bits in memory unit */

static EVNT_MemUnit EVNT_Events[((EVNT_NOF_EVENTS-1)/EVNT_MEM_UNIT_NOF_BITS)+1]; /*!< Bit set of events */		//Wenn NOF Events gr�sser 32, dann wird ein Array mit 2 Stellen � 32Bits erstellt usw. --> wenn kleiner nur 1 � 32Bits

#define SET_EVENT(event) \
  EVNT_Events[(event)/EVNT_MEM_UNIT_NOF_BITS] |= (1u<<(EVNT_MEM_UNIT_NOF_BITS-1))>>(((event)%EVNT_MEM_UNIT_NOF_BITS)) /*!< Set the event */
#define CLR_EVENT(event) \
  EVNT_Events[(event)/EVNT_MEM_UNIT_NOF_BITS] &= ~((1u<<(EVNT_MEM_UNIT_NOF_BITS-1))>>(((event)%EVNT_MEM_UNIT_NOF_BITS))) /*!< Clear the event */
#define GET_EVENT(event) \
  (EVNT_Events[(event)/EVNT_MEM_UNIT_NOF_BITS]&((1u<<(EVNT_MEM_UNIT_NOF_BITS-1))>>(((event)%EVNT_MEM_UNIT_NOF_BITS)))) /*!< Return TRUE if event is set */

void EVNT_SetEvent(EVNT_Handle event) {
	/*! \todo Make it reentrant */
	  CS1_CriticalVariable()	//Kein Semikolon, da es im #define schon ist
			  	  	  	  	  	//f�r mehr Infos F3 auf Funktion

	  CS1_EnterCritical();
	  SET_EVENT(event);
	  CS1_ExitCritical();
}

void EVNT_ClearEvent(EVNT_Handle event) {
	/*! \todo Make it reentrant */
	  CS1_CriticalVariable()

	  CS1_EnterCritical();
	  CLR_EVENT(event);
	  CS1_ExitCritical();
}

bool EVNT_EventIsSet(EVNT_Handle event) {
  /*! \todo Make it reentrant */
	  //bool res;
	  EVNT_MemUnit res;
	  CS1_CriticalVariable()

	  CS1_EnterCritical();
	  //res = (GET_EVENT(event)!=0); // Variante 1, ist in CriticalSection
	  res = GET_EVENT(event);
	  CS1_ExitCritical();
	  return res !=0;	// Variante 2 (better)
}

bool EVNT_EventIsSetAutoClear(EVNT_Handle event) {
  bool res;
  /*! \todo Make it reentrant */
  CS1_CriticalVariable()
  CS1_EnterCritical();
  res = GET_EVENT(event);
  if (res) {
    CLR_EVENT(event); /* automatically clear event */
  }
  CS1_ExitCritical();
  return res;
}

void EVNT_HandleEvent(void (*callback)(EVNT_Handle), bool clearEvent) {
   /* Handle the one with the highest priority. Zero is the event with the highest priority. */
   EVNT_Handle event;
   /*! \todo Make it reentrant */
   CS1_CriticalVariable()
   CS1_EnterCritical();
   for (event=(EVNT_Handle)0; event<EVNT_NOF_EVENTS; event++) { /* does a test on every event */
     if (GET_EVENT(event)) { /* event present? */
       if (clearEvent) {
         CLR_EVENT(event); /* clear event */
       }
       break; /* get out of loop */
     }
   }
   CS1_ExitCritical();
   if (event != EVNT_NOF_EVENTS) {
     callback(event);
     /* Note: if the callback sets the event, we will get out of the loop.
      * We will catch it by the next iteration.
      */
   }
}

void EVNT_Init(void) {
  uint8_t i;

  i = 0;
  do {
    EVNT_Events[i] = 0; /* initialize data structure */
    i++;
  } while(i<sizeof(EVNT_Events)/sizeof(EVNT_Events[0]));
}

void EVNT_Deinit(void) {
  /* nothing needed */
}

#endif /* PL_HAS_EVENTS */
