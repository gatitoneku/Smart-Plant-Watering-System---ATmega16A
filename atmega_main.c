/*****************************************************
 This program was produced by the
 CodeWizardAVR V2.05.3 Standard
 Automatic Program Generator
 � Copyright 1998-2011 Pavel Haiduc, HP InfoTech s.r.l.
 http://www.hpinfotech.com
 
 Project :
 Version :
 Date    : 18/12/2016
 Author  : M Yusuf Irfan
 Company : Microsoft
 Comments:
 
 
 Chip type               : ATmega16A
 Program type            : Application
 AVR Core Clock frequency: 12,000000 MHz
 Memory model            : Small
 External RAM size       : 0
 Data Stack size         : 256
 *****************************************************/

#include <mega16a.h>
#include <stdint.h>
#include <delay.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// 1 Wire Bus interface functions
#include <1wire.h>

// DS1820 Temperature Sensor functions
#include <ds1820.h>
#include <delay.h>

// 1 Wire Bus interface functions
#include <1wire.h>

// Alphanumeric LCD functions
#include <alcd.h>

#define ADC_VREF_TYPE 0x00
#define DHT11_PIN 6
uint8_t c=0,I_RH,D_RH,I_Temp,D_Temp,CheckSum;
char data[16];

#define F_CPU 1000000

// Read the AD conversion result
unsigned int read_adc(unsigned char adc_input)
{
    ADMUX=adc_input | (ADC_VREF_TYPE & 0xff);
    // Delay needed for the stabilization of the ADC input voltage
    delay_us(10);
    // Start the AD conversion
    ADCSRA|=0x40;
    // Wait for the AD conversion to complete
    while ((ADCSRA & 0x10)==0);
    ADCSRA|=0x10;
    return ADCW;
}

// Declare your global variables here
#define RL_VALUE                (10)    //resistansi dalam kilo ohm
#define RO_CLEAN_AIR_FACTOR   (9.83)    //sensor udara bersih/Ro berdasarkan datasheet
#define LPG                      (0)    //no identitas LPG
#define SMOKE                    (1)    //no identitas Asap
float LPGCurve[3] = {2.3,0.20,-0.45};               //nilai yang akan diambil berdasarkan datasheet
float SmokeCurve[3] = {2.3,0.53,-0.43};             //sama dengan LPG
float Ro = 10;                                      //inisialisasi resistansi Ro menjadi 10 kilo ohm
int GetPercentage(float rs_ro_ratio, float *curve);
int GetGasPercentage(float rs_ro_ratio, int gas_id);
float ReadSensor();
float ResistanceCalculation(int raw_adc);
float SensorCalibration();
unsigned char kirim[150];
char data[16];
int gas = 0;
int i;
float soil;
float soilMoisture;
float soilMoistureRaw;


//mengirimkan pulse untuk merequest data menjadikan DHT11 sebagai output
void Request()
{
    DDRD |= (1<<DHT11_PIN);
    PORTD &= ~(1<<DHT11_PIN);
    delay_ms(20);
    PORTD |= (1<<DHT11_PIN);
}

//respon DHT11 dan menjadikannya sebagai output
void Response()
{
    DDRD &= ~(1<<DHT11_PIN);
    while(PIND & (1<<DHT11_PIN));
    while((PIND & (1<<DHT11_PIN))==0);
    while(PIND & (1<<DHT11_PIN));
}

//mengambil data dari DHT11 per 8 bit
uint8_t Receive_data()
{
    int q;
    for (q=0; q<8; q++)
    {
        while((PIND & (1<<DHT11_PIN)) == 0);
        delay_us(30);
        if(PIND & (1<<DHT11_PIN))
            c = (c<<1)|(0x01);
        else
            c = (c<<1);
        while(PIND & (1<<DHT11_PIN));
    }
    return c;
}

//kalkulasi resistansi berdasarkan resistor yang telah dibagi oleh voltage divider yang dapat dilihat dalam datasheet
float ResistanceCalculation(int raw_adc)
{
    return ( ((float)RL_VALUE*(1023-raw_adc)/raw_adc));
}

//sebelum mengambil data sensor gas harus dikalibrasikan dengan keadaan ruangan
float SensorCalibration()
{
    int i;
    float val=0;
    
    for (i=0;i<50;i++) {
        
        val += ResistanceCalculation(read_adc(0));
        delay_ms(500);
    }
    val = val/50;
    val = val/RO_CLEAN_AIR_FACTOR;
    
    return val;
}

