#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "samd51.h"
#include "hal_gpio.h"

//-----------------------------------------------------------------------------
#define PERIOD_FAST    100
#define PERIOD_SLOW    500

HAL_GPIO_PIN(LED,      A, 23)
HAL_GPIO_PIN(UART_TX,  B, 17)
HAL_GPIO_PIN(UART_RX,  B, 16)


//-----------------------------------------------------------------------------
static void timer_set_period(uint16_t i)
{
  TC0->COUNT16.CC[0].reg = (F_CPU / 1000ul / 1024) * i;
  TC0->COUNT16.COUNT.reg = 0;
}


//-----------------------------------------------------------------------------
void irq_handler_tc0(void)
{
  // Check if Match Compare interrupt flag is set for CC[0]
  if (TC0->COUNT16.INTFLAG.reg & TC_INTFLAG_MC(1))
  {
    // Toggle the LED state
    HAL_GPIO_LED_toggle();
    
    // Clear the interrupt flag by writing 1 to it
    TC0->COUNT16.INTFLAG.reg = TC_INTFLAG_MC(1);
  }
}


void fault_handler(void) {
  // Handle hard fault
  while (1) {
    // Optionally, you can blink the LED or reset the device
    HAL_GPIO_LED_toggle();
    for (volatile int i = 0; i < 100000; i++);
  }
}

void *irq_handler_mm_fault = fault_handler;
void *irq_handler_nmi = fault_handler;
void *irq_handler_bus_fault = fault_handler;
void *irq_handler_usage_fault = fault_handler;


//-----------------------------------------------------------------------------
static void timer_init(void)
{
  // Enable clock for Timer/Counter 0
  MCLK->APBAMASK.reg |= MCLK_APBAMASK_TC0;

  // Configure and enable peripheral clock for TC0
  GCLK->PCHCTRL[TC0_GCLK_ID].reg = GCLK_PCHCTRL_GEN(0) | GCLK_PCHCTRL_CHEN;
  while (0 == (GCLK->PCHCTRL[TC0_GCLK_ID].reg & GCLK_PCHCTRL_CHEN));

  // Configure TC0: 16-bit mode, 1024 prescaler, resync on prescaler
  TC0->COUNT16.CTRLA.reg = TC_CTRLA_MODE_COUNT16 | TC_CTRLA_PRESCALER_DIV1024 |
      TC_CTRLA_PRESCSYNC_RESYNC;

  // Set waveform generation mode to match frequency
  TC0->COUNT16.WAVE.reg = TC_WAVE_WAVEGEN_MFRQ;

  // Reset counter to 0
  TC0->COUNT16.COUNT.reg = 0;

  // Set initial timer period to slow blink rate
  timer_set_period(PERIOD_SLOW);

  // Enable the timer
  TC0->COUNT16.CTRLA.reg |= TC_CTRLA_ENABLE;

  // Enable match compare interrupt
  TC0->COUNT16.INTENSET.reg = TC_INTENSET_MC(1);

  // Enable TC0 interrupt in NVIC
  NVIC_EnableIRQ(TC0_IRQn);
}


//-----------------------------------------------------------------------------
static void uart_init(uint32_t baud)
{
  HAL_GPIO_UART_TX_out();
  HAL_GPIO_UART_TX_pmuxen(HAL_GPIO_PMUX_C);
  HAL_GPIO_UART_RX_in();
  HAL_GPIO_UART_RX_pmuxen(HAL_GPIO_PMUX_C);

  MCLK->APBAMASK.reg |= MCLK_APBAMASK_SERCOM1;

  GCLK->PCHCTRL[SERCOM1_GCLK_ID_CORE].reg = GCLK_PCHCTRL_GEN(0) | GCLK_PCHCTRL_CHEN;
  while (0 == (GCLK->PCHCTRL[SERCOM1_GCLK_ID_CORE].reg & GCLK_PCHCTRL_CHEN));

  SERCOM1->USART.CTRLA.reg =
      SERCOM_USART_CTRLA_DORD | SERCOM_USART_CTRLA_MODE(1/*INT_CLK*/) |
      SERCOM_USART_CTRLA_RXPO(1/*PAD1*/) | SERCOM_USART_CTRLA_TXPO(0/*PAD0*/) |
      SERCOM_USART_CTRLA_SAMPR(1);

  SERCOM1->USART.CTRLB.reg = SERCOM_USART_CTRLB_RXEN | SERCOM_USART_CTRLB_TXEN |
      SERCOM_USART_CTRLB_CHSIZE(0/*8 bits*/);

  #define BAUD_VAL (F_CPU / (16 * baud))
  #define FP_VAL   ((F_CPU / baud - 16 * BAUD_VAL) / 2)

  SERCOM1->USART.BAUD.reg =
      SERCOM_USART_BAUD_FRACFP_BAUD(BAUD_VAL) |
      SERCOM_USART_BAUD_FRACFP_FP(FP_VAL);

  SERCOM1->USART.CTRLA.reg |= SERCOM_USART_CTRLA_ENABLE;
}


