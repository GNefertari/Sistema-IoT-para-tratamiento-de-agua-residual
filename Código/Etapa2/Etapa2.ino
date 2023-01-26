#include <WiFi.h>
#include <PubSubClient.h>
#include <ESP32Servo.h>

WiFiClient esp32Client;
PubSubClient mqttClient(esp32Client);

//WIFI
const char* ssid     = "wifi";
const char* password = "abcd1234";


//BROKER
char *servidor = "broker.emqx.io";
int puerto = 1883;


//PINES
int pin_Rotor = 32;
int pin_Caudalimetro = 13;
int pin_ValvPol = 26;
int pin_ValvRes = 25;


//VARIABLES
int cantidadPolimero = 0; //Para leer el valor del caudalímetro
int valorPolimero = 0; //Para leer del tópico "Test/ESP32/ValorPol"
int valorAgitacion = 0; //Para leer del tópico "Test/ESP32/ValorAgit"
int valorRapidez = 0; //Para leer del tópico "Test/ESP32/ValorRap"
int valorReposo = 0; //Para leer del tópico "Test/ESP32/ValorRep"
int publicar = 1; //Bandera para publicar la configuración usada
int etapa2 = 0; //Bandera para coordinar las etapas. Lee del tópico "Test/ESP32/Etapa2"

Servo myservo;  //Objeto Servo para controlar el rotor


//Para publicar la configuración usada
String configuracion = "";


//INICIO: Variables y funciones para lectura del caudalimetro
const int tiempoMedicion = 2000;
volatile int pulsos;
const float factorK = 7.5;// Para el modelo YF-S201
float volumen = 0;
long t0 = 0;

void ISRCountPulse() {
  pulsos++;
}

float ObtenerFrecuencia() {
  pulsos = 0;
  interrupts();
  delay(tiempoMedicion);
  noInterrupts();

  return (float)pulsos * 1000 / tiempoMedicion;
}

