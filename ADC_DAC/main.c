#include <msp430.h>

float slope = -0.002794;
float intercept = 54.8924;
float temp;
float target=60;
float diff;
char space = ' ';
float adcFlag;

float volt, ADC_Voltage, temp;

void initializeGPIO() {
        P1DIR |= BIT2;
        P1SEL |= BIT2;             //set port 1 for output
}
void initializeTimer() {

        TA0CTL = TACLR;
        TA0CTL = TASSEL_2 + MC_1 + ID_0;
        TA0CCR0 = 255;                        //set period
        TA0CCR1 = 0;                         //set duty cycle
        TA0CCTL1 = OUTMOD_7; // Sets TACCR1 to toggle
}
void initializeUART() {
      UCA1CTL1 |= UCSWRST;        // Clears the UART control register 1
      UCA1CTL1 |= UCSSEL_2;       // Sets SMCLK
      UCA1BR0 = 104;              // For baud rate of 9600
      UCA1BR1 = 0;                // For baud rate of 9600

      UCA1MCTL = UCBRS_2;              // set modulation pattern to high on bit 1 & 5
      UCA1CTL1 &= ~UCSWRST;            // initialize USCI
      UCA1IE |= UCRXIE;                // enable USCI_A1 RX interrupt

      P4SEL |= BIT4+BIT5;
      P3SEL |= BIT3;                 // UART TX pin 3.3
      P3SEL |= BIT4;                // UART RX pin 3.4
}

void initializeADC(){
    P6SEL |= 0x01;                            // Enable A/D channel A0

    ADC12CTL0 = ADC12ON+ADC12MSC+ADC12SHT0_8; // Turn on ADC12, extend sampling time to avoid overflow of results
    ADC12CTL1 = ADC12SHP+ADC12CONSEQ_3+ADC12DIV_7 ;       // Use sampling timer, repeated sequence
    ADC12MCTL0 = ADC12INCH_0;                 // ref+=AVcc, channel = A0
    ADC12IE = 0x01;                           // Enable ADC12IFG.1
    ADC12CTL0 |= ADC12ENC;                    // Enable conversions
}

int main(void)
{
  WDTCTL = WDTPW+WDTHOLD;                   // Stop watchdog timer
  initializeADC();
  initializeUART();
  initializeTimer();
  initializeGPIO();
  __bis_SR_register(LPM0_bits + GIE);       // Enter LPM4, Enable interrupts
  __no_operation();
}

#pragma vector=ADC12_VECTOR
__interrupt void ADC12ISR (void)
{

    volt = ADC12MEM0;
    ADC_Voltage = (volt/4095) * 3.3;
    temp = ((ADC_Voltage-.5)/.01);
    UCA1TXBUF = temp;
    diff = temp - target;
    volatile float P = diff*85;
    if (P == 0)
                 {
                     TA0CCR1 = 0;
                 }

                 else if (P >= 255)
                 {
                     TA0CCR1 = 255;
                 }

                 else if (P < 0)
                 {
                     TA0CCR1 = 0;
                 }
                 else
                 {
                     TA0CCR1 = P;
                 }
    __delay_cycles(100);
    ADC12IV = 0;
}

#pragma vector=USCI_A1_VECTOR
__interrupt void USCI_A1_ISR(void)
{
    target = UCA1RXBUF;
    //__delay_cycles(1000);
    ADC12CTL0 |= ADC12SC;
    P1OUT ^= BIT0;
}
