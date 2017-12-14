/**
 * @file    S2LP_Timer.c
 * @author  LowPower RF BU - AMG
 * @version 1.2.0
 * @date    October 31, 2016
 * @brief   Configuration and management of S2-LP timers.
 * @details
 *
 * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
 * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
 * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
 * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
 * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
 * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
 *
 * THIS SOURCE CODE IS PROTECTED BY A LICENSE.
 * FOR MORE INFORMATION PLEASE CAREFULLY READ THE LICENSE AGREEMENT FILE LOCATED
 * IN THE ROOT DIRECTORY OF THIS FIRMWARE PACKAGE.
 *
 * <h2><center>&copy; COPYRIGHT 2016 STMicroelectronics</center></h2>
 */


/* Includes ------------------------------------------------------------------*/

#include "S2LP_Config.h"
#include "MCU_Interface.h"


/**
 * @addtogroup S2LP_Libraries
 * @{
 */


/**
 * @addtogroup S2LP_Timer
 * @{
 */


/**
 * @defgroup Timer_Private_TypesDefinitions             Timer Private Types Definitions
 * @{
 */

/**
 *@}
 */


/**
 * @defgroup Timer_Private_Defines                      Timer Private Defines
 * @{
 */

#define LDC_FREQ_CLK            S2LPTimerGetRcoFrequency()
#define LDC_MAX_TIMER_MULT1     ((65536.0)/LDC_FREQ_CLK)
#define LDC_MAX_TIMER_MULT2     (((65536.0)/LDC_FREQ_CLK)*2)
#define LDC_MAX_TIMER_MULT4     (((65536.0)/LDC_FREQ_CLK)*4)
#define LDC_MAX_TIMER_MULT8     (((65536.0)/LDC_FREQ_CLK)*8)


/**
 *@}
 */


/**
 * @defgroup Timer_Private_Macros                       Timer Private Macros
 * @{
 */

#define IS_RX_TIMEOUT_STOP_CONDITION(COND)  ( COND == NO_TIMEOUT_STOP || \
                                                COND == TIMEOUT_ALWAYS_STOPPED || \
                                                COND == RSSI_ABOVE_THRESHOLD || \
                                                COND == SQI_ABOVE_THRESHOLD || \
                                                COND == PQI_ABOVE_THRESHOLD || \
                                                COND == RSSI_AND_SQI_ABOVE_THRESHOLD || \
                                                COND == RSSI_AND_PQI_ABOVE_THRESHOLD || \
                                                COND == SQI_AND_PQI_ABOVE_THRESHOLD || \
                                                COND == ALL_ABOVE_THRESHOLD || \
                                                COND == RSSI_OR_SQI_ABOVE_THRESHOLD || \
                                                COND == RSSI_OR_PQI_ABOVE_THRESHOLD || \
                                                COND == SQI_OR_PQI_ABOVE_THRESHOLD || \
                                                COND == ANY_ABOVE_THRESHOLD )

/**
 *@}
 */


/**
 * @defgroup Timer_Private_Variables                    Timer Private Variables
 * @{
 */

/**
 *@}
 */


/**
 * @defgroup Timer_Private_FunctionPrototypes            Timer Private Function Prototypes
 * @{
 */

void S2LPTimerComputeRxTimerValues(float* fDesiredMsec, uint8_t pcCounter, uint8_t pcPrescaler);
void S2LPTimerComputeRxTimerRegValues(float fDesiredMsec , uint8_t* pcCounter , uint8_t* pcPrescaler);
void S2LPTimerComputeWakeupTimerRegValues(float fDesiredMsec , uint8_t* pcCounter , uint8_t* pcPrescaler, uint8_t* pcMulti);
void S2LPTimerComputeWakeupTimerValues(float* fDesiredMsec , uint8_t pcCounter , uint8_t pcPrescaler, uint8_t pcMulti);

/**
 *@}
 */


/**
 * @defgroup Timer_Private_Functions                    Timer Private Functions
 * @{
 */

/**
 * @brief  Updates the gState (the global variable used to maintain memory of S2LP Status)
 *         reading the MC_STATE register of S2LP.
 * @param  None
 * @retval None
 */
