/**
 * \file
 * \brief Main application file
 * \author Erich Styger, erich.styger@hslu.ch
 *
 * This provides the main application entry point.
 */

#include "Platform.h"
#include "Application.h"
#include "Event.h"
#include "LED.h"
#include "WAIT1.h"
#include "CS1.h"
#include "Keys.h"
#include "KeyDebounce.h"
#include "KIN1.h"
#if PL_CONFIG_HAS_SHELL
  #include "CLS1.h"
  #include "Shell.h"
#endif
#if PL_CONFIG_HAS_BUZZER
  #include "Buzzer.h"
#endif
#if PL_CONFIG_HAS_RTOS
  #include "FRTOS1.h"
  #include "RTOS.h"
#endif
#if PL_CONFIG_HAS_QUADRATURE
  #include "Q4CLeft.h"
  #include "Q4CRight.h"
#endif
#if PL_CONFIG_HAS_MOTOR
  #include "Motor.h"
#endif
#if PL_CONFIG_BOARD_IS_ROBO_V2
  #include "PORT_PDD.h"
#endif
#if PL_CONFIG_HAS_LINE_FOLLOW
  #include "LineFollow.h"
#endif
#if PL_CONFIG_HAS_LCD_MENU
  #include "LCD.h"
#endif
#if PL_CONFIG_HAS_SUMO
  #include "Sumo.h"
#endif

#if PL_CONFIG_HAS_EVENTS
void APP_EventHandler(EVNT_Handle event) {
  /*! \todo handle events */
  switch(event) {
  case EVNT_STARTUP:
#if PL_CONFIG_HAS_BUZZER
	  (void)BUZ_PlayTune(BUZ_TUNE_WELCOME);
#endif
    break;
//  case EVNT_LED_HEARTBEAT:
//	  LEDPin1_NegVal();
//		#if PL_LOCAL_CONFIG_BOARD_IS_ROBO
//		  LEDPin2_NegVal();;
//		#endif
//	break;
  case EVNT_SW1_PRESSED:
	  CLS1_SendStr("Button 1 pressed\n", CLS1_GetStdio()->stdOut);
#if PL_CONFIG_HAS_BUZZER
	  (void)BUZ_PlayTune(BUZ_TUNE_BUTTON);
#endif
#if PL_CONFIG_HAS_SUMO
    if (!SUMO_IsRunningSumo()) {		//Start oder Stop des Robots
    	SUMO_StartSumo();
    }
    else {
    	SUMO_StopSumo();
    }
#endif
    break;
  case EVNT_SW1_LPRESSED:
	  CLS1_SendStr("Button 1 LongPressed\n", CLS1_GetStdio()->stdOut);
	  break;
  case EVNT_SW1_RELEASED:
	  CLS1_SendStr("Button 1 Released\n", CLS1_GetStdio()->stdOut);
	  break;
#if PL_LOCAL_CONFIG_BOARD_IS_REMOTE
  case EVNT_SW2_PRESSED:
	  CLS1_SendStr("Button 2 pressed\n", CLS1_GetStdio()->stdOut);
    break;
  case EVNT_SW3_PRESSED:
	  CLS1_SendStr("Button 3 pressed\n", CLS1_GetStdio()->stdOut);
    break;
  case EVNT_SW4_PRESSED:
	  CLS1_SendStr("Button 4 pressed\n", CLS1_GetStdio()->stdOut);
    break;
  case EVNT_SW5_PRESSED:
	  CLS1_SendStr("Button 5 pressed\n", CLS1_GetStdio()->stdOut);
    break;
  case EVNT_SW6_PRESSED:
	  CLS1_SendStr("Button 6 pressed\n", CLS1_GetStdio()->stdOut);
    break;
  case EVNT_SW7_PRESSED:
	  CLS1_SendStr("Button 7 pressed\n", CLS1_GetStdio()->stdOut);
    break;
#endif
#if PL_LOCAL_CONFIG_HAS_LCD
  case LCD_BTN_LEFT:

	  break;
#endif
  default:
    break;
   } /* switch */
}
#endif /* PL_CONFIG_HAS_EVENTS */

