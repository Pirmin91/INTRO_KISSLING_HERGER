/**
 * \file
 * \brief Module to handle the LCD display
 * \author Erich Styger, erich.styger@hslu.ch
 *
 * This module is driving the Nokia LCD Display.
 */

#include "Platform.h"
#if PL_CONFIG_HAS_LCD
#include "LCD.h"
#include "PDC1.h"
#include "GDisp1.h"
#include "GFONT1.h"
#include "FDisp1.h"
#include "Application.h"
#include "UTIL1.h"
#include "LCD_LED.h"
#include "Event.h"
#include "FRTOS1.h"
#if PL_CONFIG_HAS_RADIO  //Lab 28: at the moment there is no Radio-Module
  #include "RApp.h"
#endif
#endif
#include "LCDMenu.h"
/*! \todo Add additional includes as needed */
#include "Snake.h"


/* status variables */
static bool LedBackLightisOn = TRUE;
static bool remoteModeIsOn = FALSE;
static bool requestLCDUpdate = FALSE;

#if PL_CONFIG_HAS_LCD_MENU
typedef enum {
  LCD_MENU_ID_NONE = LCDMENU_ID_NONE, /* special value! */
  LCD_MENU_ID_MAIN,
  LCD_MENU_ID_BACKLIGHT,
  LCD_MENU_ID_NUM_VALUE,
  //eigene Menu IDs
  LCD_MENU_ID_ROBOT,
  LCD_MENU_ID_PID,
  LCD_MENU_ID_P_VALUE,
  LCD_MENU_ID_I_VALUE,
  LCD_MENU_ID_D_VALUE,
  LCD_MENU_ID_W_VALUE,
  //für snake game
  LCD_MENU_ID_GAMES,
  LCD_MENU_ID_SNAKE,
  //für remote befehle
  LCD_MENU_ID_SUMO_START_STOP,
  LCD_MENU_ID_BATTERY_VOLTAGE,
  LCD_MENU_ID_MINT_TOF_SENSOR
} LCD_MenuIDs;

//für remote befehle
static struct {
  struct {
    bool dataValid;
    bool isRunning;
    uint8_t str[sizeof("???????????")+1]; /* used to store menu string, either "Start" or "Stop" */
  } sumo;
  struct {
    bool dataValid;
    uint8_t mm[4]; /* ToF Sensor Values */
    uint8_t str[sizeof("D:??:??:??:??")+1]; /* used to store menu string */
  } tof;
  struct {
    bool dataValid;
    uint16_t centiV;
    uint8_t str[sizeof("Batt: ?.??V")+1]; /* used to store menu string */
  } battVoltage;
} remoteValues;

static LCDMenu_StatusFlags ValueChangeHandler(const struct LCDMenu_MenuItem_ *item, LCDMenu_EventType event, void **dataP) {
  static int value = 0;
  static uint8_t valueBuf[16];
  LCDMenu_StatusFlags flags = LCDMENU_STATUS_FLAGS_NONE;

  (void)item;
  if (event==LCDMENU_EVENT_GET_TEXT) {
    UTIL1_strcpy(valueBuf, sizeof(valueBuf), (uint8_t*)"Val: ");
    UTIL1_strcatNum32s(valueBuf, sizeof(valueBuf), value);
    *dataP = valueBuf;
    flags |= LCDMENU_STATUS_FLAGS_HANDLED|LCDMENU_STATUS_FLAGS_UPDATE_VIEW;
  } else if (event==LCDMENU_EVENT_GET_EDIT_TEXT) {
    UTIL1_strcpy(valueBuf, sizeof(valueBuf), (uint8_t*)"[-] ");
    UTIL1_strcatNum32s(valueBuf, sizeof(valueBuf), value);
    UTIL1_strcat(valueBuf, sizeof(valueBuf), (uint8_t*)" [+]");
    *dataP = valueBuf;
    flags |= LCDMENU_STATUS_FLAGS_HANDLED|LCDMENU_STATUS_FLAGS_UPDATE_VIEW;
  } else if (event==LCDMENU_EVENT_DECREMENT) {
    value--;
    flags |= LCDMENU_STATUS_FLAGS_HANDLED|LCDMENU_STATUS_FLAGS_UPDATE_VIEW;
  } else if (event==LCDMENU_EVENT_INCREMENT) {
    value++;
    flags |= LCDMENU_STATUS_FLAGS_HANDLED|LCDMENU_STATUS_FLAGS_UPDATE_VIEW;
  }
  return flags;
}

