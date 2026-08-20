HOW THE CAR WORKS

The code and wiring are in the same folder

This section explains the complete working of the ROVER — a 4WD Bluetooth-controlled competition RC car — from the moment a command is sent on the mobile app to the wheels physically moving.

Signal flow:

Mobile App (Bluetooth RC Controller)
        ↓
Bluetooth Classic (ESP32 BluetoothSerial, device name "ROVER")
        ↓
ESP32 Dev Board
        ↓
L298N Motor Driver
        ↓
4x TT Gear Motors (2 left, 2 right)
        ↓
Wheels
        ↓
Car Movement

Power flow:

3S 11.1V LiPo Battery
        ↓
L298N Motor Driver (motor power input)
        ↓
4x TT Gear Motors

3S 11.1V LiPo Battery
        ↓
L298N Onboard 5V Regulator
        ↓
ESP32 Dev Board

1. MOBILE APP CONTROLLER

The car is driven using the Bluetooth RC Controller Android app. The app connects to the ESP32 over Bluetooth Classic and sends single-character ASCII commands whenever the user presses a control button.

- Movement commands: F (forward), B (backward), L (left), R (right), S (stop)
- Speed commands: the digits 0–9 and q each select a fixed PWM speed level, sent as a single character before or during driving
- Each button press on the app transmits one character over the Bluetooth serial link; there is no complex packet structure — just a raw character stream
- The app does not send continuous analog data (no joystick-style variable steering); direction and speed are two independent, discrete command types

2. BLUETOOTH COMMUNICATION

Bluetooth communication is handled entirely through the ESP32's built-in Bluetooth Classic (SPP) stack, using the Arduino BluetoothSerial library.

- On boot, the ESP32 starts Bluetooth under the device name ROVER (serialBT.begin("ROVER"))
- The phone pairs with and connects to this ROVER Bluetooth device using the Bluetooth RC Controller app
- In the main loop, the ESP32 continuously checks serialBT.available() — as soon as a byte is waiting, it reads it into a char variable (btSignal)
- Each received character is compared against a set of known commands ('F', 'B', 'L', 'R', 'S', and '0'–'9'/'q') to decide what action to take
- There is no acknowledgment/handshake protocol — it's a simple one-directional command stream from phone to ESP32

3. ESP32

The ESP32 dev board is the brain of the car. It is the only microcontroller in the system and is responsible for:

- Receiving raw command characters through Bluetooth Classic
- Interpreting each character as either a movement command or a speed command
- Maintaining the current Speed variable (default 100, adjustable via commands 0–9/q, up to 255)
- Generating the correct digital direction signals (IN1–IN4) for the L298N
- Generating the correct PWM signals (via ledcWrite) for motor speed
- Running two independent PWM channels using the ESP32's LEDC hardware PWM peripheral, at 5 kHz with 8-bit resolution (0–255)

The ESP32 does not drive the motors directly — it only outputs low-power logic-level control signals to the L298N.

Pin mapping used in the code:

Function                          GPIO Pin     
PWM speed — Motor A (right side)  GPIO 14 (enA)
PWM speed — Motor B (left side)   GPIO 32 (enB)
Motor A direction IN1             GPIO 27      
Motor A direction IN2             GPIO 26      
Motor B direction IN3             GPIO 25      
Motor B direction IN4             GPIO 33      
4. MOTOR DRIVER — L298N

The ESP32's GPIO pins can only supply a few milliamps at 3.3V — nowhere near enough to spin gear motors. The L298N dual H-bridge motor driver sits between the ESP32 and the motors to solve this.

- The L298N receives low-power control signals from the ESP32: two direction pins per channel (IN1/IN2 for Motor A, IN3/IN4 for Motor B) and one PWM enable pin per channel (enA, enB)
- Separately, it receives high-power battery input directly from the 3S 11.1V LiPo on its main power terminals
- Internally, each H-bridge channel uses the IN1/IN2 (or IN3/IN4) logic levels to route battery current through the motor windings in one direction or the other, which is how direction (forward/reverse) is controlled
- The PWM signal on enA/enB switches the H-bridge output on and off rapidly (5 kHz), so the average voltage delivered to the motor — and therefore its speed — is proportional to the PWM duty cycle set by ledcWrite(channel, Speed)
- The L298N's onboard 5V regulator taps off the battery input and separately powers the ESP32, so the same battery feeds both the high-power motor side and the low-power logic side

This clearly separates the control path (ESP32 → IN1–IN4 + PWM, low power) from the power path (LiPo → L298N → motors, high power).

5. MOTORS — TT GEAR MOTORS (X4)

The car uses four TT gear motors, wired in a left/right pair configuration to the L298N's two channels:

- 2 motors (right side) wired in parallel to Motor A output (IN1/IN2, PWM on enA)
- 2 motors (left side) wired in parallel to Motor B output (IN3/IN4, PWM on enB)