void S2LPTimerComputeWakeupTimerValues(float* fDesiredMsec , uint8_t pcCounter , uint8_t pcPrescaler, uint8_t pcMulti)
{
  float rco_freq;
  rco_freq = LDC_FREQ_CLK/1000.0*(pcMulti);
  *fDesiredMsec = (float)((pcCounter+1)*(pcPrescaler+1)/rco_freq);
}

/**
 * @brief  Computes the values of the wakeup timer counter and prescaler from the user time expressed in millisecond.
 *         The prescaler and the counter values are computed maintaining the prescaler value as
 *         small as possible in order to obtain the best resolution, and in the meantime minimizing the error.
 * @param  fDesiredMsec desired wakeup timeout in millisecs.
 *         This parameter must be a float. Since the counter and prescaler are 8 bit registers the maximum
 *         reachable value is maxTime = fTclk x 256 x 256.
 * @param  pcCounter pointer to the variable in which the value for the wakeup timer counter has to be stored.
 *         This parameter must be a uint8_t*.
 * @param  pcPrescaler pointer to the variable in which the value for the wakeup timer prescaler has to be stored.
 *         This parameter must be an uint8_t*.
 * @retval None
 */
void S2LPTimerComputeWakeupTimerRegValues(float fDesiredMsec, uint8_t* pcCounter , uint8_t* pcPrescaler, uint8_t* pcMulti)
{
  float rco_freq, err;
  uint32_t n;
  uint8_t multi;
  
  if((fDesiredMsec/1e3) <= LDC_MAX_TIMER_MULT1) {
    multi = 1;
    *pcMulti = 0;
  }
  else if(((fDesiredMsec/1e3) > LDC_MAX_TIMER_MULT1) && ((fDesiredMsec/1e3) <= LDC_MAX_TIMER_MULT2)) {
    multi = 2;
    *pcMulti = 1;
  }
  else if(((fDesiredMsec/1e3) > LDC_MAX_TIMER_MULT2) && ((fDesiredMsec/1e3) <= LDC_MAX_TIMER_MULT4)) {
    multi = 4;
    *pcMulti = 2;
  }
  else if((fDesiredMsec/1e3) > LDC_MAX_TIMER_MULT4) {
    multi = 8;
    *pcMulti = 3;
  }
  
  rco_freq = LDC_FREQ_CLK/1000.0/multi;

  /* N cycles in the time base of the timer: 
     - clock of the timer is RCO frequency
     - divide times 1000 more because we have an input in ms (variable rco_freq is already this frequency divided by 1000)
  */
  n = (uint32_t)(fDesiredMsec*rco_freq);
    
  /* check if it is possible to reach that target with prescaler and counter of S2LP1 */
  if(n/0xFF>0xFD) {
    /* if not return the maximum possible value */
    (*pcCounter) = 0xFF;
    (*pcPrescaler) = 0xFF;
    return;
  }
  
  /* prescaler is really 2 as min value */
  (*pcPrescaler) = (n/0xFF)+2;
  (*pcCounter) = n / (*pcPrescaler);
  
  /* check if the error is minimum */
  err = S_ABS((float)((*pcCounter)*(*pcPrescaler)/rco_freq)-fDesiredMsec);
  
  if((*pcCounter)<=254) {
    if(S_ABS((float)((*pcCounter)+1)*(*pcPrescaler)/rco_freq-fDesiredMsec)<err)
      (*pcCounter) = (*pcCounter)+1;
  }
    
  /* decrement prescaler and counter according to the logic of this timer in S2LP */
  (*pcPrescaler)--;
  if((*pcCounter)>1)
    (*pcCounter)--;
  else
    (*pcCounter)=1;
}

/**
 * @brief  Computes the values of the rx_timeout timer counter and prescaler from the user time expressed in millisecond.
 *         The prescaler and the counter values are computed maintaining the prescaler value as
 *         small as possible in order to obtain the best resolution, and in the meantime minimizing the error.
 * @param  fDesiredMsec desired rx_timeout in millisecs.
 *         This parameter must be a float. Since the counter and prescaler are 8 bit registers the maximum
 *         reachable value is maxTime = fTclk x 255 x 255.
 * @param  pcCounter pointer to the variable in which the value for the rx_timeout counter has to be stored.
 *         This parameter must be a uint8_t*.
 * @param  pcPrescaler pointer to the variable in which the value for the rx_timeout prescaler has to be stored.
 *         This parameter must be an uint8_t*.
 * @retval None
 */
