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

// Functions for Lab 21
//static void Task2(void *pvParameters)
//{
//	for(;;)
//	{
//		vTaskDelay(10/portTICK_PERIOD_MS); // Wenn beide Task ein TaskDelay haben -> 100% Runtime bei IDLE
//	}
//}

//static void Task1(void *pvParameters)
//{
//	for(;;)
//	{
//		vTaskDelay(100/portTICK_PERIOD_MS); // Aus diesem Grund wird nur noch Task2 laufen (Runtime 100%), falls Task2 kein TaskDelay hat
//	}
//}

void RTOS_Init(void) {
  /*! \todo Create tasks here */
	//Create main task
	BaseType_t res;
	xTaskHandle taskHndl ;
	res = xTaskCreate(MainTask,"Main",configMINIMAL_STACK_SIZE+50 /*1kByte Stack*/,
		(void*)NULL,tskIDLE_PRIORITY+1 /*um eins höhere Prio als idle task*/,&taskHndl);

	// Lab 21 (create Tasks)

	// Task 1
	//res = xTaskCreate(Task1, "Task1",configMINIMAL_STACK_SIZE+50 /*1kByte Stack*/,
	//		(void*)NULL,tskIDLE_PRIORITY+1 /*um eins höhere Prio als idle task*/,&taskHndl);

	// Task 2
	//res = xTaskCreate(Task2, "Task2",configMINIMAL_STACK_SIZE+50 /*1kByte Stack*/,
	//		(void*)NULL,tskIDLE_PRIORITY+1/*um eins höhere Prio als idle task*/,&taskHndl);


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
