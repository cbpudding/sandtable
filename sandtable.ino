// Written by Alexander Hill <ahill@breadpudding.dev>

/* MIT License

Copyright (c) 2024 Alexander Hill

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE. */

// Here, you can enable or disable features you either need or don't need.
//#define GCODE_API
#define TURTLE_API

// DO NOT TOUCH. USED FOR DEFINING PATTERNS.
typedef struct {
  void *pattern;
  int length;
} pattern_t;

// Paste your patterns from tbcc.py here. You can compile patterns with:
// ./tbcc.py compile pattern.turtle pattern.bin
// ./tbcc.py dump pattern.bin
// Then paste the results below!

const char FIGUREEIGHT_BIN[] = { 0xC1, 0x80, 0x0A, 0x80, 0x12, 0x04, 0x00, 0x40, 0x14, 0xC0, 0x80, 0x12, 0x04, 0x00, 0x7F, 0xEC, 0xC0, 0x40, 0x24, 0xC1, 0xC0 };

// Finally, once the pattern has been pasted above, make sure to include it in the PATTERNS table so it is drawn by the sand table!
// All patterns are in the format of {PATTERN_NAME, sizeof(PATTERN_NAME)}. See below for an example.

const pattern_t PATTERNS[] = {
  {FIGUREEIGHT_BIN, sizeof(FIGUREEIGHT_BIN)}
};

// For the sake of simplicity, X is A and Y is B. Technically it doesn't matter
// which is which as long as it's consistent.
const int A_DIR = 5;
const int A_STEP = 2;
const int B_DIR = 6;
const int B_STEP = 3;
const float BALL_DIAMETER = 12.0;
const int LIMIT_X = 9;
const int LIMIT_Y = 10;
const int PULSE_DELAY = 90;
const float STEPS_PER_MM = 170.0;
const float TABLE_HEIGHT = 420;
const float TABLE_WIDTH = 420;

// G-Code specific configuration
#ifdef GCODE_API
const int GCODE_BUFFER_SIZE = 32;
#endif

/////////////////////////////////////////////////////////////////////////////////////
// Configuration ends here. Everything beyond this point is for developers. ~ahill //
/////////////////////////////////////////////////////////////////////////////////////

#include <stdint.h>

// THETA is exclusive to the Turtle API to keep track of the direction the Turtle should be going. ~ahill
#ifdef TURTLE_API
int THETA;
#endif

// X and Y are used by the plotter API to keep track of where the ball is on the table. ~ahill
float X;
float Y;

// Stepper API

void step() {
  digitalWrite(A_STEP, HIGH);
  digitalWrite(B_STEP, HIGH);
  delayMicroseconds(PULSE_DELAY);
  digitalWrite(A_STEP, LOW);
  digitalWrite(B_STEP, LOW);
  delayMicroseconds(PULSE_DELAY);
}

void stepDown(uint32_t steps) {
  digitalWrite(A_DIR, LOW);
  digitalWrite(B_DIR, HIGH);
  for(uint32_t i = 0; i < steps; i++) {
    step();
  }
}

void stepLeft(uint32_t steps) {
  digitalWrite(A_DIR, HIGH);
  digitalWrite(B_DIR, HIGH);
  for(uint32_t i = 0; i < steps; i++) {
    step();
  }
}

void stepRight(uint32_t steps) {
  digitalWrite(A_DIR, LOW);
  digitalWrite(B_DIR, LOW);
  for(uint32_t i = 0; i < steps; i++) {
    step();
  }
}

void stepUp(uint32_t steps) {
  digitalWrite(A_DIR, HIGH);
  digitalWrite(B_DIR, LOW);
  for(uint32_t i = 0; i < steps; i++) {
    step();
  }
}

// Plotter API

