#include <AWS_IOT.h>
#include <WiFi.h>

#include "DHT.h"
#include "ESPAsyncWebServer.h"

#define DHTPIN 27 // El pin al que estamos conectando el sensor DHT

// Descomentar el tipo de pin que estamos utilizando
// #define DHTTYPE DHT11   // DHT 11
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
//#define DHTTYPE DHT21   // DHT 21 (AM2301)

DHT dht(DHTPIN, DHTTYPE);
// Crear un objeto AsyncWebServer en el puerto 80
AsyncWebServer server(80);

AWS_IOT hornbill; // Instancia AWS_IOT 

/* Configuración de la red WIFI a la que nos estamos conectando*/
char WIFI_SSID[]="xxxxxxxx"; // Nombre de la red WIFI
char WIFI_PASSWORD[]="xxxxxxxx"; // Contraseña de la red WIFI
/* Configuración para la conexión con AWS IOT*/
char HOST_ADDRESS[]="a2dftiumtz1a0f-ats.iot.us-west-2.amazonaws.com"; // Enpoint del objeto
char CLIENT_ID[]= "dht11"; // Nombre de la política asociada al objeto
char TOPIC_NAME[]= "$aws/things/dht11_esp32/shadow/update"; // Prefijo del objeto para actualizar la información

int status = WL_IDLE_STATUS;
int tick=0,msgCount=0,msgReceived = 0;
char payload[512];
char rcvdPayload[512];

IPAddress ip; // Dirección IP en la que se despliega la aplicación

String readDHTTemperature() {
  // Leer la temperatura en grados Celsius
  float t = dht.readTemperature();
  // Comprobar que el valor de la temperatura se lee adecuadamente
  if (isnan(t)) {    
    Serial.println("¡Error al leer la temperatura!");
    return "--";
  }
  else {
    Serial.println("Temperatura: ");
    Serial.println(t);
    return String(t);
  }
}

String readDHTHumidity() {
  // Comprobar que el valor de la humedad se lee adecuadamente
  float h = dht.readHumidity();
  if (isnan(h)) {
    Serial.println("¡Error al leer la humedad!");
    return "--";
  }
  else {
    Serial.println("Humedad: ");
    Serial.println(h);
    return String(h);
  }
}

// Contenido de la página
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.7.2/css/all.css" integrity="sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr" crossorigin="anonymous">
  <style>
    html {
     font-family: Arial;
     display: inline-block;
     margin: 0px auto;
     text-align: center;
    }
    h2 { font-size: 3.0rem; }
    p { font-size: 3.0rem; }
    .units { font-size: 1.2rem; }
    .dht-labels{
      font-size: 1.5rem;
      vertical-align:middle;
      padding-bottom: 15px;
    }
  </style>
</head>
<body>
  <h2>Temperatura y humedad con DHT22</h2>
  <p>
    <i class="fas fa-thermometer-half" style="color:#059e8a;"></i> 
    <span class="dht-labels">Temperatura</span> 
    <span id="temperature">%TEMPERATURE%</span>
    <sup class="units">&deg;C</sup>
  </p>
  <p>
    <i class="fas fa-tint" style="color:#00add6;"></i> 
    <span class="dht-labels">Humedad</span>
    <span id="humidity">%HUMIDITY%</span>
    <sup class="units">&percnt;</sup>
  </p>
</body>
<script>
setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("temperature").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/temperature", true);
  xhttp.send();
}, 10000 ) ;

setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("humidity").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/humidity", true);
  xhttp.send();
}, 10000 ) ;
</script>
</html>)rawliteral";

// Reemplazamos los valores de la temperatura y la humedad
String processor(const String& var){
  if(var == "TEMPERATURE"){
    return readDHTTemperature();
  }
  else if(var == "HUMIDITY"){
    return readDHTHumidity();
  }
  return String();
}

void setup() {
    // Iniciando el puerto serie
    Serial.begin(9600);
    delay(2000);

    while (status != WL_CONNECTED)
    {
        Serial.print("Intentando conectar a la red: ");
        Serial.println(WIFI_SSID);
        Serial.println(status);
        // Conectando a una red WPA/WPA2
        status = WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

        // Imprimiendo la IP Local
        ip = WiFi.localIP();
        Serial.println(ip);

        // Esperar 5 segundos para realizar la conexión
        delay(5000);
    }

    Serial.println("Conectado a la red WIFI");

    if(hornbill.connect(HOST_ADDRESS,CLIENT_ID)== 0) // Connect to AWS using Host Address and Cliend ID
    {
        Serial.println("Conectado a a AWS");
        delay(1000);
    }
    else
    {
        Serial.println("Fallo al conectar con AWS, Comprueba el punto de enlace");
        while(1);
    }

    delay(2000);

    dht.begin(); //Inicializar el sensor DHT
    
    // Ruta raíz para la página
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send_P(200, "text/html", index_html, processor);
    });
    server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send_P(200, "text/plain", readDHTTemperature().c_str());
    });
    server.on("/humidity", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send_P(200, "text/plain", readDHTHumidity().c_str());
    });
  
    // Arrancando el servidor
    server.begin();
}

void loop() {
  
}