//-----------------------------------------------------------------------------
static void uart_putc(char c) {
  while (!(SERCOM1->USART.INTFLAG.reg & SERCOM_USART_INTFLAG_DRE));
  SERCOM1->USART.DATA.reg = c;
}


//-----------------------------------------------------------------------------
static bool uart_getc(char *c) {
  if (SERCOM1->USART.INTFLAG.reg & SERCOM_USART_INTFLAG_RXC)
  {
    *c = SERCOM1->USART.DATA.reg;
    return true;
  }

  return false;
}


//-----------------------------------------------------------------------------
static void uart_puts(char *s) {
  while (*s)
    uart_putc(*s++);
}


//-----------------------------------------------------------------------------
static char invert_case(char c) {
  if ('a' <= c && c <= 'z')
    return c + ('A' - 'a');
  else if ('A' <= c && c <= 'Z')
    return c - ('A' - 'a');
  return c;
}


//-----------------------------------------------------------------------------
static void sys_init(void) {
  // Enable external crystal oscillator (XOSC0)
  OSCCTRL->XOSCCTRL[0].reg = OSCCTRL_XOSCCTRL_ENABLE | OSCCTRL_XOSCCTRL_IMULT(4) |
      OSCCTRL_XOSCCTRL_IPTAT(3) | OSCCTRL_XOSCCTRL_STARTUP(7);

  // Configure GCLK1 to use 32kHz oscillator for PLL reference
  GCLK->GENCTRL[1].reg = GCLK_GENCTRL_SRC(GCLK_SOURCE_OSCULP32K) | GCLK_GENCTRL_RUNSTDBY | GCLK_GENCTRL_GENEN;

  // Connect GCLK1 to PLL reference clock input
  GCLK->PCHCTRL[OSCCTRL_GCLK_ID_FDPLL032K].reg = GCLK_PCHCTRL_GEN(1) | GCLK_PCHCTRL_CHEN;
  while (0 == (GCLK->PCHCTRL[OSCCTRL_GCLK_ID_FDPLL032K].reg & GCLK_PCHCTRL_CHEN));

  #define LDR (((unsigned long)F_CPU * 32) / 1000000)

  // Configure PLL multiplier ratio to achieve target frequency
  OSCCTRL->Dpll[0].DPLLRATIO.reg = OSCCTRL_DPLLRATIO_LDRFRAC(LDR % 32) |
      OSCCTRL_DPLLRATIO_LDR((LDR / 32) - 1);
  // Configure PLL control: use XOSC0 as reference, set divider
  OSCCTRL->Dpll[0].DPLLCTRLB.reg = OSCCTRL_DPLLCTRLB_REFCLK_XOSC0 |
      OSCCTRL_DPLLCTRLB_DIV(5) | OSCCTRL_DPLLCTRLB_WUF | OSCCTRL_DPLLCTRLB_LBYPASS;
  // Enable PLL
  OSCCTRL->Dpll[0].DPLLCTRLA.reg = OSCCTRL_DPLLCTRLA_ENABLE | OSCCTRL_DPLLCTRLA_RUNSTDBY;

  // Wait for PLL to lock and be ready
  while (0 == OSCCTRL->Dpll[0].DPLLSTATUS.bit.CLKRDY || 0 == OSCCTRL->Dpll[0].DPLLSTATUS.bit.LOCK);

  // Switch main clock (GCLK0) to use PLL output
  GCLK->GENCTRL[0].reg = GCLK_GENCTRL_SRC(GCLK_SOURCE_DPLL0) |
      GCLK_GENCTRL_RUNSTDBY | GCLK_GENCTRL_GENEN;
}


//-----------------------------------------------------------------------------
int main(void) {
  char rxch;

  sys_init();
  timer_init();
  uart_init(115200);

  HAL_GPIO_LED_out();
  HAL_GPIO_LED_clr();

  uart_puts("\r\nHello, world!\r\n");

  while (1) {
    if (uart_getc(&rxch)) {
      uart_putc(invert_case(rxch));
    }
  }

  return 0;
}

