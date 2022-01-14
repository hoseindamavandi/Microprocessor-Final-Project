#include <LPC17xx.h>
#include <math.h>


////////////////////////Timer
//T = 15/(1/25MHz) = 15*25*10^6     (every 15sec)
#define Value_MR0 375000000
#define PR0_Value 0

////////////////////////PWM
//FCPU = 100MHz , Fpclk = 100MHz/4 = 25MHz
//T = 1/f = 1/50 = 0.2s = 20ms/(1/25MHz)=20ms*25MHz = 500000
#define PMR0_Value  500000;       //20ms

char KeyIn;
_Bool Keyflag;
int TCRflag = 1;

/*
int HighMin = 0.4*PMR0_Value;   		 //40%      8ms
int LowMin = 0.6*PMR0_Value;         //60%      12ms
int HighMax = 0.9*PMR0_Value; 		  //90%      18ms
int LowMax = 0.1*PMR0_Value;        //10%      2ms
*/

int HighMin = 0;		//baray MR1 va MR2 
int LowMin = 0;
int HighMax = 0;
int LowMax = 0;
int flagkeyMax = 0;		//flag hay barrasi min va  max 
int flagkeyMin = 0;

double V_ref = 3.3;
int temperture;
double V_dig;
double partition;
	
////////////////////////////////////////////////////////////////////////////////

void Init_ADC()
{
	LPC_PINCON->PINSEL3 = (0x3<<30);        //select AD5 as input    port1.31
	LPC_SC->PCONP |= (1<<12);               //power on A2D
	LPC_ADC->ADCR = (1<<5)|(3<<8)|(1<<21);  //sel chn & ClkDiv & power on(PDN)
	LPC_ADC->ADCR |= (1<<24);               //manual start of conversion
}

////////////////////////////////////////////////////////////////////////////////

void Init_PWM()
{
	LPC_PINCON->PINSEL4 |= (1<<2);   //PWM1.2    port2.1	
	LPC_PWM1->MR0 = PMR0_Value;
	HighMax = 0.5*PMR0_Value;			//halat deafult dar nazar darim 
	LowMax = 0.5*PMR0_Value;
	LPC_PWM1->MR1=LowMax/2;
	LPC_PWM1->MR2 = LowMax/2+HighMax;
	//LPC_PWM1->MR1 = PMR1_Value/2;
	//LPC_PWM1->MR2 = PMR1_Value/2+MR2_Value;
	LPC_PWM1->MCR = 0x02;                  		//reset counter when equal to MR0
	LPC_PWM1->PCR = (1<<2)|(1<<10); 					//enable PWM output
	LPC_PWM1->LER = (1<<0)|(1<<1)|(1<<2);  		//update shadow reg MR0,MR1,MR2      0000 0000 0000 0111
	
	LPC_PWM1->TCR = 9;                     //Enable counter & PWM1
	
}

////////////////////////////////////////////////////////////////////////////////

void TIMER0_IRQHandler (void)
{
	LPC_TIM0->IR = 1; //Clear interrupt flag
	
	LPC_GPIO2->FIOPIN = ((LPC_ADC->ADDR5)>>4) & 0xfff;
	//V_dig = ((LPC_ADC->ADDR5)>>4) & 0xfff;
  //partition = (pow(2,12))-1;
	//temperture = (V_dig*V_ref*100)/partition;
	
	LPC_ADC->ADCR |= 0x01000000;
}

////////////////////////////////////////////////////////////////////////////////

void InitialTimer0(void)
{
	//LPC_PINCON->PINSEL3 |= (0x3<<24); //port1.28 as Match out 0
	LPC_TIM0->PR = PR0_Value;
	LPC_TIM0->MR0 = Value_MR0;
	LPC_TIM0->MCR = 0x3;				//Interrupt and reset on MR0
	LPC_TIM0->EMR = 0x30;				//bit 4 va 5 =1 toggle match register 0 
	NVIC_EnableIRQ(TIMER0_IRQn);
	LPC_TIM0->TCR = 1;    //enable timer0
}
////////////////////////////////////////////////////////////////////////////////

void keyBoard()			// barrsi zadn kelid 
{
	KeyIn = LPC_GPIO0->FIOPIN&0x3e;
	
	if(KeyIn == 0x3e)
	{
		Keyflag = 0;
		return;
	}
	
	if(Keyflag == 1)			//key ghabln zade shode ya na 
	{
		Keyflag = 0x3e;
		return;
	}	
}

////////////////////////////////////////////////////////////////////////////////

