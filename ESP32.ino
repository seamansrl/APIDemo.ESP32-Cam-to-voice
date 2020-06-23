// EL SIGUIENTE CODIGO ES DESARROLLADO COMO EJEMPLO SIN GARANTIA PARA LA VERSION BETA DE LA API DE ANALISIS DE IMAGEN POR DEEP LEARNING CON NOMBRE CLAVE "PROYECTO HORUS".
// PARA CONOCER MAS SOBRE EL PROYECTO Y SUS AVANCES ENTRA EN HTTP://www.proyectohorus.com.ar


#include "esp_http_client.h"
#include "esp_camera.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include "Arduino.h"
#include <DFMiniMp3.h>

const char* ssid = "ACA VA LA SSID DE TU WIFI";
const char* password = "ACA VA LA CLAVE DE TU WIFI";

// INDICA LOS ms ENTRE ENVIO Y ENVIO DE SOLICITUD A LA API
int capture_interval = 50;

// PARA OBTENER EL UUID DEL PERFIL COMO EN USUARIO Y LA CLAVE PODES ENTRAR EN HTTP://www.proyectohorus.com.ar Y BAJAR EL ADMINISTRADOR
String APIprofileuuid = "ACA VA EL UUID DEL PERFIL CREADO DESDE EL ADMINISTRADOR DEL PROYECTO HORUS";
String APIUser = "ACA VA TU USUARIO DE PROYECTO HORUS";
String APIPassword = "ACA VA TU CLAVE DE PROYECTO HORUS";
String APItoken = "";

// INDICA SI SE CONECTO CORRECTAMENTE AL WIFI
bool internet_connected = false;
long current_millis;
long last_capture_millis = 0;

// INDICA SI SE RECIBIO LA SOLICITUD DESDE LA API ANTES DE ENVIAR UNA NUEVA
bool Ready = true;

// CAMERA_MODEL_AI_THINKER
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

// LILYGO® TTGO T-Journal ESP32 Camera Module Development Board OV2640
// #define Y2_GPIO_NUM 17
// #define Y3_GPIO_NUM 35
// #define Y4_GPIO_NUM 34
// #define Y5_GPIO_NUM 5
// #define Y6_GPIO_NUM 39
// #define Y7_GPIO_NUM 18
// #define Y8_GPIO_NUM 36
// #define Y9_GPIO_NUM 19
// #define XCLK_GPIO_NUM 27
// #define PCLK_GPIO_NUM 21
// #define VSYNC_GPIO_NUM 22
// #define HREF_GPIO_NUM 26
// #define SIOD_GPIO_NUM 25
// #define SIOC_GPIO_NUM 23
// #define PWDN_GPIO_NUM -1
// #define RESET_GPIO_NUM 15


class Mp3Notify
{
public:
  static void PrintlnSourceAction(DfMp3_PlaySources source, const char* action)
  {
    if (source & DfMp3_PlaySources_Sd) 
    {
        Serial.print("SD Card, ");
    }
    if (source & DfMp3_PlaySources_Usb) 
    {
        Serial.print("USB Disk, ");
    }
    if (source & DfMp3_PlaySources_Flash) 
    {
        Serial.print("Flash, ");
    }
    Serial.println(action);
  }
  static void OnError(uint16_t errorCode)
  {
    // see DfMp3_Error for code meaning
    Serial.println();
    Serial.print("Com Error ");
    Serial.println(errorCode);
  }
  static void OnPlayFinished(DfMp3_PlaySources source, uint16_t track)
  {
    Serial.print("Play finished for #");
    Serial.println(track);  
  }
  static void OnPlaySourceOnline(DfMp3_PlaySources source)
  {
    PrintlnSourceAction(source, "online");
  }
  static void OnPlaySourceInserted(DfMp3_PlaySources source)
  {
    PrintlnSourceAction(source, "inserted");
  }
  static void OnPlaySourceRemoved(DfMp3_PlaySources source)
  {
    PrintlnSourceAction(source, "removed");
  }
};

DFMiniMp3<HardwareSerial, Mp3Notify> mp3(Serial);

