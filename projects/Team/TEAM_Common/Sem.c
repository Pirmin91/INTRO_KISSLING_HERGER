/**
 * \file
 * \brief Semaphore usage
 * \author Erich Styger, erich.styger@hslu.ch
 *
 * Module using semaphores.
 */

/**
 * \file
 * \brief Semaphore usage
 * \author Erich Styger, erich.styger@hslu.ch
 *
 * Module using semaphores.
 */

#include "Platform.h" /* interface to the platform */
#if PL_CONFIG_HAS_SEMAPHORE
#include "FRTOS1.h"
#include "Sem.h"
#include "LED.h"

static SemaphoreHandle_t xSemaphore;

static void vSlaveTask(void *pvParameters) {
	while(xSemaphore == NULL)
	{
		vTaskDelay(100/portTICK_PERIOD_MS);	// wait 10ms
	}
   for(;;) {
    /*! \todo Implement functionality */
	     if (xSemaphoreTake(xSemaphore, portMAX_DELAY)==pdPASS) { /* block on semaphore */
	       LED1_Neg();
	     }
   }
}

static void vMasterTask(void *pvParameters) {
  /*! \todo send semaphore from master task to slave task */
	while(xSemaphore == NULL)
	{
		vTaskDelay(100/portTICK_PERIOD_MS);	// wait 10ms
	}
  for(;;) {
    /*! \todo Implement functionality */
	    (void)xSemaphoreGive(xSemaphore); /* give control to other task */
	    vTaskDelay(1000/portTICK_PERIOD_MS);
  }
}


void SEM_Deinit(void) {
}

/*! \brief Initializes module */
void SEM_Init(void)
{
	xSemaphore = xSemaphoreCreateBinary(); // Creating Binary Semaphore
	if (xSemaphore == NULL)
	{
		// Failed! not enough heap memory! FOLIE 90
		for(;;)
		{

		}
	}
	else {
		  vQueueAddToRegistry(xSemaphore, "IPC_Sem");
			// Master -> function above
			if (xTaskCreate(vMasterTask, "Master", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY+1, NULL) != pdPASS) {
				for(;;){} /* error */
			}

			// Slave -> function above
			if (xTaskCreate(vSlaveTask, "Slave", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY+1, NULL) != pdPASS) {
				for(;;){} /* error */
			}
	}
}
#endif /* PL_CONFIG_HAS_SEMAPHORE */
