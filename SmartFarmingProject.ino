#include <WiFi.h>             //Wifi library
#include <WiFiClient.h>

#include <HTTPClient.h>  //Weather library


#include <BlynkSimpleEsp32.h> //Blynk library
#include <esp_task_wdt.h>

#define BLYNK_PRINT Serial  
#include <SimpleTimer.h>

#include <DHT.h>   //Temperature Sensor library

#include <Wire.h>                //Basic library
#include <LiquidCrystal_I2C.h>   //Display library
#include <TridentTD_LineNotify.h> //Line library

#define BLYNK_PRINT Serial

//Line's Token
#define LINE_TOKEN  "vSNNsVIJEW5BCEa2oXXpnhiqVTVBHTWPZdgRYdZYNsc" 
#define SERVER_PORT 80  //Define Port 80

// Set the LCD address to 0x27 for a 16 chars and 2 line display
LiquidCrystal_I2C lcd(0x27, 16, 2);

#define DHTPIN 17   // Digital pin connected to the DHT sensor
#define DHTTYPE DHT11     // DHT 11



//Blynk's Token
char auth[] = "Xj1RLUItuq4rSrJx9DJMq2ufX0qmgM-E";

//Weather Server
WiFiServer server(SERVER_PORT);     //Open TCP Port 80
WiFiClient client;           

char ssid[] = "Janraluek2_2.4G"; // Your WiFi credentials.
char pass[] = "023611662"; // Set password to "" for open networks.

//Weather part 
const char* server_ip = "api.openweathermap.org";  //The name of the weather server.
String city = "Phra%20Khanong"; // Bangkok
String str_get1  = "GET /data/2.5/weather?q="; // Command GET HTTP
String str_get2  = "&mode=xml&appid=c0170e7360d1d8e20dea89c1f6cadf1b HTTP/1.1\r\n";
String str_host = "Host: api.openweathermap.org\r\n\r\n";
//For check api.openweathermap.org/data/2.5/weather?q=Phra%20Khanong&mode=xml&appid=c0170e7360d1d8e20dea89c1f6cadf1b

//UV index part
//http://api.openweathermap.org/data/2.5/uvi?lat=13.695495&lon=100.635601&mode=xml&appid=c0170e7360d1d8e20dea89c1f6cadf1b
const String endpoint = "http://api.openweathermap.org/data/2.5/uvi?lat=13.695495&lon=100.635601&appid=";
const String key = "c0170e7360d1d8e20dea89c1f6cadf1b";


unsigned long previousMillis = 0;       //กำหนดตัวแปรเก็บค่า เวลาสุดท้ายที่ทำงาน    
const long interval = 20000;            //กำหนดค่าตัวแปร ให้ทำงานทุกๆ 20 วินาที

String name_data;
String weather_data;
float FAN;
int CLight=0;
int CFan=0;
int FanOn=0;
int LightOn=0;
int CDisplay=0;
int UV_data;
String payload;

WidgetLED ledL(V27);
WidgetLED ledF(V28);
WidgetLCD lcd2(V30);

DHT dht(DHTPIN, DHTTYPE);
SimpleTimer timer;


//Sync all controll on blynk app.
BLYNK_CONNECTED(){
    Blynk.syncAll();
 }
//Functions of blynk 
BLYNK_WRITE(V25){
    int value = param.asInt();
    if(value==1){
      CLight=1;
      }else{ CLight=0; }
 }
BLYNK_WRITE(V26){
    int valueF = param.asInt();
    if(valueF==1){
      CFan=1;
      }else{ CFan=0; }
 }
BLYNK_WRITE(V29){
  int valueD = param.asInt();
        if(valueD==1){
         LCD_OFF();
         CDisplay=1;
        }else{ 
         LCD_ON();
         CDisplay=0;
        }
      }
 
//FreeRTOS Area
void Blynk_Task(void *p){
     while(1){
        
        Blynk.run(); // Initiates Blynk
        delay(300);
      }  
}

void Task4(void *p) {
  pinMode(LED_BUILTIN, OUTPUT);
  while(1) {
    if(!Blynk.connected()){
      Blynk.begin(auth, ssid, pass);
      digitalWrite(LED_BUILTIN, LOW);
      Blynk.connected();
      }else {
      digitalWrite(LED_BUILTIN, HIGH);
    }
  vTaskDelay(5000 / portTICK_PERIOD_MS);
 } //while
    //เช็คว่าติดต่อ wifi ได้หรือไม่ ถ้าไม่ได้ ให้ reconnect ใหม่
//    if(WiFi.status() != WL_CONNECTED) {
//      digitalWrite(LED_BUILTIN, HIGH);
//      Serial.println("WiFi Disconnected.");
//      WiFi.begin((char*)ssid, (char*)pass);
//    } else {
//      digitalWrite(LED_BUILTIN, HIGH);
//    }
//  vTaskDelay(5000 / portTICK_PERIOD_MS);
// } //while
}

