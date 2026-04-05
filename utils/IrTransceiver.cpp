#include <Arduino.h>
#include <IRrecv.h>
#include <IRsend.h>
#include <IRutils.h>

#define IR_CODE_ON 0xFFC23D
#define IR_CODE_OFF 0xFFB04F
#define IR_CODE_FULL 0xFF10EF
#define IR_CODE_SEMI 0xFF5AA5
#define IR_CODE_3H 0xFF22DD
#define IR_CODE_5H 0xFFA857
#define IR_CODE_8H 0xFF6897

const uint16_t kRecvPin = 15;
const uint16_t kSendPin = 17;

IRrecv irrecv(kRecvPin);
IRsend irsend(kSendPin);
decode_results results;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n--- IR Manager Started ---");
  irrecv.enableIRIn();
  irsend.begin();
  Serial.println("System Ready! Type 'on', 'off', etc. to send.");
}

void transmitAndListen(uint64_t code, String commandName) {
  Serial.print("Sending ");
  Serial.println(commandName);
  irsend.sendNEC(code, 32); 
  delay(150); 
  Serial.println("IR Sent! Waiting for echo...");
}

void loop() {
  //Listening for incoming IR signals
  if (irrecv.decode(&results)) {
    Serial.println("\n>>> Signal Detected!");
    Serial.print("Protocol: ");
    Serial.println(typeToString(results.decode_type));
    Serial.print("Code (HEX): ");
    Serial.println(resultToHexidecimal(&results));
    irrecv.resume();
  }

  // Sending IR commands based on serial input
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n'); 
    command.trim();
    if (command.length() == 0) return;
    if (command == "on")        transmitAndListen(IR_CODE_ON, "IR_CODE_ON");
    else if (command == "off")  transmitAndListen(IR_CODE_OFF, "IR_CODE_OFF");
    else if (command == "full") transmitAndListen(IR_CODE_FULL, "IR_CODE_FULL");
    else if (command == "semi") transmitAndListen(IR_CODE_SEMI, "IR_CODE_SEMI");
    else if (command == "3h")   transmitAndListen(IR_CODE_3H, "IR_CODE_3H");
    else if (command == "5h")   transmitAndListen(IR_CODE_5H, "IR_CODE_5H");
    else if (command == "8h")   transmitAndListen(IR_CODE_8H, "IR_CODE_8H");
    else {
      Serial.println("Unknown command. Use: on, off, full, semi, 3h, 5h, or 8h.");
    }
  }
}