static LCDMenu_StatusFlags BackLightMenuHandler(const struct LCDMenu_MenuItem_ *item, LCDMenu_EventType event, void **dataP) {
  LCDMenu_StatusFlags flags = LCDMENU_STATUS_FLAGS_NONE;

  (void)item;
  if (event==LCDMENU_EVENT_GET_TEXT && dataP!=NULL) {
    if (LedBackLightisOn) {
      *dataP = "Backlight is ON";
    } else {
      *dataP = "Backlight is OFF";
    }
    flags |= LCDMENU_STATUS_FLAGS_HANDLED|LCDMENU_STATUS_FLAGS_UPDATE_VIEW;
  } else if (event==LCDMENU_EVENT_ENTER) { /* toggle setting */
    LedBackLightisOn = !LedBackLightisOn;
    flags |= LCDMENU_STATUS_FLAGS_HANDLED|LCDMENU_STATUS_FLAGS_UPDATE_VIEW;
  }
  return flags;
}

//Eigene Handler für P,I,D,W Wert
static LCDMenu_StatusFlags PChangeHandler(const struct LCDMenu_MenuItem_ *item, LCDMenu_EventType event, void **dataP) {
	  /*Kopie von Value Changed Handler*/
	  static int value = 0;   //aktueller P-Wert abfragen /*ToDo*/
	  static uint8_t valueBuf[16];
	  LCDMenu_StatusFlags flags = LCDMENU_STATUS_FLAGS_NONE;

	  (void)item;
	  if (event==LCDMENU_EVENT_GET_TEXT) {
	    UTIL1_strcpy(valueBuf, sizeof(valueBuf), (uint8_t*)"P: ");
	    UTIL1_strcatNum32s(valueBuf, sizeof(valueBuf), value);
	    *dataP = valueBuf;
	    flags |= LCDMENU_STATUS_FLAGS_HANDLED|LCDMENU_STATUS_FLAGS_UPDATE_VIEW;
	  } else if (event==LCDMENU_EVENT_GET_EDIT_TEXT) {
	    UTIL1_strcpy(valueBuf, sizeof(valueBuf), (uint8_t*)"[-] ");
	    UTIL1_strcatNum32s(valueBuf, sizeof(valueBuf), value);
	    UTIL1_strcat(valueBuf, sizeof(valueBuf), (uint8_t*)" [+]");
	    *dataP = valueBuf;
	    flags |= LCDMENU_STATUS_FLAGS_HANDLED|LCDMENU_STATUS_FLAGS_UPDATE_VIEW;
	  } else if (event==LCDMENU_EVENT_DECREMENT) {
			if (value > 0) {
			  value--;
			}
	    flags |= LCDMENU_STATUS_FLAGS_HANDLED|LCDMENU_STATUS_FLAGS_UPDATE_VIEW;
	  } else if (event==LCDMENU_EVENT_INCREMENT) {
	    value++;
	    flags |= LCDMENU_STATUS_FLAGS_HANDLED|LCDMENU_STATUS_FLAGS_UPDATE_VIEW;
	  }
	  /*ToDo*/ //neuer Value an PID übergeben
	  return flags;
}