//Send value of Tem to Blynk app
void sendSensor(){
   float hB = dht.readHumidity();
   float tB = dht.readTemperature(); // or dht.readTemperature(true) for Fahrenheit
        if (isnan(hB) || isnan(tB)) {
         Serial.println("Failed to read from DHT sensor!");
          return;
          }
          Blynk.virtualWrite(V5, hB);
          Blynk.virtualWrite(V6, tB);   
}

void checkLight(){
  int numw1 = weather_data.indexOf("rain");
  int numw2 = weather_data.indexOf("cloud");
  if (CLight==0){
    if((numw1!=-1||numw2!=-1)|UV_data<=8){
          digitalWrite(25,LOW);
          ledL.on();
          LightOn=1;
     }else{
            digitalWrite(25,HIGH);
            ledL.off();
            LightOn=0;
       }
  }else{
        digitalWrite(25,HIGH);
        ledL.off();
        LightOn=0;
   }
 }

void checkFan(){
  if(CFan==0){
    if( FAN >= 31){
          digitalWrite(26,LOW);
           ledF.on();
           FanOn=14;
          }else{
            digitalWrite(26,HIGH);
            ledF.off();
            FanOn=0;
            }
    }else{
            digitalWrite(26,HIGH);
            ledF.off();
            FanOn=0;
            }
}
//Request the weather function 
void Client_Request()
{
    Serial.println("Connect TCP Server");
    int cnt=0;
    while (!client.connect(server_ip,SERVER_PORT))  //เชื่อมต่อกับ Server และรอจนกว่าเชื่อมต่อสำเร็จ
    {
          Serial.print(".");
          delay(100);
          cnt++;
          if(cnt>50)                 //ถ้าหากใช้เวลาเชื่อมต่อเกิน 5 วินาที ให้ออกจาก ฟังก์ชั่น
          return;
    } 
    Serial.println("Success");
    HTTPClient http;
    http.begin(endpoint + key); //Specify the URL
    int httpCode = http.GET();  //Make the request
    payload = http.getString();
    client.print(str_get1+city+str_get2+str_host);       //ส่งคำสั่ง HTTP GET ไปยัง Server
    Serial.print(str_get1+city+str_get2+str_host);
}

//Cut_string and value back function (Weather)
String cut_string(String input,String header,String get_string)
{
    if (input.indexOf(header) != -1)    //ตรวจสอบว่าใน input มีข้อความเหมือนใน header หรือไม่
    {
        int num_get = input.indexOf(get_string);  //หาตำแหน่งของข้อความ get_string ใน input
        if (num_get != -1)      //ตรวจสอบว่าตำแหน่งที่ได้ไม่ใช่ -1 (ไม่มีข้อความ get_string ใน input)
        {   
            int num_header = input.indexOf(header);
            int num_value = input.indexOf(get_string,num_header);
      
            int start_val = input.indexOf("\"",num_value)+1;  // หาตำแหน่งแรกของ “ 
            int stop_val = input.indexOf("\"",start_val);   // หาตำแหน่งสุดท้ายของ “
            return(input.substring(start_val,stop_val));    //ตัดเอาข้อความระหว่า “แรก และ ”สุดท้าย  
        }
        else
        {
          return("NULL");   //Return ข้อความ NULL เมื่อไม่ตรงเงื่อนไข
        }
    }
     
    return("NULL"); //Return ข้อความ NULL เมื่อไม่ตรงเงื่อนไข
}
byte tempIcon[8] = // ไอคอนปรอทอุณหภูมิ
  {
    0b00100, 
    0b01010, 
    0b01010,
    0b01110, 
    0b01110, 
    0b11111, 
    0b11111, 
    0b01110
  } ;
  byte humiIcon[8] = // ไอคอนหยดน้ำ
  {
    0b00100,
    0b00100, 
    0b01010, 
    0b01010,
    0b10001, 
    0b10001, 
    0b10001,  
    0b01110
  } ;
  byte thunIcon[] = { // ไอคอนสายฟ้า
  B00111,
  B00110,
  B01100,
  B11111,
  B00110,
  B01100,
  B01000,
  B10000
};
byte lightIcon[] = { //ไอคอนไฟ
  B00000,
  B01110,
  B11011,
  B10001,
  B11011,
  B01010,
  B01110,
  B01110
};
byte fanIcon[] = { // ไอคอนพัดลม
  B00000,
  B10111,
  B10100,
  B11111,
  B00101,
  B11101,
  B00100,
  B01110
};
byte uvIcon[] = { //ไอคอนUV
  B10100,
  B10100,
  B10100,
  B11100,
  B00101,
  B00101,
  B00101,
  B00010
};


