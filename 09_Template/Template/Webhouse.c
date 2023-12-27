/******************************************************************************/
/** \file       Webhouse.c
 *******************************************************************************
 *
 *  \brief      functions for the webhouse
 *              
 *
 *  \author     gsl2 Lorenzo Giusto
 *
 *  \date       November 2020
 *
 *  \remark     Last Modification
 *              gsl2, November 2020, Created
 *
 ******************************************************************************/
/*
 *  functions  global:
 * 				initWebhouse
 * 				closeWebhouse
 * 				turnTVOn
 * 				turnTVOff
 * 				getTVState
 * 				dimDLed
 * 				dimSLed
 * 				turnLED1On
 * 				turnLED1Off
 * 				getLED1State
 * 				turnLED2On
 * 				turnLED2Off
 * 				getLED2State
 * 				turnHeizOn
 * 				turnHeizOff
 * 				getHeizState
 * 				getAlarmState
 *             
 ******************************************************************************/
 
//----- Header-Files -----------------------------------------------------------
#include <bcm2835.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <limits.h>
#include <unistd.h>
#include <pthread.h>

#include "Webhouse.h"

//----- Macros -----------------------------------------------------------------
//PWM can only be used in privilege mode
#undef PWM

//GPIO PIN
#define GPIO_TV RPI_BPLUS_GPIO_J8_03			//Pin 3
#define GPIO_dimSLamp RPI_BPLUS_GPIO_J8_32 	    //Pin 32
#define GPIO_dimRLamp RPI_BPLUS_GPIO_J8_33		//Pin 33
#define GPIO_Heat RPI_BPLUS_GPIO_J8_05 			//Pin 5
#define GPIO_Alarm RPI_BPLUS_GPIO_J8_15			//Pin 15
#define GPIO_LED1 RPI_BPLUS_GPIO_J8_07			//Pin 7
#define GPIO_LED2 RPI_BPLUS_GPIO_J8_11			//Pin 11

//PWM Channel
#ifdef PWM
#define PWM_CHANNEL0 0
#define PWM_CHANNEL1 1
#endif

//Range of the PWM
#define RANGE 100

//INPUT OUTPUT
#define INPUT BCM2835_GPIO_FSEL_INPT
#define OUTPUT BCM2835_GPIO_FSEL_OUTP

#define MAX_TEMP 40
#define MIN_TEMP 0

#define HEIZ_ON 1
#define HEIZ_OFF 0

//----- Function prototypes ----------------------------------------------------
static void * threadTemp(void *pdata);
#ifndef PWM
static void * threadDimRLamp(void *pdata);
static void * threadDimSLamp(void *pdata);
#endif

//----- Data -------------------------------------------------------------------
static pthread_t pThreadTemp;
static int stateHeiz = HEIZ_OFF;
static float localTemp = 16.0;
#ifndef PWM
static pthread_t pThreadDimRLamp;
static pthread_t pThreadDimSLamp;
static int dutyCycleRL = 0;
static int dutyCycleSL = 0;
#endif

//----- Implementation ---------------------------------------------------------

/*******************************************************************************
 *  function :    initWebhouse
 ******************************************************************************/
/** \brief        Initializes all used hardware within the webhouse.
 *                Before any other function can be used, the webhouse must be
 *                initialized by calling this function.
 *
 *  \type         global
 *
 *  \return       
 *
 ******************************************************************************/
void initWebhouse(void){
	bcm2835_init();
	printf("GPIO initializion\n");
	
	bcm2835_gpio_fsel(GPIO_TV, OUTPUT);
	bcm2835_gpio_fsel(GPIO_Heat, OUTPUT);
	bcm2835_gpio_fsel(GPIO_Alarm, INPUT);
	bcm2835_gpio_fsel(GPIO_LED1, OUTPUT);
	bcm2835_gpio_fsel(GPIO_LED2, OUTPUT);
#ifdef PWM
	bcm2835_gpio_fsel(GPIO_dimRLamp, BCM2835_GPIO_FSEL_ALT0);
	bcm2835_gpio_fsel(GPIO_dimSLamp, BCM2835_GPIO_FSEL_ALT0);
#else
	bcm2835_gpio_fsel(GPIO_dimRLamp, OUTPUT);
	bcm2835_gpio_fsel(GPIO_dimSLamp, OUTPUT);
#endif

#ifdef PWM
	bcm2835_pwm_set_clock(BCM2835_PWM_CLOCK_DIVIDER_16);
    bcm2835_pwm_set_mode(PWM_CHANNEL0, 1, 1);
    bcm2835_pwm_set_range(PWM_CHANNEL0, RANGE);
    bcm2835_pwm_set_mode(PWM_CHANNEL1, 1, 1);
    bcm2835_pwm_set_range(PWM_CHANNEL1, RANGE);
#endif

    pthread_create(&pThreadTemp, NULL, threadTemp, NULL);
#ifndef PWM
    pthread_create(&pThreadDimRLamp, NULL, threadDimRLamp, NULL);
    pthread_create(&pThreadDimSLamp, NULL, threadDimSLamp, NULL);
#endif
}