static LCDMenu_StatusFlags IChangeHandler(const struct LCDMenu_MenuItem_ *item, LCDMenu_EventType event, void **dataP) {
	  /*Kopie von Value Changed Handler*/
	  static int value = 0;
	  static uint8_t valueBuf[16];
	  LCDMenu_StatusFlags flags = LCDMENU_STATUS_FLAGS_NONE;

	  (void)item;
	  if (event==LCDMENU_EVENT_GET_TEXT) {
	    UTIL1_strcpy(valueBuf, sizeof(valueBuf), (uint8_t*)"I: ");
	    UTIL1_strcatNum32s(valueBuf, sizeof(valueBuf), value);
	    *dataP = valueBuf;
	    flags |= LCDMENU_STATUS_FLAGS_HANDLED|LCDMENU_STATUS_FLAGS_UPDATE_VIEW;
	  } else if (event==LCDMENU_EVENT_GET_EDIT_TEXT) {
	    UTIL1_strcpy(valueBuf, sizeof(valueBuf), (uint8_t*)"[-] ");
	    UTIL1_strcatNum32s(valueBuf, sizeof(valueBuf), value);
	    UTIL1_strcat(valueBuf, sizeof(valueBuf), (uint8_t*)" [+]");
	    *dataP = valueBuf;
	    flags |= LCDMENU_STATUS_FLAGS_HANDLED|LCDMENU_STATUS_FLAGS_UPDATE_VIEW;
	  } else if (event==LCDMENU_EVENT_DECREMENT) {
			if (value > 0) {
		      value--;
			}
	    flags |= LCDMENU_STATUS_FLAGS_HANDLED|LCDMENU_STATUS_FLAGS_UPDATE_VIEW;
	  } else if (event==LCDMENU_EVENT_INCREMENT) {
	    value++;
	    flags |= LCDMENU_STATUS_FLAGS_HANDLED|LCDMENU_STATUS_FLAGS_UPDATE_VIEW;
	  }
	  return flags;
}

static LCDMenu_StatusFlags DChangeHandler(const struct LCDMenu_MenuItem_ *item, LCDMenu_EventType event, void **dataP) {
	  /*Kopie von Value Changed Handler*/
	  static int value = 0;
	  static uint8_t valueBuf[16];
	  LCDMenu_StatusFlags flags = LCDMENU_STATUS_FLAGS_NONE;

	  (void)item;
	  if (event==LCDMENU_EVENT_GET_TEXT) {
	    UTIL1_strcpy(valueBuf, sizeof(valueBuf), (uint8_t*)"D: ");
	    UTIL1_strcatNum32s(valueBuf, sizeof(valueBuf), value);
	    *dataP = valueBuf;
	    flags |= LCDMENU_STATUS_FLAGS_HANDLED|LCDMENU_STATUS_FLAGS_UPDATE_VIEW;
	  } else if (event==LCDMENU_EVENT_GET_EDIT_TEXT) {
	    UTIL1_strcpy(valueBuf, sizeof(valueBuf), (uint8_t*)"[-] ");
	    UTIL1_strcatNum32s(valueBuf, sizeof(valueBuf), value);
	    UTIL1_strcat(valueBuf, sizeof(valueBuf), (uint8_t*)" [+]");
	    *dataP = valueBuf;
	    flags |= LCDMENU_STATUS_FLAGS_HANDLED|LCDMENU_STATUS_FLAGS_UPDATE_VIEW;
	  } else if (event==LCDMENU_EVENT_DECREMENT) {
			if (value > 0) {
		      value--;
			}
	    flags |= LCDMENU_STATUS_FLAGS_HANDLED|LCDMENU_STATUS_FLAGS_UPDATE_VIEW;
	  } else if (event==LCDMENU_EVENT_INCREMENT) {
	    value++;
	    flags |= LCDMENU_STATUS_FLAGS_HANDLED|LCDMENU_STATUS_FLAGS_UPDATE_VIEW;
	  }
	  return flags;
}

