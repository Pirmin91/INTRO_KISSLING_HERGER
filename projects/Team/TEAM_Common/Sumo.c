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

typedef enum {
  SUMO_STATE_IDLE,
  SUMO_STATE_DRIVING,
  SUMO_STATE_TURNING,
} SUMO_State_t;

static SUMO_State_t sumoState = SUMO_STATE_IDLE;
static TaskHandle_t sumoTaskHndl;

/* direct task notification bits */
#define SUMO_START_SUMO (1<<0)  /* start sumo mode */
#define SUMO_STOP_SUMO  (1<<1)  /* stop stop sumo */

bool SUMO_IsRunningSumo(void) {
  return sumoState==SUMO_STATE_DRIVING;
}

void SUMO_StartSumo(void) {
  (void)xTaskNotify(sumoTaskHndl, SUMO_START_SUMO, eSetBits);
}

void SUMO_StopSumo(void) {
  (void)xTaskNotify(sumoTaskHndl, SUMO_STOP_SUMO, eSetBits);
}

void SUMO_StartStopSumo(void) {
  if (SUMO_IsRunningSumo()) {
    (void)xTaskNotify(sumoTaskHndl, SUMO_STOP_SUMO, eSetBits);
  } else {
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
          DRV_SetSpeed(1000, 1000);
          DRV_SetMode(DRV_MODE_SPEED);
          sumoState = SUMO_STATE_DRIVING;
          break; /* handle next state */
        }
        return;
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
        return;
      case SUMO_STATE_TURNING:
        DRV_SetMode(DRV_MODE_STOP);
        TURN_Turn(TURN_RIGHT180, NULL);
        DRV_SetMode(DRV_MODE_SPEED);
        sumoState = SUMO_STATE_DRIVING;
        break; /* handle next state */
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
  if (xTaskCreate(SumoTask, "Sumo", 500/sizeof(StackType_t), NULL, tskIDLE_PRIORITY+2, &sumoTaskHndl) != pdPASS) {
    for(;;){} /* error case only, stay here! */
  }
}

void SUMO_Deinit(void) {
}

#endif /* PL_CONFIG_HAS_SUMO */