// EN ESTA FUNCION OBTENEMOS EL TOKEN
String GetToken(String user, String passwd, String profileuuid)
{
      String Token = "";
      
      HTTPClient http;
      
      http.begin("http://server1.proyectohorus.com.ar/api/v2/functions/login"); 
      http.addHeader("Content-Type", "application/x-www-form-urlencoded");
      int httpCode = http.POST("user=" + user + "&password=" + passwd + "&profileuuid=" + profileuuid);
      
      if (httpCode > 0) 
      {
            String payload = http.getString();
            
            if (getValue(payload,'|',0) == "200")
            {
                  Token = "Bearer " + getValue(payload,'|',1);
                  Serial.println(Token);
            }
            else
            {
                  Serial.println(getValue(payload,'|',1));
            }
      }
      
      http.end();
      
      return Token;
}

// ESTA FUNCION ES SIMILAR A SPLIT EN C# O PYTHON
String getValue(String data, char separator, int index)
{
      int found = 0;
      int strIndex[] = {0, -1};
      int maxIndex = data.length()-1;
      
      for(int i=0; i<=maxIndex && found<=index; i++)
      {
            if(data.charAt(i)==separator || i==maxIndex)
            {
                  found++;
                  strIndex[0] = strIndex[1]+1;
                  strIndex[1] = (i == maxIndex) ? i+1 : i;
            }
      }
      
      return found>index ? data.substring(strIndex[0], strIndex[1]) : "";
}

// CONECTA AL WIFI
bool init_wifi()
{
      int connAttempts = 0;
      Serial.println("\r\nConnecting to: " + String(ssid));
      WiFi.begin(ssid, password);
      
      while (WiFi.status() != WL_CONNECTED ) 
      {
            delay(500);
            Serial.print(".");
            if (connAttempts > 10) 
                  return false;
                  
            connAttempts++;
      }
      return true;
}


