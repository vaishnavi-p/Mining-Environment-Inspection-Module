/**
  ******************************************************************************
  * @file           : statemachine.c
  * @brief          : State Machine for the Environment inspection decision algorithm
  * @author			: Vaishnavi Patekar (vaishnavi.patekar@colorado.edu)
  * @course			: Embedded System Design (Spring'23)
  * @instructor     : Prof. Linden McClure
  * @Date           : April 20th, 2023
  * @Assignment     : Final Project
  ******************************************************************************
**/

#include "string.h"
#include "bme680.h"
#include "stdio.h"
#include "fonts.h"
#include "ssd1306.h"

#include <stdarg.h> //for va_list var arg functions

#include <stdbool.h>
#include "statemachine.h"

#include "statemachine.h"

/* -------------------------------------------------- */
//          MACRO DEFINITIONS
/* -------------------------------------------------- */

/* Macros for Temperature Calibration Threshold */

#define TempHiModerate 25.00 		//High Temperature threshold value for moderate state
#define TempHiDanger 26.00			//High Temperature threshold value for danger state
#define TempLoModerate 23.00		//For hysteresis, delta change from high temperature i.e. (hightemp - delta)
#define TempLoDanger 23.00			//For hysteresis, delta change from high temperature i.e. (hightemp - delta)
#define TempCtrThd 2				//Accuracy, confirming the state for the counter time to avoid sudden spikes and glitches

/* Macros for Pressure Calibration Threshold */

#define PresHiModerate 834.0		//High Pressure threshold value for moderate state
#define PresHiDanger 835.0			//High Pressure threshold value for danger state
#define PresLoModerate 830.00		//For hysteresis, delta change from high pressure i.e. (hightPress - delta)
#define PresLoDanger 833.60			//For hysteresis, delta change from high pressure i.e. (highPress - delta)
#define PresCtrThd 2				//Accuracy, confirming the state for the counter time to avoid sudden spikes and glitches

/* Macros for Humidity Calibration Threshold */

#define HumHiModerate 28.0
#define HumHiDanger 30.0
#define HumLoModerate 27.0
#define HumLoDanger 26.0
#define HumCtrThd 1

/* Macros for Gas Calibration Threshold */
#define GasHiModerate 14.0
#define GasHiDanger 15.0
#define GasLoModerate 13.5
#define GasLoDanger 14.5
#define GasCtrThd 2

#define FinalCounterCtrThd 1

/* -------------------------------------------------- */
//          GLOABAL VARIABLES
/* -------------------------------------------------- */


/* Variables to store the actual sensor values */
float Temperature_Actual;
float Pressure_Actual;
float Humidity_Actual;
float Gas_Actual;

/* Variables to store the state: Safe(0), moderate(1), dangerous(2) based on the threshold */
uint8_t TempState=0;
uint8_t PresState=0;
uint8_t GasState=0;
uint8_t HumState=0;

/* Variables to store the previous values of the hysteresis o/p in the moderate state */
bool TempModerate_Old=0;
bool PresModerate_Old=0;
bool HumModerate_Old=0;
bool GasModerate_Old=0;

/* Variables to store the previous values of the hysteresis o/p in the danger state */
bool TempDanger_Old=0;
bool PresDanger_Old=0;
bool HumDanger_Old=0;
bool GasDanger_Old=0;

/* Variables to store final state */
uint8_t TempState_Confirmed=0;
uint8_t PresState_Confirmed=0;
uint8_t HumState_Confirmed=0;
uint8_t GasState_Confirmed=0;

/* Variables to store counter to check state stability in order to decide confirmed_state*/
uint8_t TempCounter=0;
uint8_t PresCounter=0;
uint8_t HumCounter=0;
uint8_t GasCounter=0;

/* Variables to store the old values of the state */
uint8_t TempState_Old=0;
uint8_t PresState_Old=0;
uint8_t HumState_Old=0;
uint8_t GasState_Old=0;

