/*
 * Sumo.c
 *
 *  Created on: 16.05.2017
 *      Author: Erich Styger
 */
#include "Platform.h"

#if PL_CONFIG_HAS_SUMO
#include "Sumo.h"
#include "FRTOS1.h"
#include "Drive.h"
#include "Reflectance.h"
#include "Turn.h"
#include "CLS1.h"
#include "Drive.h"
#include "Q4CLeft.h"
#include "Q4CRight.h"
#include "Buzzer.h"
#include "Distance.h"
#include "LED.h"

typedef enum {
  SUMO_STATE_IDLE,
  SUMO_STATE_SHOVEL,	//in diesem Status wird ein Ruck gemacht, damit die Schaufel runterfällt
#if PL_HAS_DISTANCE_SENSOR
  SUMO_STATE_SEARCH_OPPONENT,
  SUMO_STATE_ATTACK_OPPONENT,
#endif
  SUMO_STATE_DRIVING,
  SUMO_STATE_TURNING,
} SUMO_State_t;

static SUMO_State_t sumoState = SUMO_STATE_IDLE;
static TaskHandle_t sumoTaskHndl;
// set drive speed of robot
//static int32_t speed = 5000;  //eher knapp zum Linien erkennen
static int32_t speed = 5000;
// set drive new position speed
static int32_t newPosSpeed = 3000;
// set turning speed while searching opponent
static int32_t turningSpeed = 1750;
// set drive backwards speed after detecting line and before turning 180 degree
static int32_t backwardSpeed = 10000;
// set drive attack speed
static int32_t attackSpeed = 5000;
// difference of ToF Sensor values to decide for attack
static int32_t distanceSensorMM = 100;
// time for a turn in millisec
static int32_t timeForATurnMS = 2000;
static TickType_t timeBegin;
static TickType_t timeEnd;
static bool timeMeasureDone = FALSE;
//counter to change in random mode
static int32_t counterRandomMode = 0;
//
static bool doRandom = FALSE;
//
static int32_t count10MS = 30;

/* direct task notification bits */
#define SUMO_START_SUMO (1<<0)  /* start sumo mode */
#define SUMO_STOP_SUMO  (1<<1)  /* stop stop sumo */

bool SUMO_IsRunningSumo(void) {
#if PL_CONFIG_HAS_DISTANCE
  return (sumoState==SUMO_STATE_DRIVING || sumoState==SUMO_STATE_SEARCH_OPPONENT || sumoState==SUMO_STATE_ATTACK_OPPONENT);
#else
  return (sumoState==SUMO_STATE_DRIVING);
#endif
}

void SUMO_StartSumo(void) {
  (void)xTaskNotify(sumoTaskHndl, SUMO_START_SUMO, eSetBits);
}

void SUMO_StopSumo(void) {
  //Random Mode und Counter zurücksetzen
   doRandom = FALSE;
   counterRandomMode = 0;
  (void)xTaskNotify(sumoTaskHndl, SUMO_STOP_SUMO, eSetBits);
}

void SUMO_StartStopSumo(void) {
  if (SUMO_IsRunningSumo()) {
    (void)xTaskNotify(sumoTaskHndl, SUMO_STOP_SUMO, eSetBits);
  } else {
	//Random Mode und Counter zurücksetzen
	 doRandom = FALSE;
	 counterRandomMode = 0;
    (void)xTaskNotify(sumoTaskHndl, SUMO_START_SUMO, eSetBits);
  }
}