// ESTE EVENTO SE EJECUTA AL ENTRAR UN RESPONSE DESDE LA API
esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
      switch (evt->event_id) 
      {
            case HTTP_EVENT_ON_DATA:
                  String Data = (char*)evt->data;
                  String Content = Data.substring(0,evt->data_len);
                  
                  if (Content != "")
                  {
                        String ErrorCode = getValue(Content,'|',0);

                        // SI LA API RESPONDIO CON CODIGO DE ERROR 200 SIGNIFICA QUE TODO LLEGO OK.
                        if (ErrorCode == "200")
                        {
                              String StatusCode = getValue(Content,'|',1);
                              String ymin = getValue(Content,'|',2);
                              String xmin = getValue(Content,'|',3);
                              String ymax = getValue(Content,'|',4);
                              String xmax = getValue(Content,'|',5);
                              String UUIDDetection = getValue(Content,'|',6);
                              String UUIDProfile = getValue(Content,'|',7);
                              String Confidence = getValue(Content,'|',8);
                              
                              if (UUIDDetection != "NOT FOUND" and UUIDDetection != "FAIL")
                              {
                                    if (UUIDDetection == "07bd1b29563911eabb289c5a44391055") {mp3.playMp3FolderTrack(1); waitMilliseconds(2000);}
                                    if (UUIDDetection == "07bd1b2a563911ea928b9c5a44391055") {mp3.playMp3FolderTrack(2); waitMilliseconds(2000);}
                                    if (UUIDDetection == "07bd1b2b563911ea984e9c5a44391055") {mp3.playMp3FolderTrack(3); waitMilliseconds(2000);}
                                    if (UUIDDetection == "07bd1b2c563911eabc739c5a44391055") {mp3.playMp3FolderTrack(4); waitMilliseconds(2000);}
                                    if (UUIDDetection == "07bd1b2d563911ea82959c5a44391055") {mp3.playMp3FolderTrack(5); waitMilliseconds(2000);}
                                    if (UUIDDetection == "07bd1b2e563911ea84859c5a44391055") {mp3.playMp3FolderTrack(6); waitMilliseconds(2000);}
                                    if (UUIDDetection == "07bd1b2f563911eaa7799c5a44391055") {mp3.playMp3FolderTrack(7); waitMilliseconds(2000);}
                                    if (UUIDDetection == "07bd2e86563911eaad989c5a44391055") {mp3.playMp3FolderTrack(8); waitMilliseconds(2000);}
                                    if (UUIDDetection == "07bd2e87563911ea976c9c5a44391055") {mp3.playMp3FolderTrack(9); waitMilliseconds(2000);}
                                    if (UUIDDetection == "07bd2e88563911ea900b9c5a44391055") {mp3.playMp3FolderTrack(10); waitMilliseconds(2000);}
                                    if (UUIDDetection == "07bd2e89563911eaaa749c5a44391055") {mp3.playMp3FolderTrack(11); waitMilliseconds(2000);}
                                    if (UUIDDetection == "07bd2e8a563911eab6ff9c5a44391055") {mp3.playMp3FolderTrack(12); waitMilliseconds(2000);}
                                    if (UUIDDetection == "07bd420c563911eaaa6a9c5a44391055") {mp3.playMp3FolderTrack(13); waitMilliseconds(2000);}
                                    if (UUIDDetection == "07bd420d563911ea8e909c5a44391055") {mp3.playMp3FolderTrack(14); waitMilliseconds(2000);}
                                    if (UUIDDetection == "07bd420e563911ea8ac59c5a44391055") {mp3.playMp3FolderTrack(15); waitMilliseconds(2000);}
                                    if (UUIDDetection == "07bd420f563911ea92ad9c5a44391055") {mp3.playMp3FolderTrack(16); waitMilliseconds(2000);}
                                    if (UUIDDetection == "07bd4210563911ea8c8d9c5a44391055") {mp3.playMp3FolderTrack(17); waitMilliseconds(2000);}
                                    if (UUIDDetection == "07bd4211563911eaacff9c5a44391055") {mp3.playMp3FolderTrack(18); waitMilliseconds(2000);}
                                    if (UUIDDetection == "07bd4212563911ea944f9c5a44391055") {mp3.playMp3FolderTrack(19); waitMilliseconds(2000);}
                                    if (UUIDDetection == "07bd4213563911ea85099c5a44391055") {mp3.playMp3FolderTrack(20); waitMilliseconds(2000);}
                                    if (UUIDDetection == "07bd5598563911eaa5489c5a44391055") {mp3.playMp3FolderTrack(21); waitMilliseconds(2000);}
                                    if (UUIDDetection == "07bd5599563911eabb7d9c5a44391055") {mp3.playMp3FolderTrack(22); waitMilliseconds(2000);}
                                    if (UUIDDetection == "07bd559a563911eaaf429c5a44391055") {mp3.playMp3FolderTrack(23); waitMilliseconds(2000);}
                                    if (UUIDDetection == "07bd559b563911eaaf129c5a44391055") {mp3.playMp3FolderTrack(24); waitMilliseconds(2000);}
                                    if (UUIDDetection == "07bd559b563911eaaf129c5a44391055") {mp3.playMp3FolderTrack(25); waitMilliseconds(2000);}
                                    if (UUIDDetection == "07bd559d563911eab6039c5a44391055") {mp3.playMp3FolderTrack(26); waitMilliseconds(2000);}
                                    if (UUIDDetection == "07bd559e563911ea8cfd9c5a44391055") {mp3.playMp3FolderTrack(27); waitMilliseconds(2000);}
                                    if (UUIDDetection == "07bd559f563911eaba2d9c5a44391055") {mp3.playMp3FolderTrack(28); waitMilliseconds(2000);}
                                    if (UUIDDetection == "07bd691c563911eab9f49c5a44391055") {mp3.playMp3FolderTrack(29); waitMilliseconds(2000);}
                                    if (UUIDDetection == "07bd691d563911eabff99c5a44391055") {mp3.playMp3FolderTrack(30); waitMilliseconds(2000);}
                                    if (UUIDDetection == "07bd691e563911eaad7e9c5a44391055") {mp3.playMp3FolderTrack(31); waitMilliseconds(2000);}
                                    if (UUIDDetection == "07bd691f563911ea96f29c5a44391055") {mp3.playMp3FolderTrack(32); waitMilliseconds(2000);}
                                    if (UUIDDetection == "07bd6920563911eab35d9c5a44391055") {mp3.playMp3FolderTrack(33); waitMilliseconds(2000);}
                                    if (UUIDDetection == "07bd7cb6563911eaa7349c5a44391055") {mp3.playMp3FolderTrack(34); waitMilliseconds(2000);}
                                    if (UUIDDetection == "07bd7cb7563911ea8d099c5a44391055") {mp3.playMp3FolderTrack(35); waitMilliseconds(2000);}
                                    if (UUIDDetection == "07bd7cb8563911ea96a19c5a44391055") {mp3.playMp3FolderTrack(36); waitMilliseconds(2000);}
                                    if (UUIDDetection == "07bd7cb9563911eaa8819c5a44391055") {mp3.playMp3FolderTrack(37); waitMilliseconds(2000);}
                                    if (UUIDDetection == "07bd7cb9563911eaa8819c5a44391055") {mp3.playMp3FolderTrack(38); waitMilliseconds(2000);}
                                    if (UUIDDetection == "07bd7cb9563911eaa8819c5a44391055") {mp3.playMp3FolderTrack(39); waitMilliseconds(2000);}
                                    if (UUIDDetection == "07bd7cbc563911ea90799c5a44391055") {mp3.playMp3FolderTrack(40); waitMilliseconds(2000);}
                                    if (UUIDDetection == "07bd7cbd563911eaa2f49c5a44391055") {mp3.playMp3FolderTrack(41); waitMilliseconds(2000);}
                                    if (UUIDDetection == "07bd902e563911eaa0c39c5a44391055") {mp3.playMp3FolderTrack(42); waitMilliseconds(2000);}
                                    if (UUIDDetection == "07bd902f563911eabd629c5a44391055") {mp3.playMp3FolderTrack(43); waitMilliseconds(2000);}
                                    if (UUIDDetection == "07bd9030563911eaa0c99c5a44391055") {mp3.playMp3FolderTrack(44); waitMilliseconds(2000);}
                                    if (UUIDDetection == "07bd9031563911eaa4f09c5a44391055") {mp3.playMp3FolderTrack(45); waitMilliseconds(2000);}
                                    if (UUIDDetection == "07bd9032563911ea90209c5a44391055") {mp3.playMp3FolderTrack(46); waitMilliseconds(2000);}
                                    if (UUIDDetection == "07bda3b8563911ea91b59c5a44391055") {mp3.playMp3FolderTrack(47); waitMilliseconds(2000);}
                                    if (UUIDDetection == "07bda3b9563911ea87b39c5a44391055") {mp3.playMp3FolderTrack(48); waitMilliseconds(2000);}
                                    if (UUIDDetection == "07bda3ba563911ea83599c5a44391055") {mp3.playMp3FolderTrack(49); waitMilliseconds(2000);}
                                    if (UUIDDetection == "07bda3bb563911eaade49c5a44391055") {mp3.playMp3FolderTrack(50); waitMilliseconds(2000);}
                                    if (UUIDDetection == "07bdb73a563911ea8ff79c5a44391055") {mp3.playMp3FolderTrack(51); waitMilliseconds(2000);}
                                    if (UUIDDetection == "07bdb73b563911ea8b189c5a44391055") {mp3.playMp3FolderTrack(52); waitMilliseconds(2000);}
                                    if (UUIDDetection == "07bdb73c563911eab2819c5a44391055") {mp3.playMp3FolderTrack(53); waitMilliseconds(2000);}
                                    if (UUIDDetection == "07bdb73d563911eaa99e9c5a44391055") {mp3.playMp3FolderTrack(54); waitMilliseconds(2000);}
                                    if (UUIDDetection == "07bdb73e563911eabd379c5a44391055") {mp3.playMp3FolderTrack(55); waitMilliseconds(2000);}
                                    if (UUIDDetection == "07bdb73f563911ea8ae69c5a44391055") {mp3.playMp3FolderTrack(56); waitMilliseconds(2000);}
                                    if (UUIDDetection == "07bdb740563911ea9ec29c5a44391055") {mp3.playMp3FolderTrack(57); waitMilliseconds(2000);}
                                    if (UUIDDetection == "07bdcac6563911ea942a9c5a44391055") {mp3.playMp3FolderTrack(58); waitMilliseconds(2000);}
                                    if (UUIDDetection == "07bdcac7563911ea8e1d9c5a44391055") {mp3.playMp3FolderTrack(59); waitMilliseconds(2000);}
                                    if (UUIDDetection == "07be058a563911ea834c9c5a44391055") {mp3.playMp3FolderTrack(60); waitMilliseconds(2000);}
                                    if (UUIDDetection == "07be058a563911ea834c9c5a44391055") {mp3.playMp3FolderTrack(61); waitMilliseconds(2000);}
                                    if (UUIDDetection == "07be7a9a563911ea8c3e9c5a44391055") {mp3.playMp3FolderTrack(62); waitMilliseconds(2000);}
                                    if (UUIDDetection == "07be7a9b563911eaa0419c5a44391055") {mp3.playMp3FolderTrack(63); waitMilliseconds(2000);}
                                    if (UUIDDetection == "07be7a9c563911ea86239c5a44391055") {mp3.playMp3FolderTrack(64); waitMilliseconds(2000);}
                                    if (UUIDDetection == "07be7a9d563911eabd849c5a44391055") {mp3.playMp3FolderTrack(65); waitMilliseconds(2000);}
                                    if (UUIDDetection == "07be7a9e563911ea985d9c5a44391055") {mp3.playMp3FolderTrack(66); waitMilliseconds(2000);}
                                    if (UUIDDetection == "07be7a9f563911eaae269c5a44391055") {mp3.playMp3FolderTrack(67); waitMilliseconds(2000);}
                                    if (UUIDDetection == "07be7aa0563911eaad299c5a44391055") {mp3.playMp3FolderTrack(68); waitMilliseconds(2000);}
                                    if (UUIDDetection == "07be8e0c563911ea84ff9c5a44391055") {mp3.playMp3FolderTrack(69); waitMilliseconds(2000);}
                                    if (UUIDDetection == "07be8e0d563911eabec69c5a44391055") {mp3.playMp3FolderTrack(70); waitMilliseconds(2000);}
                                    if (UUIDDetection == "07be8e0e563911eaa43d9c5a44391055") {mp3.playMp3FolderTrack(71); waitMilliseconds(2000);}
                                    if (UUIDDetection == "07be8e0f563911eaac5e9c5a44391055") {mp3.playMp3FolderTrack(72); waitMilliseconds(2000);}
                                    if (UUIDDetection == "07be8e10563911ea891b9c5a44391055") {mp3.playMp3FolderTrack(73); waitMilliseconds(2000);}
                                    if (UUIDDetection == "07be8e11563911ea8cf09c5a44391055") {mp3.playMp3FolderTrack(74); waitMilliseconds(2000);}
                                    if (UUIDDetection == "07be8e12563911eab2b99c5a44391055") {mp3.playMp3FolderTrack(75); waitMilliseconds(2000);}
                                    if (UUIDDetection == "07be8e13563911ea900a9c5a44391055") {mp3.playMp3FolderTrack(76); waitMilliseconds(2000);}
                                    if (UUIDDetection == "07be8e14563911ea80619c5a44391055") {mp3.playMp3FolderTrack(77); waitMilliseconds(2000);}
                                    if (UUIDDetection == "07be8e15563911ea8d4f9c5a44391055") {mp3.playMp3FolderTrack(78); waitMilliseconds(2000);}
                                    if (UUIDDetection == "07be8e16563911eab3369c5a44391055") {mp3.playMp3FolderTrack(79); waitMilliseconds(2000);}
                                    if (UUIDDetection == "07be8e17563911eab7e79c5a44391055") {mp3.playMp3FolderTrack(80); waitMilliseconds(2000);}
                              }
                        }
                        else
                        {
                              // SI NO LLEGO CON CODIGO 200 IMPLICA QUE ALGO OCURRIO Y PONEMOS EN CERO LA VARIABLE DE TOKEN PARA QUE INTENTE RECONECTAR
                              APItoken = "";
                        }
                        
                        Ready = true;
                  }
                  break;
      }
      return ESP_OK;
}