/* Final state variables considering all the sensor values */
uint8_t FinalCounter=0;
uint8_t FinalState_Confirmed=0;
uint8_t FinalState_Old=0;
uint8_t FinalState=0;

/* -------------------------------------------------- */
//          FUNCTION DEFINITIONS
/* -------------------------------------------------- */

/***********************************************************************
 * @name Hyst()
 * @brief Calculates the hysteresis based on threshold values to avoid the fluctuations around the threshold values
 * @return boolean value 0 or 1 based on a high or low temp.
 ***********************************************************************/
bool Hyst(float Hy,float Hy_L,float Hy_H,bool Hy_Out_Old)
{
	bool Hy_Out=0;
	if(Hy_L>Hy)
		Hy_Out=0;
	else if(Hy>Hy_H || Hy_Out_Old)
		Hy_Out=1;

	return Hy_Out;
}


/***********************************************************************
 * @name Temperature()
 * @brief Finalizes the temperature state
 * @return void
 ***********************************************************************/
void Temperature()
{
	bool TempModerate,TempDanger;
	TempModerate=Hyst(Temperature_Actual,TempLoModerate,TempHiModerate,TempModerate_Old);
	TempModerate_Old=TempModerate;

	TempDanger=Hyst(Temperature_Actual,TempLoDanger,TempHiDanger,TempDanger_Old);
	TempDanger_Old=TempDanger;
	if(TempDanger)
		TempState=2;
	else if(TempModerate)
		TempState=1;
	else
		TempState=0;

	if(TempState_Old==TempState)
		TempCounter=TempCounter+1;

	if(TempCounter>TempCtrThd)
	{
		TempState_Confirmed=TempState;
		TempCounter=0;
	}

	TempState_Old=TempState;
}


/***********************************************************************
 * @name Pressure()
 * @brief Finalizes the pressure state
 * @return void
 ***********************************************************************/
void Pressure()
{
	bool PresModerate,PresDanger;
	PresModerate=Hyst(Pressure_Actual,PresLoModerate,PresHiModerate,PresModerate_Old);
	PresModerate_Old=PresModerate;

	PresDanger=Hyst(Pressure_Actual,PresLoDanger,PresHiDanger,PresDanger_Old);
	PresDanger_Old=PresDanger;
	if(PresDanger)
		PresState=2;
	else if(PresModerate)
		PresState=1;
	else
		PresState=0;

	if(PresState_Old==PresState)
		PresCounter=PresCounter+1;

	if(PresCounter>PresCtrThd)
	{
		PresState_Confirmed=PresState;
		PresCounter=0;
	}

	PresState_Old=PresState;
}


/***********************************************************************
 * @name Humidity()
 * @brief Finalizes the humidity state
 * @return void
 ***********************************************************************/
void Humidity()
{
	bool HumModerate,HumDanger;
	HumModerate=Hyst(Humidity_Actual,HumLoModerate,HumHiModerate,HumModerate_Old);


	HumModerate_Old = HumModerate;

	HumDanger=Hyst(Humidity_Actual,HumLoDanger,HumHiDanger,HumDanger_Old);
	HumDanger_Old=HumDanger;

	if(HumDanger)
		HumState=2;
	else if(HumModerate)
		HumState=1;
	else
		HumState=0;

	if(HumState_Old==HumState)
		HumCounter=HumCounter+1;

	if(HumCounter>=HumCtrThd)
	{
		HumState_Confirmed=HumState;
		HumCounter=0;
	}

	HumState_Old=HumState;
}


/***********************************************************************
 * @name Gas()
 * @brief Finalizes the air quality state
 * @return void
 ***********************************************************************/