/*******************************************************************************
 *  function :    closeWebhouse
 ******************************************************************************/
/** \brief        Release all resource, and sets the webhouse in a defined
 *                state (everything is turned off).
 *
 *  \type         global
 *
 *  \return       
 *
 ******************************************************************************/
void closeWebhouse(void){
	pthread_cancel(pThreadTemp);
#ifndef PWM
	pthread_cancel(pThreadDimRLamp);
	pthread_cancel(pThreadDimSLamp);
#endif
	bcm2835_close();
}

/*******************************************************************************
 *  function :    turnTVOn
 ******************************************************************************/
/** \brief        Turn on the TV
 *                The webhouse must be initialized (initWebhouse) before this
 *                function can be called.
 *
 *  \type         global
 *
 *  \return       
 *
 ******************************************************************************/
void turnTVOn(void){
	bcm2835_gpio_write(GPIO_TV, HIGH);
}

/*******************************************************************************
 *  function :    turnTVOff
 ******************************************************************************/
/** \brief        Turn off the TV
 *                The webhouse must be initialized (initWebhouse) before this
 *                function can be called.
 *
 *  \type         global
 *
 *  \return      
 *
 ******************************************************************************/
void turnTVOff(void){
	bcm2835_gpio_write(GPIO_TV, LOW);
}

/*******************************************************************************
 *  function :    getTVState
 ******************************************************************************/
/** \brief        Get the state (1 equals to ON, 0 equals to OFF) of the TV
 *                The webhouse must be initialized (initWebhouse) before this
 *                function can be called.
 *
 *  \type         global
 *
 *  \return       1 for TV is ON, 0 for TV is OFF
 *
 ******************************************************************************/
int getTVState(void){
	uint8_t value = bcm2835_gpio_lev(GPIO_TV);
	return value;
}

/*******************************************************************************
 *  function :    dimSLamp
 ******************************************************************************/
/** \brief        Dim the stand lamp from 0 (dark) to 100 (brightest)
 *                The webhouse must be initialized (initWebhouse) before this
 *                function can be called.
 *
 *  \type         global
 *
 *  \param[in]    Duty_Cycle   Dim level [0,100]
 *
 *  \return       
 *
 ******************************************************************************/
void dimSLamp(uint16_t dudtyCycle){
	if(dudtyCycle > 100){
		dudtyCycle = 100;
	}
#ifdef PWM
	bcm2835_pwm_set_data(PWM_CHANNEL0, dudtyCycle);
#else
	dutyCycleSL = dudtyCycle;
#endif
}

/*******************************************************************************
 *  function :    dimRLamp
 ******************************************************************************/
/** \brief        Dim the roof lamp from 0 (dark) to 100 (brightest)
 *                <p>
 *                The webhouse must be initialized (initWebhouse) before this
 *                function can be called.
 *
 *  \type         global
 *
 *  \param[in]    Duty_cycle   Dim level [0,100]
 *
 *  \return       
 *
 ******************************************************************************/
void dimRLamp(uint16_t dudtyCycle){
	if(dudtyCycle > 100){
		dudtyCycle = 100;
	}
#ifdef PWM
	bcm2835_pwm_set_data(PWM_CHANNEL1, dudtyCycle);
#else
	dutyCycleRL = dudtyCycle;
#endif
}

/*******************************************************************************
 *  function :    turnLED1On
 ******************************************************************************/
/** \brief        Turn on the LED 1
 *                The webhouse must be initialized (initWebhouse) before this
 *                function can be called.
 *
 *  \type         global
 *
 *  \return      
 *
 ******************************************************************************/
void turnLED1On(void){
	bcm2835_gpio_write(GPIO_LED1, HIGH);
}

/*******************************************************************************
 *  function :    turnLED1Off
 ******************************************************************************/
/** \brief        Turn off the LED 1
 *                The webhouse must be initialized (initWebhouse) before this
 *                function can be called.
 *
 *  \type         global
 *
 *  \return       
 *
 ******************************************************************************/
void turnLED1Off(void){
	bcm2835_gpio_write(GPIO_LED1, LOW);
}

/*******************************************************************************
 *  function :    getLED1State
 ******************************************************************************/
/** \brief        Get the state (1 equals to ON, 0 equals to OFF) of the LED 1
 *                The webhouse must be initialized (initWebhouse) before this
 *                function can be called.
 *
 *  \type         global
 *
 *  \return       1 for LED is ON, 0 for LED is OFF
 *
 ******************************************************************************/
int getLED1State(void){
    uint8_t value = bcm2835_gpio_lev(GPIO_LED1);
	return value;
}

/*******************************************************************************
 *  function :    turnLED2On
 ******************************************************************************/
/** \brief        Turn on the LED 2
 *                The webhouse must be initialized (initWebhouse) before this
 *                function can be called.
 *
 *  \type         global
 *
 *  \return      
 *
 ******************************************************************************/
void turnLED2On(void){
	bcm2835_gpio_write(GPIO_LED2, HIGH);
}

/*******************************************************************************
 *  function :    turnLED2Off
 ******************************************************************************/