//read sensor gas dengan adc dan kalkulasi resistansi
float ReadSensor()
{
    int i;
    float rs=0;
    
    for (i=0;i<5;i++) {
        rs += ResistanceCalculation(read_adc(0));
        delay_ms(50);
    }
    
    rs = rs/5;
    
    return rs;
}

//untuk mengambil persentasi dari gas atau smoke tetapi yang smoke tidak dipakai
int GetGasPercentage(float rs_ro_ratio, int gas_id)
{
    if ( gas_id == LPG ) {
        return GetPercentage(rs_ro_ratio,LPGCurve);
        
    } else if ( gas_id == SMOKE ) {
        return GetPercentage(rs_ro_ratio,SmokeCurve);
    }
    
    return 0;
}

//perhitungan untuk mendapatkan persentasi dari gas berdasarkan datasheet
int  GetPercentage(float rs_ro_ratio, float *curve)
{
    return (pow(10,( ((log(rs_ro_ratio)-curve[1])/curve[2]) + curve[0])));
    
}

void uart_transmit (unsigned char data)
{
    while (!( UCSRA & (1<<UDRE)));                // wait while register is free
    UDR = data;                                   // load data in the register
}

void send_string(char s[])
{
    int i =0;
    
    while (s[i] != 0x00)
    {
        uart_transmit(s[i]);
        i++;
    }
}

void sendData(int sensor){
    //PORTC=0x01;
    sprintf(kirim," %d",sensor);
    send_string(kirim);
    //delay_ms(50);
    //PORTC=0x00;
    delay_ms(500);
}

void startUp(){
    delay_ms(10000);
    sprintf(kirim,"+");
    send_string(kirim);
    //PORTC=0x01;
    //delay_ms(50);
    //PORTC=0x00;
    delay_ms(500);
}