Each TT gear motor is a brushed DC motor with an integrated gearbox that reduces the motor's raw RPM in exchange for higher torque, which is what allows the car to move a 3D-printed chassis and handle obstacles rather than just spinning its wheels.

- The L298N delivers switched DC current to each motor pair based on the IN pin states and PWM duty cycle
- Inside each motor, this current passes through the armature windings, which generates a magnetic field that interacts with the motor's permanent magnets, producing rotational torque on the output shaft
- The gearbox output shaft is directly connected to a wheel hub, so motor rotation is transferred straight into wheel rotation
- Because the two motors on each side are wired in parallel to the same channel, they always spin together at the same speed and direction — the car has two independently controllable drive sides (left side and right side), not four independently controlled wheels

6. MOVEMENT LOGIC

There is no steering servo — this is a differential (skid-steer) drive 4WD car. Turning is achieved purely by driving the left-side and right-side motor pairs at different directions/speeds relative to each other, not by turning the wheels.

FORWARD (F)

Mobile App sends 'F'
→ Bluetooth → ESP32 receives 'F'
→ ESP32 calls forward():
     IN1=HIGH, IN2=LOW   (right side forward)
     IN3=LOW,  IN4=HIGH  (left side forward)
     PWM = current Speed on both channels
→ L298N drives both motor pairs forward
→ Wheels spin forward
→ Car moves forward

BACKWARD (B)

Mobile App sends 'B'
→ ESP32 calls backward():
     IN1=LOW,  IN2=HIGH  (right side reversed)
     IN3=HIGH, IN4=LOW   (left side reversed)
→ Both motor pairs spin in reverse
→ Car moves backward

LEFT (L)

Mobile App sends 'L'
→ ESP32 calls left():
     IN1=HIGH, IN2=LOW   (right side forward)
     IN3=HIGH, IN4=LOW   (left side reversed relative to its forward wiring)
→ Right-side wheels and left-side wheels spin in opposite senses
→ Car pivots/turns left

RIGHT (R)

Mobile App sends 'R'
→ ESP32 calls right():
     IN1=LOW,  IN2=HIGH  (right side reversed)
     IN3=LOW,  IN4=HIGH  (left side forward relative to its forward wiring)
→ Right-side and left-side wheels spin in opposite senses (mirrored from LEFT)
→ Car pivots/turns right

STOP (S)

Mobile App sends 'S'
→ ESP32 calls stopMotors():
     PWM on both channels set to 0
     IN1=LOW, IN2=LOW, IN3=LOW, IN4=LOW
→ Both motor pairs de-energized
→ Car stops

SPEED CONTROL
Speed is controlled by the ESP32's hardware PWM (LEDC peripheral, 5 kHz, 8-bit resolution) on the enA/enB pins. Sending a digit 0–9 or q updates the Speed variable to a preset value between 100 and 255, which is then applied via ledcWrite() the next time a movement command runs. Higher PWM values mean the H-bridge output is "on" for a larger fraction of each cycle, so more average voltage — and more speed — is delivered to the motors.

7. POWER SYSTEM

                 3S 11.1V LiPo Battery
                          │
            ┌─────────────┴─────────────┐
            ▼                           ▼
      L298N motor input         L298N onboard 5V regulator
            │                           │
            ▼                           ▼
     4x TT Gear Motors                ESP32
     (2 right / 2 left)          (logic + Bluetooth)

- The 3S 11.1V LiPo is the single power source for the entire car
- It connects directly to the L298N's high-voltage input terminals, which supply the motors
- The L298N's onboard 5V linear regulator steps the 11.1V battery voltage down to a stable 5V, which powers the ESP32 — no separate buck converter is used
- Voltage regulation is necessary because the ESP32 is only rated for 5V (VIN) / 3.3V (logic) input; feeding it 11.1V directly would destroy the board
- This means motor power and logic power come from the same battery but are cleanly separated after the L298N — motor current spikes (e.g. from stalling against an obstacle) are isolated from the ESP32's regulated 5V rail

8. SOFTWARE WORKING

1. ESP32 boots and starts the serial monitor at 115200 baud
2. serialBT.begin("ROVER") initializes Bluetooth Classic and advertises the device as ROVER
3. GPIO pins for enA, enB, IN1–IN4 are configured as outputs
4. Two LEDC PWM channels are set up (5 kHz, 8-bit) and attached to enA and enB
5. stopMotors() is called once at startup so the car begins in a safe, stopped state
6. The mobile app connects to ROVER over Bluetooth
7. In loop(), the ESP32 continuously polls serialBT.available()
8. When a character arrives, it's read into btSignal and printed to the serial monitor for debugging
9. If btSignal is a digit ('0'–'9') or 'q', the Speed variable is updated to the corresponding preset value
10. If btSignal matches 'F', 'B', 'L', 'R', or 'S', the corresponding function (forward(), backward(), left(), right(), stopMotors()) is called
11. That function sets the IN1–IN4 direction pins and writes the current Speed value to both PWM channels via ledcWrite()
12. The L298N responds to the new pin states, driving the motors accordingly, and the wheels/car move
13. The loop repeats indefinitely, waiting for the next Bluetooth character