#define LDC_DIVIDER_NEW         1210
#define LDC_DIVIDER_CUT1        1000
#define LDC_DIVIDER             LDC_DIVIDER_NEW//LDC_DIVIDER_CUT1
void S2LPTimerComputeRxTimerRegValues(float fDesiredMsec , uint8_t* pcCounter , uint8_t* pcPrescaler)
{
  uint32_t nXtalFrequency = S2LPRadioGetXtalFrequency();
  uint32_t n;
  float err;
  
  /* if xtal is doubled divide it by 2 */
  if(nXtalFrequency>DIG_DOMAIN_XTAL_THRESH) {
    nXtalFrequency >>= 1;
  }
  
  /* N cycles in the time base of the timer: 
     - clock of the timer is xtal/1210
     - divide times 1000 more because we have an input in ms
  */
  n=(uint32_t)(fDesiredMsec*nXtalFrequency/LDC_DIVIDER/1000);
  
  /* check if it is possible to reach that target with prescaler and counter of S2LP1 */
  if(n/0xFF>0xFD) {
    /* if not return the maximum possible value */
    (*pcCounter) = 0xFF;
    (*pcPrescaler) = 0xFF;
    return;
  }
  
  /* prescaler is really 2 as min value */
  (*pcPrescaler)=(n/0xFF)+2;
  (*pcCounter) = n / (*pcPrescaler);
  
  /* check if the error is minimum */
  err=S_ABS((((float)(*pcCounter))*(*pcPrescaler)-1)*LDC_DIVIDER/nXtalFrequency*1000-fDesiredMsec);
  
  if((*pcCounter)<=254) {
    if(S_ABS(((((float)(*pcCounter))+1)*(*pcPrescaler)-1)*LDC_DIVIDER/nXtalFrequency*1000-fDesiredMsec)<err)
      (*pcCounter)=(*pcCounter)+1;
  }
    
  /* decrement prescaler and counter according to the logic of this timer in S2LP1 */
  
  
  (*pcPrescaler)--;
  
  /*if((*pcCounter)>1)
    (*pcCounter)--;
  else*/
  if((*pcCounter)==0)
    (*pcCounter)=1;
}

/**
 * @brief  Computes the values of the rx_timeout timer counter and prescaler from the user time expressed in millisecond.
 *         The prescaler and the counter values are computed maintaining the prescaler value as
 *         small as possible in order to obtain the best resolution, and in the meantime minimizing the error.
 * @param  fDesiredMsec desired rx_timeout in millisecs.
 *         This parameter must be a float. Since the counter and prescaler are 8 bit registers the maximum
 *         reachable value is maxTime = fTclk x 255 x 255.
 * @param  pcCounter pointer to the variable in which the value for the rx_timeout counter has to be stored.
 *         This parameter must be a uint8_t*.
 * @param  pcPrescaler pointer to the variable in which the value for the rx_timeout prescaler has to be stored.
 *         This parameter must be an uint8_t*.
 * @retval None
 */
void S2LPTimerComputeRxTimerValues(float* fDesiredMsec, uint8_t pcCounter, uint8_t pcPrescaler)
{
  float nXtalFrequency;
  
  if(S2LPRadioGetXtalFrequency() > DIG_DOMAIN_XTAL_THRESH) {
    nXtalFrequency = S2LPRadioGetXtalFrequency()/2.0;
  }
  nXtalFrequency /= 1000.0;
  *fDesiredMsec = (float)((pcPrescaler+1)*pcCounter*(1210.0/nXtalFrequency));
}

/**
 *@}
 */


/**
 * @defgroup Timer_Public_Functions                    Timer Public Functions
 * @{
 */


/**
 * @brief  Enables or Disables the LDCR mode.
 * @param  xNewState new state for LDCR mode.
 *         This parameter can be: S_ENABLE or S_DISABLE.
 * @retval None.
 */
void S2LPTimerLdcrMode(SFunctionalState xNewState)
{
  uint8_t tmp;
  s_assert_param(IS_SFUNCTIONAL_STATE(xNewState));
  
  S2LPSpiReadRegisters(PROTOCOL1_ADDR, 1, &tmp);
  if(xNewState==S_ENABLE) {
    tmp |= LDC_MODE_REGMASK;
  }
  else {
    tmp &= ~LDC_MODE_REGMASK;
  }
  g_xStatus = S2LPSpiWriteRegisters(PROTOCOL1_ADDR, 1, &tmp);
}