void ObtenerVolumen(float dV) {
  volumen += dV / 60 * (millis() - t0) / 1000.0;
  t0 = millis();
}
//FIN: Variables y funciones para lectura del caudalimetro


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

  //Guarda el valor recibido de la bandera de Etapa 2
  if (strcmp(topic,"Test/ESP32/Etapa2")==0) {
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

    //Suscribirse a tópicos para conocer:
    /* La bandera de Etapa 2
     * La cantidad de Polímero que se debe agregar
     * El Tiempo de de agitación del Rotor
     * La Rapidez de agitación del Rotor
     * El Tiempo de Reposo */
    if (mqttClient.connect("ESP32Prueba_Etapa2")) {
      Serial.println("Conectado");
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
  pinMode(pin_ValvRes,OUTPUT);
  pinMode(pin_ValvPol,OUTPUT);
  // CAUDALIMETRO
  attachInterrupt(digitalPinToInterrupt(pin_Caudalimetro), ISRCountPulse, RISING);
  // ROTOR
  myservo.setPeriodHertz(50); 
  myservo.attach(pin_Rotor);
  t0 = millis();
  
  Serial.begin(115200);
  delay(100);
  wifiInit();
  mqttClient.setServer(servidor, puerto);
  mqttClient.setCallback(callback);
  delay(100);
}

void loop() {
  if (!mqttClient.connected()) {
    reconnect();
  }
  mqttClient.loop();
  
  
  //-------- INICIA ETAPA 2 --------
  if(etapa2>0){
    //-------- Publicar el Estado de Sedimentación --------    
    mqttClient.publish("Test/ESP32/EstadoSedimentacion","1");
    

    //----------- Publicar los valores de configuración -----------
    if(publicar==1){
      configuracion = "\"{\\\"varPol\\\":" + (String)valorPolimero 
        + ",\\\"varRap\\\":" + (String)valorRapidez 
        + ",\\\"varAgit\\\":" + (String)valorAgitacion 
        + ",\\\"varRep\\\":" + (String)valorReposo + "}\"";  
      delay(500);
      mqttClient.publish("Test/ESP32/IntoConfig",configuracion.c_str());
      
      //Cambiar bandera para publicar la configuración
      publicar = 0;
      delay(500);
    }


    //----------- SUBETAPA: POLIMERO -----------
    if(etapa2==1){
      mqttClient.publish("Test/ESP32/DetallesPolimero","1");

      //Lectura del caudalímetro L/min
      float frecuencia = ObtenerFrecuencia(); //Obtener frecuencia en Hz
      float flujo_mlMin = frecuencia / factorK;
      ObtenerVolumen(flujo_mlMin);      
      cantidadPolimero = volumen*1000; //Mililitros de polímero

      //Imprimir la lectura del caudalimetro
      Serial.print(" Caudal: ");
      Serial.print(flujo_mlMin, 3); //El valor del flujo con 3 decimales
      Serial.print(" (ml/min)\tConsumo:");
      Serial.print(volumen, 1);
      Serial.println(" (L)");
      
      //Encender y Apagar valvula de Polimero
      if(cantidadPolimero < valorPolimero){
        digitalWrite(pin_ValvPol,LOW);
      }else{
        digitalWrite(pin_ValvPol,HIGH);
        mqttClient.publish("Test/ESP32/DetallesPolimero","0");
        cantidadPolimero = 0; //Reiniciar la lectura del caudalimetro
        volumen = 0; //Reiniciar el valor del volumen
        etapa2 = 2;
      }
    }
    
    
    //----------- SUBETAPA: ROTOR -----------
    if(etapa2==2){
      mqttClient.publish("Test/ESP32/DetallesAgitacion","1");

      //Velocidad: Lento
      if(valorRapidez==1){
        for(int j=0 ; j<valorAgitacion; j=j+18){
          for(int i=0 ; i<=180 ; i=i+2){
            myservo.write(i);
            delay(100);
          }
          for(int i=180 ; i>=0 ; i=i-2){
            myservo.write(i);
            delay(100);
          }
        }
      }

      //Velocidad: Medio
      else if(valorRapidez==2){
        for(int j=0 ; j<valorAgitacion; j=j+9){
          for(int i=0 ; i<=180 ; i=i+4){
            myservo.write(i);
            delay(100);
          }
          for(int i=180 ; i>=0 ; i=i-4){
            myservo.write(i);
            delay(100);
          }
        }
      }

      //Velocidad: Rapido
      else if(valorRapidez==3){
        for(int j=0 ; j<valorAgitacion; j=j+4){
          for(int i=0 ; i<=180 ; i=i+9){
            myservo.write(i);
            delay(100);
          }
          for(int i=180 ; i>=0 ; i=i-9){
            myservo.write(i);
            delay(100);
          }
        }
      }
      mqttClient.publish("Test/ESP32/DetallesAgitacion","0");
      etapa2 = 3;
    }


    //----------- SUBETAPA: REPOSO -----------
    if(etapa2==3){
      mqttClient.publish("Test/ESP32/DetallesReposo","1");
      delay(valorReposo*1000);
      mqttClient.publish("Test/ESP32/DetallesReposo","0");
      etapa2 = 4;
    }


    //----------- SUBETAPA: RESIDUO -----------
    if(etapa2==4){
      mqttClient.publish("Test/ESP32/DetallesResiduo","1");
      digitalWrite(pin_ValvRes,LOW);
      delay(5000); //5 segundos de extracción de lodos
      digitalWrite(pin_ValvRes,HIGH);
      mqttClient.publish("Test/ESP32/DetallesResiduo","0");
      mqttClient.publish("Test/ESP32/Etapa2","-1");
      //Cambiar bandera para publicar la configuración
      publicar = 1;
    }

    delay(1000);
  }
  //-------- TERMINA ETAPA 2 --------
  else{
    mqttClient.publish("Test/ESP32/EstadoSedimentacion","0");
    digitalWrite(pin_ValvPol,HIGH);
    digitalWrite(pin_ValvRes,HIGH);
  }

}