void moveTo(float x, float y) {
  float accumulator;
  float delta = x - X;
  uint32_t distance;
  float slope;
  if(delta == 0) {
    // Vertical Line
    delta = y - Y;
    distance = (uint32_t)floor(abs(delta) * STEPS_PER_MM);
    if(delta > 0) {
      stepUp(distance);
    } else {
      stepDown(distance);
    }
  } else {
    // Slope-Intercept Line
    accumulator = 0.0;
    distance = (uint32_t)floor(abs(delta));
    slope = (y - Y) / abs(delta);
    for(uint32_t i = 0; i < distance; i++) {
      if(delta > 0.0) {
        // FIXME: This doesn't account for partial steps! ~ahill
        stepLeft(floor(STEPS_PER_MM));
      } else {
        stepRight(floor(STEPS_PER_MM));
      }
      accumulator += slope;
      if(abs(accumulator) >= 1.0) {
        if(accumulator > 0.0) {
          stepUp((uint32_t)floor(abs(floor(accumulator)) * STEPS_PER_MM));
        } else {
          stepDown((uint32_t)floor(abs(floor(accumulator)) * STEPS_PER_MM));
        }
        accumulator -= floor(accumulator);
      }
    }
  }
  X = x;
  Y = y;
}

// Utilties

int bufferParseInt(char *buffer, int *length = NULL) {
  int len = 0;
  int value = 0;
  while(isDigit(buffer[len])) {
    value = (value * 10) + (buffer[len++] - 48);
  }
  if(length != NULL) {
    *length = len;
  }
  return value;
}

void clearTable() {
  int iterations = max(TABLE_HEIGHT, TABLE_WIDTH) / (BALL_DIAMETER * 2);
  Serial.println("Clearing the table");
  resetTable();
  // TODO: How should we deal with table dimensions that are not divisble by the ball diameter? ~ahill
  for(int i = 0; i < iterations; i++) {
    moveTo(TABLE_WIDTH - (i * BALL_DIAMETER), i * BALL_DIAMETER);
    moveTo(TABLE_WIDTH - (i * BALL_DIAMETER), TABLE_HEIGHT - (i * BALL_DIAMETER));
    moveTo(i * BALL_DIAMETER, TABLE_HEIGHT - (i * BALL_DIAMETER));
    moveTo(i * BALL_DIAMETER, (i + 1) * BALL_DIAMETER);
  }
}

void resetTable() {
  while(digitalRead(LIMIT_Y) != LOW) {
    stepDown(1);
  }
  while(digitalRead(LIMIT_X) != LOW) {
    while(digitalRead(LIMIT_Y) != LOW) {
      stepDown(1);
    }
    stepRight(1);
  }
#ifdef TURTLE_API
  THETA = 0.0;
#endif
  X = 0.0;
  Y = 0.0;
}

// G-Code API

#ifdef GCODE_API
int gcBufferParseInt(char *buffer, int *length = NULL) {
  int len = 0;
  int value = 0;
  while(isDigit(buffer[len])) {
    value = (value * 10) + (buffer[len++] - 48);
  }
  if(length != NULL) {
    *length = len;
  }
  return value;
}

bool gcEvalG1(char *buffer) {
  int index = 0;
  int offset;
  // TODO: Read X and Y as floats instead of ints for accuracy's sake. ~ahill
  int x = 0;
  int y = 0;
  while(buffer[index] != 0) {
    switch(buffer[index]) {
      case 'X':
        x = gcBufferParseInt(buffer + (index + 1), &offset);
        break;
      case 'Y':
        y = gcBufferParseInt(buffer + (index + 1), &offset);
        break;
      default:
        Serial.print("STOP: Unknown axis: ");
        Serial.println(buffer[index]);
        return false;
    }
    index += offset + 1;
    // Skip the decimal point by finding the next whitespace or end of line. ~ahill
    while(buffer[index] != 0 && buffer[index] != ' ') {
      index++;
    }
    if(buffer[index] != 0) {
      index++;
    }
  }
  moveTo((float)x, (float)y);
  return true;
}

bool gcEvalM118(char *buffer) {
  Serial.println(buffer);
  return true;
}

bool gcEvalG(char *buffer) {
  int offset;
  int opcode = gcBufferParseInt(buffer, &offset);
  switch(opcode) {
    case 0:
    case 1:
      return gcEvalG1(buffer + 2);
    default:
      Serial.print("STOP: Unknown opcode: G");
      Serial.println(opcode, DEC);
      return false;
  }
}