void LCD_ON() {
  lcd.display(); // เปิดการแสดงตัวอักษร
  lcd.backlight(); // เปิดไฟแบล็กไลค์
}

void LCD_OFF() {
  lcd.noDisplay(); // ปิดการแสดงตัวอักษร
  lcd.noBacklight(); // ปิดไฟแบล็กไลค์
}

unsigned long previousMillis2 = 0;       //กำหนดตัวแปรเก็บค่า เวลาสุดท้ายที่ทำงาน    
const long interval2 = 300000; 


void AutoReset(){
  unsigned long currentMillis2 = millis();           //อ่านค่าเวลาที่ ESP เริ่มทำงานจนถึงเวลาปัจจุบัน
  if(currentMillis2 - previousMillis2 >= interval2)         /*ถ้าหากเวลาปัจจุบันลบกับเวลาก่อหน้านี้ มีค่า
                            มากกว่าค่า interval ให้คำสั่งภายใน if ทำงาน*/
  {
    previousMillis2 = currentMillis2;              /*ให้เวลาปัจจุบัน เท่ากับ เวลาก่อนหน้าเพื่อใช้
                         คำนวณเงื่อนไขในรอบถัดไป*/
    ESP.restart();
  }
 }
 void CheckConnection(){
    if(!Blynk.connected()){
      Blynk.begin(auth, ssid, pass);
      }
}

  
void setup()
{
  dht.begin();
  lcd.begin();
  lcd.createChar(1,tempIcon) ; lcd.createChar(2,humiIcon) ;lcd.createChar(3,thunIcon);
  lcd.createChar(4,lightIcon);lcd.createChar(5,fanIcon); lcd.createChar(6,uvIcon);
  Serial.begin(115200);
  lcd.clear();
  lcd.setCursor(0, 0);lcd.write(3);
  lcd.setCursor(0, 1);lcd.write(1); lcd.setCursor(5, 1); lcd.write(2) ;
  lcd.setCursor(12, 1); lcd.write(4);lcd.setCursor(14, 1); lcd.write(5) ;
  delay(1000);
  pinMode(25,OUTPUT);
  pinMode(26,OUTPUT);
  digitalWrite(25,1);    
  digitalWrite(26,1);
  xTaskCreate(&Blynk_Task, "Blynk_Task", 2048, NULL, 10, NULL);
  xTaskCreate(&Task4, "Task4", 3000, NULL, 9, NULL);

//  LINE.setToken(LINE_TOKEN);
//  LINE.notify("เครื่องเปิด");
  timer.setInterval(2000L, sendSensor);//1เสี้ยววินาทิ
}

void loop()
{ 
   float h = dht.readHumidity();
   float t = dht.readTemperature();  
   FAN = t;
  checkLight();
  checkFan();
  timer.run();
  AutoReset(); 
                //ตรวจเช็คว่ามีการส่งค่ากลับมาจาก Server หรือไม่
    while(client.available()){
          String line = client.readStringUntil('\n');       //อ่านค่าที่ Server ตอบหลับมาทีละบรรทัด
          name_data = cut_string(line,"city","name"); //ตัด string ข้อมูลส่วนชื่อจังหวัด
         // Serial.println(name_data);
          weather_data = cut_string(line,"weather","value");    //ตัด string ข้อมูลส่วน อุณหภูม
          //Serial.println(weather_data);
          String UV = cut_string(payload,"lon","value");
          UV = UV.substring(1,6);
          UV_data = UV.toInt();
 
    }
  unsigned long currentMillis = millis();           //อ่านค่าเวลาที่ ESP เริ่มทำงานจนถึงเวลาปัจจุบัน
  if(currentMillis - previousMillis >= interval)         /*ถ้าหากเวลาปัจจุบันลบกับเวลาก่อหน้านี้ มีค่า
                            มากกว่าค่า interval ให้คำสั่งภายใน if ทำงาน*/
  {
    previousMillis = currentMillis;              /*ให้เวลาปัจจุบัน เท่ากับ เวลาก่อนหน้าเพื่อใช้
                         คำนวณเงื่อนไขในรอบถัดไป*/
    Client_Request();
    CheckConnection();
  }
  
      lcd.setCursor(1, 0);lcd.print(weather_data);
      lcd.setCursor(1, 1);lcd.print(t,1);lcd.setCursor(6, 1);lcd.print(h,1);
      lcd.setCursor(13, 1);lcd.print(LightOn); lcd.setCursor(15, 1);lcd.print(FanOn);lcd.setCursor(10, 1);lcd.print(UV_data);
      delay(200);
      lcd2.print(1,0,weather_data);lcd2.print(17,0,UV_data);
    
}