/**
 * @brief  Enables or Disables the LDCR timer reloading with the value stored in the LDCR_RELOAD registers.
 * @param  xNewState new state for LDCR reloading.
 *         This parameter can be: S_ENABLE or S_DISABLE.
 * @retval None.
 */
void S2LPTimerLdcrAutoReload(SFunctionalState xNewState)
{
  uint8_t tmp;
  s_assert_param(IS_SFUNCTIONAL_STATE(xNewState));

  S2LPSpiReadRegisters(PROTOCOL1_ADDR, 1, &tmp);
  if(xNewState==S_ENABLE) {
    tmp |= LDC_RELOAD_ON_SYNC_REGMASK;
  }
  else {
    tmp &= ~LDC_RELOAD_ON_SYNC_REGMASK;
  }
  g_xStatus = S2LPSpiWriteRegisters(PROTOCOL1_ADDR, 1, &tmp);
}


/**
 * @brief  Return the LDCR timer reload bit.
 * @param  None.
 * @retval SFunctionalState: value of the reload bit.
 */
SFunctionalState S2LPTimerLdcrGetAutoReload(void)
{
  uint8_t tmp;
  g_xStatus = S2LPSpiReadRegisters(PROTOCOL1_ADDR, 1, &tmp);
  return (SFunctionalState)(tmp & LDC_RELOAD_ON_SYNC_REGMASK);
}

/**
 * @brief  Set the RX timeout timer initialization registers with the values of COUNTER and PRESCALER according to the formula: Trx=PRESCALER*COUNTER*Tck.
 *         Remember that it is possible to have infinite RX_Timeout writing 0 in the RX_Timeout_Counter and/or RX_Timeout_Prescaler registers.
 * @param  cCounter value for the timer counter.
 *         This parameter must be an uint8_t.
 * @param  cPrescaler value for the timer prescaler.
 *         This parameter must be an uint8_t.
 * @retval None.
 */
void S2LPTimerSetRxTimer(uint8_t cCounter , uint8_t cPrescaler)
{
  uint8_t tmpBuffer[2] = {cCounter, cPrescaler};
  g_xStatus = S2LPSpiWriteRegisters(TIMERS5_ADDR, 2, tmpBuffer);
}


/**
 * @brief  Set the RX timeout timer counter and prescaler from the desired value in ms. it is possible to fix the RX_Timeout to
 *         a minimum value of 50.417us to a maximum value of about 3.28 s.
 * @param  fDesiredMsec desired timer value.
 *         This parameter must be a float.
 * @retval None
 */
void S2LPTimerSetRxTimerMs(float fDesiredMsec)
{
  uint8_t tmpBuffer[2];
  S2LPTimerComputeRxTimerRegValues(fDesiredMsec , &tmpBuffer[0] , &tmpBuffer[1]);
  g_xStatus = S2LPSpiWriteRegisters(TIMERS5_ADDR, 2, tmpBuffer);
}


/**
 * @brief  Set the RX timeout timer counter. If it is equal to 0 the timeout is infinite.
 * @param  cCounter value for the timer counter.
 *         This parameter must be an uint8_t.
 * @retval None.
 */
void S2LPTimerSetRxTimerCounter(uint8_t cCounter)
{
  g_xStatus = S2LPSpiWriteRegisters(TIMERS5_ADDR, 1, &cCounter);
}


/**
 * @brief  Set the RX timeout timer prescaler. If it is equal to 0 the timeout is infinite.
 * @param  cPrescaler value for the timer prescaler.
 *         This parameter must be an uint8_t.
 * @retval None
 */
void S2LPTimerSetRxTimerPrescaler(uint8_t cPrescaler)
{
  g_xStatus = S2LPSpiWriteRegisters(TIMERS4_ADDR, 1, &cPrescaler);
}


/**
 * @brief  Return the RX timeout timer.
 * @param  pfTimeoutMsec pointer to the variable in which the timeout expressed in milliseconds has to be stored.
 *         If the returned value is 0, it means that the RX_Timeout is infinite.
 *         This parameter must be a float*.
 * @param  pcCounter pointer to the variable in which the timer counter has to be stored.
 *         This parameter must be an uint8_t*.
 * @param  pcPrescaler pointer to the variable in which the timer prescaler has to be stored.
 *         This parameter must be an uint8_t*.
 * @retval None.
 */