static LCDMenu_StatusFlags WChangeHandler(const struct LCDMenu_MenuItem_ *item, LCDMenu_EventType event, void **dataP) {
	  /*Kopie von Value Changed Handler*/
	  static int value = 0;
	  static uint8_t valueBuf[16];
	  LCDMenu_StatusFlags flags = LCDMENU_STATUS_FLAGS_NONE;

	  (void)item;
	  if (event==LCDMENU_EVENT_GET_TEXT) {
	    UTIL1_strcpy(valueBuf, sizeof(valueBuf), (uint8_t*)"W: ");
	    UTIL1_strcatNum32s(valueBuf, sizeof(valueBuf), value);
	    *dataP = valueBuf;
	    flags |= LCDMENU_STATUS_FLAGS_HANDLED|LCDMENU_STATUS_FLAGS_UPDATE_VIEW;
	  } else if (event==LCDMENU_EVENT_GET_EDIT_TEXT) {
	    UTIL1_strcpy(valueBuf, sizeof(valueBuf), (uint8_t*)"[-] ");
	    UTIL1_strcatNum32s(valueBuf, sizeof(valueBuf), value);
	    UTIL1_strcat(valueBuf, sizeof(valueBuf), (uint8_t*)" [+]");
	    *dataP = valueBuf;
	    flags |= LCDMENU_STATUS_FLAGS_HANDLED|LCDMENU_STATUS_FLAGS_UPDATE_VIEW;
	  } else if (event==LCDMENU_EVENT_DECREMENT) {
			if (value > 0) {
		     //value--;
			 value-=10;
			}
	    flags |= LCDMENU_STATUS_FLAGS_HANDLED|LCDMENU_STATUS_FLAGS_UPDATE_VIEW;
	  } else if (event==LCDMENU_EVENT_INCREMENT) {
	    //value++;
		value+=10;
	    flags |= LCDMENU_STATUS_FLAGS_HANDLED|LCDMENU_STATUS_FLAGS_UPDATE_VIEW;
	  }
	  return flags;
}

//Handler für Start des snake games
static LCDMenu_StatusFlags SnakeGameHandler(const struct LCDMenu_MenuItem_ *item, LCDMenu_EventType event, void **dataP) {
	//Initialisierung des snake games und erstellung des Tasks
	LCDMenu_StatusFlags flags = LCDMENU_STATUS_FLAGS_NONE;

	if (event==LCDMENU_EVENT_ENTER) {
		if (!snakeGameStarted()) {
			startSnakeGame(); 	//snake game soll gestartet werden, falls noch nicht gemacht
		}
	}
	return flags;
}

