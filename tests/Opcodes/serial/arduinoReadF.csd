Arduino-Joystick2Csound1g-fm1-port (version with smoothing)

- Push Joystick button to turn on note
- Use JoystickX to offset and control the modulation index of
  the foscil opcode from 0-40 via the scale opcode
- Use JoystickY to offset and control the Frequency of the foscil
  opcode up/down two octaves via the scale opcode

<CsoundSynthesizer>

<CsInstruments>

sr = 44100
ksmps = 441
nchnls = 2
0dbfs = 1

giport init 0

// NOTE: change USB port "/dev/cu.usbmodem1414301" to correspond
//       with USB port used by Arduino on your system
giport arduinoStart  "//dev/ttyACM0", 9600    // for GNULinux

instr 1

kk  arduinoReadF  giport, 1, 2, 3
printks "kk=%f\n", 0.5, kk
 endin

</CsInstruments>

<CsScore>

f 1 0 16384 10 1

i 1 0 1

e

</CsScore>

</CsoundSynthesizer>

<arduinoSketch>
// Float Example

// John ffitch & Richard Boulanger
// December 7, 2020

// BreadBoard & Arduino Setup

void setup() {
              // NOTE: Digital pins can be either inputs or outputs.

  Serial.begin(9600);
}

val// put_val( ) - a function to send data values to the Csound
//              "arduinoRead" opcode
// The first argument of the put_val function "int senChan" sets
// the software channel number that Csound reads
// NOTE: "senChan" does "not" define the input pin that is used on
// the Arduino for a specific sensor
// The specific Arduino input pin used by any sensor is assigned
// and set elsewhere in the Arduino sketch and mapped to a
// user-defined put_val "senChan" channel

void put_val(int senChan, int senVal)
        // Set the Csound receive channel "senChan", and read from
        // the sensor data stream "senVal"
{       // The packing of the data is sssssvvv 0vvvvvvv where s is a
        // senChan bit, v a senVal bit and 0 is zero` bit
  int low = senVal&0x7f;
  int hi = ((senVal>>7)&0x07) | ((senChan&0x1f)<<3);
  Serial.write(low); Serial.write(hi);
}

void put_float(int senChan1, int senChan2, int senChan3, float senVal)
{
  typedef union {
    float f;
    long i;
  } JOINT;
  JOINT x;
  x.f = senVal;
  put_val(senChan1, (x.i>> 2)& 0x3ff);
  put_val(senChan2, (x.i>>12)& 0x3ff);
  put_val(senChan3, (x.i>>22)& 0x3ff);
}

void loop() {

   Serial.write(0xf8);   // Synchronization Byte - (essential!) marks beginning of new serial frame

  put_float(1,2,3,1.23456789);

  delay(10);

}

</arduinoSketch>
