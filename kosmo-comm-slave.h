#ifndef KosmoCommSlave_h
#define KosmoCommSlave_h

#include <Wire.h>

#define SLAVE_ADDR 10

struct SamplerRegisters {
  uint8_t bank = 0;
  uint16_t mix[5] = {0};
};

bool newPartData = false;
SamplerRegisters registers;
SamplerRegisters nextRegisters;
String command = "";
const size_t totalSize = sizeof(SamplerRegisters); 


void receivePartData() {
  Wire.readBytes((char*)&nextRegisters, totalSize);
}

void receiveCommand(int size) {

  if(command == "set") {
    receivePartData();

    // Optionally, print the received values for debugging
    char s[100];
    sprintf(s, "Received Bank: %d", nextRegisters.bank);
    Serial.println(s);
    for(int i=0; i<5; i++) {
      sprintf(s, "Mix %d: %d", i, nextRegisters.mix[i]);
      Serial.println(s);
    }
  }

  command = "";
  while (Wire.available()) {
    char c = Wire.read();
    command += c;
  }

  Serial.print("Transmission from master - command: ");
  Serial.println(command);

  if (command == "prg") {
    // programming = true;
    // readyToSendRegisters = false;
    Serial.println("Programming started");
  } else if (command == "end") {
    //programming = false;
    //readyToSendRegisters = false;
    Serial.println("Programming ended");
  } else if (command.startsWith("set")) {
    command = "set";
    Serial.println("SET started");
  } else if (command == "endset") {
    Serial.println("SET ended");
    newPartData = true;
  }
}


void onRequest() {
  if (command == "get") {
    Wire.write(1); // Send the status
    Wire.write((byte*)&registers, sizeof(registers)); // Send the registers
  }  else {
    Wire.write(0); // send status
  }
}



void setupSlave() {
  registers.bank = 0;
  for(int i=0; i<5; i++) {
    registers.mix[i] = 0;
  }

  Wire.begin(SLAVE_ADDR);
  Wire.setClock(400000);
  Wire.onReceive(receiveCommand);
  Wire.onRequest(onRequest);
  Serial.println("kosmo slave ready");
}


#endif