//Handler für Remote Operationen
#if PL_CONFIG_HAS_RADIO
static LCDMenu_StatusFlags RobotRemoteMenuHandler(const struct LCDMenu_MenuItem_ *item, LCDMenu_EventType event, void **dataP) {
  LCDMenu_StatusFlags flags = LCDMENU_STATUS_FLAGS_NONE;

  if (event==LCDMENU_EVENT_GET_TEXT && dataP!=NULL) {
    if (item->id==LCD_MENU_ID_MINT_TOF_SENSOR) {
      unsigned int i;

      UTIL1_strcpy(remoteValues.tof.str, sizeof(remoteValues.tof.str), (uint8_t*)"D:");
      for(i=0;i<sizeof(remoteValues.tof.mm);i++) {
        if (remoteValues.tof.dataValid) {
          UTIL1_strcatNum8Hex(remoteValues.tof.str, sizeof(remoteValues.tof.str), remoteValues.tof.mm[i]);
        } else {
          UTIL1_strcat(remoteValues.tof.str, sizeof(remoteValues.tof.str), (uint8_t*)"??");
        }
        if (i<sizeof(remoteValues.tof.mm)-1) {
          UTIL1_chcat(remoteValues.tof.str, sizeof(remoteValues.tof.str), ':');
        }
        *dataP = remoteValues.tof.str;
      }
      flags |= LCDMENU_STATUS_FLAGS_HANDLED|LCDMENU_STATUS_FLAGS_UPDATE_VIEW;
    } else if (item->id==LCD_MENU_ID_SUMO_START_STOP) {
      if (remoteValues.sumo.dataValid) { /* have valid data */
        if (remoteValues.sumo.isRunning) {
          UTIL1_strcpy(remoteValues.sumo.str, sizeof(remoteValues.sumo.str), (uint8_t*)"Start/Stop");
        } else {
          UTIL1_strcpy(remoteValues.sumo.str, sizeof(remoteValues.sumo.str), (uint8_t*)"Start/Stop");
        }
      } else { /* request values */
        (void)RNETA_SendIdValuePairMessage(RAPP_MSG_TYPE_QUERY_VALUE, RAPP_MSG_TYPE_DATA_ID_START_STOP, 0, RNWK_ADDR_BROADCAST, RPHY_PACKET_FLAGS_NONE);
        /* use ??? for now until we get the response */
        UTIL1_strcpy(remoteValues.sumo.str, sizeof(remoteValues.sumo.str), (uint8_t*)"Start/Stop?");
      }
      *dataP = remoteValues.sumo.str;
      flags |= LCDMENU_STATUS_FLAGS_HANDLED|LCDMENU_STATUS_FLAGS_UPDATE_VIEW;
    } else if (item->id==LCD_MENU_ID_BATTERY_VOLTAGE) {
      UTIL1_strcpy(remoteValues.battVoltage.str, sizeof(remoteValues.battVoltage.str), (uint8_t*)"Batt: ");
      if (remoteValues.battVoltage.dataValid) { /* use valid data */
        UTIL1_strcatNum32sDotValue100(remoteValues.battVoltage.str, sizeof(remoteValues.battVoltage.str), remoteValues.battVoltage.centiV);
      } else { /* request value from robot */
        (void)RNETA_SendIdValuePairMessage(RAPP_MSG_TYPE_QUERY_VALUE, RAPP_MSG_TYPE_DATA_ID_BATTERY_V, 0, RNWK_ADDR_BROADCAST, RPHY_PACKET_FLAGS_NONE);
        /* use ??? for now until we get the response */
        UTIL1_strcat(remoteValues.battVoltage.str, sizeof(remoteValues.battVoltage.str), (uint8_t*)"?.??");
      }
      UTIL1_strcat(remoteValues.battVoltage.str, sizeof(remoteValues.battVoltage.str), (uint8_t*)"V");
      *dataP = remoteValues.battVoltage.str;
      flags |= LCDMENU_STATUS_FLAGS_HANDLED|LCDMENU_STATUS_FLAGS_UPDATE_VIEW;
    }
  } else if (event==LCDMENU_EVENT_ENTER || event==LCDMENU_EVENT_RIGHT) { /* force update */
    uint16_t dataID = RAPP_MSG_TYPE_DATA_ID_NONE; /* default value, will be overwritten below */
    uint8_t msgType = 0;
    uint32_t value = 0;

    switch(item->id) {
      case LCD_MENU_ID_SUMO_START_STOP:
        if (event==LCDMENU_EVENT_ENTER) {
          msgType = RAPP_MSG_TYPE_REQUEST_SET_VALUE;
          value = 1; /* start/stop */
        } else {
          msgType = RAPP_MSG_TYPE_QUERY_VALUE;
          value = 0; /* don't care */
        }
        dataID = RAPP_MSG_TYPE_DATA_ID_START_STOP;
        break;
      case LCD_MENU_ID_MINT_TOF_SENSOR:
        remoteValues.tof.dataValid = FALSE;
        msgType = RAPP_MSG_TYPE_QUERY_VALUE;
        dataID = RAPP_MSG_TYPE_DATA_ID_TOF_VALUES;
        value = 0; /* don't care */
        break;
      case LCD_MENU_ID_BATTERY_VOLTAGE:
        remoteValues.battVoltage.dataValid = FALSE;
        msgType = RAPP_MSG_TYPE_QUERY_VALUE;
        dataID = RAPP_MSG_TYPE_DATA_ID_BATTERY_V;
        value = 0; /* don't care */
        break;
    }
    if (dataID!=RAPP_MSG_TYPE_DATA_ID_NONE) { /* request data */
      (void)RNETA_SendIdValuePairMessage(msgType, dataID, value, RNWK_ADDR_BROADCAST, RPHY_PACKET_FLAGS_NONE);
      flags |= LCDMENU_STATUS_FLAGS_HANDLED|LCDMENU_STATUS_FLAGS_UPDATE_VIEW;
    }
  }
  return flags;
}
#endif
static const LCDMenu_MenuItem menus[] =
{/* id,                                     grp, pos,   up,                       down,                             text,           callback                      flags                  */
    {LCD_MENU_ID_MAIN,                        0,   0,   LCD_MENU_ID_NONE,         LCD_MENU_ID_BACKLIGHT,            "General",      NULL,                         LCDMENU_MENU_FLAGS_NONE},
      {LCD_MENU_ID_BACKLIGHT,                 1,   0,   LCD_MENU_ID_MAIN,         LCD_MENU_ID_NONE,                 NULL,           BackLightMenuHandler,         LCDMENU_MENU_FLAGS_NONE},
      {LCD_MENU_ID_NUM_VALUE,                 1,   1,   LCD_MENU_ID_MAIN,         LCD_MENU_ID_NONE,                 NULL,           ValueChangeHandler,           LCDMENU_MENU_FLAGS_EDITABLE},
	 //eigene Menus
	  {LCD_MENU_ID_ROBOT,                     0,  1,   LCD_MENU_ID_NONE,          LCD_MENU_ID_PID,            		"Robot",        NULL,                         LCDMENU_MENU_FLAGS_NONE},
	  {LCD_MENU_ID_PID,                       2,  0,   LCD_MENU_ID_ROBOT,         LCD_MENU_ID_P_VALUE,            	"PID",          NULL,                         LCDMENU_MENU_FLAGS_NONE},
	  //für Remote Befehle
#if PL_CONFIG_HAS_RADIO
      {LCD_MENU_ID_SUMO_START_STOP,           2,   1,   LCD_MENU_ID_ROBOT,        LCD_MENU_ID_NONE,                 NULL,           RobotRemoteMenuHandler,     LCDMENU_MENU_FLAGS_NONE},
      {LCD_MENU_ID_BATTERY_VOLTAGE,           2,   2,   LCD_MENU_ID_ROBOT,        LCD_MENU_ID_NONE,                 NULL,           RobotRemoteMenuHandler,     LCDMENU_MENU_FLAGS_NONE},
      {LCD_MENU_ID_MINT_TOF_SENSOR,           2,   3,   LCD_MENU_ID_ROBOT,        LCD_MENU_ID_NONE,                 NULL,           RobotRemoteMenuHandler,     LCDMENU_MENU_FLAGS_NONE},
#endif
	  //PID und anti reset windup
	  {LCD_MENU_ID_P_VALUE,                   3,  0,   LCD_MENU_ID_PID,           LCD_MENU_ID_NONE,            	    NULL,           PChangeHandler,               LCDMENU_MENU_FLAGS_EDITABLE},
	  {LCD_MENU_ID_I_VALUE,                   3,  1,   LCD_MENU_ID_PID,           LCD_MENU_ID_NONE,            	    NULL,           IChangeHandler,     	      LCDMENU_MENU_FLAGS_EDITABLE},
	  {LCD_MENU_ID_D_VALUE,                   3,  2,   LCD_MENU_ID_PID,           LCD_MENU_ID_NONE,            	    NULL,           DChangeHandler,      		  LCDMENU_MENU_FLAGS_EDITABLE},
	  {LCD_MENU_ID_W_VALUE,                   3,  3,   LCD_MENU_ID_PID,           LCD_MENU_ID_NONE,            	    NULL,           WChangeHandler,     		  LCDMENU_MENU_FLAGS_EDITABLE},
	  //für snake game
	  {LCD_MENU_ID_GAMES,                     0,  2,   LCD_MENU_ID_NONE,          LCD_MENU_ID_SNAKE,            	"Games",        NULL,     		  			  LCDMENU_MENU_FLAGS_NONE},
	  {LCD_MENU_ID_SNAKE,                     4,  0,   LCD_MENU_ID_GAMES,         LCD_MENU_ID_NONE,            	    "Snake",        SnakeGameHandler,     		  LCDMENU_MENU_FLAGS_NONE},
};

