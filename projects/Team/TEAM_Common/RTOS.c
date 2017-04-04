/**
 * \file
 * \brief Real Time Operating System (RTOS) main program.
 * \author Erich Styger, erich.styger@hslu.ch
 */

#include "Platform.h"
#if PL_CONFIG_HAS_RTOS
#include "RTOS.h"
#include "FRTOS1.h"
/* User includes (#include below this line is not maintained by Processor Expert) */
#include "LED.h"

void RTOS_Init(void) {
  /*! \todo Create tasks here */
	//Create main task
	BaseType_t res;
	xTaskHandle taskHndl ;
	res = xTaskCreate(MainTask,"Main",configMINIMAL_STACK_SIZE+50 /*1kByte Stack*/,
		(void*)NULL,tskIDLE_PRIORITY+1 /*um eins höhere Prio als idle task*/,&taskHndl);

	//error handling /*! \todo do Error Handling here */
	//if ( res!=pdPASS) {
	//}

}

void RTOS_Deinit(void) {
  /* nothing needed for now */
}

static void MainTask(void *pvParameters) {
	for(;;) {
		//Blinking an LED
		LEDPin1_NegVal();
#if PL_LOCAL_CONFIG_BOARD_IS_ROBO
		LEDPin2_NegVal();
#endif
		vTaskDelay(200/portTICK_PERIOD_MS);		//200ms Blinkperiode
	}
}

#endif /* PL_CONFIG_HAS_RTOS */