void S2LPTimerGetRxTimer(float* pfTimeoutMsec, uint8_t* pcCounter , uint8_t* pcPrescaler)
{
  uint8_t tmpBuffer[2];
  
  g_xStatus = S2LPSpiReadRegisters(TIMERS5_ADDR, 2, tmpBuffer);
  
  (*pcCounter) = tmpBuffer[0];
  (*pcPrescaler) = tmpBuffer[1];
  S2LPTimerComputeRxTimerValues(pfTimeoutMsec, tmpBuffer[0], tmpBuffer[1]);
}

/**
 * @brief  Set the LDCR wake up timer initialization registers with the values of
 *         COUNTER and PRESCALER according to the formula: Twu=(PRESCALER +1)*(COUNTER+1)*Tck, where
 *         Tck = 28.818 us. The minimum vale of the wakeup timeout is 28.818us (PRESCALER and
 *         COUNTER equals to 0) and the maximum value is about 1.89 s (PRESCALER anc COUNTER equals
 *         to 255).
 * @param  cCounter value for the timer counter.
 *         This parameter must be an uint8_t.
 * @param  cPrescaler value for the timer prescaler.
 *         This parameter must be an uint8_t.
 * @retval None.
 */
void S2LPTimerSetWakeUpTimer(uint8_t cCounter , uint8_t cPrescaler)
{
  uint8_t tmpBuffer[2] = {cPrescaler, cCounter};
  g_xStatus = S2LPSpiWriteRegisters(TIMERS3_ADDR, 2, tmpBuffer);
}


/**
 * @brief  Set the LDCR wake up timer counter and prescaler from the desired value in ms,
 *         according to the formula: Twu=(PRESCALER +1)*(COUNTER+1)*Tck, where Tck = 28.818 us.
 *         The minimum vale of the wakeup timeout is 28.818us (PRESCALER and COUNTER equals to 0)
 *         and the maximum value is about 1.89 s (PRESCALER anc COUNTER equals to 255).
 * @param  fDesiredMsec desired timer value.
 *         This parameter must be a float.
 * @retval None.
 */
void S2LPTimerSetWakeUpTimerMs(float fDesiredMsec)
{
  uint8_t tmpBuffer[2], multi, tmp;

  /* Computes counter and prescaler */
  S2LPTimerComputeWakeupTimerRegValues(fDesiredMsec , &tmpBuffer[1] , &tmpBuffer[0], &multi);

  S2LPSpiReadRegisters(PROTOCOL2_ADDR, 1, &tmp);
  tmp &= ~LDC_TIMER_MULT_REGMASK;
  tmp |= multi;
  S2LPSpiWriteRegisters(PROTOCOL2_ADDR, 1, &tmp);

  g_xStatus = S2LPSpiWriteRegisters(TIMERS3_ADDR, 2, tmpBuffer);

}


/**
 * @brief  Set the LDCR wake up timer counter. Remember that this value is incresead by one in the Twu calculation.
 *         Twu=(PRESCALER +1)*(COUNTER+1)*Tck, where Tck = 28.818 us
 * @param  cCounter value for the timer counter.
 *         This parameter must be an uint8_t.
 * @retval None.
 */
void S2LPTimerSetWakeUpTimerCounter(uint8_t cCounter)
{
  g_xStatus = S2LPSpiWriteRegisters(TIMERS2_ADDR, 1, &cCounter);
}


/**
 * @brief  Set the LDCR wake up timer prescaler. Remember that this value is incresead by one in the Twu calculation.
 *         Twu=(PRESCALER +1)*(COUNTER+1)*Tck, where Tck = 28.818 us
 * @param  cPrescaler value for the timer prescaler.
 *         This parameter must be an uint8_t.
 * @retval None.
 */
void S2LPTimerSetWakeUpTimerPrescaler(uint8_t cPrescaler)
{
  g_xStatus = S2LPSpiWriteRegisters(TIMERS3_ADDR, 1, &cPrescaler);
}


/**
 * @brief  Return the LDCR wake up timer, according to the formula: Twu=(PRESCALER +1)*(COUNTER+1)*Tck, where Tck = 28.818 us.
 * @param  pfWakeUpMsec pointer to the variable in which the wake-up time expressed in milliseconds has to be stored.
 *         This parameter must be a float*.
 * @param  pcCounter pointer to the variable in which the timer counter has to be stored.
 *         This parameter must be an uint8_t*.
 * @param  pcPrescaler pointer to the variable in which the timer prescaler has to be stored.
 *         This parameter must be an uint8_t*.
 * @retval None.
 */