9. COMPLETE SYSTEM DIAGRAM

            ┌──────────────────────────┐
            │  Bluetooth RC Controller │
            │        (Mobile App)      │
            └────────────┬─────────────┘
                          │
                    Bluetooth Classic
                     (device: ROVER)
                          │
                          ▼
            ┌──────────────────────────┐
            │          ESP32           │
            │      Main Controller     │
            └────────────┬─────────────┘
                          │
              Direction pins (IN1–IN4)
                + PWM (enA / enB)
                          │
                          ▼
            ┌──────────────────────────┐
            │       L298N Driver       │
            └──────────┬───┬───────────┘
                        │   │
              Motor A   │   │  Motor B
             (right side)   (left side)
                        ▼   ▼
              ┌───────────┐ ┌───────────┐
              │ 2x TT Gear│ │ 2x TT Gear│
              │  Motors   │ │  Motors   │
              └─────┬─────┘ └─────┬─────┘
                    ▼             ▼
                Right Wheels   Left Wheels
                    │             │
                    └──────┬──────┘
                           ▼
                     CAR MOVEMENT

Power:

             ┌──────────────────┐
             │ 3S 11.1V LiPo     │
             └─────────┬─────────┘
                        │
         ┌──────────────┴──────────────┐
         ▼                             ▼
   L298N Motor Input           L298N Onboard 5V Reg
         │                             │
         ▼                             ▼
  4x TT Gear Motors                  ESP32

10. MECHANICAL WORKING

- The chassis is a custom 3D-printed body, printed in two main parts (vision_robo_race_part_1, vision_robo_race_part_2) which are joined together with glue after printing to form the complete chassis
- A dedicated 3D-printed fan holder mounts a cooling fan directly onto/near the L298N, keeping the motor driver cool under sustained high-current draw during the obstacle course
- Each TT gear motor sits in a dedicated 3D-printed TT motor mount (printable with no supports needed), which is fastened to the chassis using M2 x 15mm screws
- All four TT gear motors are mounted to the chassis at fixed positions — two per side, each in its own printed mount — with their output shafts driving the wheel hubs directly
- Wheels are mounted directly on the gearbox output shafts of each TT motor
- The L298N, ESP32, and LiPo battery are mounted on the chassis body, positioned to keep the car's weight balanced across all four wheels for stable handling over obstacles
- Because there is no steering linkage, turning is purely electronic (differential drive) — a mechanically simpler and more robust system for an obstacle-course competition, since there are fewer moving parts that can jam or break on impact

11. WHY EACH COMPONENT IS USED

Component                             Purpose                                                                               Connected To                                   
ESP32 Dev Board                       Main controller — receives Bluetooth commands and generates motor control signals     Bluetooth (built-in), L298N (IN1–IN4, enA, enB)
Bluetooth RC Controller (Mobile App)  Sends driving and speed commands wirelessly                                           ESP32 via Bluetooth Classic                    
3S 11.1V LiPo Battery                 Main power source for the entire car                                                  L298N motor input + L298N onboard 5V regulator 
L298N Motor Driver                    Converts low-power ESP32 signals into high-power motor drive; regulates 5V for ESP32  ESP32 (control) + LiPo (power) + 4x TT motors  
TT Gear Motors (x4)                   Produce rotational movement, geared down for torque                                   L298N (2 per channel) + wheels                 
Wheels                                Convert motor rotation into vehicle movement                                          TT gear motor output shafts                    
3D-Printed Chassis/Parts              Mechanical structure holding all components together                                  Motors, wheels, ESP32, L298N, battery          
3D-Printed L298N Fan Holder           Mounts a cooling fan to prevent L298N overheating                                     L298N                                          
3D-Printed TT Motor Mount             Holds each TT gear motor to the chassis, fastened with M2 x 15mm screws               TT gear motors + chassis                       
12. COMPETITION RESULT

This car, ROVER, was designed, built, and competed in the Vision Robo Race — an obstacle-course racing competition held at Silicon Sphere, VIS, Sector 10, Dwarka, Delhi.

🏆 1st Prize — Vision Robo Race (Obstacle Course)

The car's 4WD differential-drive layout, L298N-based motor control with PWM speed adjustment, and 3D-printed chassis (including a dedicated fan mount to keep the motor driver cool) were built specifically to handle the demands of navigating an obstacle course, and the design was validated by taking first place in the competition.