void Gas()
{
	bool GasModerate,GasDanger;
	GasModerate=Hyst(Gas_Actual,GasLoModerate,GasHiModerate,GasModerate_Old);
	GasModerate_Old=GasModerate;

	GasDanger=Hyst(Gas_Actual,GasLoDanger,GasHiDanger,GasDanger_Old);
	GasDanger_Old=GasDanger;
	if(GasDanger)
		GasState=2;
	else if(GasModerate)
		GasState=1;
	else
		GasState=0;

	if(GasState_Old==GasState)
		GasCounter=GasCounter+1;

	if(GasCounter>GasCtrThd)
	{
		GasState_Confirmed=GasState;
		GasCounter=0;
	}

	GasState_Old=GasState;
}



/***********************************************************************
 * @name sensor_statemachine()
 * @brief State Transition logic
 * @return void
 ***********************************************************************/
void sensor_statemachine(float t1, float h1, float p1, float g1)
{

	Temperature_Actual = t1;
	Pressure_Actual= p1;
	Humidity_Actual = h1;
	Gas_Actual= g1;

	Temperature();
	Pressure();
	Humidity();
	Gas();

	if(GasState_Confirmed == 2 && TempState_Confirmed == 2 && PresState_Confirmed == 2)
	{
		FinalState=4;
	}

	else if((PresState_Confirmed==2||GasState_Confirmed==2) && TempState_Confirmed==2)
	{
		FinalState=3;
	}
	else if(GasState_Confirmed==2 || HumState_Confirmed==2 || PresState_Confirmed==2 || TempState_Confirmed==2)
	{
		FinalState=2;

	}
	else if(GasState_Confirmed==1 || HumState_Confirmed==1 || PresState_Confirmed==1 || TempState_Confirmed==1)
	{
		FinalState=1;

	}
	else
	{
		FinalState=0;

	}
	if(FinalState_Old == FinalState)
			FinalCounter=FinalCounter+1;

	if(FinalCounter > FinalCounterCtrThd || FinalState > 2)
	{
		FinalState_Confirmed=FinalState;
		FinalCounter=0;
	}
	FinalState_Old = FinalState;


	switch(FinalState_Confirmed)
	{
		case 4:
			myprintf("\r\n Environment Condition =  Highly Dangerous, due to high temperature, pressure and bad air quality; High chances of fire!");
			HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, GPIO_PIN_SET);			//RED LED ON
			HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, GPIO_PIN_RESET);			//Green LED Off
			HAL_GPIO_WritePin(GPIOD, GPIO_PIN_15, GPIO_PIN_RESET);			//Blue LED Off
			break;

		case 3:
			myprintf("\r\n Environment Condition = Moderately Dangerous, due to high temperature and pressure/bad air quality. ");
			HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, GPIO_PIN_SET);			//RED LED ON
			HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, GPIO_PIN_RESET);			//Green LED Off
			HAL_GPIO_WritePin(GPIOD, GPIO_PIN_15, GPIO_PIN_RESET);			//Blue LED Off
			break;

		case 2:
			myprintf("\r\n Environment Condition = Dangerous ");
			HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, GPIO_PIN_SET);			//RED LED ON
			HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, GPIO_PIN_RESET);			//Green LED Off
			HAL_GPIO_WritePin(GPIOD, GPIO_PIN_15, GPIO_PIN_RESET);			//Blue LED Off
			break;

		case 1:
			myprintf("\r\n Environment Condition = Moderate ");
			HAL_GPIO_WritePin(GPIOD, GPIO_PIN_15, GPIO_PIN_SET);			//Blue LED ON
			HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, GPIO_PIN_RESET);			//RED LED OFF
			HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, GPIO_PIN_RESET);			//Green LED Off

			break;

		case 0:
			myprintf("\r\n Environment Condition = Safe ");
			HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, GPIO_PIN_SET);			//Green LED ON
			HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, GPIO_PIN_RESET);			//RED LED OFF
			HAL_GPIO_WritePin(GPIOD, GPIO_PIN_15, GPIO_PIN_RESET);			//Blue LED Off
			break;

	}

}
