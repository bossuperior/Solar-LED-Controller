#include <Arduino.h>
#include <IRrecv.h>
#include <IRutils.h>

const uint16_t kRecvPin = 14;
IRrecv irrecv(kRecvPin);
decode_results results;

void setup() {
  Serial.begin(115200);
  irrecv.enableIRIn();
}

void loop() {
  if (irrecv.decode(&results)) {
    Serial.print("Protocol: ");
    Serial.println(typeToString(results.decode_type));
    Serial.print("Code (HEX): 0x");
    Serial.println(resultToHexidecimal(&results));
    irrecv.resume();
  }
}