static void SumoRun(void) {
  uint32_t notifcationValue;

  (void)xTaskNotifyWait(0UL, SUMO_START_SUMO|SUMO_STOP_SUMO, &notifcationValue, 0); /* check flags */
  for(;;) { /* breaks */
    switch(sumoState) {
      case SUMO_STATE_IDLE:
        if ((notifcationValue&SUMO_START_SUMO) && REF_GetLineKind()==REF_LINE_FULL) {
          for (int i=0; i<5; i++) {
#if PL_CONFIG_HAS_BUZZER
	        (void)BUZ_PlayTune(BUZ_TUNE_BUTTON);		//Ton an Button ausgeben
#endif
	  	    FRTOS1_vTaskDelay(1000/portTICK_PERIOD_MS);	//insgesamt wird etwas über 5 Sekunden gewartet nach Start
          }
          sumoState = SUMO_STATE_SHOVEL;	//beim Start einen Ruck geben
          break; /* handle next state */
        }
        return;
      case SUMO_STATE_SHOVEL:
	    //Ruck geben, damit Schaufel runter fällt
	    DRV_SetMode(DRV_MODE_SPEED);
	    DRV_SetSpeed(-backwardSpeed,-backwardSpeed);
	    FRTOS1_vTaskDelay(100/portTICK_PERIOD_MS);
	    DRV_SetSpeed(backwardSpeed,backwardSpeed);
	    FRTOS1_vTaskDelay(100/portTICK_PERIOD_MS);
	    //DRV_SetSpeed(0,0);		//Anhalten
	    DRV_SetMode(DRV_MODE_STOP);
	    FRTOS1_vTaskDelay(100/portTICK_PERIOD_MS);
#if PL_HAS_DISTANCE_SENSOR
          //sumoState = SUMO_STATE_SEARCH_OPPONENT;	//in Status "Gegner suchen" wechseln
          DRV_SetSpeed(speed, speed);
          DRV_SetMode(DRV_MODE_SPEED);
	      sumoState = SUMO_STATE_DRIVING;
          break; /* handle next state */
#else
          DRV_SetSpeed(speed, speed);
          DRV_SetMode(DRV_MODE_SPEED);
          sumoState = SUMO_STATE_DRIVING;
          break; /* handle next state */
#endif
      return;
#if PL_HAS_DISTANCE_SENSOR
      case SUMO_STATE_SEARCH_OPPONENT:
    	  //Logik einbauen dass Gegner gesucht wird und anschliessend in State "Gegner attackieren" wechseln
          if (notifcationValue&SUMO_STOP_SUMO) {
             DRV_SetMode(DRV_MODE_STOP);
             sumoState = SUMO_STATE_IDLE;
             break; /* handle next state */
          }
          if (counterRandomMode >= 3) {
        	  doRandom = TRUE;		//Nur noch in Random Mode fahren, da Gegner Roboter nicht gefunden
        	  sumoState = SUMO_STATE_DRIVING;
        	  break; /* handle next state */
          }
          //Wenn Gegner gefunden --> attackieren! / ansonsten drehen und suchen
          if (((DIST_GetDistance(DIST_SENSOR_RIGHT)-DIST_GetDistance(DIST_SENSOR_LEFT))<=distanceSensorMM ||
          (DIST_GetDistance(DIST_SENSOR_LEFT)-DIST_GetDistance(DIST_SENSOR_RIGHT))<=distanceSensorMM) &&
          DIST_GetDistance(DIST_SENSOR_RIGHT)!=-1 && DIST_GetDistance(DIST_SENSOR_LEFT)!=-1) {
        	 LEDPin1_NegVal();	//Anzeigen dass Gegner erkannt wurde
        	 counterRandomMode = 0;	//Counter zurücksetzen da Gegner gefunden
             //Gegner attackieren
             DRV_SetMode(DRV_MODE_SPEED);
             DRV_SetSpeed(attackSpeed,attackSpeed);
        	 sumoState = SUMO_STATE_ATTACK_OPPONENT;
        	 break; /* handle next state */
          }
          else {
        	 //Wenn eine gewisse Zeit nichts gefunden wurde Roboter bewegen
        	 if (!timeMeasureDone) {
        		 timeBegin = xTaskGetTickCount();
        		 timeEnd = xTaskGetTickCount(); //zum initalisieren
        		 timeMeasureDone = TRUE;
        	 }
        	 else {
        		 timeEnd = xTaskGetTickCount();
        	 }
        	 if(((int32_t)(timeEnd - timeBegin)/portTICK_PERIOD_MS)<=timeForATurnMS) {
               //Robot drehen und Gegner suchen
               DRV_SetMode(DRV_MODE_SPEED);
               DRV_SetSpeed(-turningSpeed,turningSpeed);
        	 }
        	 else {
        	   counterRandomMode++;
               DRV_SetMode(DRV_MODE_SPEED);
               DRV_SetSpeed(newPosSpeed,newPosSpeed);
               timeMeasureDone = FALSE;
               for (int y=0; y<count10MS; y++) {
				   if (REF_GetLineKind()!=REF_LINE_FULL) {
					 sumoState = SUMO_STATE_TURNING;
					 break; /* handle next state */
				   }
				   FRTOS1_vTaskDelay(10/portTICK_PERIOD_MS);		//Anzahl Millisekunden warten zum neue Position einzunehmen
               }
        	 }
          }

    	  break; /* handle next state */
    	  return;
      case SUMO_STATE_ATTACK_OPPONENT:
    	  //Logik einbauen dass Gegner attackiert wird mit vollem Tempo, aber weiterhin gecheckt wird, ob
    	  //der Gegner genau vor einem ist, nicht dass man über das Ziel hinausschiesst
          if (notifcationValue&SUMO_STOP_SUMO) {
            DRV_SetMode(DRV_MODE_STOP);
            sumoState = SUMO_STATE_IDLE;
            break; /* handle next state */
          }
          if (REF_GetLineKind()!=REF_LINE_FULL) {
            sumoState = SUMO_STATE_TURNING;
            break; /* handle next state */
          }
    	  break; /* handle next state */
    	  return;
#endif
      case SUMO_STATE_DRIVING:
        if (notifcationValue&SUMO_STOP_SUMO) {
           DRV_SetMode(DRV_MODE_STOP);
           sumoState = SUMO_STATE_IDLE;
           break; /* handle next state */
        }
        if (REF_GetLineKind()!=REF_LINE_FULL) {
          sumoState = SUMO_STATE_TURNING;
          break; /* handle next state */
        }
        break;
        return;
      case SUMO_STATE_TURNING:
        DRV_SetMode(DRV_MODE_STOP);
        // Try to drive backwards
        DRV_SetMode(DRV_MODE_SPEED);
        DRV_SetSpeed(-backwardSpeed,-backwardSpeed);
        vTaskDelay(250/portTICK_PERIOD_MS);
        //Drehen
        TURN_Turn(TURN_RIGHT180, NULL);
#if PL_HAS_DISTANCE_SENSOR
        if (!doRandom) {
          sumoState = SUMO_STATE_SEARCH_OPPONENT;	//in Status "Gegner suchen" wechseln
          break; /* handle next state */
        }
        else {
		  DRV_SetMode(DRV_MODE_SPEED);
		  DRV_SetSpeed(speed, speed);
		  sumoState = SUMO_STATE_DRIVING;
		  break; /* handle next state */
        }
#else
          DRV_SetMode(DRV_MODE_SPEED);
          DRV_SetSpeed(speed, speed);
          sumoState = SUMO_STATE_DRIVING;
          break; /* handle next state */
#endif
        break;
        return;
      default: /* should not happen? */
        return;
    } /* switch */
  } /* for */
}

