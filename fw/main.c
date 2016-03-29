////dmx_repiter test ////crazzypol@gmail.com
////reciever and transmitter dmx512 interface
////timeout disable transmit 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 sec
////PORTC pin 0-3 switch timeout
////PORTB pin 5 LED
////PORTD pin 7 connect to the ground in shematic
//CPU FOSC = 8000000
//USART BAUD = 250000

//variables
unsigned char usartRxBuf;         //temporary rx byte
unsigned char Dmx[512];           //Dmx array
unsigned char dip;
unsigned int RxByteCount, TxByteCount;         //counter input and output data
unsigned int TimerCount, RxTimeout;         //counter timeout 25ms tick
unsigned char EnableTransmit;     //flag enabled transmission
//unsigned int LedCount;            //counter timeout led light

//enumerates
enum {
  RxIdle,
  RxBreak,
  RxData
} RxState;

enum {
  TxIdle,
  TxStart,
  TxData
} TxState;

//declare functions
void SetPins();
void Timer0_init();
void InitTimer1();
void StartDMX();
unsigned int GetDelaymSec();

//interrupt functions
void Timer0Overflow_ISR() iv IVT_ADDR_TIMER0_OVF {
  TCNT0 = 60;
  TimerCount++;
  if (TimerCount>=35*dip+4){
     EnableTransmit = 0;
     TimerCount = 0;
     TxState = TxIdle;
     PORTB.B5 = 0;
  }
  if (RxTimeout>0){
     RxTimeout--;
  }
  if (RxTimeout==0){
     RxState = RxIdle;
  }
}

void Timer1Overflow_ISR() org IVT_ADDR_TIMER1_COMPA {
  OCR1AH = 0xC3;
  OCR1AL = 0x4F;
  if (TxState==TxIdle){
     if (EnableTransmit==1) {
          StartDMX(); //start transmitter
     }
  }
}

void Usart_RXC_ISR() iv IVT_ADDR_USART__RXC {
  if (UCSRA&(1<<FE)){
    usartRxBuf = UDR;
    RxState = RxBreak;
  }else{
    usartRxBuf = UDR;
    if (RxState != RxIdle){
       if (RxState == RxData){
          if (RxByteCount<512){
             Dmx[RxByteCount] = usartRxBuf;
             RxByteCount++;
          }else{

             RxState = RxIdle;
          }
       }else{
             TimerCount = 0;
             EnableTransmit = 1;
             RxByteCount = 0;
             RxTimeout = 48;
          if(usartRxBuf == 0)
          {
             RxState = RxData;
          }
          else{
             RxState = RxIdle;
          }
       }
    }
  }
}

void Usart_TXC_ISR() iv IVT_ADDR_USART__TXC {
  if (TxByteCount<512)
  {
    UDR = Dmx[TxByteCount];
    TxByteCount++;
  }else{
    UCSRB &= ~(0<<TXCIE);
    UCSRB &= ~(0<<TXEN);
    TxState = TxIdle;
  }

}

//Timer1 Prescaler = 8; Preload = 49999; Actual Interrupt Time = 50 ms
void InitTimer1(){
  SREG_I_bit = 1;
  TCCR1A = 0x80;
  TCCR1B = 0x0A;
  OCR1AH = 0xC3;
  OCR1AL = 0x4F;
  OCIE1A_bit = 1;
}

//init function
void Timer0_init(){
  TCCR0 = (1<<CS12)|(0<<CS11)|(1<<CS10); // divider FOSC/1024
  TIMSK = (1<<TOIE0);                    // enabled timer interrupt
  TCNT0 = 60;                            // start timer TCNT0 (one tick every 25 ms)
}

void SetPins(){
  DDRC = 0xF0;   //pins C0-C3 input, C4-C6 output
  DDRD = 0x7E;   //pin D0-D6 output D7 input (D7 connecting to the ground in shematic)
  DDRB = 0x3F;   //pin B0-B5 output
  PORTC = 0xFF;  //all out pins high level
  PORTB = 0xDF;  //pin B5 low level all other out pins high level
  PORTD = 0x7F;  //all out pins high level
}

//main function
void main() {
  TxState = TxIdle;
  RxState = RxIdle;
  SetPins();
  Timer0_init();
  InitTimer1();
  UART1_Init_Advanced(250000, _UART_NOPARITY, _UART_TWO_STOPBITS);
  Delay_ms(70);
  RxByteCount=0;
  while (RxByteCount<512){
     Dmx[RxByteCount]=0;
     RxByteCount++;
  }
  dip =  (~(PINC|0xF0));
  RxByteCount=0;
  EnableTransmit = 0;
  TxByteCount = 0;
  RxTimeout = 48;
  
  UCSRB = (1<<RXEN) | (1<<RXCIE);      //enable rx interrupt
  SREG = (1<<SREG_I);   //enable all interrupts


  while(1){
    Delay_ms(150);
    dip = ~(PINC|0xF0);
  }
}


//other functions
void StartDMX(){
  PORTB.B5 = 1;
  TxByteCount = 0;
  UCSRB &= ~(1<<TXEN);// | (0<<TXCIE);
  PORTD.B1 = 0;
  Delay_us(90);
  PORTD.B1 = 1;
  Delay_us(40);
  UCSRB |= (1<<TXEN) | (1<<TXCIE);
  UDR = 0;                            //starting first byte is always 0
  TxState = TxData;                   //state transmitter for transmit data
}