bool gcEvalM(char *buffer) {
  int offset;
  int opcode = gcBufferParseInt(buffer, &offset);
  switch(opcode) {
    case 0:
    case 1:
      // Here lies gcEvalM1. We don't really need to have our own function for this, do we? ~ahill
      return false;
    case 118:
      return gcEvalM118(buffer + 4);
    default:
      Serial.print("STOP: Unknown opcode: M");
      Serial.println(opcode, DEC);
      return false;
  }
}

bool gcEval(char *buffer) {
  switch(buffer[0]) {
    case 0:
      // Blank line lol ~ahill
      return true;
    case 'G':
      return gcEvalG(buffer + 1);
    case 'M':
      return gcEvalM(buffer + 1);
    default:
      Serial.print("STOP: Unknown prefix: ");
      Serial.print(buffer[0]);
      return false;
  }
}

// Used for testing the G-Code API. To be removed later. ~ahill
void gcrepl() {
  char buffer[GCODE_BUFFER_SIZE];
  int inbound;
  int length = 0;
  int line = 0;
  bool running = true;
  Serial.println("Ready");
  while(running) {
    if(Serial.available() > 0) {
      inbound = Serial.read();
      if(inbound == 10 || inbound == 13) {
        Serial.println();
        buffer[length] = 0;
        running = gcEval(buffer);
        buffer[0] = 0;
        length = 0;
        line++;
        if(running) {
          Serial.println("Ready");
        }
      } else if(inbound == 8) {
        Serial.print("\b \b");
        buffer[length--] = 0;
      } else if(length < GCODE_BUFFER_SIZE - 1) {
        Serial.write(inbound);
        buffer[length++] = (char)(inbound & 0xFF);
      }
    }
  }
  Serial.print("G-Code stopped at line ");
  Serial.println(line, DEC);
}
#endif

// Turtle API

// The Turtle API is a bytecode interpreter I created to emulate Python's Turtle
// library in an embedded environment. The idea is to encode complex patterns in
// a concise manner to preserve resources. For example, circles encoded in a
// vector format like G-Code takes a lot of storage space. With Turtle, a circle
// can be created with a few simple instructions in a loop, greatly saving on
// storage space. ~ahill

// TURTLE BYTECODE FORMAT

// Instructions are encoded as 16-bit integers (or 8-bit in the case of atomic
// instructions) in the following format:

// IIDD DDDD DDDD DDDD
// I - Instruction
// D - Data

// Instruction codes are as follows:
// 00 - Forward (Move D units forward)
// 01 - Turn Left (Turn D degrees to the left)
// 10 - Loop (Repeat the following code, delimited by the "end loop" instruction, D times.)
// 11 - Atomic (More information below)

// ATOMIC INSTRUCTIONS
// Atomic instructions are 8-bit instructions that provide simple functionality
// such as ending loops and returning to the home position. Put simply, these
// are things that don't require a lot of information and shouldn't use the full
// 16-bit format in an attempt to remain frugal with system resources. They are
// encoded in the following format:

// 11SS SSSS
// S - Subroutine

// Subroutine codes are as follows:
// 000000 - End Loop
// 000001 - Center
// All others are reserved for the time being. ~ahill

#ifdef TURTLE_API
const float TURTLE_SCALE = 16384; // 2^14 for the 14 bits of data we have available. ~ahill

const float MM_TO_UNITS_X = TURTLE_SCALE / TABLE_WIDTH;
const float MM_TO_UNITS_Y = TURTLE_SCALE / TABLE_HEIGHT;
const float UNITS_TO_MM_X = TABLE_WIDTH / TURTLE_SCALE;
const float UNITS_TO_MM_Y = TABLE_HEIGHT / TURTLE_SCALE;

int trtlGetX() {
  return (int)((X * MM_TO_UNITS_X) - (TURTLE_SCALE / 2));
}

int trtlGetY() {
  return (int)((Y * MM_TO_UNITS_Y) - (TURTLE_SCALE / 2));
}