static void SumoTask(void* param) {
  sumoState = SUMO_STATE_IDLE;
  for(;;) {
    SumoRun();
    FRTOS1_vTaskDelay(10/portTICK_PERIOD_MS);
  }
}

static uint8_t SUMO_PrintHelp(const CLS1_StdIOType *io) {
  CLS1_SendHelpStr("sumo", "Group of sumo commands\r\n", io->stdOut);
  CLS1_SendHelpStr("  help|status", "Print help or status information\r\n", io->stdOut);
  CLS1_SendHelpStr("  start|stop", "Start and stop Sumo mode\r\n", io->stdOut);
  return ERR_OK;
}

/*!
 * \brief Prints the status text to the console
 * \param io StdIO handler
 * \return ERR_OK or failure code
 */
static uint8_t SUMO_PrintStatus(const CLS1_StdIOType *io) {
  CLS1_SendStatusStr("sumo", "\r\n", io->stdOut);
  if (sumoState==SUMO_STATE_IDLE) {
    CLS1_SendStatusStr("  running", "no\r\n", io->stdOut);
  } else {
    CLS1_SendStatusStr("  running", "yes\r\n", io->stdOut);
  }
  return ERR_OK;
}

uint8_t SUMO_ParseCommand(const unsigned char *cmd, bool *handled, const CLS1_StdIOType *io) {
  if (UTIL1_strcmp((char*)cmd, CLS1_CMD_HELP)==0 || UTIL1_strcmp((char*)cmd, "sumo help")==0) {
    *handled = TRUE;
    return SUMO_PrintHelp(io);
  } else if (UTIL1_strcmp((char*)cmd, CLS1_CMD_STATUS)==0 || UTIL1_strcmp((char*)cmd, "sumo status")==0) {
    *handled = TRUE;
    return SUMO_PrintStatus(io);
  } else if (UTIL1_strcmp(cmd, "sumo start")==0) {
    *handled = TRUE;
    SUMO_StartSumo();
  } else if (UTIL1_strcmp(cmd, "sumo stop")==0) {
    *handled = TRUE;
    SUMO_StopSumo();
  }
  return ERR_OK;
}

void SUMO_Init(void) {
  if (xTaskCreate(SumoTask, "Sumo", 700/sizeof(StackType_t), NULL, tskIDLE_PRIORITY+2, &sumoTaskHndl) != pdPASS) {
    for(;;){} /* error case only, stay here! */
  }
}

void SUMO_Deinit(void) {
}

#endif /* PL_CONFIG_HAS_SUMO */