void S2LPTimerGetWakeUpTimer(float* pfWakeUpMsec, uint8_t* pcCounter, uint8_t* pcPrescaler, uint8_t* pcMulti)
{
  uint8_t tmpBuffer[2], tmp;
  
  S2LPSpiReadRegisters(PROTOCOL2_ADDR, 1, &tmp);
  tmp &= LDC_TIMER_MULT_REGMASK;
  *pcMulti = tmp;

  g_xStatus = S2LPSpiReadRegisters(TIMERS3_ADDR, 2, tmpBuffer);
  *pcCounter = tmpBuffer[1];
  *pcPrescaler = tmpBuffer[0];

  S2LPTimerComputeWakeupTimerValues(pfWakeUpMsec, tmpBuffer[0], tmpBuffer[1], tmp);
}


/**
 * @brief  Set the LDCR wake up timer reloading registers with the values of
 *         COUNTER and PRESCALER according to the formula: Twu=(PRESCALER +1)*(COUNTER+1)*Tck, where
 *         Tck = 28.818 us. The minimum vale of the wakeup timeout is 28.818us (PRESCALER and
 *         COUNTER equals to 0) and the maximum value is about 1.89 s (PRESCALER anc COUNTER equals
 *         to 255).
 * @param  cCounter reload value for the timer counter.
 *         This parameter must be an uint8_t.
 * @param  cPrescaler reload value for the timer prescaler.
 *         This parameter must be an uint8_t.
 * @retval None.
 */
void S2LPTimerSetWakeUpTimerReload(uint8_t cCounter , uint8_t cPrescaler, uint8_t cMulti)
{
  uint8_t tmpBuffer[2] = {cPrescaler, cCounter}, tmp;

  S2LPSpiReadRegisters(PROTOCOL2_ADDR, 1, &tmp);
  tmp &= LDC_TIMER_MULT_REGMASK;
  tmp |= cMulti;
  S2LPSpiWriteRegisters(PROTOCOL2_ADDR, 1, &tmp);

  g_xStatus = S2LPSpiWriteRegisters(TIMERS1_ADDR, 2, tmpBuffer);
}


/**
 * @brief  Set the LDCR wake up reload timer counter and prescaler from the desired value in ms,
 *         according to the formula: Twu=(PRESCALER +1)*(COUNTER+1)*Tck, where Tck = 28.818 us.
 *         The minimum vale of the wakeup timeout is 28.818us (PRESCALER and COUNTER equals to 0)
 *         and the maximum value is about 1.89 s (PRESCALER anc COUNTER equals to 255).
 * @param  fDesiredMsec desired timer value.
 *         This parameter must be a float.
 * @retval None.
 */
void S2LPTimerSetWakeUpTimerReloadMs(float fDesiredMsec)
{
  uint8_t tmpBuffer[2], multi, tmp;

  /* Computes counter and prescaler */
  S2LPTimerComputeWakeupTimerRegValues(fDesiredMsec , &tmpBuffer[1] , &tmpBuffer[0], &multi);

  S2LPSpiReadRegisters(PROTOCOL2_ADDR, 1, &tmp);
  tmp &= ~LDC_TIMER_MULT_REGMASK;
  tmp |= multi;
  S2LPSpiWriteRegisters(PROTOCOL2_ADDR, 1, &tmp);

  g_xStatus = S2LPSpiWriteRegisters(TIMERS1_ADDR, 2, tmpBuffer);
}


/**
 * @brief  Set the LDCR wake up timer reload counter. Remember that this value is incresead by one in the Twu calculation.
 *         Twu=(PRESCALER +1)*(COUNTER+1)*Tck, where Tck = 28.818 us
 * @param  cCounter value for the timer counter.
 *         This parameter must be an uint8_t.
 * @retval None
 */
void S2LPTimerSetWakeUpTimerReloadCounter(uint8_t cCounter)
{
  g_xStatus = S2LPSpiWriteRegisters(TIMERS0_ADDR, 1, &cCounter);
}


