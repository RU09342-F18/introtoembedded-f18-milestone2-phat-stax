/* Authors: Nick Scamardi & Nick Setaro
 * Written: November 20, 2018
 * Last Update: November 30, 2018
 */

#include <msp430.h>
#include <math.h>

// Variable Initializations
float ADC;
float ADC_Voltage, temp;
volatile int target = 40;   // Initial Target Temperature
volatile float difference;    // Difference between desired temperature and current temperature detected by the PTAT

// PWM configuration using Timer
void initializePWM() {
   P1DIR |= BIT2;   // Sets P1.2 as output
   P1SEL |= BIT2;   // Enables Timer output on P1.2 (Used for PWM)
   TA0CTL = TASSEL_2 | MC_1 | TACLR;    // TimerA0 Control: SMCLK, Up-mode, Clear
   TA0CCR0 = 255;   // Maximum PWM (Full speed)
   TA0CCR1 = 50;    // Initialize Fan PWM;
   TA0CCTL1 = OUTMOD_7; // Run Timer in Set/Reset mode
}

// UART Configuration
void initializeUART() {
   P4SEL |= BIT5 + BIT4;    // Enables RX and TX buffer
   UCA1CTL1 |= UCSWRST;     // Software reset enable
   UCA1CTL1 |= UCSSEL_1;    // Sets USCI Clock source to use ACLK
   UCA1BR0 = 3;             // 9600 BAUD
   UCA1BR1 = 0;             // 9600 BAUD
   UCA1MCTL |= UCBRS_3 | UCBRF_0;   // First and second stage modulation for higher accuracy baud rate
   UCA1CTL1 &= ~UCSWRST;
   UCA1IE |= UCTXIE;                // Enables Transfer buffer interrupt
   UCA1IE |= UCRXIE;                // Enables Receiver buffer interrupt
}

// ADC Configuration
void initializeADC() {
   P6DIR &= ~BIT0;  // Sets P6.0 to input direction
   P6SEL |= BIT0;   // Sets P6.0 as input for ADC12_A sample and conversion
   ADC12CTL2 = ADC12RES_2;  // AD12_A resolution set to 12-bit
   ADC12CTL1 = ADC12SHP;    // ADC12_A sample-and-hold pulse-mode select

   // ADC12_A Control Register 0 - 1024 cycles in a sampling period - Auto Trigger - Ref Volt off - Conversion overflow enable - Conversion enable - Start Sampling
   ADC12CTL0 = ADC12SHT1_15 | ADC12SHT0_15 | ADC12MSC | ADC12ON | ADC12TOVIE | ADC12ENC | ADC12SC;
   ADC12IE = ADC12IE0;  // Enable interrupt on ADC12
   ADC12IFG &= ~ADC12IFG0;  // Clears ADC12 interrupt flag
}

int main(void)
{
   UCSCTL4 = SELA_0;    // Enables UART ACLK (32.768 kHz signal)
   WDTCTL = WDTPW | WDTHOLD;    // Disables the watchdog timer

   // Call configuration functions defined above
   initializePWM();
   initializeUART();
   initializeADC();

   __bis_SR_register(GIE);  // Enables Global Interrupt - ADC/UART interrupt support
   __no_operation();    // No operation
}

// ADC12 Interrupt Service Routine
#pragma vector=ADC12_VECTOR
__interrupt void newADC(void)
{
   switch (ADC12IV)
   {
   case 6:
   {
        // Temperature calculations
        ADC = ADC12MEM0;    // Sets the digital voltage value to float ADC for math
        ADC_Voltage = (ADC/4095)*3.3;    // Converts the value in ADC12MEM0 to Voltage
        temp = ((ADC_Voltage-.5)/.01)-5;  // Converts the voltage from PTAT to Temperature reading (Celsius)
        UCA1TXBUF = (int) temp;          // Transmit the temperature in Celsius to the UART Transmit Buffer
        difference = temp - target;            // Difference between detected temperature and target temperature of PTAT
        volatile float PC = difference * 70;    // Proportion control constant

        // Proportion control functions for PWM of Fan
              if (PC == 0)
              {
                  TA0CCR1 = 0;
              }

              else if (PC >= 255)
              {
                  TA0CCR1 = 255;
              }

              else if (PC < 0)
              {
                  TA0CCR1 = 0;
              }
              else
              {
                  TA0CCR1 = PC;
              }
                break;
   }
   default:// Do nothing
   }
}

// UART interrupt service routine
#pragma vector = USCI_A1_VECTOR
__interrupt void USCI_A1_ISR(void)
{
          target = UCA1RXBUF;   // Receives value from UART to set new target temperature
          __delay_cycles(1000);
          ADC12CTL0 |= ADC12SC; // Begin ADC conversion sampling
}
