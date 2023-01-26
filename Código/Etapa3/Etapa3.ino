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
int pin_PH = 32;
int pin_NivelAgua5 = 35;
int pin_NivelAgua6 = 34;
int pin_NivelAgua7 = 39;
int pin_NivelAgua8 = 36;
int pin_Bomba2 = 25;
int pin_Bomba3 = 26;


//VARIABLES
int nivel5 = 0; //Para leer el valor del sensor de nivel de agua 5
int nivel6 = 0; //Para leer el valor del sensor de nivel de agua 6
int nivel7 = 0; //Para leer el valor del sensor de nivel de agua 7
int nivel8 = 0; //Para leer el valor del sensor de nivel de agua 8
int bomba2 = -1; //Bandera para saber si está activada la bomba de agua 2
int bomba3 = -1; //Bandera para saber si está activada la bomba de agua 3

int valorPolimero = 0; //Para leer del tópico "Test/ESP32/ValorPol"
int valorAgitacion = 0; //Para leer del tópico "Test/ESP32/ValorAgit"
int valorRapidez = 0; //Para leer del tópico "Test/ESP32/ValorRap"
int valorReposo = 0; //Para leer del tópico "Test/ESP32/ValorRep"

float ph = -1; //Para calcular el valor de PH
unsigned long int promedio; //Para calcular el promedio de lecturas del sensor de pH
int bufferPH[10]; //Para la lectura del sensor de pH
const int referencia = 4000; //Para la lectura del nivel de agua
int nivel3 = 0; //Para leer del tópico "Test/ESP32/Nivel3"
int etapa2 = 0; //Bandera para coordinar las etapas. Lee del tópico "Test/ESP32/Etapa2"
int publicar = -1; //Bandera para publicar el resultado del PH


//Para publicar el resultado
String resultadoPH = "";


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
  Serial.println("] ");    

  char payload_string[length + 1];  
  int resultado;
  
  //Guarda el valor recibido de Nivel de agua 3
  if (strcmp(topic,"Test/ESP32/Nivel3")==0) {
    memcpy(payload_string, payload, length);
    payload_string[length] = '\0';
    resultado = atoi(payload_string);
    nivel3 = resultado;
  }
  //Guarda el valor recibido de la bandera de Etapa 2
  else if (strcmp(topic,"Test/ESP32/Etapa2")==0) {
    memcpy(payload_string, payload, length);
    payload_string[length] = '\0';
    resultado = atoi(payload_string);
    etapa2 = resultado;
  }
  //Guarda el valor recibido Cantidad de Polímero
  else if(strcmp(topic,"Test/ESP32/ValorPol")==0) {
    memcpy(payload_string, payload, length);
    payload_string[length] = '\0';
    resultado = atoi(payload_string);
    valorPolimero = resultado;
  }
  //Guarda el valor recibido de Tiempo de Agitación
  else if(strcmp(topic,"Test/ESP32/ValorAgit")==0) {
    memcpy(payload_string, payload, length);
    payload_string[length] = '\0';
    resultado = atoi(payload_string);
    valorAgitacion = resultado;
  }
  //Guarda el valor recibido de Rapidez del rotor
  else if(strcmp(topic,"Test/ESP32/ValorRap")==0) {
    memcpy(payload_string, payload, length);
    payload_string[length] = '\0';
    resultado = atoi(payload_string);
    valorRapidez = resultado;
  }
  //Guarda el valor recibido de Tiempo de Reposo
  else if(strcmp(topic,"Test/ESP32/ValorRep")==0) {
    memcpy(payload_string, payload, length);
    payload_string[length] = '\0';
    resultado = atoi(payload_string);
    valorReposo = resultado;
  }
}


