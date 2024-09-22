#include <Wire.h>
#include <SPI.h>
#include <Adafruit_MLX90614.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Ticker.h>
#include<WiFi.h>
#include <PubSubClient.h>
#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);

Adafruit_MLX90614 mlx = Adafruit_MLX90614();
int motion_pin=2;
int motion=0;
int Green=4;
int Red=15;
int Relay_pin=25;
int Buzzer_pin=16;
char msg[100];
Ticker Read_sensor;
Ticker Display_RUNON;
double Temp_ambient;
double Temp_object;
int Oven_status=0;   //0 is oven off,1 is oven on
//...........Wifi setup................//
const char* myssid = "BANONGLEE_2.4G";
const char* mypassword = "WANVIM27";
//.............Mqtt......................//
const char* mqtt_server = "broker.netpie.io";
const int mqtt_port = 1883;
const char* mqtt_Client = "d1fac620-4ec6-41e6-bea5-7c84090dd0ee";
const char* mqtt_username = "yPfcthAH5w8EWqxwUN3VECBaZkKNsDEn";
const char* mqtt_password = "mJS9MiKcyC8x4f7TPNkWaS4kKStDKLCu";

// ...anoucement the mqtt function...//
WiFiClient espClient;
PubSubClient client(espClient);
void setup() {
  Serial.begin(9600);
  //.....connect wifi.....//
  Serial.print("Connecting to "); 
  Serial.println(myssid);
  WiFi.begin(myssid, mypassword);
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  client.setServer(mqtt_server, mqtt_port);//ให้ทำการสร้างพอร์ต mqtt
//.....connect wifi.....//
 
  Serial.println("Adafruit MLX90614 test");  
  pinMode(motion_pin,INPUT);
  pinMode(Relay_pin,OUTPUT);

  mlx.begin();  

  display.begin(SSD1306_SWITCHCAPVCC, 0x3c); //initialize I2C addr 0x3c
  display.clearDisplay(); // clears the screen and buffer
  display.drawPixel(127, 63, WHITE);
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println(Oven_status);
  pinMode(Red, OUTPUT);
  pinMode(Green, OUTPUT);
  pinMode(Buzzer_pin, OUTPUT);
  Read_sensor.attach(1,read_oven_status);
  Display_RUNON.attach(1,Display_run);
  display.display();
}

void read_oven_status()
{
  motion = digitalRead(motion_pin);
  Serial.println(motion);
  Temp_ambient = mlx.readAmbientTempC();
  Temp_object = mlx.readObjectTempC();

  if(Temp_object>=50&&motion==1)
  {
    Oven_status=1;
    digitalWrite(Green,HIGH);
    digitalWrite(Red,LOW);
    Serial.println("oven status ON");
    Serial.println(motion);
    Serial.println("motion=ON");
    tone(Buzzer_pin,554);
    delay(1000);
    noTone(Buzzer_pin);
    Serial.println("Motion detect object");
  }
  else if(Temp_object>=55&&motion==0)
  {
    Oven_status=1;
    digitalWrite(Green,LOW);
    digitalWrite(Red,HIGH);
    Serial.println("oven status ON");
    Serial.println(motion);
    Serial.println("motion=OFF");
    Serial.println("Motion not detect object");
  }
  if(Temp_object<=30&&motion==1)
  {
    Oven_status=0;
    digitalWrite(Red,LOW);
    digitalWrite(Green,HIGH);
    Serial.println("Oven OFF");
    Serial.println(motion);
    Serial.println("motion=ON");
    Serial.println("Motion detect object");
    tone(Buzzer_pin,554);
    delay(1000);
    noTone(Buzzer_pin);

  }
  else
  {
    Oven_status=0;
    digitalWrite(Red,LOW);
    digitalWrite(Green,LOW);
    Serial.println("Oven LOW");
    Serial.println(motion);
    Serial.println("motion=LOW");
    Serial.println("Motion not detect object");
  }
}
void Cut_out()
{
  if(Temp_object>=50&&motion==1)
  {
    digitalWrite(Relay_pin,LOW);
    Serial.println("Relay=OFF");
  }
  else if(Temp_object>=55&&motion==0)
  {
    tone(Buzzer_pin,26);
    delay(1000);
    tone(Buzzer_pin,300);
    delay(1000);
    noTone(Buzzer_pin); 
    delay(50000);
    digitalWrite(Relay_pin,HIGH);
    Serial.println("Relay=ON");
    Serial.println("turn off power");
  }

  if(Temp_object<=30&&motion==0)
  {
    delay(300000);
    digitalWrite(Relay_pin,HIGH);
    Serial.println("Relay=ON");
  }
  else
  {
    Oven_status=0;
    digitalWrite(Red,LOW);
    digitalWrite(Green,LOW);
    Serial.println("Oven LOW");
    Serial.println(motion);
    Serial.println("motion=LOW");
    Serial.println("Motion not detect object");  
  }
}
void Display_run()
{
  display.setCursor(10,10);
  display.print("Temp_t=");
  display.println(Temp_object ,0);
  display.setCursor(10,20);
  display.print("Ambient=");
  display.println(Temp_ambient , 0);
  Serial.printf("Ambient=%.3f",Temp_ambient); 
  Serial.printf("object=%.3f",Temp_object );
  display.display();
 
}
void reconnect() 
{
  while (!client.connected()) //! mean not
  {
    Serial.print("Sensor MQTT connection…");
    if (client.connect(mqtt_Client, mqtt_username,mqtt_password)) 
    {
      Serial.println("connected");
      client.subscribe("@msg/OUT1");
    }
    else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println("try again in 5 seconds");
      delay(5000);
    }
  }
}
void loop() 

{
  
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  String data = "{\"data\": {\"Temp_object\":" + String(Temp_object) +",\"Ambient\":" + String(Temp_ambient)+",\"Motion\":" + String(motion)+"}}";
  Serial.println(data);
  data.toCharArray(msg, (data.length() + 1));
  client.publish("@shadow/data/update", msg);
  delay(5000);
  //Cut_out();

}