// Lab 29: there is no Radio-Module at the moment (reason of comment the function)
#if PL_CONFIG_HAS_RADIO
uint8_t LCD_HandleRemoteRxMessage(RAPP_MSG_Type type, uint8_t size, uint8_t *data, RNWK_ShortAddrType srcAddr, bool *handled, RPHY_PacketDesc *packet) {
  (void)size;
  (void)packet;
  switch(type) {
     default:
      break;
  } /* switch */
  return ERR_OK;
}
#endif




static void ShowTextOnLCD(unsigned char *text) {
  FDisp1_PixelDim x, y;

  GDisp1_Clear();
  GDisp1_UpdateFull();
  x = 0;
  y = 10;
  FDisp1_WriteString(text, GDisp1_COLOR_BLACK, &x, &y, GFONT1_GetFont());
  GDisp1_UpdateFull();
  vTaskDelay(pdMS_TO_TICKS(1000));

}

static void DrawFont(void) {
  FDisp1_PixelDim x, y;

  GDisp1_Clear();
  GDisp1_UpdateFull();

  x = 0;
  y = 10;
  FDisp1_WriteString("TextOnDisplay!", GDisp1_COLOR_BLACK, &x, &y, GFONT1_GetFont());
  GDisp1_UpdateFull();
  vTaskDelay(pdMS_TO_TICKS(500));
  x = 0;
  y += GFONT1_GetBoxHeight();
  FDisp1_WriteString("Lab 29!", GDisp1_COLOR_BLACK, &x, &y, GFONT1_GetFont());
  GDisp1_UpdateFull();
  WAIT1_Waitms(1000);
}

