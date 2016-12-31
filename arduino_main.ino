#include <SoftwareSerial.h>
#include <stdio.h>

//*-- Hardware Serial
#define _baudrate 9600

//*-- Software Serial
//
#define _rxpin      2
#define _txpin      3
SoftwareSerial Serial1( _rxpin, _txpin );

//*-- IoT Information
#define SSID "<SSID WIFI>"
#define PASS "<PASSWD WIFI>"
#define IP "184.106.153.149" // IP ADDRESS WEB

// GET /update?key=[THINGSPEAK_KEY]&field1=[data 1]&field2=[data 2]...;
String GET = "GET /update?key=TOI5RNNO2E331QY2";

//TalkBack information
String thingSpeakAPI = "api.thingspeak.com";
String talkBackAPIKey = "IBQ9ORXYKZSS97NT";
String talkBackID = "11796";
String talkBackURL = "GET /talkbacks/" + talkBackID + "/commands/3224092?api_key=" + talkBackAPIKey;
//Variabel penampung bacaan sensor dari ATMega
char kelembaban[16];
char suhu[16];
char soil[16];

void setup() {
  
   Serial.begin(9600);
   Serial1.begin( _baudrate );
   Serial1.println("AT+RST");
   sendSerial1("AT");
   delay(5000);
  {
    Serial1.println("RECEIVED: OK\nData ready to sent!");
    connectWiFi();
  }
}

  void wait(){
  while(Serial.available()==0){
   }
  Serial.read();
  }
  
 void readData(char sensor[]){
    wait();
    delay(100);
    int i=0;
    while(Serial.available()!=0)
    {
      sensor[i] = Serial.read();
      i++;
    }
  }

//Fungsi untuk mengirim data ke web
void updateTS(String T,int fieldno)
{
  // ESP8266 Client
  String cmd = "AT+CIPSTART=\"TCP\",\"";// Setup TCP connection
  cmd += IP;
  cmd += "\",80";
  sendSerial1(cmd);

  cmd = GET + "&field" + fieldno + "=" + T +"\r\n"; 
  Serial1.print( "AT+CIPSEND=" );
  delay(1000);
  Serial1.println( cmd.length() );
  delay(1000);
  Serial1.print(cmd);
  delay(2000);
  if( Serial1.find("OK") )
  {
    Serial1.println( "RECEIVED: OK" );
  }
  else
  {
    Serial1.println( "RECEIVED: Error\nExit2" );
  }
}

void sendSerial1(String cmd)
{
  Serial1.print("SEND: ");
  Serial1.println(cmd);
  delay(5000);
}

//Fungsi untuk koneksi ke wifi
boolean connectWiFi()
{
  Serial1.println("AT+CWMODE=1");//WiFi STA mode - if '3' it is both client and AP
  delay(2000);
  //Connect to Router with AT+CWJAP="SSID","Password";
  // Check if connected with AT+CWJAP?
  String cmd="AT+CWJAP=\""; // Join accespoint
  cmd+=SSID;
  cmd+="\",\"";
  cmd+=PASS;
  cmd+="\"";
  sendSerial1(cmd);
  delay(1000);
  if(Serial1.find("OK"))
  {
    Serial1.println("RECEIVED: OK");
    return true;
  }
  else
  {
    Serial1.println("RECEIVED: Error");
    return false;
  }

  cmd = "AT+CIPMUX=0";// Set Single connection
  sendSerial1( cmd );
  if( Serial1.find( "Error") )
  {
    Serial1.print( "RECEIVED: Error" );
    return false;
  }
}


void checkTalkBack()
{  
  String talkBackCommand;
  char charIn;
  String cmd;

  // Make a HTTP GET request to the TalkBack API:
  cmd = "AT+CIPSTART=\"TCP\",\"";
  cmd += IP;
  cmd += "\",80";
  Serial1.print("SEND: ");
  Serial1.println(cmd);
  delay(5000);
   if( Serial1.find( "Error" ) )
  {
    Serial1.print( "RECEIVED: Error\nExit1" );
  }
  cmd = talkBackURL + "\r\n";
  Serial1.print("AT+CIPSEND=");
  delay(1000);
  Serial1.println(cmd.length());
  delay(10000);
   Serial1.println( "AT+CIPCLOSE" );//close TCP connection
  Serial1.flush(); 
  delay(1000);

  }

void loop() {
  //Menerima data dari ATMega
  readData(soil);           
  readData(kelembaban);
  readData(suhu);
  
  updateTS(soil,1);                 //update web
  updateTS(kelembaban,2);           //update web
  updateTS(suhu,3);                 //update web
  checkTalkBack();                //check if there is a command to manual spray
  delay(1000); //tunggu sebentar
}