/** \brief        Turn off the LED 2
 *                The webhouse must be initialized (initWebhouse) before this
 *                function can be called.
 *
 *  \type         global
 *
 *  \return       
 *
 ******************************************************************************/
void turnLED2Off(void){
	bcm2835_gpio_write(GPIO_LED2, LOW);
}

/*******************************************************************************
 *  function :    getLED2State
 ******************************************************************************/
/** \brief        Get the state (1 equals to ON, 0 equals to OFF) of the LED 2
 *                The webhouse must be initialized (initWebhouse) before this
 *                function can be called.
 *
 *  \type         global
 *
 *  \return       1 for LED is ON, 0 for LED is OFF
 *
 ******************************************************************************/
int getLED2State(void){
    uint8_t value = bcm2835_gpio_lev(GPIO_LED2);
	return value;
}

/*******************************************************************************
 *  function :    turnHeatOn
 ******************************************************************************/
/** \brief        Turn on the heater
 *                The webhouse must be initialized (initWebhouse) before this
 *                function can be called.
 *
 *  \type         global
 *
 *  \return      
 *
 ******************************************************************************/
void turnHeatOn(void){
	bcm2835_gpio_write(GPIO_Heat, HIGH);
	stateHeiz = HEIZ_ON;
}

/*******************************************************************************
 *  function :    turnHeatOff
 ******************************************************************************/
/** \brief        Turn off the heater
 *                The webhouse must be initialized (initWebhouse) before this
 *                function can be called.
 *
 *  \type         global
 *
 *  \return       
 *
 ******************************************************************************/
void turnHeatOff(void){
	bcm2835_gpio_write(GPIO_Heat, LOW);
	stateHeiz = HEIZ_OFF;
}

/*******************************************************************************
 *  function :    getHeatState
 ******************************************************************************/
/** \brief        Get the state of the heater (1 equals to ON, 0 equals to OFF) 
 *                The webhouse must be initialized (initWebhouse) before this
 *                function can be called.
 *
 *  \type         global
 *
 *  \return       1 for heater is ON, 0 for heater is OFF
 *
 ******************************************************************************/
int getHeatState(void){
	uint8_t value = bcm2835_gpio_lev(GPIO_Heat);
	return value;
}

/*******************************************************************************
 *  function :    getTemp
 ******************************************************************************/
/** \brief        Return the temperature inside the webhouse
 *
 *  \type         global
 *
 *  \return       1 for heater is ON, 0 for heater is OFF
 *
 ******************************************************************************/
float getTemp(void){
	return localTemp;
}

/*******************************************************************************
 *  function :    getAlarmState
 ******************************************************************************/
/** \brief        Get the state of the alarm
 *                The webhouse must be initialized (initWebhouse) before this
 *                function can be called.
 *
 *  \type         global
 *
 *  \return       1 for alarm has detected something, 0 for alarm has nothing detected
 *
 ******************************************************************************/
int getAlarmState(void){
    uint8_t value = bcm2835_gpio_lev(GPIO_Alarm);
    return value;
}

/*******************************************************************************
 *  function :    threadTemp
 ******************************************************************************/
/** \brief        simulate the variation of temperature
 *                according to the state of the heating system
 *
 *  \type         module
 *
 *  \return
 *
 ******************************************************************************/
static void * threadTemp(void *pdata){
	// Never ending loop
	for (;;) {
		if (stateHeiz == HEIZ_ON) {
			if (localTemp < MAX_TEMP) {
				localTemp += 0.05f;
			}
		}
		else {
			if (localTemp > MIN_TEMP) {
			   localTemp -= 0.05f;
			}
		}

		usleep(1000000);
	}
	return NULL;
}

#ifndef PWM
/*******************************************************************************
 *  function :    threadDimRLamp
 ******************************************************************************/
/** \brief        manage the duty cycle of the roof lamp
 *
 *  \type         module
 *
 *  \return
 *
 ******************************************************************************/
static void * threadDimRLamp(void *pdata){
	int time = 0;
	// Never ending loop
	for (;;) {
		if (time <= dutyCycleRL) {
			bcm2835_gpio_write(GPIO_dimRLamp, HIGH);
		} else if (time < RANGE) {
			bcm2835_gpio_write(GPIO_dimRLamp, LOW);
		} else {
			time = 0;
		}
		time++;
		usleep(100);
	}
	return NULL;
}

/*******************************************************************************
 *  function :    threadDimSLamp
 ******************************************************************************/
/** \brief        manage the duty cycle of the stand lamp
 *
 *  \type         module
 *
 *  \return
 *
 ******************************************************************************/
static void * threadDimSLamp(void *pdata){
	int time = 0;
	// Never ending loop
	for (;;) {
		if (time <= dutyCycleSL) {
			bcm2835_gpio_write(GPIO_dimSLamp, HIGH);
		} else if (time <= RANGE) {
			bcm2835_gpio_write(GPIO_dimSLamp, LOW);
		} else {
			time = 0;
		}
		time++;
		usleep(100);
	}
	return NULL;
}
#endif

	
	