void trtlMoveTo(int x, int y) {
  float destX = (x + (TURTLE_SCALE / 2)) * UNITS_TO_MM_X;
  float destY = (y + (TURTLE_SCALE / 2)) * UNITS_TO_MM_Y;
  moveTo(destX, destY);
}

void trtlForward(int units) {
  float theta = THETA * DEG_TO_RAD;
  int x = (int)(cos(theta) * (float)units) + trtlGetX();
  int y = (int)(sin(theta) * (float)units) + trtlGetY();
  trtlMoveTo(x, y);
}

int trtlLoop(char *code, int length, int offset, int times) {
  char *block = code + offset + 2;
  int depth = 1;
  int len = 0;
  char temp;
  while(depth && ((offset + len + 2) < length)) {
    temp = *(block + len);
    if((temp & 0xC0) == 0xC0) {
      if((temp & 0xFF) == 0xC0) {
        depth--;
      }
      len++;
    } else {
      if((temp & 0xC0) == 0x80) {
        depth++;
      }
      len += 2;
    }
  }
  // If we have a loop with no end, this happens. ~ahill
  if((offset + len) >= length) {
    return 0;
  }
  // Otherwise, run the loop.
  for(int i = 0; i < times; i++) {
    trtlRun(block, len);
  }
  return len + 2;
}

void trtlAtomic(char sub) {
  switch(sub) {
    // Center
    case 1:
      moveTo(TABLE_WIDTH / 2, TABLE_HEIGHT / 2);
      break;
    default:
      // Everything else is a no-op. :) ~ahill
      break;
  }
}

int trtlExec(char *code, int length, int offset) {
  int data;
  char temp = *(code + offset);

  // Prevent out of bounds memory references. ~ahill
  if(temp & 0xC0 == 0xC0) {
    if(offset + 1 > length) {
      return 0;
    }
  } else {
    if(offset + 2 > length) {
      return 0;
    }
  }

  // Start parsing the instruction
  if((temp & 0xC0) != 0xC0) {
    // Apparently, casting to int without performing a bitwise AND results in
    // automatic sign extension. Not good... ~ahill
    data = (((int)temp & 0x3F) << 8) | ((int)*(code + offset + 1) & 0xFF);
    // Don't forget to extend the sign, unless we're looping! ~ahill
    if((data & 0x2000) && ((temp & 0xC0) != 0x80)) {
      data |= 0xC000;
    }
  } else {
    // Not actually needed, but saves what little sanity I have left when
    // troubleshooting. ~ahill
    data = (int)temp & 0x3F;
  }

  // Finally, figure out what we need to do and do it.
  switch((temp & 0xC0) >> 6) {
    // Forward
    case 0b00:
      trtlForward(data);
      return 2;
    // Turn Left
    case 0b01:
      THETA = (THETA + data) % 360;
      return 2;
    // Loop
    case 0b10:
      return trtlLoop(code, length, offset, data);
    // Atomic Instructions
    case 0b11:
      trtlAtomic(temp & 0x3F);
      return 1;
    default:
      // If we get to this point, I have no idea what I did wrong. ~ahill
      return 0;
  }
}

void trtlRun(char *code, int length) {
  int offset = 0;
  int result = 1;
  while(result && offset < length) {
    result = trtlExec(code, length, offset);
    offset += result;
  }
}
#endif

// Main Code

void loop() {
  for(int i = 0; i < (sizeof(PATTERNS) / sizeof(pattern_t)); i++) {
    clearTable();
    trtlRun(PATTERNS[i].pattern, PATTERNS[i].length);
  }
}

void setup() {
  Serial.begin(9600, SERIAL_8E1);
  pinMode(A_DIR, OUTPUT);
  pinMode(A_STEP, OUTPUT);
  pinMode(B_DIR, OUTPUT);
  pinMode(B_STEP, OUTPUT);
  pinMode(LIMIT_X, INPUT_PULLUP);
  pinMode(LIMIT_Y, INPUT_PULLUP);
  resetTable();
}