/**
 * @brief  Set the LDCR wake up timer reload prescaler. Remember that this value is incresead by one in the Twu calculation.
 *         Twu=(PRESCALER +1)*(COUNTER+1)*Tck, where Tck = 28.818 us
 * @param  cPrescaler value for the timer prescaler.
 *         This parameter must be an uint8_t.
 * @retval None
 */
void S2LPTimerSetWakeUpTimerReloadPrescaler(uint8_t cPrescaler)
{
  g_xStatus = S2LPSpiWriteRegisters(TIMERS1_ADDR, 1, &cPrescaler);
}


/**
 * @brief  Return the LDCR wake up reload timer, according to the formula: Twu=(PRESCALER +1)*(COUNTER+1)*Tck, where Tck = 28.818 us.
 * @param  pfWakeUpReloadMsec pointer to the variable in which the wake-up reload time expressed in milliseconds has to be stored.
 *         This parameter must be a float*.
 * @param  pcCounter pointer to the variable in which the timer counter has to be stored.
 *         This parameter must be an uint8_t*.
 * @param  pcPrescaler pointer to the variable in which the timer prescaler has to be stored.
 *         This parameter must be an uint8_t*.
 * @retval None.
 */
void S2LPTimerGetWakeUpTimerReload(float* pfWakeUpReloadMsec, uint8_t* pcCounter, uint8_t* pcPrescaler, uint8_t* pcMulti)
{
  uint8_t tmpBuffer[2], tmp;
  
  S2LPSpiReadRegisters(PROTOCOL2_ADDR, 1, &tmp);
  tmp &= LDC_TIMER_MULT_REGMASK;
  *pcMulti = tmp;

  g_xStatus = S2LPSpiReadRegisters(TIMERS3_ADDR, 2, tmpBuffer);
  *pcCounter = tmpBuffer[1];
  *pcPrescaler = tmpBuffer[0];

  S2LPTimerComputeWakeupTimerValues(pfWakeUpReloadMsec, tmpBuffer[1], tmpBuffer[0], tmp);
}


/**
 * @brief  Set the RX timeout stop conditions.
 * @param  xStopCondition new stop condition.
 *         This parameter can be any value of @ref RxTimeoutStopCondition.
 * @retval None
 */
void S2LPTimerSetRxTimerStopCondition(RxTimeoutStopCondition xStopCondition)
{
  uint8_t tmp;
  s_assert_param(IS_RX_TIMEOUT_STOP_CONDITION(xStopCondition));

  S2LPSpiReadRegisters(PROTOCOL2_ADDR, 1, &tmp);
  tmp &= ~(CS_TIMEOUT_MASK_REGMASK | SQI_TIMEOUT_MASK_REGMASK | PQI_TIMEOUT_MASK_REGMASK);
  tmp |= (((uint8_t)xStopCondition) << 5);
  S2LPSpiWriteRegisters(PROTOCOL2_ADDR, 1, &tmp);

  S2LPSpiReadRegisters(PCKT_FLT_OPTIONS_ADDR, 1, &tmp);
  tmp &= ~RX_TIMEOUT_AND_OR_SEL_REGMASK;
  tmp |= (((uint8_t)xStopCondition) >> 1);
  g_xStatus = S2LPSpiWriteRegisters(PCKT_FLT_OPTIONS_ADDR, 1, &tmp);
}


/**
 * @brief  Computes and Return the RCO frequency. 
 *         This frequency depends on the xtal frequency and the XTAL bit in register 0x01.
 * @retval RCO frequency in Hz as an uint16_t.
 */
uint16_t S2LPTimerGetRcoFrequency(void)
{
  uint32_t xtal=S2LPRadioGetXtalFrequency();
  
  switch(xtal)
  {
  case 24000000:
  case 48000000:
    return 32000;
  case 25000000:
  case 50000000:
    return 33300;
  }
  return 34700;
}


/**
 * @brief  Enables the Fast RX termination timer.
 * @param  None
 * @retval None
 */
void S2LpTimerFastRxTermTimer(SFunctionalState xNewState)
{
  uint8_t tmp;
  s_assert_param(IS_SFUNCTIONAL_STATE(xNewState));

  S2LPSpiReadRegisters(PROTOCOL1_ADDR, 1, &tmp);
  if(xNewState == S_ENABLE) {
    tmp |= FAST_CS_TERM_EN_REGMASK;
  }
  else {
    tmp &= ~FAST_CS_TERM_EN_REGMASK;
  }
  g_xStatus = S2LPSpiWriteRegisters(PROTOCOL1_ADDR, 1, &tmp);  
}