void main(void)
{
    // Declare your local variables here
    
    // Input/Output Ports initialization
    // Port A initialization
    // Func7=In Func6=In Func5=In Func4=In Func3=In Func2=In Func1=In Func0=In
    // State7=T State6=T State5=T State4=T State3=T State2=T State1=T State0=T
    PORTA=0x02;
    DDRA=0x02;
    
    // Port B initialization
    // Func7=In Func6=In Func5=In Func4=In Func3=In Func2=In Func1=In Func0=In
    // State7=T State6=T State5=T State4=T State3=T State2=T State1=T State0=T
    PORTB=0x00;
    DDRB=0x00;
    
    // Port C initialization
    // Func7=In Func6=In Func5=In Func4=In Func3=In Func2=In Func1=In Func0=In
    // State7=T State6=T State5=T State4=T State3=T State2=T State1=T State0=T
    PORTC=0x0;
    DDRC=0x00;
    
    // Port D initialization
    // Func7=In Func6=In Func5=In Func4=In Func3=In Func2=In Func1=In Func0=In
    // State7=T State6=T State5=T State4=T State3=T State2=T State1=T State0=T
    PORTD=0x00;
    DDRD=0x00;
    
    // Timer/Counter 0 initialization
    // Clock source: System Clock
    // Clock value: Timer 0 Stopped
    // Mode: Normal top=0xFF
    // OC0 output: Disconnected
    TCCR0=0x00;
    TCNT0=0x00;
    OCR0=0x00;
    
    // Timer/Counter 1 initialization
    // Clock source: System Clock
    // Clock value: Timer1 Stopped
    // Mode: Normal top=0xFFFF
    // OC1A output: Discon.
    // OC1B output: Discon.
    // Noise Canceler: Off
    // Input Capture on Falling Edge
    // Timer1 Overflow Interrupt: Off
    // Input Capture Interrupt: Off
    // Compare A Match Interrupt: Off
    // Compare B Match Interrupt: Off
    TCCR1A=0x00;
    TCCR1B=0x00;
    TCNT1H=0x00;
    TCNT1L=0x00;
    ICR1H=0x00;
    ICR1L=0x00;
    OCR1AH=0x00;
    OCR1AL=0x00;
    OCR1BH=0x00;
    OCR1BL=0x00;
    
    // Timer/Counter 2 initialization
    // Clock source: System Clock
    // Clock value: Timer2 Stopped
    // Mode: Normal top=0xFF
    // OC2 output: Disconnected
    ASSR=0x00;
    TCCR2=0x00;
    TCNT2=0x00;
    OCR2=0x00;
    
    // External Interrupt(s) initialization
    // INT0: Off
    // INT1: Off
    // INT2: Off
    MCUCR=0x00;
    MCUCSR=0x00;
    
    // Timer(s)/Counter(s) Interrupt(s) initialization
    TIMSK=0x00;
    
    // USART initialization
    // Communication Parameters: 8 Data, 1 Stop, No Parity
    // USART Receiver: On
    // USART Transmitter: On
    // USART Mode: Asynchronous
    // USART Baud Rate: 9600
    UCSRA=0x00;
    UCSRB=0x18;
    UCSRC=0x86;
    UBRRH=0x00;
    UBRRL=0x47;
    
    // Analog Comparator initialization
    // Analog Comparator: Off
    // Analog Comparator Input Capture by Timer/Counter 1: Off
    ACSR=0x80;
    SFIOR=0x00;
    
    // ADC initialization
    // ADC Clock frequency: 750,000 kHz
    // ADC Voltage Reference: AREF pin
    // ADC Auto Trigger Source: ADC Stopped
    ADMUX=ADC_VREF_TYPE & 0xff;
    ADCSRA=0x84;
    
    // SPI initialization
    // SPI disabled
    SPCR=0x00;
    
    // TWI initialization
    // TWI disabled
    TWCR=0x00;
    
    // 1 Wire Bus initialization
    // 1 Wire Data port: PORTD
    // 1 Wire Data bit: 6
    // Note: 1 Wire port settings are specified in the
    // Project|Configure|C Compiler|Libraries|1 Wire menu.
    w1_init();
    
    // Alphanumeric LCD initialization
    // Connections are specified in the
    // Project|Configure|C Compiler|Libraries|Alphanumeric LCD menu:
    // RS - PORTB Bit 0
    // RD - PORTB Bit 1
    // EN - PORTB Bit 2
    // D4 - PORTB Bit 4
    // D5 - PORTB Bit 5
    // D6 - PORTB Bit 6
    // D7 - PORTB Bit 7
    // Characters/line: 16
    lcd_init(16);
    startUp();
    
    while (1)
    {
        // Place your code here
        for(i=0;i<30;i++){
            
            Request();                        //merequest pulse DHT11
            Response();                       //mendapat pesan balasan DHT11
            I_RH=Receive_data();              //mengambil 8 bit pertama untuk kelembaban
            D_RH=Receive_data();              //mengambil 8 bit terakhir untuk kelembaban
            I_Temp=Receive_data();            //mengambil 8 bit pertama untuk suhu
            D_Temp=Receive_data();            //mengambil 8 bit terakhir untuk suhu
            CheckSum=Receive_data();          //mengambil 8 bit untuk checksum keseluruhan data yang diambil
            
            if ((I_RH + D_RH + I_Temp + D_Temp) != CheckSum) //mengecek apakah semua data berhasil diambil
            {
                lcd_gotoxy(0,0);
                sprintf(data,"Error");
                lcd_puts(data);
            }
            
            else
            {
                sprintf(data,"%d.%d%% ",I_RH,D_RH);     //menampilkan kelembaban pada LCD
                lcd_gotoxy(0,1);
                lcd_puts(data);
                sprintf(data, "%d.%dC",I_Temp,D_Temp);  //menampilkan suhu pada LCD
                lcd_puts(data);
            }    
            
            soilMoistureRaw=read_adc(0);//baca ADC channel 0 (ADC0 atau PA0)
            
            soilMoistureRaw= soilMoistureRaw*(3.3/1024);      //Perhitungan untuk menghitung kelembaban tanah
            
            if (soilMoistureRaw < 1.1) {
                soilMoisture = (10 * soilMoistureRaw) - 1;
            }
            else if (soilMoistureRaw < 1.3) {
                soilMoisture = (25 * soilMoistureRaw) - 17.5;
            }
            else if (soilMoistureRaw < 1.82) {
                soilMoisture = (48.08 * soilMoistureRaw) - 47.5;
            }
            else if (soilMoistureRaw < 2.2) {
                soilMoisture = (26.32 * soilMoistureRaw) - 7.89;
            }
            else {
                soilMoisture = (62.5 * soilMoistureRaw) - 87.5;
            }
            
            sprintf(data, "%d",soilMoisture);  //menampilkan kelembaban tanah pada LCD
            lcd_puts(data);
            
            PORTA = 0x00;                             //Matikan dulu penyiram otomatis
            if (soilMoisture < 30 && I_Temp > 34) {       //Menyalakan penyiram otomatis jika kelembaban dan suhu sekian
                PORTA = 0x02;
            }
            
            delay_ms(250);
            delay_ms(400);
            
            
        }	      
        lcd_clear();
        lcd_putsf("Mengirim Data...");
        sendData(I_Temp);
        sendData(I_RH);
        sendData(soil);
        delay_ms(3000);  
        lcd_clear();
    }
}
