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
const size_t totalSize = sizeof(SamplerRegisters); 

void onRequest() {
  Wire.write((byte*)&registers, sizeof(registers)); // Send the registers
}

void onRecieve(int size) {
  Wire.readBytes((char*)&nextRegisters, totalSize);
  newPartData = true;
}


void setupSlave() {
  registers.bank = 0;
  for(int i=0; i<5; i++) {
    registers.mix[i] = 0;
  }

  Wire.begin(SLAVE_ADDR);
  Wire.setClock(400000);
  Wire.onReceive(onRecieve);
  Wire.onRequest(onRequest);
}


#endif