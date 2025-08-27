// #include <Arduino.h>

// void setup() {
//     pinMode(LED_BUILTIN, OUTPUT);
//     digitalWrite(LED_BUILTIN, LOW);
//     Serial.begin(9600);
//     Serial.println("Serial communication started.");
//     delay(1000); // Wait for the serial connection to establish
//     Serial.println("Hello, World!");
//     Serial.println("This is a test message.");
//     Serial.println("Serial communication is working.");
//     Serial.println("Begin listening...");
//     Serial.println("Ready to receive data...");
// }

// void loop() {
//     // Read from serial port
//     if (Serial.available()) {
//         String line = Serial.readStringUntil('\n');
//         Serial.print("sup: ");
//         Serial.println(line);
//     }
//     // digitalWrite(LED_BUILTIN, HIGH);
//     // delay(100);
//     // digitalWrite(LED_BUILTIN, LOW);
//     // delay(100);
// }