/**
 * @brief  Set the Fast RX termination timer word. When the timer counter will reach this word,
 *              the timer expires.
 *             The timer counter is clocked at frequency: fCLK/24/2^CHFLT_E  
 * @param  fast_rx_word : the FAST_RX_TIMER word (register 0x54 value).
 * @retval None
 */
void S2LpSetTimerFastRxTermTimer(uint8_t fast_rx_word)
{
  g_xStatus = S2LPSpiWriteRegisters(FAST_RX_TIMER_ADDR, 1, &fast_rx_word);  
}


/**
 * @brief  Set the Fast RX termination timer word starting from a us value.
 *             The timer counter is clocked at frequency: fCLK/24/2^CHFLT_E.
 * @param  fast_rx_us : fast rx termination target value in us.
 * @retval None
 * @note: the user should care about the max value that can be set unless asserts are used.
 */
void S2LpSetTimerFastRxTermTimerUs(uint32_t fast_rx_us)
{
  uint8_t tmp,fast_rx_word;
  
  S2LPSpiReadRegisters(CHFLT_ADDR, 1, &tmp);
    
  uint32_t f_dig=S2LPRadioGetXtalFrequency();
  if(f_dig > DIG_DOMAIN_XTAL_THRESH) {
    f_dig = f_dig/2;
  }
     
  s_assert_param(fast_rx_us<(1000000*0xff)/(f_dig/24/(1<<(tmp&0x0F))));
  
  
  fast_rx_word=((f_dig/24/(1<<(tmp&0x0F)))*fast_rx_us)/1000000;
  g_xStatus = S2LPSpiWriteRegisters(FAST_RX_TIMER_ADDR, 1, &fast_rx_word);  
}


/**
 * @brief  Enables the RCO autocalibration when the device make the transition READY -> SLEEP.
 * @param  en:
 *              if it is S_ENABLE: RCO calibration is enabled.
 *              if it is S_DISABLE: RCO calibration is disabled.
 * @retval None
 */
void S2LPTimerCalibrationRco(SFunctionalState xCalibration)
{
  uint8_t tmp;
  
  s_assert_param(IS_SFUNCTIONAL_STATE(xCalibration));
  
  S2LPSpiReadRegisters(XO_RCO_CONF0_ADDR, 1, &tmp);
  
  if(xCalibration == S_ENABLE) {
    tmp |= RCO_CALIBRATION_REGMASK;
  } else {
    tmp &= ~RCO_CALIBRATION_REGMASK;
  }
  g_xStatus = S2LPSpiWriteRegisters(XO_RCO_CONF0_ADDR, 1, &tmp);  
}

/**
 * @brief  Enable the SLEEP_B mode. SLEEP_A and SLEEP_B are mutually exclusive.
 * @param  en:
 *              if it is S_ENABLE: SLEEP_B will be set
 *              if it is S_DISABLE: SLEEP_A will be set
 * @retval None
 */
void S2LPTimerSleepB(SFunctionalState en)
{
  uint8_t tmp;
  
  s_assert_param(IS_SFUNCTIONAL_STATE(en));
  
  S2LPSpiReadRegisters(PM_CONF0_ADDR, 1, &tmp);
  
  if(en == S_ENABLE) {
    tmp |= SLEEP_MODE_SEL_REGMASK;
  } else {
    tmp &= ~SLEEP_MODE_SEL_REGMASK;
  }
  g_xStatus = S2LPSpiWriteRegisters(PM_CONF0_ADDR, 1, &tmp); 
}

void S2LPTimerLdcIrqWa(SFunctionalState en)
{
  uint8_t tmp[2]={0x00,0x60};

  if(en)
  {
    tmp[0]=0x01; tmp[1]=0x64;
    
    do
    {
      S2LPRefreshStatus();
    }
    while(g_xStatus.MC_STATE!=0x7C);
  }

  g_xStatus = S2LPSpiWriteRegisters(0x7B, 1, &tmp[1]); 
  g_xStatus = S2LPSpiWriteRegisters(0x7A, 1, &tmp[0]); 
}


/**
 *@}
 */


/**
 *@}
 */


/**
 *@}
 */



/******************* (C) COPYRIGHT 2016 STMicroelectronics *****END OF FILE****/