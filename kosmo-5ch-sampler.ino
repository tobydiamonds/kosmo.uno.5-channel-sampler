#include <Button.h>

#define MAX_BANK 99
#define BANK_BLINK_INTERVAL 300

const uint8_t bank_indicator_led_pin = 8;
const uint8_t bank_load_pin = 9;
const uint8_t bank_up_pin = 10;
const uint8_t bank_down_pin = 11;
const uint8_t channel_1_mix_pins[5] = {A0, A1, A2, A3, A4};
const uint8_t sampler_threshold_pin = A5;

const int threshold = 5;
int prevValue[5] = {0};
int prevSamplerThresholdValue = 0;

Button bankLoad(bank_load_pin);
Button bankUp(bank_up_pin);
Button bankDown(bank_down_pin);


uint8_t bank = 0;
uint16_t mixlevel[5] = {512};
bool channelArmed[5] = {false};

uint16_t samplerThreshold = 512;
bool samplerArmed = false;

bool bankChanged = false;
unsigned long lastBankLedTime = 0;
bool bankLedState = false;
unsigned long now = 0;

void sendBank() {
  Serial.print(0x00); Serial.print(' '); Serial.println(bank);
}

void handleBankCommand(uint16_t payload) {
  if(payload == bank) {
    bankChanged = false;
    digitalWrite(bank_indicator_led_pin, HIGH);
  }
}

void sendChannel(int channel) {
  /* bitmask
  *  15: channel armed
  *  10-14: not used
  *  0-9: mix level 
  */
  uint8_t address = channel+1;
  uint16_t value = channelArmed[channel] ? 0x8000 : 0x0;
  value |= mixlevel[channel];
  Serial.print(address); Serial.print(' '); Serial.println(value);
}

void handleChannelCommand(int channel, uint16_t payload) {
  /* bitmask
  *  15: channel armed
  *  10-14: not used
  *  0-9: mix level 
  */
  channelArmed[channel] = payload & 0x8000;
  mixlevel[channel] = payload & 0x03FF;
}

void sendSampler() {
  /* bitmask
  *  15: armed
  *  10-14: not used
  *  0-9: sampler input threshold
  */  
  uint8_t address = 0x10;
  uint16_t value = samplerArmed ? 0x8000 : 0x0;
  value |= samplerThreshold;
  Serial.print(address); Serial.print(' '); Serial.println(value);  
}

void handleSamplerCommand(uint16_t payload) {
  /* bitmask
  *  15: armed
  *  10-14: not used
  *  0-9: sampler input threshold
  */  
  samplerArmed = payload & 0x8000;  
  samplerThreshold = payload & 0x03FF;
}

void setup() {
  Serial.begin(115200);

  bankLoad.begin();
  bankUp.begin();
  bankDown.begin();

  pinMode(bank_indicator_led_pin, OUTPUT);
  digitalWrite(bank_indicator_led_pin, LOW);

  // send defaults to rpi
  sendBank();

}

void printIntArray(const int* arr, int size) {
  for (int i = 0; i < size; i++) {
    Serial.print(arr[i]);
    if (i < size - 1) {
      Serial.print(", ");
    }
  }
  Serial.println(); // Newline after printing
}

String allowedChars = "0123456789";
bool isIntValue(String s) {
  if(s.length()==0)
    return false;

  for(int i=0; i<s.length(); i++) {
    if(allowedChars.indexOf(s.charAt(i)) == -1)
      return false;
  }
  return true;
}

bool tryGetInt(String data, int offset, int end, int& value) {
  value = 0;
  if(data.length()==0) return false;
  if(offset < 0) return false;
  if(offset > end) return false;
  if(end > data.length()) return false;

  String v = data.substring(offset, end);
  v.trim();
  if(isIntValue(v)) {
    value = v.toInt();
    return true;
  }
  return false;
}

bool getSerialData(int& command, int& payload) {
  while(Serial.available()) {
      String data = Serial.readString();
      data.trim();

      // find seperators
      int seperators[4] = {};
      int length = 0;
      for(int i=0; i<data.length(); i++) {
        if(data.charAt(i) == ' ') {
          seperators[length] = i;
          length++;
        }
      }

      // extract data values
      int values[5] = {};      
      int offset = 0;
      int valueCount = 0;
      for(int i=0; i<length; i++) {
        int value;
        if(tryGetInt(data, offset, seperators[i], value))
          values[valueCount++] = value;
        offset = seperators[i];
      }
      // get the last value
      if(seperators[length-1]<data.length()) {
        int value;
        if(tryGetInt(data, seperators[length-1]+1, data.length(), value))
          values[valueCount++] = value;
      }

      if(valueCount!=2) return false;
      command = values[0];
      payload = values[1];
      return true;
    }
}

void handleCommand(int command, int payload) {
  switch(command) {
    case 0x00:  // bank change acknowlegded
      handleBankCommand(payload);
      break;
    case 0x01:
    case 0x02:
    case 0x03:
    case 0x04:
    case 0x05:
      handleChannelCommand(command-1, payload);
      break;
    case 0x10:
      handleSamplerCommand(payload);
      break;
  }
}

void loop() {
  now = millis();
  
  if(Serial.available()) {
    int command;
    int payload;
    if(getSerialData(command, payload)) {
      handleCommand(command, payload);
    }
  }

  if(bankLoad.pressed()) {
    sendBank();
  }

  if(bankUp.pressed()) {
    if(bank==MAX_BANK)
      bank=0;
    else
      bank++;
    //start led blink
    digitalWrite(bank_indicator_led_pin, LOW);
    bankChanged = true;
  }

  if(bankDown.pressed()) {
    if(bank==0)
      bank=MAX_BANK;
    else
      bank--;
    //start led blink
    digitalWrite(bank_indicator_led_pin, LOW);
    bankChanged = true;
  }

  for(int i=0; i<5; i++) {
    int value = analogRead(channel_1_mix_pins[i]);
    if (abs(value - prevValue[i]) >= threshold) {
      if(value < threshold)
        value = 0;
      if(value > 1018)
        value = 1023;

      mixlevel[i] = value;
      prevValue[i] = value;
      sendChannel(i);

    }
  }

  int samplerThresholdValue = analogRead(sampler_threshold_pin);
  if (abs(samplerThresholdValue - prevSamplerThresholdValue) >= threshold) {
    prevSamplerThresholdValue = samplerThresholdValue;
    if(samplerThresholdValue < threshold)
      samplerThresholdValue = 0;
    if(samplerThresholdValue > 1018)
      samplerThresholdValue = 1023;

      samplerThreshold = samplerThresholdValue;

    sendSampler();
  }

  if(bankChanged && now >= (lastBankLedTime + BANK_BLINK_INTERVAL)) {
    lastBankLedTime = now;
    bankLedState = !bankLedState;
    digitalWrite(bank_indicator_led_pin, bankLedState ? HIGH : LOW);
  }

}