void reconnect() {
  while (!mqttClient.connected()) {
    Serial.print("Intentando conectarse MQTT...");

    //Suscribirse a tópicos para conocer el Nivel de agua 3 y la bandera de Etapa 2
    if(mqttClient.connect("ESP32Prueba_Etapa3")) {
      Serial.println("Conectado");      
      mqttClient.subscribe("Test/ESP32/Nivel3");
      mqttClient.subscribe("Test/ESP32/Etapa2");
      mqttClient.subscribe("Test/ESP32/ValorPol");
      mqttClient.subscribe("Test/ESP32/ValorAgit");
      mqttClient.subscribe("Test/ESP32/ValorRap");
      mqttClient.subscribe("Test/ESP32/ValorRep");
    } else {
      Serial.print("Fallo, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" intentar de nuevo en 5 segundos");
      delay(5000);
    }
  }
}


void setup() {
  pinMode(pin_Bomba2,OUTPUT);
  pinMode(pin_Bomba3,OUTPUT);
  pinMode(pin_PH,INPUT);

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
  nivel5 = analogRead(pin_NivelAgua5);
  nivel6 = analogRead(pin_NivelAgua6);
  nivel7 = analogRead(pin_NivelAgua7);
  nivel8 = analogRead(pin_NivelAgua8);
  //Imprimir el valor de los niveles de agua
  Serial.print("Nivel 5: ");
  Serial.println(nivel5);
  Serial.print("Nivel 6: ");
  Serial.println(nivel6);
  Serial.print("Nivel 7: ");
  Serial.println(nivel7);
  Serial.print("Nivel 8: ");
  Serial.println(nivel8);
  delay(500);
  


  //-------- Encender y Apagar Bomba2 --------
  if(etapa2==-1 && nivel3==1 && nivel6>referencia){
    Serial.println("Encender bomba 2");
    digitalWrite(pin_Bomba2,LOW);
    bomba2 = 1;
    delay(1000);
  }    
  else{
    digitalWrite(pin_Bomba2,HIGH);
    if(etapa2==-1 && bomba2==1){
      mqttClient.publish("Test/ESP32/Etapa2","0");
      etapa2 = 0;
    }
    bomba2 = 0;
    delay(1000);
  }
  

  //-------- Encender y Apagar Bomba3 --------
  if(nivel5>referencia && nivel8<referencia){
    Serial.println("Encender bomba 3");
    digitalWrite(pin_Bomba3,LOW);
    bomba3 = 1;
    publicar = 0;
    delay(1000);
  }
  else{
    digitalWrite(pin_Bomba3,HIGH);
    if(publicar==0 && bomba3==1){
      publicar = 1;
    }    
    bomba3 = 0;
    delay(1000);
  }
  

  //-------- Publicar el Estado de Filtrado --------
  if(bomba2==1 || bomba3==1){
    mqttClient.publish("Test/ESP32/EstadoFiltrado","1");
  }
  else{
    mqttClient.publish("Test/ESP32/EstadoFiltrado","0");
  }
  delay(1000);


  //-------- Publicar detalles del Tanque3 --------
  if(nivel5<referencia && nivel6>referencia){
    mqttClient.publish("Test/ESP32/DetallesTanque3","Vacio");
  }
  else if(nivel5>referencia && nivel6>referencia){
    mqttClient.publish("Test/ESP32/DetallesTanque3","Con agua");
  }
  else{
    mqttClient.publish("Test/ESP32/DetallesTanque3","Lleno");
  }
  delay(1000);
  

  //-------- Publicar detalles del Tanque4 --------
  if(nivel7<referencia && nivel8<referencia){
    mqttClient.publish("Test/ESP32/DetallesTanque4","Vacio");
  }
  else if(nivel7>referencia && nivel8<referencia){
    mqttClient.publish("Test/ESP32/DetallesTanque4","Con agua");
  }
  else{
    mqttClient.publish("Test/ESP32/DetallesTanque4","Lleno");
  }
  delay(1000);


  //-------- Sensor de PH --------
  if(publicar==1){
    //Obtener 10 valores del sensor para promediar
    for(int i=0;i<10;i++){ 
      bufferPH[i]=analogRead(pin_PH);
      Serial.print("Valor de lectura: ");
      Serial.println(bufferPH[i]);
      delay(200);
    }
  
    promedio=0;
    for(int i=0;i<10;i++){
      promedio+=bufferPH[i]; //Calcular el promedio
      delay(100);
    }
    
    float ph=(float)promedio*5.0/1024/10; //Convertir de análogo a mV
    ph=3.5*ph; //Convertir mV a pH
    delay(100);
    //Imprimir el resultado
    Serial.print("PH: ");
    Serial.println(ph);    
    //Publica el resultado
    resultadoPH = "\"{\\\"ph\\\":" + String(ph) 
      + ",\\\"varPol\\\":" + (String)valorPolimero 
      + ",\\\"varRap\\\":" + (String)valorRapidez 
      + ",\\\"varAgit\\\":" + (String)valorAgitacion 
      + ",\\\"varRep\\\":" + (String)valorReposo + "}\"";
    delay(100);
    mqttClient.publish("Test/ESP32/IntoResult",resultadoPH.c_str());
    //Cambiar bandera para publicar el resultado
    publicar = -1;
    delay(1000);
  }
  
}