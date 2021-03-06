#include "XBee.h"
#include "DHT.h"

#define DHT_TYPE DHT22
#define DHT_PIN1 3
#define DHT_PIN2 6


const int BITRATE = 9600;
DHT sensor1(DHT_PIN1, DHT_TYPE);
DHT sensor2(DHT_PIN2, DHT_TYPE);
XBee xbee = XBee();
XBeeAddress64 addr64;
ZBRxResponse  rxResponse;
ZBTxRequest   txRequest;



inline void convertFloatToChar(char* str, float value)
{
    int whl;
    unsigned int frac;
    boolean inverse = false; //符号
 
    whl = (int)value;                //整数
    frac = (unsigned int)(value * 100) % 100; //少数
 
    if(whl < 0){
        whl *= -1;
        inverse = true;
    }
 
    if(inverse == true){
        sprintf(str, "-%d.%02d", whl, frac);
    }
    else{
        sprintf(str, "%d.%02d", whl, frac);
    }

}



/*
inline bool canGetResponse(uint8_t api_id) {
  if (xbee.getResponse().isAvailablie()) {
    if (xbee.getResponse().getApiId() == api_id) return true;
    else return false;
  }
  else return false;
}
*/



inline bool canGetResponse(uint8_t api_id) {
  if (xbee.getResponse().getApiId() == api_id) return true;
  else return false;
}



inline bool canReadRxResponse(uint32_t timeout, uint8_t api_id) {
  bool can_read_packet_bool = true;
  
  if(timeout == 0) {
    xbee.readPacket();
  }
  else if(timeout > 0) {
    can_read_packet_bool = xbee.readPacket(timeout);
  }
  else return false;

  if (can_read_packet_bool && canGetResponse(api_id)) {
    xbee.getResponse().getZBRxResponse(rxResponse);
    return true;
  }
  else return false;

}



inline String getRecievedData() {
    return (String)((char*)(rxResponse.getFrameData() + 11));
}



inline void startHandShake() {
  // ブロードキャストを受信
  while(true) {
    while(!canReadRxResponse(0, ZB_RX_RESPONSE));
    if(rxResponse.getFrameData()[10] == 2 && getRecievedData().compareTo("start")) break; 
  }

  // ゲートウェイ側のアドレスを取得
  addr64 = rxResponse.getRemoteAddress64();
}



inline bool isFinishHandShake() {
  // ブロードキャストを受信
  while(!canReadRxResponse(0, ZB_RX_RESPONSE));
  if(rxResponse.getFrameData()[10] == 2 && getRecievedData().compareTo("finish")) true; 
  else false;
}



void reset() {
  asm volatile ("  jmp 0");  
} 



void setup() {
  pinMode(DHT_PIN1, INPUT);
  pinMode(DHT_PIN2, INPUT);
  Serial.begin(BITRATE);
  xbee.setSerial(Serial);
  startHandShake();
}



void loop(){
  int start_time_m = millis();

  //if(isFinishHandShake()) reset();
  
  float tem_val1 = sensor1.readTemperature();
  float hum_val1 = sensor1.readHumidity();
  float tem_val2 = sensor2.readTemperature();
  float hum_val2 = sensor2.readHumidity();

  /*
  if(isnan(tem_val1)) tem_val1 = sensor1.readTemperature();
  if(isnan(hum_val1)) hum_val1 = sensor1.readHumidity();
  if(isnan(tem_val2)) tem_val2 = sensor2.readTemperature();
  if(isnan(hum_val2)) hum_val2 = sensor2.readHumidity();
  */

  char data_json[256];
  char tem_val1_str[16];
  char hum_val1_str[16];
  char tem_val2_str[16];
  char hum_val2_str[16];

  memset(data_json, 0, 256);
  memset(tem_val1_str, 0, 16);
  memset(hum_val1_str, 0, 16);
  memset(tem_val2_str, 0, 16);
  memset(hum_val2_str, 0, 16);

  convertFloatToChar(tem_val1_str, tem_val1);
  convertFloatToChar(hum_val1_str, hum_val1);
  convertFloatToChar(tem_val2_str, tem_val2);
  convertFloatToChar(hum_val2_str, hum_val2);

  sprintf(data_json, "{'tem_val1':%s, 'hum_val1':%s, 'tem_val2':%s, 'hum_val2':%s}", tem_val1_str, hum_val1_str, tem_val2_str, hum_val2_str);
  
  /*
  Serial.println(data_json);
  Serial.println(tem_val1);
  Serial.println(hum_val1);
  Serial.println(tem_val2);
  Serial.println(hum_val2);
  */

  ZBTxRequest zbTx = ZBTxRequest(addr64, data_json, strlen(data_json));
  xbee.send(zbTx);

  /*** mesuring run time ***/
  int finish_time_m = millis();
  /*
  Serial.print(finish_time_m - start_time_m);
  Serial.print(" + ");
  */
  int delta = finish_time_m - start_time_m;

  if (delta < 5000) {
    delay(5000 - delta);
  }

}
