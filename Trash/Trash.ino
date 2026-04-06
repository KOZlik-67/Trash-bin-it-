#include <Servo.h>
#include <avr/sleep.h>
#include <avr/wdt.h>

Servo myservo1;
Servo myservo2;

const int trigPin = 9;
const int echoPin = 10;
const int servo1Pin = 5; 
const int servo2Pin = 6; 
const int ledPin = 13; 

// Настройки углов (настраивай под свою механику)
const int closedAngle = 10; // Угол закрытия (чуть больше 0, чтобы не давило)
const int openAngle = 100;  // Угол открытия

const int lowBatteryThreshold = 3400; 
const int chargingThreshold = 4400;    

volatile bool wakeup = false;

ISR(WDT_vect) {
  wakeup = true; 
}

long readVcc() {
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  delay(2); 
  ADCSRA |= _BV(ADSC); 
  while (bit_is_set(ADCSRA, ADSC));
  uint8_t low  = ADCL;
  uint8_t high = ADCH;
  long result = (high << 8) | low;
  result = 1125300L / result; 
  return result;
}

void setup() {
  Serial.begin(9600);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(ledPin, OUTPUT);
  
  // Калибровка при включении
  myservo1.attach(servo1Pin);
  myservo2.attach(servo2Pin);
  
  myservo1.write(closedAngle);
  myservo2.write(180 - closedAngle); // ЗЕРКАЛЬНО
  
  delay(1000);
  myservo1.detach();
  myservo2.detach();

  setup_watchdog(4); 
}

void setup_watchdog(uint8_t prescaler) {
  uint8_t wdtcsr = prescaler & 7;
  if (prescaler & 8) wdtcsr |= _BV(WDP3);
  MCUSR &= ~_BV(WDRF);
  WDTCSR |= _BV(WDCE) | _BV(WDE);
  WDTCSR = _BV(WDIE) | wdtcsr;
}

void enterSleep() {
  Serial.flush();
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();
  sleep_mode();
  sleep_disable();
  wdt_reset();
}

void loop() {
  if (wakeup) {
    wakeup = false;
    
    long vcc = readVcc();
    // Логика индикации (зарядка/разрядка)
    if (vcc > chargingThreshold) digitalWrite(ledPin, HIGH);
    else if (vcc < lowBatteryThreshold) { digitalWrite(ledPin, HIGH); delay(50); digitalWrite(ledPin, LOW); }
    else digitalWrite(ledPin, LOW);

    // Работа датчика
    long duration, distance;
    digitalWrite(trigPin, LOW); delayMicroseconds(2);
    digitalWrite(trigPin, HIGH); delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
    
    duration = pulseIn(echoPin, HIGH);
    distance = (duration / 2) / 29.1;

    if (distance < 20 && distance > 0) {
      myservo1.attach(servo1Pin);
      myservo2.attach(servo2Pin);
      
      // ОТКРЫВАЕМ
      myservo1.write(openAngle);
      myservo2.write(180 - openAngle); // ЗЕРКАЛЬНО
      
      delay(3000); 
      
      // ЗАКРЫВАЕМ
      myservo1.write(closedAngle);
      myservo2.write(180 - closedAngle); // ЗЕРКАЛЬНО
      
      delay(1000); 
      
      myservo1.detach();
      myservo2.detach();
    }
  }
  enterSleep();
}