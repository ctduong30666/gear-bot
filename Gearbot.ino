#include <AFMotor.h>
#include <Servo.h>
#include <PS2X_lib.h>
#include <Adafruit_TCS34725.h>

#define PS2_DAT        A5      
#define PS2_CMD        A4  
#define PS2_SEL        A3  
#define PS2_CLK        A2

#define pressures   false
#define rumble      false

// Khai báo các chân cho động cơ và servo
AF_DCMotor dbl(1);  // Động cơ DC 12V 1 (Driver Base Trái)
AF_DCMotor dbr(2);  // Động cơ DC 12V 2 (Driver Base Phải)
AF_DCMotor intake(3);  // Động cơ DC 12V 3 (Intake)
AF_DCMotor shooter(4);  // Động cơ DC 12V 4 (Shooter)

Servo sorter;  // Servo điều khiển phân loại
Servo angle;  // Servo điều chỉnh góc bắn
Servo doorb;  // Servo mở cửa thùng chứa (L1/L2)
Servo doorw;  // Servo mở cửa thùng chứa (R1/R2)

// Khai báo PS2 controller
PS2X ps2x; 

int error = 0;
byte type = 0;
byte vibrate = 0;

// Khai báo cảm biến màu
Adafruit_TCS34725 colorSensor = Adafruit_TCS34725();

// Hàm setup
void setup() {
  Serial.begin(9600);

  // Khởi tạo PS2 controller
  ps2x.config_gamepad(PS2_CLK, PS2_CMD, PS2_SEL, PS2_DAT, pressures, rumble);
  ps2x.read_gamepad();

  // Khởi tạo cảm biến màu
  if (colorSensor.begin()) {
    Serial.println("TCS34725 sensor initialized.");
  } else {
    Serial.println("Failed to initialize TCS34725 sensor.");
    while (1);
  }

  // Khởi tạo servo
  angle.attach(9);
  doorw.attach(10);
  sorter.attach(11);
  doorb.attach(12);

  // Đặt các giá trị mặc định cho servo
  angle.write(0);
  doorw.write(0);
  sorter.write(0);
  doorb.write(0);
}

// Chạy chương trình
void loop() {
  Ingame();
}

// Tiến
void Forward(long speed) {
  dbl.setSpeed(speed);
  dbr.setSpeed(speed);
  dbl.run(FORWARD);
  dbr.run(FORWARD);
}

// Lùi
void Backward(long speed) {
  dbl.setSpeed(speed);
  dbr.setSpeed(speed);
  dbl.run(BACKWARD);
  dbr.run(BACKWARD);
}

// Xoay trái
void Turn_left(long speed) {
  dbl.setSpeed(speed);
  dbr.setSpeed(speed);
  dbl.run(BACKWARD);
  dbr.run(FORWARD);
}

// Xoay phải
void Turn_right(long speed) {
  dbl.setSpeed(speed);
  dbr.setSpeed(speed);
  dbl.run(FORWARD);
  dbr.run(BACKWARD);
}

// Tiến bên trái
void One_left_up(long speed) {
  dbr.setSpeed(speed);
  dbr.run(FORWARD);
}

// Tiến bên phải
void One_right_up(long speed) {
  dbl.setSpeed(speed);
  dbl.run(FORWARD);
}

// Lùi bên trái
void One_left_down(long speed) {
  dbr.setSpeed(speed);
  dbr.run(BACKWARD);
}

// Lùi bên phải
void One_right_down(long speed) {
  dbl.setSpeed(speed);
  dbl.run(BACKWARD);
}

// Không di chuyển
void Stop() {
  dbl.setSpeed(0);
  dbr.setSpeed(0);
  dbl.run(RELEASE);
  dbr.run(RELEASE);
}

void Ingame() {
  ps2x.read_gamepad(false, vibrate); // Đọc trạng thái nút

  // Đọc giá trị từ thanh điều khiển analog
  int LeftY = ps2x.Analog(PSS_LY);
  int RightY = ps2x.Analog(PSS_RY);

  // Lấy giá trị di chuyển
  int LY = map(LeftY, 0, 255, -255, 255);
  int RY = map(RightY, 0, 255, -255, 255);

  // Di chuyển
  if (LY < 10 && LY > -10 && RY < 10 && RY > -10) {  // Gần 0
    Stop();
  } 
  else if (LY > 10 && RY > 10) {
    Forward(LY);
  } 
  else if (LY < -10 && RY < -10) {
    Backward(-LY);
  } 
  else if (LY < -10 && RY > 10) {
    Turn_right(RY);
  } 
  else if (LY > 10 && RY < -10) {
    Turn_left(LY);
  } 
  else if (LY == 10 && RY > 10) {
    One_right_up(RY);
  }
  else if (RY == 10 && LY > 10) {
    One_left_up(LY);
  }
  else if (LY == 10 && RY < -10) {
    One_right_down(-RY);
  }
  else if (RY == 10 && LY > 10) {
    One_left_down(LY);
  }

  // Điều khiển intake và shooter
  if (ps2x.Button(PSB_TRIANGLE)) {
    shooter.setSpeed(255);
    shooter.run(FORWARD); // Shooter hoạt động
  } 
  if (ps2x.Button(PSB_SQUARE)) {
    intake.setSpeed(255);
    intake.run(FORWARD); // Intake hoạt động
  } 
  if (ps2x.Button(PSB_CIRCLE)) {
    intake.setSpeed(255);
    intake.run(BACKWARD); // Nhả bóng ra
  } 
  if (!ps2x.Button(PSB_TRIANGLE) && !ps2x.Button(PSB_SQUARE) && !ps2x.Button(PSB_CIRCLE)) {
    intake.setSpeed(0);
    shooter.setSpeed(0);
  }

  // Đọc màu từ cảm biến và điều khiển sorter
  uint16_t r, g, b, c;
  colorSensor.getRawData(&r, &g, &b, &c);
  float red, green, blue;
  colorSensor.getRGB(&red, &green, &blue);
  
  if (red < 100 && green < 100 && blue < 100) { // Điều kiện cho màu đen
    sorter.write(0); // Không xoay
  } else if (red > 100 && green > 100 && blue > 100) { // Điều kiện cho màu trắng
    sorter.write(90); // Xoay 90 độ
  } else {
    sorter.write(45); // Xoay 45 độ
  }

  // Điều chỉnh góc bắn
  if (ps2x.Button(PSB_PAD_UP)) {
    int angles = angle.read();
    angle.write(constrain(angles + 10, 0, 180));
  } else if (ps2x.Button(PSB_PAD_DOWN)) {
    int angles = angle.read();
    angle.write(constrain(angles - 10, 0, 180));
  }

  // Điều khiển các servo mở cửa thùng chứa
  if (ps2x.Button(PSB_L1)) {
    doorw.write(90);  // Mở cửa thùng chứa 1
  } else if (ps2x.Button(PSB_L2)) {
    doorw.write(0); // Đóng cửa thùng chứa 1
  } else if (ps2x.Button(PSB_R1)) {
    doorb.write(90);  // Mở cửa thùng chứa 2
  } else if (ps2x.Button(PSB_R2)) {
    doorb.write(0); // Đóng cửa thùng chứa 2
  }
}