// ENVIA LA IMAGEN CAPTURADA DE LA CAMARA A LA API DE HORUS
static esp_err_t take_send_photo()
{
      camera_fb_t * fb = NULL;
      esp_err_t res = ESP_OK;
      
      fb = esp_camera_fb_get();
      
      if (!fb) 
      {
            Serial.println("Camera capture failed");
            return ESP_FAIL;
      }
      
      esp_http_client_handle_t http_client;
      esp_http_client_config_t config_client = {0};
      
      config_client.url = "http://server1.proyectohorus.com.ar/api/v2/functions/object/detection?responseformat=pipe";
      config_client.event_handler = _http_event_handler;
      config_client.method = HTTP_METHOD_POST;
      
      http_client = esp_http_client_init(&config_client);
      
      esp_http_client_set_post_field(http_client, (const char *)fb->buf, fb->len);
      esp_http_client_set_header(http_client, "Content-Type", "image/jpg");
      
      esp_http_client_set_header(http_client, "Authorization",  APItoken.c_str());
      
      esp_err_t err = esp_http_client_perform(http_client);
      
      esp_http_client_cleanup(http_client);
      
      esp_camera_fb_return(fb);
}

void waitMilliseconds(uint16_t msWait)
{
  uint32_t start = millis();
  
  while ((millis() - start) < msWait)
  {
    // calling mp3.loop() periodically allows for notifications 
    // to be handled without interrupts
    mp3.loop(); 
    delay(1);
  }
}