static void DrawText(void) {
  GDisp1_Clear();
  GDisp1_UpdateFull();
  /*PDC1_WriteLineStr(1, "Segment_1");
  vTaskDelay(pdMS_TO_TICKS(2000));
  PDC1_WriteLineStr(2, "Segment_2");
  vTaskDelay(pdMS_TO_TICKS(2000));*/
  PDC1_WriteLineStr(1, "Welcome to Remote");
  PDC1_WriteLineStr(2, "INTRO FS17");
  vTaskDelay(pdMS_TO_TICKS(2000));
  //GDisp1_UpdateFull();
}

void updateLCD(void) {
	requestLCDUpdate = TRUE;
}

static void LCD_Task(void *param) {
  (void)param; /* not used */
#if 1
  //ShowTextOnLCD("Press a key!");
  //vTaskDelay(pdMS_TO_TICKS(1000));
  //DrawText();	//Welcome Text
  /* \todo extend */
  //DrawFont();

  //Welcome Text
  ShowTextOnLCD("Welcome!");
  vTaskDelay(pdMS_TO_TICKS(2000));	//2Sekunden lang anzeigen lassen

  // Lab 28 draw line and circle, set pixel
  /*GDisp1_Clear();
  GDisp1_UpdateFull();
  GDisp1_SetPixel(0,GDisp1_GetHeight()-1); // Achtung: GetHeight()-1
  GDisp1_DrawLine(0,0,GDisp1_GetWidth(),GDisp1_GetHeight(),GDisp1_COLOR_BLACK);
  GDisp1_DrawCircle(GDisp1_GetWidth()/2,GDisp1_GetHeight()/2, 10,GDisp1_COLOR_BLACK);
  GDisp1_UpdateFull();
  vTaskDelay(pdMS_TO_TICKS(3000));*/

  //DrawLines(); /*! \todo */
  //DrawCircles();
#endif
#if PL_CONFIG_HAS_LCD_MENU
  LCDMenu_InitMenu(menus, sizeof(menus)/sizeof(menus[0]), 1);
  LCDMenu_OnEvent(LCDMENU_EVENT_DRAW, NULL);
#endif
  for(;;) {
    if (LedBackLightisOn) {
      LCD_LED_On(); /* LCD backlight on */
    } else {
      LCD_LED_Off(); /* LCD backlight off */
    }
#if PL_CONFIG_HAS_LCD_MENU
    if (requestLCDUpdate) {
      requestLCDUpdate = FALSE;
      LCDMenu_OnEvent(LCDMENU_EVENT_DRAW, NULL);
    }
#if 1 /*! \todo Change this to for your own needs, or use direct task notification */
    //LCD Menu Events nur handeln falls Snake Game noch nicht gestartet wurde
    if (snakeGameStarted() == 0) {
		if (EVNT_EventIsSetAutoClear(EVNT_LCD_BTN_LEFT)) { /* left */
		  LCDMenu_OnEvent(LCDMENU_EVENT_LEFT, NULL);
		  //ShowTextOnLCD("left");
		}
		if (EVNT_EventIsSetAutoClear(EVNT_LCD_BTN_RIGHT)) { /* right */
		  LCDMenu_OnEvent(LCDMENU_EVENT_RIGHT, NULL);
		  //ShowTextOnLCD("right");
		}
		if (EVNT_EventIsSetAutoClear(EVNT_LCD_BTN_UP)) { /* up */
		  LCDMenu_OnEvent(LCDMENU_EVENT_UP, NULL);
		  //ShowTextOnLCD("up");
		}
		if (EVNT_EventIsSetAutoClear(EVNT_LCD_BTN_DOWN)) { /* down */
		  LCDMenu_OnEvent(LCDMENU_EVENT_DOWN, NULL);
		  //ShowTextOnLCD("down");
		}
		if (EVNT_EventIsSetAutoClear(EVNT_LCD_BTN_CENTER)) { /* center */
		  LCDMenu_OnEvent(LCDMENU_EVENT_ENTER, NULL);
		  //ShowTextOnLCD("center");
		}
		if (EVNT_EventIsSetAutoClear(EVNT_LCD_SIDE_BTN_UP)) { /* side up */
		  LCDMenu_OnEvent(LCDMENU_EVENT_UP, NULL);
		  //ShowTextOnLCD("side up");
		}
		if (EVNT_EventIsSetAutoClear(EVNT_LCD_SIDE_BTN_DOWN)) { /* side down */
		  LCDMenu_OnEvent(LCDMENU_EVENT_DOWN, NULL);
		  //ShowTextOnLCD("side down");
		}
    }
#endif
#endif /* PL_CONFIG_HAS_LCD_MENU */
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

void LCD_Deinit(void) {
#if PL_CONFIG_HAS_LCD_MENU
  LCDMenu_Deinit();
#endif
}

void LCD_Init(void) {
  LedBackLightisOn =  TRUE;
  if (xTaskCreate(LCD_Task, "LCD", configMINIMAL_STACK_SIZE+100, NULL, tskIDLE_PRIORITY, NULL) != pdPASS) {
    for(;;){} /* error! probably out of memory */
  }
#if PL_CONFIG_HAS_LCD_MENU
  LCDMenu_Init();
#endif
}

#endif /* PL_CONFIG_HAS_LCD_MENU */