static const KIN1_UID RoboIDs[] = {
  /* 0: L20, V2 */ {{0x00,0x03,0x00,0x00,0x67,0xCD,0xB7,0x21,0x4E,0x45,0x32,0x15,0x30,0x02,0x00,0x13}},
  /* 1: L21, V2 */ {{0x00,0x05,0x00,0x00,0x4E,0x45,0xB7,0x21,0x4E,0x45,0x32,0x15,0x30,0x02,0x00,0x13}},
  /* 2: L4, V1  */ {{0x00,0x0B,0xFF,0xFF,0x4E,0x45,0xFF,0xFF,0x4E,0x45,0x27,0x99,0x10,0x02,0x00,0x24}}, /* revert right motor */
  /* 3: L23, V2 */ {{0x00,0x0A,0x00,0x00,0x67,0xCD,0xB8,0x21,0x4E,0x45,0x32,0x15,0x30,0x02,0x00,0x13}}, /* revert left & right motor */
  /* 4: L11, V2 */ {{0x00,0x19,0x00,0x00,0x67,0xCD,0xB9,0x11,0x4E,0x45,0x32,0x15,0x30,0x02,0x00,0x13}}, /* revert right encoder, possible damaged motor driver? */
  /* 5: L4, V1 */  {{0x00,0x0B,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x4E,0x45,0x27,0x99,0x10,0x02,0x00,0x24}},
  /* 6: L6, V1 */  {{0x00,0x17,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x4E,0x45,0x27,0x99,0x10,0x02,0x00,0x06}},
  /* 1: L1, V1 */  {{0x00,0x19,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x4E,0x45,0x27,0x99,0x10,0x02,0x00,0x25}},
};

static void APP_AdoptToHardware(void) {
  KIN1_UID id;
  uint8_t res;

  res = KIN1_UIDGet(&id);
  if (res!=ERR_OK) {
    for(;;); /* error */
  }
#if PL_CONFIG_HAS_MOTOR
  if (KIN1_UIDSame(&id, &RoboIDs[2])) { /* L4 */
    MOT_Invert(MOT_GetMotorHandle(MOT_MOTOR_LEFT), TRUE); /* revert left motor */
    (void)Q4CLeft_SwapPins(TRUE);
    (void)Q4CRight_SwapPins(TRUE);
  } else if (KIN1_UIDSame(&id, &RoboIDs[0])) { /* L20 */
    (void)Q4CRight_SwapPins(TRUE);
    MOT_Invert(MOT_GetMotorHandle(MOT_MOTOR_LEFT), TRUE); /* revert left motor */
    MOT_Invert(MOT_GetMotorHandle(MOT_MOTOR_RIGHT), TRUE); /* revert left motor */
  } else if (KIN1_UIDSame(&id, &RoboIDs[3])) { /* L23 */
    (void)Q4CRight_SwapPins(TRUE);
    MOT_Invert(MOT_GetMotorHandle(MOT_MOTOR_LEFT), TRUE); /* revert left motor */
    MOT_Invert(MOT_GetMotorHandle(MOT_MOTOR_RIGHT), TRUE); /* revert left motor */
  } else if (KIN1_UIDSame(&id, &RoboIDs[4])) { /* L11 */
    (void)Q4CRight_SwapPins(TRUE);
  } else if (KIN1_UIDSame(&id, &RoboIDs[5])) { /* L4 */
    MOT_Invert(MOT_GetMotorHandle(MOT_MOTOR_LEFT), TRUE); /* revert left motor */
    (void)Q4CLeft_SwapPins(TRUE);
    (void)Q4CRight_SwapPins(TRUE);

    // Robot L1 und L6 of Team Herger & Kissling
  } else if (KIN1_UIDSame(&id, &RoboIDs[6])) { /* L6 */
      MOT_Invert(MOT_GetMotorHandle(MOT_MOTOR_LEFT), TRUE); /* revert left motor */
      MOT_Invert(MOT_GetMotorHandle(MOT_MOTOR_RIGHT), FALSE); /* revert right motor */
      (void)Q4CLeft_SwapPins(TRUE);
      (void)Q4CRight_SwapPins(TRUE);

  } else if (KIN1_UIDSame(&id, &RoboIDs[7])) { /* L1 */
	    MOT_Invert(MOT_GetMotorHandle(MOT_MOTOR_LEFT), TRUE); /* revert left motor */
	    MOT_Invert(MOT_GetMotorHandle(MOT_MOTOR_RIGHT), FALSE); /* revert right motor */
	    (void)Q4CLeft_SwapPins(TRUE);
	    (void)Q4CRight_SwapPins(TRUE);
  }
#endif
#if PL_CONFIG_HAS_QUADRATURE && PL_CONFIG_BOARD_IS_ROBO_V2
  /* pull-ups for Quadrature Encoder Pins */
  PORT_PDD_SetPinPullSelect(PORTC_BASE_PTR, 10, PORT_PDD_PULL_UP);
  PORT_PDD_SetPinPullEnable(PORTC_BASE_PTR, 10, PORT_PDD_PULL_ENABLE);
  PORT_PDD_SetPinPullSelect(PORTC_BASE_PTR, 11, PORT_PDD_PULL_UP);
  PORT_PDD_SetPinPullEnable(PORTC_BASE_PTR, 11, PORT_PDD_PULL_ENABLE);
  PORT_PDD_SetPinPullSelect(PORTC_BASE_PTR, 16, PORT_PDD_PULL_UP);
  PORT_PDD_SetPinPullEnable(PORTC_BASE_PTR, 16, PORT_PDD_PULL_ENABLE);
  PORT_PDD_SetPinPullSelect(PORTC_BASE_PTR, 17, PORT_PDD_PULL_UP);
  PORT_PDD_SetPinPullEnable(PORTC_BASE_PTR, 17, PORT_PDD_PULL_ENABLE);
#endif
}

