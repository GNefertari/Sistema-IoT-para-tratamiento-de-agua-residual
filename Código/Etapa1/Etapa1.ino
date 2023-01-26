#include <WiFi.h>
#include <PubSubClient.h>

WiFiClient esp32Client;
PubSubClient mqttClient(esp32Client);

//WIFI
const char* ssid     = "wifi";
const char* password = "abcd1234";


//BROKER
char *servidor = "broker.emqx.io";
int puerto = 1883;


//PINES
int pin_NivelAgua1 = 35;
int pin_NivelAgua2 = 34;
int pin_NivelAgua3 = 39;
int pin_NivelAgua4 = 36;
int pin_BombaS = 32;
int pin_Bomba1 = 33;


//VARIABLES
int nivel1 = 0; //Para leer el valor del sensor de nivel de agua 1
int nivel2 = 0; //Para leer el valor del sensor de nivel de agua 2
int nivel3 = 0; //Para leer el valor del sensor de nivel de agua 3
int nivel4 = 0; //Para leer el valor del sensor de nivel de agua 4
int bombaS = -1; //Bandera para saber si está activa la bomba de agua S
const int referencia = 3000; //Para la lectura del nivel de agua
int bomba1 = -1; //Bandera para saber si está activa la bomba de agua 1
int start = -1; //Para leer del tópico "Test/ESP32/Start"
int etapa2 = 0; //Bandera para coordinar las etapas. Lee del tópico "Test/ESP32/Etapa2"


void wifiInit() {
  Serial.print("Conectándose a ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);  
  }
  Serial.println("");
  Serial.println("Conectado a WiFi");
  Serial.println("Dirección IP: ");
  Serial.println(WiFi.localIP());
}


void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Mensaje recibido [");
  Serial.print(topic);
  Serial.print("] ");    

  char payload_string[length + 1];  
  int resultado;

  //Guarda el valor recibido del estado de la lavadora (extraer o no extraer agua)
  if (strcmp(topic,"Test/ESP32/Start")==0) {
    memcpy(payload_string, payload, length);
    payload_string[length] = '\0';
    resultado = atoi(payload_string);
    start = resultado;
  }
  //Guarda el valor recibido de la banera de Etapa 2
  else if (strcmp(topic,"Test/ESP32/Etapa2")==0) {
    memcpy(payload_string, payload, length);
    payload_string[length] = '\0';
    resultado = atoi(payload_string);
    etapa2 = resultado;
  }

}


void reconnect() {
  while (!mqttClient.connected()) {
    Serial.print("Intentando conectarse MQTT...");

    //Suscribirse a tópicos para conocer el estado de la lavadora y la bandera de Etapa 2
    if(mqttClient.connect("ESP32Prueba_Etapa1")) {
      Serial.println("Conectado");      
      mqttClient.subscribe("Test/ESP32/Start");
      mqttClient.subscribe("Test/ESP32/Etapa2");      
    } else {
      Serial.print("Fallo, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" intentar de nuevo en 5 segundos");
      delay(5000);
    }
  }
}


void setup() {
  pinMode(pin_BombaS,OUTPUT);
  pinMode(pin_Bomba1,OUTPUT);
  Serial.begin(115200);
  delay(10);
  wifiInit();
  mqttClient.setServer(servidor, puerto);
  mqttClient.setCallback(callback);
}


void loop() {
  if (!mqttClient.connected()) {
    reconnect();
  }
  mqttClient.loop();


  //-------- Leer sensores --------
  nivel1 = analogRead(pin_NivelAgua1);
  nivel2 = analogRead(pin_NivelAgua2);
  nivel3 = analogRead(pin_NivelAgua3);
  nivel4 = analogRead(pin_NivelAgua4);
  //Imprimir el valor de los niveles de agua
  Serial.print("Nivel 1: ");
  Serial.println(nivel1);
  Serial.print("Nivel 2: ");
  Serial.println(nivel2);
  Serial.print("Nivel 3: ");
  Serial.println(nivel3);
  Serial.print("Nivel 4: ");
  Serial.println(nivel4);
  delay(500);


  //-------- Publicar el valor del nivel de agua 3 --------
  if(nivel3<referencia){
    mqttClient.publish("Test/ESP32/Nivel3","0");
  }
  else{
    mqttClient.publish("Test/ESP32/Nivel3","1");    
  }


  //-------- Encender y Apagar BombaS --------
  if(start==1 && nivel2>referencia){
    digitalWrite(pin_BombaS,LOW);
    bombaS = 1;
    delay(1000);
  }    
  else{
    digitalWrite(pin_BombaS,HIGH);
    bombaS = 0;
    delay(1000);
  }
  

  //-------- Encender y Apagar Bomba1 --------
  if(nivel1>referencia && nivel4<referencia && etapa2==0){
    digitalWrite(pin_Bomba1,LOW);
    bomba1 = 1;
    delay(1000);
  }
  else{
    digitalWrite(pin_Bomba1,HIGH);
    if(etapa2==0 && bomba1==1){      
      mqttClient.publish("Test/ESP32/Etapa2","1");
    }
    bomba1 = 0;
    delay(1000);
  }


  //-------- Publicar el Estado de Llenado --------
  if(bombaS==1 || bomba1==1){
    mqttClient.publish("Test/ESP32/EstadoLlenado","1");
  }
  else{
    mqttClient.publish("Test/ESP32/EstadoLlenado","0");
  }
  delay(1000);


  //-------- Publicar detalles del Tanque1 --------
  if(nivel1<referencia && nivel2>referencia){
    mqttClient.publish("Test/ESP32/DetallesTanque1","Vacio");
  }
  else if(nivel1>referencia && nivel2>referencia){
    mqttClient.publish("Test/ESP32/DetallesTanque1","Con agua");
  }
  else{
    mqttClient.publish("Test/ESP32/DetallesTanque1","Lleno");
  }
  delay(1000);


  //-------- Publicar detalles del Tanque2 --------
  if(nivel3<referencia && nivel4<referencia){
    mqttClient.publish("Test/ESP32/DetallesTanque2","Vacio");
  }
  else if(nivel3>referencia && nivel4<referencia){
    mqttClient.publish("Test/ESP32/DetallesTanque2","Con agua");
  }
  else{
    mqttClient.publish("Test/ESP32/DetallesTanque2","Lleno");
  }
  delay(1000);

}