void Manual(char Key)		//  00(11 110)0  ---> bit 1 =0 bashe : manual 
{
	LPC_TIM0->TCR = 0;
	//111110
	if((Key&0x4) == 0)		// power off = 00(11 100)0 ---> bit 2 = 0 bashe : off 
	{	
		LPC_PWM1->MR1=  PMR0_Value;
		LPC_PWM1->TCR = 0;
		TCRflag = 0;
	}

		//if(((Key&0x10) != 0))
	if((Key&0x4) != 0)				// power on = 00(11 110)0 ---> bit 2 = 1 bashe : on
	{
		LPC_PWM1->TCR = 0;
		LPC_PWM1->TCR = 9;
		TCRflag=1;
	}
	
	
	if(TCRflag == 1){					// min(40%) = 00(11 010)0 ---> bit 3 = 0 bashe : on
	if(((Key&0x8) == 0))
	{
		LPC_PWM1->TCR = 0;
		LPC_PWM1->TCR = 9;
		HighMin = 0.4*PMR0_Value;
		LowMin = 0.6*PMR0_Value;
		LPC_PWM1->MR1=  LowMin/2;
		LPC_PWM1->MR2 = LowMin/2+HighMin;
		//NVIC_EnableIRQ(PWM1_IRQn);
		LPC_PWM1->LER = 0x3;
		flagkeyMin = 1;
		flagkeyMax = 0;
	}
}

	if(TCRflag == 1){					// max(90%) = 00(10 110)0 ---> bit 4 = 0 bashe : on
	if(((Key&0x10) == 0))
	{
		LPC_PWM1->TCR = 0;
		LPC_PWM1->TCR = 9;
		LPC_PWM1->MCR = 0x02;
		HighMax = 0.9*PMR0_Value;
		LowMax = 0.1*PMR0_Value;
		LPC_PWM1->MR1 = LowMax/2;
		LPC_PWM1->MR2 = LowMax/2+HighMax;
		LPC_PWM1->LER = 0x3;
		flagkeyMax = 1;
		flagkeyMin = 0;
	}
}
	
	if(TCRflag == 1){					// ferecuency (50Hz) = 00(01 110)0 ---> bit 5 = 0 bashe : on
	if(((Key&0x20) == 0))
	{
		LPC_PWM1->TCR = 0;
		LPC_PWM1->TCR = 9;
		LPC_PWM1->MCR = 0x02;
		
		//FCPU = 1MHz , Fpclk = 100MHz/4 = 25MHz
		//T = 1/f = 1/40 = 0.25s = 25ms/(1/25MHz)=25ms*25MHz = 625000
		
		if(flagkeyMin == 1){
		LPC_PWM1->MR0 = 625000;
		HighMin = 0.4*625000;
		LowMin = 0.6*625000;
		LPC_PWM1->MR1=  LowMin/2;
		LPC_PWM1->MR2 = LowMin/2+HighMin;
		LPC_PWM1->LER = 0x7;
		}
		if(flagkeyMax == 1){
		LPC_PWM1->MR0 = 625000;			//23ms
		HighMax = 0.9*625000;
		LowMax = 0.1*625000;
		LPC_PWM1->MR1 = LowMax/2;
		LPC_PWM1->MR2 = LowMax/2+HighMax;
		LPC_PWM1->LER = 0x7;
		}
	}	
}	
}
////////////////////////////////////////////////////////////////////////////////

void Init()
{
	Init_PWM();
	Init_ADC();
	InitialTimer0();
} 

/////////////////////////////////////////////////////////////////////////////////
/*
int Get_Temp(int raw_temp ) {
	
  float voltage = (raw_temp * 5.0 / 1024) * 100; // Calculate temp based on raw reading
///float F_voltage  = (voltage  * 9 / 5) + 32 ; // C to F
  return voltage;
	
}
*/
////////////////////////////////////////////////////////////////////////////////

void Automatic(){   // halat auotomatic 00(11 111)0  bit 1 = 1 : auto
	
	
	LPC_TIM0->TCR = 1;
	
	//LPC_GPIO1->FIODIRL = ((LPC_ADC->ADDR5)>>4) & 0xfff;
	V_dig = ((LPC_ADC->ADDR5)>>4) & 0xfff;
//	V_dig = LPC_GPIO1->FIOPIN;
  partition = (pow(2,12))-1;
	temperture = (V_dig*V_ref*100)/partition;
	
	//temperture = Get_Temp(LPC_GPIO2->FIOPIN);
	
	
	//temperture=32;
	
	if(temperture<26)
			LPC_PWM1->TCR = 0;
	
		if(26<=temperture && temperture<30)
		{
			LPC_PWM1->TCR = 0;
			LPC_PWM1->TCR = 9;
			HighMin = 0.4*PMR0_Value;  	// 8ms
			LowMin = 0.6*PMR0_Value;
			LPC_PWM1->MR1=  LowMin/2;
			LPC_PWM1->MR2 = LowMin/2+HighMin;
			LPC_PWM1->LER = 0x3;
		}
		if (30<=temperture && temperture<34)
		{
			LPC_PWM1->TCR = 0;
			LPC_PWM1->TCR = 9;
			HighMin = 0.6*PMR0_Value;			//0.4*PMR0_Value + 0.2*PMR0_Value   12ms
			LowMin = 0.4*PMR0_Value;			//06*PMR0_Value - 0.2*PMR0_Value
			LPC_PWM1->MR1=  LowMin/2;
			LPC_PWM1->MR2 = LowMin/2+HighMin;
			LPC_PWM1->LER = 0x3;
			
		}
		/*
		else if (34<=temperture && temperture<38)
		{
			LPC_PWM1->TCR = 0;
			LPC_PWM1->TCR = 9;
			HighMin = 0.8*PMR0_Value;			//0.6*PMR0_Value + 0.2*PMR0_Value
			LowMin = 0.2*PMR0_Value;			//04*PMR0_Value - 0.2*PMR0_Value
			LPC_PWM1->MR1=  LowMin/2;
			LPC_PWM1->MR2 = LowMin/2+HighMin;
			LPC_PWM1->LER = 0x3;
		}
		*/
		
}

////////////////////////////////////////////////////////////////////////////////

void Select_Mode()  	// manual or automatic
{

  if(((KeyIn & 0x2)) == 0)  
    Manual(KeyIn);
	else
		Automatic();
}

////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////
int main(void)
{
	LPC_GPIO2->FIODIR=0xffff;

	Init();
	while(1)
	{
		keyBoard();
		Select_Mode();
	}
}