void setup()
{
      Serial.begin(115200);

      if (init_wifi()) 
      { 
            internet_connected = true;
            Serial.println("Internet connected");
      }
      
      camera_config_t config;
      config.ledc_channel = LEDC_CHANNEL_0;
      config.ledc_timer = LEDC_TIMER_0;
      config.pin_d0 = Y2_GPIO_NUM;
      config.pin_d1 = Y3_GPIO_NUM;
      config.pin_d2 = Y4_GPIO_NUM;
      config.pin_d3 = Y5_GPIO_NUM;
      config.pin_d4 = Y6_GPIO_NUM;
      config.pin_d5 = Y7_GPIO_NUM;
      config.pin_d6 = Y8_GPIO_NUM;
      config.pin_d7 = Y9_GPIO_NUM;
      config.pin_xclk = XCLK_GPIO_NUM;
      config.pin_pclk = PCLK_GPIO_NUM;
      config.pin_vsync = VSYNC_GPIO_NUM;
      config.pin_href = HREF_GPIO_NUM;
      config.pin_sscb_sda = SIOD_GPIO_NUM;
      config.pin_sscb_scl = SIOC_GPIO_NUM;
      config.pin_pwdn = PWDN_GPIO_NUM;
      config.pin_reset = RESET_GPIO_NUM;
      config.xclk_freq_hz = 20000000;
      config.pixel_format = PIXFORMAT_JPEG;
      config.frame_size = FRAMESIZE_SVGA;
      config.jpeg_quality = 12;
      config.fb_count = 1;
      
      esp_err_t err = esp_camera_init(&config);
      
      if (err != ESP_OK) 
      {
            Serial.printf("Camera init failed with error 0x%x", err);
            return;
      }

      mp3.begin();

      uint16_t volume = mp3.getVolume();
      Serial.print("volume ");
      Serial.println(volume);
      mp3.setVolume(24);
      
      uint16_t count = mp3.getTotalTrackCount(DfMp3_PlaySource_Sd);
}

void loop()
{
      // SI EL TOKEN ESTA EN NULO SOLICITO UN TOKEN
      if (APItoken == "")
      {
            APItoken = GetToken(APIUser, APIPassword, APIprofileuuid);
      }
      else
      {
            // SI TENGO TOKEN HAGO MI SOLICITUD DE ANALISIS A LA API
            
            current_millis = millis();
            if (current_millis - last_capture_millis > capture_interval) 
            { 
                  last_capture_millis = millis();
                  if (Ready == true)
                  {
                        Ready =  false;
                        take_send_photo();
                  }
            }
      }
}
