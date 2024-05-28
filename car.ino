#define MAX_SPEED 0.3
#define KP 0.0005
#define SEND_RAW_SENSOR_VALUES 0x86
#define SEND_CALIBRATED_SENSOR_VALUES 0x87
#define SEND_BATTERY_MILLIVOLTS 0xB1
#define DO_PRINT 0xB8
#define M1_FORWARD 0xC1
#define M1_BACKWARD 0xC2
#define M2_FORWARD 0xC5
#define M2_BACKWARD 0xC6
#define AUTO_CALIBRATE 0xBA
#define CLEAR 0xB7
#include <Ultrasonic.h>

float m1Speed;
float m2Speed;
bool isRunning = false;
void initialize() {

  Serial1.begin(115200);
  Serial.begin(9600);
  m1Speed = 0;
  m2Speed = 0;

}


void pololuReset() {

  pinMode(5, OUTPUT);
  digitalWrite(5, LOW);
  delay(10);
  digitalWrite(5, HIGH);
  delay(100);  

}

void stopSmooth() {

  while(m1Speed > 0 || m2Speed > 0) {
    if((m1Speed - 0.05) >= 0) {
      m1Speed -= 0.05;
    } else {
      m1Speed = 0;    
    }
    if((m2Speed - 0.05) >= 0) {
      m2Speed -= 0.05;        
    } else {
      m2Speed = 0;
  }

  activateMotor(0, m1Speed);
  activateMotor(1, m2Speed);
  delay(35);
  }  
  
}
void activateMotor(int motor, float speed) {

  char opcode = 0x0;
  if (speed > 0.0) {
    if (motor == 0) {
      opcode = M1_FORWARD;
      m1Speed = speed;
    } else {
      opcode = M2_FORWARD;
      m2Speed = speed;
    }
  } else {
    if (motor == 0) {
      opcode = M1_BACKWARD;
      m1Speed = speed;
    } else {
      opcode = M2_BACKWARD;
      m2Speed = speed;
    }
  }
  unsigned char arg = 0x7f * abs(speed);
  Serial1.write(opcode);
  Serial1.flush(); 
  Serial1.write(arg);
  Serial1.flush();
  
}

char sensorAutoCalibrate() {

  Serial1.write(AUTO_CALIBRATE);
  Serial1.flush();
  return Serial1.read();    
    
}
void setup() {
  initialize();
  pololuReset();
  sensorAutoCalibrate();
  pinMode(A3, INPUT);  
  pinMode(20, OUTPUT);  
}
void calibratedSensors(int sensors[5]) {
  
  int sensor1 = 0;
  int sensor2 = 0;
  int sensor3 = 0;
  int sensor4 = 0;
  int sensor5 = 0;
  
  Serial1.write(SEND_CALIBRATED_SENSOR_VALUES);
  Serial1.flush();

  char sensor1High = Serial1.read();
  char sensor1Low = Serial1.read();
  sensor1 = sensor1Low + (sensor1High << 8);   
  char sensor2High = Serial1.read();
  char sensor2Low = Serial1.read();
  sensor2 = sensor2Low + (sensor2High << 8);   
  char sensor3High = Serial1.read();
  char sensor3Low = Serial1.read();
  sensor3 = sensor3Low + (sensor3High << 8);   
  char sensor4High = Serial1.read();
  char sensor4Low = Serial1.read();
  sensor4 = sensor4Low + (sensor4High << 8);   
  char sensor5High = Serial1.read();
  char sensor5Low = Serial1.read();
  sensor5 = sensor5Low + (sensor5High << 8);   

  sensors[0] = sensor1;
  sensors[1] = sensor2;
  sensors[2] = sensor3;
  sensors[3] = sensor4;
  sensors[4] = sensor5;

}
void obstacleAvoidance() {
  Ultrasonic ultrasonic(10);
  int distance = ultrasonic.read();
  if (distance <= OBSTACLE_DISTANCE) {
    // 停止
    activateMotor(LEFT_MOTOR, 0);
    activateMotor(RIGHT_MOTOR, 0);
    while (distance <= OBSTACLE_DISTANCE) {
      distance = ultrasonic.read();
      delay(100);
    }
  }
}


void loop() {
  int sensors[5];
  calibratedSensors(sensors);
  
  // Calculate error value based on the sensor readings
  int error = 0;
  for (int i = 0; i < 5; i++) {
    error += (i - 2) * sensors[i];
  }

  // If the center sensor is on the line (value is more than 500)
  if (sensors[2] > 500) {
    if (!isRunning) {
      isRunning = true;
    }
    // Adjust motor speeds based on the error value
    float m1Speed = MAX_SPEED + (KP * error);
    float m2Speed = MAX_SPEED - (KP * error);

    // Ensure motor speeds are within allowable limits
    m1Speed = constrain(m1Speed, 0, MAX_SPEED);
    m2Speed = constrain(m2Speed, 0, MAX_SPEED);

    // Send commands to the motors
    activateMotor(0, m1Speed);
    activateMotor(1, m2Speed);
  } else {
    if (isRunning) {
      stopSmooth();
      isRunning = false;
    }
  }
  //obstacleAvoidance()
  float output = analogRead(A3);  /
  int lightpercent = (1023-output)/1023 * 100;  

  if (lightpercent < 90) {
    digitalWrite(20, HIGH);  /
  } else {
    digitalWrite(20, LOW); 
  }

  delay(50);
}