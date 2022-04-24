#include "esp_camera.h"

#include "driver/rtc_io.h"
#include <WiFi.h>
#include <WiFiClient.h>
#include <ESP32_FTPClient.h>
#include <WiFiUdp.h>
#include "time.h"
#include "src/credential_define.h"

// Select camera model
#define CAMERA_MODEL_WROVER_KIT // Has PSRAM
//#define CAMERA_MODEL_ESP_EYE // Has PSRAM
//#define CAMERA_MODEL_M5STACK_PSRAM // Has PSRAM
//#define CAMERA_MODEL_M5STACK_V2_PSRAM // M5Camera version B Has PSRAM
//#define CAMERA_MODEL_M5STACK_WIDE // Has PSRAM
//#define CAMERA_MODEL_M5STACK_ESP32CAM // No PSRAM
//#define CAMERA_MODEL_AI_THINKER // Has PSRAM
//#define CAMERA_MODEL_TTGO_T_JOURNAL // No PSRAM
#include "src/camera_pins.h"

//Set Baud Rate
#define BAUD_RATE 115200

ESP32_FTPClient ftp(FTP_SERVER, FTP_USER, FTP_PASS, 5000, 2);//短いとアップロードが完了しない。5秒。

camera_config_t config;

int cnt_img = 0;

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); 
 
  Serial.begin(BAUD_RATE);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
 
  Serial.println("Connecting Wifi...");
  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.println("Connecting to WiFi..");   
  }
  Serial.println("IP address: ");   
  Serial.println(WiFi.localIP());
  initCamera();
  Serial.println("CAMERA:initialized");  
  setFTP();
}
void loop() {
  takePhoto();
}
void initCamera() {
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
  
  if(psramFound()){
    config.frame_size = FRAMESIZE_UXGA;//FRAMESIZE_UXGA; // FRAMESIZE_ + QVGA|CIF|VGA|SVGA|XGA|SXGA|UXGA
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }  
  // Init Camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }  
}

void setFTP(){
  ftp.OpenConnection();
  Serial.println("FTP:connected");  
  ftp.ChangeWorkDir(FTP_PATH);
  Serial.println("FTP:change dir");  
  ftp.InitFile("Type I");
}
void takePhoto() {
  //ファイル名
  String archive =  "tekephoto" + String(cnt_img) + ".jpg";
  Serial.println("CAMERA:"+archive);
  int str_len = archive.length() + 1; 
  char char_array[str_len];
  archive.toCharArray(char_array, str_len);

  //画像をバッファに保持
  camera_fb_t * fb = NULL;
  fb = esp_camera_fb_get();  
  if(!fb) {
    Serial.println("Camera capture failed");
    return;
  }
  
  /*
   * Upload to ftp server
   */
  //FTP導通チェック
  if(!ftp.isConnected()){
    setFTP()
  }
  
  // Get directory content
  String list[128];
  ftp.ContentList("", list);
  Serial.println("\nDirectory info: ");
  for(int i = 0; i < sizeof(list); i++){
    if(list[i].length() > 0)
      Serial.println(list[i]);
    else
      break;
  }
  
  //もしファイルが存在したら
  ftp.DeleteFile(char_array);
  ftp.NewFile(char_array);
  ftp.WriteData( fb->buf, fb->len );
  ftp.CloseFile();//TODO 毎回Closeさせたくない。
  Serial.println("uploaded");
  cnt_img += 1;
  delay(5000);
  /*
   * Free buffer
   */
  esp_camera_fb_return(fb); 
}
