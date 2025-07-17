#include <unistd.h>
#include <samd51j19a.h>

#include "hw-abstract/hal_gpio.h"

#define PERIOD 100

HAL_GPIO_PIN(LED,      A, 23)
HAL_GPIO_PIN(UART_TX,  B, 16)
HAL_GPIO_PIN(UART_RX,  B, 17)


static void timer_set_period(uint16_t i) {
  TC0->COUNT16.CC[0].reg = (F_CPU / 1000ul / 1024) * i;
  TC0->COUNT16.COUNT.reg = 0;
}


void irq_handler_tc0(void) {
  if (TC0->COUNT16.INTFLAG.reg & TC_INTFLAG_MC(1)) {
    HAL_GPIO_LED_toggle();
    TC0->COUNT16.INTFLAG.reg = TC_INTFLAG_MC(1);
  }
}


static void timer_init(void) {
  MCLK->APBAMASK.reg |= MCLK_APBAMASK_TC0;

  GCLK->PCHCTRL[TC0_GCLK_ID].reg = GCLK_PCHCTRL_GEN(0) | GCLK_PCHCTRL_CHEN;
  OSCCTRL->XOSCCTRL[0].reg = OSCCTRL_XOSCCTRL_ENABLE | OSCCTRL_XOSCCTRL_XTALEN |
    OSCCTRL_XOSCCTRL_IMULT(4) | OSCCTRL_XOSCCTRL_IPTAT(3) | OSCCTRL_XOSCCTRL_STARTUP(7);

  while (0 == (OSCCTRL->STATUS.reg & OSCCTRL_STATUS_XOSCRDY(1))); // Wait for XOSC0 to be ready

  TC0->COUNT16.CTRLA.reg = TC_CTRLA_MODE_COUNT16 | TC_CTRLA_PRESCALER_DIV1024 |
      TC_CTRLA_PRESCSYNC_RESYNC;

  TC0->COUNT16.WAVE.reg = TC_WAVE_WAVEGEN_MFRQ;

  TC0->COUNT16.COUNT.reg = 0;

  timer_set_period(PERIOD);

  TC0->COUNT16.CTRLA.reg |= TC_CTRLA_ENABLE;

  TC0->COUNT16.INTENSET.reg = TC_INTENSET_MC(1);

  NVIC_EnableIRQ(TC0_IRQn);
}

//-----------------------------------------------------------------------------
static void sys_init(void) {
   // Set NVM Wait States to 5 for 120MHz operation.
  // We write the value 5 to the RWS (Read Wait States) bits, which are bits 1-4
  // of the NVMCTRL CTRLB register.
  NVMCTRL->CTRLB.reg = (NVMCTRL->CTRLB.reg & ~0x1E) | (5 << 1);

  // Configure GCLK1 to use the 32kHz oscillator as a source for the DPLL.
  GCLK->GENCTRL[1].reg = GCLK_GENCTRL_SRC_OSCULP32K | GCLK_GENCTRL_GENEN;
  while (GCLK->SYNCBUSY.reg & (1 << 3)) {}; // Wait for GCLK1 to sync (ID 3)

  // Configure the DPLL to use GCLK1 as the reference clock.
  // The GCLK ID for DPLL0 is 1.
  GCLK->PCHCTRL[1].reg = GCLK_PCHCTRL_GEN_GCLK1 | GCLK_PCHCTRL_CHEN;
  while (GCLK->SYNCBUSY.reg & (1 << (12 + 1))) {}; // Wait for PCHCTRL[1] to sync

  // Configure the DPLL for 120MHz operation
  OSCCTRL->Dpll[0].DPLLRATIO.reg = OSCCTRL_DPLLRATIO_LDR(3661);

  // Enable the DPLL
  OSCCTRL->Dpll[0].DPLLCTRLB.reg = OSCCTRL_DPLLCTRLB_REFCLK_GCLK;
  OSCCTRL->Dpll[0].DPLLCTRLA.reg = OSCCTRL_DPLLCTRLA_ENABLE;

  // Wait for the DPLL to lock
  while (!(OSCCTRL->Dpll[0].DPLLSTATUS.reg & OSCCTRL_DPLLSTATUS_LOCK)) {};

  // Configure GCLK0 (Main Clock) to use the 120MHz DPLL as its source.
  GCLK->GENCTRL[0].reg = GCLK_GENCTRL_SRC_DPLL0 | GCLK_GENCTRL_GENEN;
  while (GCLK->SYNCBUSY.reg & (1 << 2)) {}; // Wait for GCLK0 to sync (ID 2)
}


int main(void) {
  sys_init();
  timer_init();

  HAL_GPIO_LED_out();
  HAL_GPIO_LED_clr();

  while (1) {}

  return 0;
}