#include "CLS1.h"

void APP_Start(void) {
#if PL_CONFIG_HAS_RTOS
#if configUSE_TRACE_HOOKS /* FreeRTOS using Percepio Trace */
  #if TRC_CFG_RECORDER_MODE==TRC_RECORDER_MODE_SNAPSHOT
    vTraceEnable(TRC_START); /* start streaming */
  #elif TRC_CFG_RECORDER_MODE==TRC_RECORDER_MODE_STREAMING
  //  vTraceEnable(TRC_INIT); /* Streaming trace, start streaming */
  #endif
#endif /* configUSE_TRACE_HOOKS */
#endif
  PL_Init();
#if PL_CONFIG_HAS_EVENTS
  EVNT_SetEvent(EVNT_STARTUP);
#endif
#if PL_CONFIG_HAS_SHELL && CLS1_DEFAULT_SERIAL
  CLS1_SendStr((uint8_t*)"Hello World!\r\n", CLS1_GetStdio()->stdOut);
#endif
  APP_AdoptToHardware();
#if PL_CONFIG_HAS_RTOS
  vTaskStartScheduler(); /* start the RTOS, create the IDLE task and run my tasks (if any) */
  /* does usually not return! */
#else

#if PL_CONFIG_HAS_SHELL
  (void)CLS1_SetStdio(RTT1_GetStdio());		//W�hlt die Default Serial Schnittstelle aus
#endif

  __asm volatile("cpsie i"); /* enable interrupts */
  for(;;) {
	/* check local platform configuration */
	#if PL_LOCAL_CONFIG_BOARD_IS_ROBO

	  //#Lab 15 Keys
	  //WAIT1_Waitms(50);		//polling time
	  //Polling Key
	  //KEY_Scan();

	  //#Lab 16 Console
	  //WAIT1_Waitms(100);
	  //CLS1_SendStr("Hello World\n", CLS1_GetStdio()->stdOut);
	  //KEY_Scan();

	#elif PL_LOCAL_CONFIG_BOARD_IS_REMOTE

	  //#Lab 15 Keys
	  //WAIT1_Waitms(50);		//polling time
	  //Polling Key
	  //KEY_Scan();

	  //#Lab 16 Console
	  //WAIT1_Waitms(100);
	  //CLS1_SendStr("Hello World\n", CLS1_GetStdio()->stdOut);
	  //KEY_Scan();

	  //#Lab 18 Debouncing
	  // Polling Key
	  KEYDBNC_Process(); // Falls ein Button gedr�ckt wird, wird mit dieser Funktion entprellt

	#else
	  #error "One board type has to be defined in Platform_Local.h!"
	#endif

    EVNT_HandleEvent(APP_EventHandler, TRUE);	//Eventhandler aufrufen
  }
#endif
}
