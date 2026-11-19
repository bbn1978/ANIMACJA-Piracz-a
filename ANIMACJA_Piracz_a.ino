#include <Arduino.h>
#include <hardware/clocks.h>
#include <hardware/gpio.h>
#include <pico/time.h>

#include <cstring>

#include "sleigh_line_art.h"

constexpr uint8_t PANEL_WIDTH = 64;
constexpr uint8_t PANEL_HEIGHT = 32;
constexpr uint8_t PANEL_ROWS = 2;
constexpr uint8_t PANEL_COLUMNS_PER_SCREEN = 4;
constexpr uint8_t SCREEN_COUNT = 2;
constexpr uint8_t SCREEN_PANEL_COUNT = PANEL_COLUMNS_PER_SCREEN * PANEL_ROWS;
constexpr uint8_t PANEL_COLUMNS = PANEL_COLUMNS_PER_SCREEN * SCREEN_COUNT;
constexpr uint8_t PANEL_COUNT = PANEL_COLUMNS * PANEL_ROWS;
constexpr uint16_t WIDTH = PANEL_COLUMNS * PANEL_WIDTH;
constexpr uint16_t SCREEN_WIDTH = PANEL_COLUMNS_PER_SCREEN * PANEL_WIDTH;
constexpr uint8_t HEIGHT = PANEL_ROWS * PANEL_HEIGHT;
constexpr uint8_t SOURCE_ROW_FOR_DISPLAY_ROW[PANEL_HEIGHT] = {
    1,  2,  3,  4,  5,  6,  7,  0,  9,  10, 11, 12, 13, 14, 15, 8,
    17, 18, 19, 20, 21, 22, 23, 16, 25, 26, 27, 28, 29, 30, 31, 24};
constexpr char SCROLLING_TEXT[] =
    " WESOŁYCH ŚWIĄT BOŻEGO NARODZENIA ";
constexpr uint8_t PS2_CLOCK_PIN = 21;
constexpr uint8_t PS2_DATA_PIN = 22;
constexpr uint32_t BASE_SCROLL_DELAY_US = 8296;
constexpr uint32_t HOLIDAY_ANIMATION_DURATION_MS = 30000;
constexpr uint32_t COCA_COLA_ANIMATION_DURATION_MS = 30000;
constexpr uint32_t STAR_ANIMATION_DURATION_MS = 30000;
constexpr uint32_t BALL_ANIMATION_DURATION_MS = 30000;
constexpr uint32_t REINDEER_ANIMATION_DURATION_MS = 10000;
constexpr uint32_t REINDEER_END_HOLD_MS = 5000;
constexpr uint32_t SLEIGH_ANIMATION_DURATION_MS = 30000;
constexpr uint32_t SLEIGH_TRAVEL_LEG_MS = SLEIGH_ANIMATION_DURATION_MS / 2;
constexpr int32_t SLEIGH_GROUP_WIDTH = 210;
constexpr int32_t SLEIGH_IMAGE_WIDTH = 256;
constexpr uint8_t PLAYBACK_SCENE_COUNT = 8;
constexpr uint8_t BALL_COUNT = 24;
constexpr uint8_t BREAKOUT_COLUMNS = 10;
constexpr uint8_t BREAKOUT_ROWS = 4;
constexpr uint8_t BREAKOUT_BRICK_COUNT = BREAKOUT_COLUMNS * BREAKOUT_ROWS;
constexpr uint8_t BREAKOUT_BRICK_GAP = 2;
constexpr uint8_t BREAKOUT_BRICK_WIDTH =
    (WIDTH - 8 - (BREAKOUT_COLUMNS - 1) * BREAKOUT_BRICK_GAP) /
    BREAKOUT_COLUMNS;
constexpr uint16_t BREAKOUT_GRID_WIDTH =
    BREAKOUT_COLUMNS * BREAKOUT_BRICK_WIDTH +
    (BREAKOUT_COLUMNS - 1) * BREAKOUT_BRICK_GAP;
constexpr uint16_t BREAKOUT_GRID_LEFT = (WIDTH - BREAKOUT_GRID_WIDTH) / 2;
constexpr uint8_t BREAKOUT_BRICK_HEIGHT = 5;
constexpr uint8_t BREAKOUT_PADDLE_WIDTH = 32;
constexpr uint8_t BREAKOUT_PADDLE_HEIGHT = 3;
constexpr uint8_t BREAKOUT_PADDLE_Y = HEIGHT - 5;
constexpr uint32_t TRUCK_WIDTH = 112;
constexpr uint32_t TRUCK_START_DELAY_MS = 2000;
constexpr int32_t HOUSE_LIGHT_LEAD_PX = 80;
constexpr uint32_t SANTA_STEP_INTERVAL_MS = 250;
constexpr uint8_t SANTA_WIDTH = 100;
constexpr uint8_t SANTA_HEIGHT = 60;
constexpr uint8_t REINDEER_WIDTH = 56;
constexpr uint8_t FLAG_WIDTH = 68;
constexpr uint8_t FLAG_HEIGHT = 32;
constexpr uint8_t FLAG_WHITE_HEIGHT = FLAG_HEIGHT / 2;
constexpr uint8_t SCROLL_SPEED_LEVELS = 12;
constexpr uint8_t DEFAULT_SCROLL_SPEED_INDEX = 5;
constexpr uint8_t SCROLL_SPEED_NUMERATORS[SCROLL_SPEED_LEVELS] = {
    1, 1, 1, 1, 1, 1, 2, 4, 8, 16, 25, 32};
constexpr uint8_t SCROLL_SPEED_DENOMINATORS[SCROLL_SPEED_LEVELS] = {
    32, 16, 8, 4, 2, 1, 1, 1, 1, 1, 1, 1};
constexpr uint8_t TEXT_COLORS[10] = {
    0b001, 0b010, 0b100, 0b011, 0b101, 0b110, 0b111, 0b001, 0b010, 0b100};
constexpr uint8_t TEXT_COLOR_COUNT = sizeof(TEXT_COLORS) / sizeof(TEXT_COLORS[0]);
constexpr uint16_t MAX_INPUT_CHARACTERS = 1000;
constexpr uint16_t INPUT_TEXT_CAPACITY = MAX_INPUT_CHARACTERS * 3 + 1;
constexpr uint8_t GLYPH_HEIGHT = 44;
constexpr uint8_t GLYPH_ADVANCE = 44;
constexpr uint8_t GLYPH_STROKE_WIDTH = 5;
constexpr uint8_t PANEL_SCAN_WIDTH = PANEL_WIDTH * 2;
constexpr uint16_t CHAIN_SCAN_WIDTH = PANEL_COUNT * PANEL_SCAN_WIDTH;
constexpr uint8_t ADDRESS_COUNT = 8;
constexpr bool ROW_MAPPING_TEST_MODE = false;
constexpr bool DIAGNOSTIC_MODE = false;
constexpr uint32_t ROW_MAPPING_TEST_INTERVAL_MS = 500;
constexpr uint32_t DIAGNOSTIC_INTERVAL_MS = 1000;
constexpr uint8_t DATA_PIN_BASE = 2;
constexpr uint8_t CLOCK_PIN = 13;
constexpr uint32_t DATA_MASK = 0xFC;
constexpr uint32_t CLOCK_MASK = 1u << CLOCK_PIN;
constexpr uint32_t ADDRESS_MASK = (1u << 18) | (1u << 19) | (1u << 20);
constexpr uint32_t LATCH_MASK = 1u << 16;
constexpr uint32_t OUTPUT_ENABLE_MASK = 1u << 17;
constexpr uint32_t MATRIX_GPIO_MASK =
    DATA_MASK | CLOCK_MASK | ADDRESS_MASK | LATCH_MASK | OUTPUT_ENABLE_MASK;
constexpr uint32_t CPU_CLOCK_KHZ = 125000;
constexpr uint32_t ROW_SLOT_US = 1000;
constexpr uint32_t ROW_ON_TIME_US = 3;
static_assert(CHAIN_SCAN_WIDTH == WIDTH * PANEL_ROWS * 2,
              "Scan width must include both panel rows and RGB streams");

enum class PlaybackScene : uint8_t {
  Text,
  Santa,
  CocaColaTruck,
  Stars,
  Balls,
  Breakout,
  Reindeer,
  SantaSleigh,
};

constexpr uint32_t encodeAddress(uint8_t row) {
  return (static_cast<uint32_t>(row & 0x03) << 18) |
         (static_cast<uint32_t>(row & 0x04) << 18);
}

uint8_t framebuffers[2][WIDTH * HEIGHT] = {};
uint16_t sourcePixelIndices[ADDRESS_COUNT][CHAIN_SCAN_WIDTH][2] = {};
volatile uint8_t activeFramebuffer = 0;
volatile bool scanFrameComplete = true;
volatile bool scanEnabled = false;
char inputText[INPUT_TEXT_CAPACITY] = {};
char scrollingText[INPUT_TEXT_CAPACITY] = {};
uint16_t inputTextLength = 0;
uint16_t inputCharacterCount = 0;
uint8_t scrollSpeedIndex = DEFAULT_SCROLL_SPEED_INDEX;
uint8_t textColorIndex = 0;
bool inputEditing = false;
bool pendingTextSubmit = false;
bool inputTextChanged = false;
bool pendingColorRestart = false;
int8_t pendingSceneNavigation = 0;
PlaybackScene playbackScene = PlaybackScene::Text;
uint32_t holidayAnimationStartMs = 0;
bool breakoutBricks[BREAKOUT_BRICK_COUNT] = {};
uint8_t breakoutBricksRemaining = 0;
int16_t breakoutPaddleX = (WIDTH - BREAKOUT_PADDLE_WIDTH) / 2;
int16_t breakoutBallX = WIDTH / 2;
int16_t breakoutBallY = BREAKOUT_PADDLE_Y - 4;
int8_t breakoutBallVelocityX = 1;
int8_t breakoutBallVelocityY = -1;
uint8_t breakoutPaddleBounceCount = 0;
volatile uint8_t ps2ScanQueue[32] = {};
volatile uint8_t ps2QueueHead = 0;
volatile uint8_t ps2QueueTail = 0;

const uint8_t* glyphFor(uint32_t character) {
  static const uint8_t blank[7] = {0, 0, 0, 0, 0, 0, 0};
  static const uint8_t glyphA[7] = {0b01110, 0b10001, 0b10001, 0b11111,
                                    0b10001, 0b10001, 0b10001};
  static const uint8_t glyphAogonek[7] = {0b01110, 0b10001, 0b10001, 0b11111,
                                          0b10001, 0b10001, 0b10011};
  static const uint8_t glyphB[7] = {0b11110, 0b10001, 0b10001, 0b11110,
                                    0b10001, 0b10001, 0b11110};
  static const uint8_t glyphC[7] = {0b01111, 0b10000, 0b10000, 0b10000,
                                    0b10000, 0b10000, 0b01111};
  static const uint8_t glyphCacute[7] = {0b00100, 0b01111, 0b10000, 0b10000,
                                         0b10000, 0b10000, 0b01111};
  static const uint8_t glyphD[7] = {0b11110, 0b10001, 0b10001, 0b10001,
                                    0b10001, 0b10001, 0b11110};
  static const uint8_t glyphE[7] = {0b11111, 0b10000, 0b10000, 0b11110,
                                    0b10000, 0b10000, 0b11111};
  static const uint8_t glyphEogonek[7] = {0b11111, 0b10000, 0b10000, 0b11110,
                                          0b10000, 0b10010, 0b01111};
  static const uint8_t glyphF[7] = {0b11111, 0b10000, 0b10000, 0b11110,
                                    0b10000, 0b10000, 0b10000};
  static const uint8_t glyphG[7] = {0b01111, 0b10000, 0b10000, 0b10111,
                                    0b10001, 0b10001, 0b01110};
  static const uint8_t glyphH[7] = {0b10001, 0b10001, 0b10001, 0b11111,
                                    0b10001, 0b10001, 0b10001};
  static const uint8_t glyphI[7] = {0b01110, 0b00100, 0b00100, 0b00100,
                                    0b00100, 0b00100, 0b01110};
  static const uint8_t glyphJ[7] = {0b00111, 0b00010, 0b00010, 0b00010,
                                    0b00010, 0b10010, 0b01100};
  static const uint8_t glyphK[7] = {0b10001, 0b10010, 0b10100, 0b11000,
                                    0b10100, 0b10010, 0b10001};
  static const uint8_t glyphL[7] = {0b10000, 0b10000, 0b10000, 0b10000,
                                    0b10000, 0b10000, 0b11111};
  static const uint8_t glyphLslashUpper[7] = {
      0b10000, 0b10001, 0b10010, 0b10100, 0b11000, 0b10000, 0b11111};
  static const uint8_t glyphLslashLower[7] = {
      0b00100, 0b00101, 0b00110, 0b01100, 0b00100, 0b00100, 0b00100};
  static const uint8_t glyphM[7] = {0b10001, 0b11011, 0b10101, 0b10101,
                                    0b10001, 0b10001, 0b10001};
  static const uint8_t glyphN[7] = {0b10001, 0b11001, 0b10101, 0b10011,
                                    0b10001, 0b10001, 0b10001};
  static const uint8_t glyphO[7] = {0b01110, 0b10001, 0b10001, 0b10001,
                                    0b10001, 0b10001, 0b01110};
  static const uint8_t glyphOacute[7] = {0b00010, 0b00100, 0b01110, 0b10001,
                                        0b10001, 0b10001, 0b01110};
  static const uint8_t glyphP[7] = {0b11110, 0b10001, 0b10001, 0b11110,
                                    0b10000, 0b10000, 0b10000};
  static const uint8_t glyphR[7] = {0b11110, 0b10001, 0b10001, 0b11110,
                                    0b10100, 0b10010, 0b10001};
  static const uint8_t glyphS[7] = {0b01111, 0b10000, 0b10000, 0b01110,
                                    0b00001, 0b00001, 0b11110};
  static const uint8_t glyphSacute[7] = {0b00100, 0b01111, 0b10000, 0b01110,
                                         0b00001, 0b00001, 0b11110};
  static const uint8_t glyphT[7] = {0b11111, 0b00100, 0b00100, 0b00100,
                                    0b00100, 0b00100, 0b00100};
  static const uint8_t glyphU[7] = {0b10001, 0b10001, 0b10001, 0b10001,
                                    0b10001, 0b01010, 0b00100};
  static const uint8_t glyphV[7] = {0b10001, 0b10001, 0b10001, 0b10001,
                                    0b10001, 0b01010, 0b00100};
  static const uint8_t glyphW[7] = {0b10001, 0b10001, 0b10001, 0b10101,
                                    0b10101, 0b10101, 0b01010};
  static const uint8_t glyphX[7] = {0b10001, 0b10001, 0b01010, 0b00100,
                                    0b01010, 0b10001, 0b10001};
  static const uint8_t glyphY[7] = {0b10001, 0b10001, 0b01010, 0b00100,
                                    0b00100, 0b00100, 0b00100};
  static const uint8_t glyphZ[7] = {0b11111, 0b00001, 0b00010, 0b00100,
                                    0b01000, 0b10000, 0b11111};
  static const uint8_t glyphZdot[7] = {0b00100, 0b11111, 0b00001, 0b00010,
                                       0b00100, 0b01000, 0b11111};
  static const uint8_t glyphZacute[7] = {0b00010, 0b00100, 0b11111, 0b00010,
                                         0b00100, 0b01000, 0b11111};
  static const uint8_t glyphDot[7] = {0b00000, 0b00000, 0b00000, 0b00000,
                                      0b00000, 0b00100, 0b00100};
  static const uint8_t glyph0[7] = {0b01110, 0b10001, 0b10011, 0b10101,
                                    0b11001, 0b10001, 0b01110};
  static const uint8_t glyph1[7] = {0b00100, 0b01100, 0b00100, 0b00100,
                                    0b00100, 0b00100, 0b01110};
  static const uint8_t glyph2[7] = {0b01110, 0b10001, 0b00001, 0b00010,
                                    0b00100, 0b01000, 0b11111};
  static const uint8_t glyph3[7] = {0b11110, 0b00001, 0b00001, 0b01110,
                                    0b00001, 0b00001, 0b11110};
  static const uint8_t glyph4[7] = {0b00010, 0b00110, 0b01010, 0b10010,
                                    0b11111, 0b00010, 0b00010};
  static const uint8_t glyph5[7] = {0b11111, 0b10000, 0b10000, 0b11110,
                                    0b00001, 0b00001, 0b11110};
  static const uint8_t glyph6[7] = {0b01110, 0b10000, 0b10000, 0b11110,
                                    0b10001, 0b10001, 0b01110};
  static const uint8_t glyph7[7] = {0b11111, 0b00001, 0b00010, 0b00100,
                                    0b01000, 0b01000, 0b01000};
  static const uint8_t glyph8[7] = {0b01110, 0b10001, 0b10001, 0b01110,
                                    0b10001, 0b10001, 0b01110};
  static const uint8_t glyph9[7] = {0b01110, 0b10001, 0b10001, 0b01111,
                                    0b00001, 0b00001, 0b01110};

  switch (character) {
    case 'A': case 'a':
      return glyphA;
    case 0x0104: case 0x0105:
      return glyphAogonek;
    case 'B': case 'b':
      return glyphB;
    case 'C': case 'c':
      return glyphC;
    case 0x0106: case 0x0107:
      return glyphCacute;
    case 'D': case 'd':
      return glyphD;
    case 'E': case 'e':
      return glyphE;
    case 0x0118: case 0x0119:
      return glyphEogonek;
    case 'F': case 'f':
      return glyphF;
    case 'G': case 'g':
      return glyphG;
    case 'H': case 'h':
      return glyphH;
    case 'I': case 'i':
      return glyphI;
    case 'J': case 'j':
      return glyphJ;
    case 'K': case 'k':
      return glyphK;
    case 'L': case 'l':
      return glyphL;
    case 0x0141:
      return glyphLslashUpper;
    case 0x0142:
      return glyphLslashLower;
    case 'M': case 'm':
      return glyphM;
    case 'N': case 'n':
      return glyphN;
    case 'O': case 'o':
      return glyphO;
    case 0x00D3: case 0x00F3:
      return glyphOacute;
    case 'P': case 'p':
      return glyphP;
    case 'R': case 'r':
      return glyphR;
    case 'S': case 's':
      return glyphS;
    case 0x015A: case 0x015B:
      return glyphSacute;
    case 'T': case 't':
      return glyphT;
    case 'U': case 'u':
      return glyphU;
    case 'V': case 'v':
      return glyphV;
    case 'W': case 'w':
      return glyphW;
    case 'X': case 'x':
      return glyphX;
    case 'Y': case 'y':
      return glyphY;
    case 'Z': case 'z':
      return glyphZ;
    case 0x0179: case 0x017A:
      return glyphZacute;
    case 0x017B: case 0x017C:
      return glyphZdot;
    case '0':
      return glyph0;
    case '1':
      return glyph1;
    case '2':
      return glyph2;
    case '3':
      return glyph3;
    case '4':
      return glyph4;
    case '5':
      return glyph5;
    case '6':
      return glyph6;
    case '7':
      return glyph7;
    case '8':
      return glyph8;
    case '9':
      return glyph9;
    case '.':
      return glyphDot;
    default:
      return blank;
  }
}

void ps2ClockCallback(uint gpio, uint32_t events) {
  if (gpio != PS2_CLOCK_PIN || (events & GPIO_IRQ_EDGE_FALL) == 0) {
    return;
  }

  static uint8_t bitIndex = 0;
  static uint8_t scanCode = 0;
  static uint8_t parityBit = 0;
  const bool bit = gpio_get(PS2_DATA_PIN);

  if (bitIndex == 0) {
    if (!bit) {
      scanCode = 0;
      parityBit = 0;
      bitIndex = 1;
    }
    return;
  }

  if (bitIndex <= 8) {
    scanCode |= static_cast<uint8_t>(bit) << (bitIndex - 1);
  } else if (bitIndex == 9) {
    parityBit = bit;
  } else {
    uint8_t ones = parityBit;
    for (uint8_t value = scanCode; value != 0; value >>= 1) {
      ones += value & 1;
    }
    if (bit && (ones & 1) != 0) {
      const uint8_t nextHead = (ps2QueueHead + 1) & 0x1F;
      if (nextHead != ps2QueueTail) {
        ps2ScanQueue[ps2QueueHead] = scanCode;
        ps2QueueHead = nextHead;
      }
    }
    bitIndex = 0;
    return;
  }
  ++bitIndex;
}

uint32_t keyCodepoint(uint8_t scanCode, bool shifted, bool capsLock,
                      bool altGr) {
  const bool uppercase = shifted != capsLock;
  if (altGr) {
    uint32_t accented = 0;
    switch (scanCode) {
      case 0x1C: accented = uppercase ? 0x0104 : 0x0105; break;
      case 0x21: accented = uppercase ? 0x0106 : 0x0107; break;
      case 0x24: accented = uppercase ? 0x0118 : 0x0119; break;
      case 0x44: accented = uppercase ? 0x00D3 : 0x00F3; break;
      case 0x1B: accented = uppercase ? 0x015A : 0x015B; break;
      case 0x4B: accented = shifted ? 0x0142 : 0x0141; break;
      case 0x1A: accented = uppercase ? 0x017B : 0x017C; break;
      case 0x22: accented = uppercase ? 0x0179 : 0x017A; break;
      default: return 0;
    }
    return accented;
  }

  switch (scanCode) {
    case 0x1C: return uppercase ? 'A' : 'a';
    case 0x32: return uppercase ? 'B' : 'b';
    case 0x21: return uppercase ? 'C' : 'c';
    case 0x23: return uppercase ? 'D' : 'd';
    case 0x24: return uppercase ? 'E' : 'e';
    case 0x2B: return uppercase ? 'F' : 'f';
    case 0x34: return uppercase ? 'G' : 'g';
    case 0x33: return uppercase ? 'H' : 'h';
    case 0x43: return uppercase ? 'I' : 'i';
    case 0x3B: return uppercase ? 'J' : 'j';
    case 0x42: return uppercase ? 'K' : 'k';
    case 0x4B: return uppercase ? 'L' : 'l';
    case 0x3A: return uppercase ? 'M' : 'm';
    case 0x31: return uppercase ? 'N' : 'n';
    case 0x44: return uppercase ? 'O' : 'o';
    case 0x4D: return uppercase ? 'P' : 'p';
    case 0x2D: return uppercase ? 'R' : 'r';
    case 0x1B: return uppercase ? 'S' : 's';
    case 0x2C: return uppercase ? 'T' : 't';
    case 0x3C: return uppercase ? 'U' : 'u';
    case 0x2A: return uppercase ? 'V' : 'v';
    case 0x1D: return uppercase ? 'W' : 'w';
    case 0x22: return uppercase ? 'X' : 'x';
    case 0x35: return uppercase ? 'Y' : 'y';
    case 0x1A: return uppercase ? 'Z' : 'z';
    case 0x45: return shifted ? 0 : '0';
    case 0x16: return shifted ? 0 : '1';
    case 0x1E: return shifted ? 0 : '2';
    case 0x26: return shifted ? 0 : '3';
    case 0x25: return shifted ? 0 : '4';
    case 0x2E: return shifted ? 0 : '5';
    case 0x36: return shifted ? 0 : '6';
    case 0x3D: return shifted ? 0 : '7';
    case 0x3E: return shifted ? 0 : '8';
    case 0x46: return shifted ? 0 : '9';
    case 0x29: return ' ';
    case 0x49: return shifted ? 0 : '.';
    default: return 0;
  }
}

void appendInputCodepoint(uint32_t codepoint) {
  const uint8_t* blankGlyph = glyphFor('?');
  if (codepoint != ' ' && glyphFor(codepoint) == blankGlyph) {
    return;
  }

  uint8_t encoded[3];
  uint8_t encodedLength;
  if (codepoint < 0x80) {
    encoded[0] = codepoint;
    encodedLength = 1;
  } else if (codepoint < 0x800) {
    encoded[0] = 0xC0 | (codepoint >> 6);
    encoded[1] = 0x80 | (codepoint & 0x3F);
    encodedLength = 2;
  } else {
    encoded[0] = 0xE0 | (codepoint >> 12);
    encoded[1] = 0x80 | ((codepoint >> 6) & 0x3F);
    encoded[2] = 0x80 | (codepoint & 0x3F);
    encodedLength = 3;
  }
  if (!inputEditing) {
    inputTextLength = 0;
    inputCharacterCount = 0;
    inputText[0] = '\0';
    inputEditing = true;
  }
  if (inputCharacterCount >= MAX_INPUT_CHARACTERS ||
      inputTextLength + encodedLength >= INPUT_TEXT_CAPACITY) {
    return;
  }
  for (uint8_t index = 0; index < encodedLength; ++index) {
    inputText[inputTextLength++] = encoded[index];
  }
  inputText[inputTextLength] = '\0';
  ++inputCharacterCount;
  inputTextChanged = true;
}

int8_t textColorForFunctionKey(uint8_t scanCode);

void processPs2ScanCode(uint8_t scanCode) {
  static bool extended = false;
  static bool breakCode = false;
  static bool leftShift = false;
  static bool rightShift = false;
  static bool leftAlt = false;
  static bool rightAlt = false;
  static bool capsLock = false;

  if (scanCode == 0xE0) {
    extended = true;
    return;
  }
  if (scanCode == 0xF0) {
    breakCode = true;
    return;
  }

  const bool pressed = !breakCode;
  if (!extended && scanCode == 0x12) {
    leftShift = pressed;
  } else if (!extended && scanCode == 0x59) {
    rightShift = pressed;
  } else if (!extended && scanCode == 0x11) {
    leftAlt = pressed;
  } else if (extended && scanCode == 0x11) {
    rightAlt = pressed;
  } else if (pressed && extended &&
             (scanCode == 0x75 || scanCode == 0x7D)) {
    if (scrollSpeedIndex + 1 < SCROLL_SPEED_LEVELS) {
      ++scrollSpeedIndex;
    }
  } else if (pressed && extended &&
             (scanCode == 0x72 || scanCode == 0x7A)) {
    if (scrollSpeedIndex > 0) {
      --scrollSpeedIndex;
    }
  } else if (pressed && extended &&
             (scanCode == 0x6B || scanCode == 0x74)) {
    if (!inputEditing) {
      pendingSceneNavigation = scanCode == 0x74 ? 1 : -1;
    }
  } else if (pressed && !extended &&
             textColorForFunctionKey(scanCode) >= 0) {
    textColorIndex = textColorForFunctionKey(scanCode);
    pendingColorRestart = true;
  } else if (!extended && scanCode == 0x58 && pressed) {
    capsLock = !capsLock;
  } else if (pressed && !extended && scanCode == 0x66) {
    if (!inputEditing) {
      inputTextLength = 0;
      inputCharacterCount = 0;
      inputText[0] = '\0';
      inputEditing = true;
      inputTextChanged = true;
    } else if (inputTextLength > 0) {
      do {
        --inputTextLength;
      } while (inputTextLength > 0 &&
               (static_cast<uint8_t>(inputText[inputTextLength]) & 0xC0) ==
                   0x80);
      inputText[inputTextLength] = '\0';
      --inputCharacterCount;
      inputTextChanged = true;
    }
  } else if (pressed && ((!extended && scanCode == 0x5A) ||
                         (extended && scanCode == 0x5A))) {
    if (inputTextLength > 0) {
      pendingTextSubmit = true;
      inputEditing = false;
    }
  } else if (pressed && !extended) {
    appendInputCodepoint(keyCodepoint(
        scanCode, leftShift || rightShift, capsLock, leftAlt || rightAlt));
  }

  extended = false;
  breakCode = false;
}

void processPs2Keyboard() {
  while (ps2QueueTail != ps2QueueHead) {
    const uint8_t scanCode = ps2ScanQueue[ps2QueueTail];
    ps2QueueTail = (ps2QueueTail + 1) & 0x1F;
    processPs2ScanCode(scanCode);
  }
}

int8_t textColorForFunctionKey(uint8_t scanCode) {
  switch (scanCode) {
    case 0x05: return 0;
    case 0x06: return 1;
    case 0x04: return 2;
    case 0x0C: return 3;
    case 0x03: return 4;
    case 0x0B: return 5;
    case 0x83: return 6;
    case 0x0A: return 7;
    case 0x01: return 8;
    case 0x09: return 9;
    default: return -1;
  }
}

uint32_t nextCodepoint(const char*& text) {
  const uint8_t first = static_cast<uint8_t>(*text++);
  if (first < 0x80) {
    return first;
  }
  if ((first & 0xE0) == 0xC0) {
    const uint8_t second = static_cast<uint8_t>(*text++);
    return ((first & 0x1F) << 6) | (second & 0x3F);
  }
  if ((first & 0xF0) == 0xE0) {
    const uint8_t second = static_cast<uint8_t>(*text++);
    const uint8_t third = static_cast<uint8_t>(*text++);
    return ((first & 0x0F) << 12) | ((second & 0x3F) << 6) |
           (third & 0x3F);
  }
  return '?';
}

int32_t measureTextWidth(const char* text) {
  int32_t glyphCount = 0;
  while (*text != '\0') {
    nextCodepoint(text);
    ++glyphCount;
  }
  return glyphCount * GLYPH_ADVANCE;
}

void setPixel(uint8_t* framebuffer, int32_t x, int32_t y, uint8_t color) {
  if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT) {
    return;
  }
  framebuffer[y * WIDTH + x] = color;
}

void drawLine(uint8_t* framebuffer, int32_t x0, int32_t y0, int32_t x1,
              int32_t y1, uint8_t color) {
  const int32_t dx = abs(x1 - x0);
  const int32_t sx = x0 < x1 ? 1 : -1;
  const int32_t dy = -abs(y1 - y0);
  const int32_t sy = y0 < y1 ? 1 : -1;
  int32_t error = dx + dy;

  while (true) {
    for (uint8_t strokeColumn = 0; strokeColumn < GLYPH_STROKE_WIDTH;
         ++strokeColumn) {
      setPixel(framebuffer, x0 + strokeColumn, y0, color);
    }
    if (x0 == x1 && y0 == y1) {
      break;
    }
    const int32_t doubledError = 2 * error;
    if (doubledError >= dy) {
      error += dy;
      x0 += sx;
    }
    if (doubledError <= dx) {
      error += dx;
      y0 += sy;
    }
  }
}

void renderText(uint8_t* framebuffer, const char* text, int32_t textX,
                uint8_t textColor) {
  constexpr int32_t textY = (HEIGHT - GLYPH_HEIGHT) / 2;

  memset(framebuffer, 0, WIDTH * HEIGHT);
  int32_t characterX = textX;
  for (const char* character = text; *character != '\0';) {
    const uint32_t codepoint = nextCodepoint(character);
    const uint8_t* glyph = glyphFor(codepoint);
    for (uint8_t row = 0; row < 7; ++row) {
      for (uint8_t column = 0; column < 5; ++column) {
        if ((glyph[row] & (1u << (4 - column))) == 0) {
          continue;
        }
        const int32_t x =
            characterX + column * GLYPH_ADVANCE / 6 +
            GLYPH_STROKE_WIDTH / 2;
        const int32_t y = textY + row * (GLYPH_HEIGHT - 1) / 6;
        constexpr int8_t neighborOffsets[4][2] = {
            {1, 0}, {-1, 1}, {0, 1}, {1, 1}};
        for (const auto& offset : neighborOffsets) {
          const int8_t neighborColumn = column + offset[0];
          const int8_t neighborRow = row + offset[1];
          if (neighborColumn < 0 || neighborColumn >= 5 ||
              neighborRow >= 7 ||
              (glyph[neighborRow] & (1u << (4 - neighborColumn))) == 0) {
            continue;
          }
          drawLine(framebuffer, x, y,
                   characterX + neighborColumn * GLYPH_ADVANCE / 6 +
                       GLYPH_STROKE_WIDTH / 2,
                   textY + neighborRow * (GLYPH_HEIGHT - 1) / 6, textColor);
        }
      }
    }
    characterX += GLYPH_ADVANCE;
  }
}

void fillRect(uint8_t* framebuffer, int32_t x, int32_t y, uint16_t width,
              uint8_t height, uint8_t color) {
  for (uint8_t row = 0; row < height; ++row) {
    for (uint16_t column = 0; column < width; ++column) {
      setPixel(framebuffer, x + column, y + row, color);
    }
  }
}

void drawChristmasTree(uint8_t* framebuffer) {
  constexpr int32_t treeX = 12;
  constexpr int32_t treeY = 14;
  fillRect(framebuffer, treeX + 13, treeY, 4, 4, 0b011);
  fillRect(framebuffer, treeX + 10, treeY + 4, 10, 7, 0b010);
  fillRect(framebuffer, treeX + 7, treeY + 11, 16, 8, 0b010);
  fillRect(framebuffer, treeX + 4, treeY + 19, 22, 10, 0b010);
  fillRect(framebuffer, treeX + 12, treeY + 29, 6, 8, 0b001);
  fillRect(framebuffer, treeX + 9, treeY + 8, 3, 3, 0b100);
  fillRect(framebuffer, treeX + 18, treeY + 15, 3, 3, 0b101);
  fillRect(framebuffer, treeX + 7, treeY + 22, 3, 3, 0b011);
  fillRect(framebuffer, treeX + 19, treeY + 24, 3, 3, 0b100);
  fillRect(framebuffer, treeX + 3, treeY + 37, 10, 8, 0b001);
  fillRect(framebuffer, treeX + 16, treeY + 36, 11, 9, 0b100);
  fillRect(framebuffer, treeX + 7, treeY + 37, 2, 8, 0b111);
  fillRect(framebuffer, treeX + 3, treeY + 40, 10, 2, 0b111);
  fillRect(framebuffer, treeX + 20, treeY + 36, 3, 9, 0b111);
  fillRect(framebuffer, treeX + 16, treeY + 39, 11, 2, 0b111);
}

int8_t flagWaveOffset(uint8_t column, uint32_t elapsedMs) {
  constexpr int8_t offsets[8] = {0, 1, 2, 1, 0, -1, -2, -1};
  const uint8_t phase = (elapsedMs / 100) & 7;
  return offsets[(column / 5 + phase) & 7];
}

void drawFlagTextLine(uint8_t* framebuffer, const char* text, int32_t x,
                      int32_t y, uint32_t elapsedMs, uint8_t textColor) {
  uint8_t glyphCount = 0;
  for (const char* character = text; *character != '\0';) {
    nextCodepoint(character);
    ++glyphCount;
  }

  int32_t characterX = x + (FLAG_WIDTH - glyphCount * 6) / 2;
  for (const char* character = text; *character != '\0';) {
    const uint32_t codepoint = nextCodepoint(character);
    const uint8_t* glyph = glyphFor(codepoint);
    for (uint8_t row = 0; row < 7; ++row) {
      for (uint8_t column = 0; column < 5; ++column) {
        if ((glyph[row] & (1u << (4 - column))) == 0) {
          continue;
        }
        const int32_t pixelX = characterX + column;
        const uint8_t flagColumn = pixelX - x;
        const int32_t pixelY =
            y + row + flagWaveOffset(flagColumn, elapsedMs);
        setPixel(framebuffer, pixelX, pixelY, textColor);
        setPixel(framebuffer, pixelX + 1, pixelY, textColor);
      }
    }
    characterX += 6;
  }
}

void drawPolishFlag(uint8_t* framebuffer, int32_t x, int32_t y,
                    uint32_t elapsedMs) {
  const int32_t flagX = x + 32;
  const int32_t flagY = y + 12;
  fillRect(framebuffer, flagX - 3, flagY - 5, 2, 43, 0b011);

  for (uint8_t column = 0; column < FLAG_WIDTH; ++column) {
    const int32_t wave = flagWaveOffset(column, elapsedMs);
    for (uint8_t row = 0; row < FLAG_HEIGHT; ++row) {
      const uint8_t color = row < FLAG_WHITE_HEIGHT ? 0b111 : 0b001;
      setPixel(framebuffer, flagX + column, flagY + row + wave, color);
    }
  }

  drawFlagTextLine(framebuffer, "WESOŁYCH", flagX, flagY + 4, elapsedMs,
                   0b001);
  drawFlagTextLine(framebuffer, "ŚWIĄT", flagX,
                   flagY + FLAG_WHITE_HEIGHT + 4, elapsedMs, 0b111);
}

void drawSanta(uint8_t* framebuffer, int32_t x, int32_t y,
               uint8_t walkingFrame, uint32_t elapsedMs) {
  fillRect(framebuffer, x + 13, y, 6, 5, 0b111);
  fillRect(framebuffer, x + 10, y + 4, 12, 4, 0b001);
  fillRect(framebuffer, x + 7, y + 8, 18, 4, 0b001);
  fillRect(framebuffer, x + 5, y + 12, 22, 4, 0b001);
  fillRect(framebuffer, x + 4, y + 15, 24, 4, 0b111);
  fillRect(framebuffer, x + 9, y + 19, 14, 11, 0b011);
  fillRect(framebuffer, x + 11, y + 22, 2, 2, 0b100);
  fillRect(framebuffer, x + 19, y + 22, 2, 2, 0b100);
  fillRect(framebuffer, x + 7, y + 28, 18, 10, 0b111);
  fillRect(framebuffer, x + 9, y + 35, 14, 5, 0b111);
  fillRect(framebuffer, x + 13, y + 39, 6, 4, 0b111);
  fillRect(framebuffer, x + 14, y + 26, 4, 3, 0b011);
  fillRect(framebuffer, x + 6, y + 34, 20, 4, 0b111);
  fillRect(framebuffer, x + 7, y + 37, 18, 12, 0b001);
  fillRect(framebuffer, x + 7, y + 45, 18, 3, 0b111);
  fillRect(framebuffer, x + 14, y + 45, 4, 3, 0b011);

  const int32_t armOffset = walkingFrame ? 3 : 0;
  fillRect(framebuffer, x + 2, y + 37 + armOffset, 6, 8, 0b001);
  fillRect(framebuffer, x + 24, y + 37 - armOffset, 6, 8, 0b001);
  fillRect(framebuffer, x + 2, y + 44 + armOffset, 6, 3, 0b111);
  fillRect(framebuffer, x + 24, y + 44 - armOffset, 6, 3, 0b111);
  fillRect(framebuffer, x + 3, y + 47 + armOffset, 4, 3, 0b010);
  fillRect(framebuffer, x + 25, y + 47 - armOffset, 4, 3, 0b010);

  const int32_t leftLegOffset = walkingFrame ? 2 : 0;
  const int32_t rightLegOffset = walkingFrame ? 0 : 2;
  fillRect(framebuffer, x + 9 + leftLegOffset, y + 49, 6, 7, 0b100);
  fillRect(framebuffer, x + 17 + rightLegOffset, y + 49, 6, 7, 0b100);
  fillRect(framebuffer, x + 7 + leftLegOffset, y + 56, 8, 4, 0b011);
  fillRect(framebuffer, x + 17 + rightLegOffset, y + 56, 8, 4, 0b011);
  drawPolishFlag(framebuffer, x, y, elapsedMs);
}

void drawSnowfall(uint8_t* framebuffer, uint32_t elapsedMs) {
  const uint32_t drift = elapsedMs / 80;
  for (uint8_t flake = 0; flake < 18; ++flake) {
    const int32_t x = (flake * 47 + drift * 3) % WIDTH;
    const int32_t y = (flake * 19 + drift) % HEIGHT;
    const uint8_t color = flake % 3 == 0 ? 0b110 : 0b111;
    setPixel(framebuffer, x, y, color);
    if (flake % 2 == 0) {
      setPixel(framebuffer, x + 1, y + 1, color);
    }
  }
}

void drawPixelLine(uint8_t* framebuffer, int32_t x0, int32_t y0, int32_t x1,
                   int32_t y1, uint8_t color) {
  const int32_t dx = abs(x1 - x0);
  const int32_t sx = x0 < x1 ? 1 : -1;
  const int32_t dy = -abs(y1 - y0);
  const int32_t sy = y0 < y1 ? 1 : -1;
  int32_t error = dx + dy;

  while (true) {
    setPixel(framebuffer, x0, y0, color);
    if (x0 == x1 && y0 == y1) {
      break;
    }
    const int32_t doubledError = 2 * error;
    if (doubledError >= dy) {
      error += dy;
      x0 += sx;
    }
    if (doubledError <= dx) {
      error += dx;
      y0 += sy;
    }
  }
}

void drawReindeer(uint8_t* framebuffer, int32_t x, int32_t y,
                  uint8_t runningFrame) {
  constexpr uint8_t bodyColor = 0b011;
  constexpr uint8_t antlerColor = 0b111;
  constexpr uint8_t hoofColor = 0b001;
  const int8_t stride = runningFrame ? 3 : -3;

  fillRect(framebuffer, x + 11, y + 22, 26, 12, bodyColor);
  fillRect(framebuffer, x + 16, y + 19, 17, 5, bodyColor);
  fillRect(framebuffer, x + 31, y + 17, 9, 14, bodyColor);
  fillRect(framebuffer, x + 35, y + 10, 13, 12, bodyColor);
  fillRect(framebuffer, x + 43, y + 15, 8, 5, bodyColor);
  fillRect(framebuffer, x + 49, y + 16, 3, 3, 0b001);
  setPixel(framebuffer, x + 45, y + 13, 0b111);
  fillRect(framebuffer, x + 33, y + 21, 8, 2, 0b001);
  setPixel(framebuffer, x + 37, y + 21, 0b111);
  drawPixelLine(framebuffer, x + 12, y + 24, x + 6, y + 20, 0b111);

  drawPixelLine(framebuffer, x + 37, y + 12, x + 35, y + 7, antlerColor);
  drawPixelLine(framebuffer, x + 35, y + 7, x + 31, y + 4, antlerColor);
  drawPixelLine(framebuffer, x + 35, y + 7, x + 38, y + 3, antlerColor);
  drawPixelLine(framebuffer, x + 44, y + 12, x + 43, y + 7, antlerColor);
  drawPixelLine(framebuffer, x + 43, y + 7, x + 40, y + 3, antlerColor);
  drawPixelLine(framebuffer, x + 43, y + 7, x + 48, y + 4, antlerColor);

  drawPixelLine(framebuffer, x + 16, y + 32, x + 16 + stride, y + 39,
                bodyColor);
  drawPixelLine(framebuffer, x + 16 + stride, y + 39, x + 16 + stride * 2,
                y + 46, bodyColor);
  drawPixelLine(framebuffer, x + 24, y + 32, x + 24 - stride, y + 39,
                bodyColor);
  drawPixelLine(framebuffer, x + 24 - stride, y + 39, x + 24 - stride * 2,
                y + 46, bodyColor);
  drawPixelLine(framebuffer, x + 33, y + 30, x + 33 - stride, y + 38,
                bodyColor);
  drawPixelLine(framebuffer, x + 33 - stride, y + 38, x + 33 - stride * 2,
                y + 46, bodyColor);
  drawPixelLine(framebuffer, x + 39, y + 29, x + 39 + stride, y + 37,
                bodyColor);
  drawPixelLine(framebuffer, x + 39 + stride, y + 37, x + 39 + stride * 2,
                y + 46, bodyColor);
  fillRect(framebuffer, x + 13 + stride, y + 46, 4, 2, hoofColor);
  fillRect(framebuffer, x + 21 - stride, y + 46, 4, 2, hoofColor);
  fillRect(framebuffer, x + 30 - stride * 2, y + 46, 4, 2, hoofColor);
  fillRect(framebuffer, x + 36 + stride * 2, y + 46, 4, 2, hoofColor);
}

void drawSleighLine(uint8_t* framebuffer, int32_t originX, int32_t originY,
                    int8_t direction, int32_t x0, int32_t y0, int32_t x1,
                    int32_t y1) {
  const int32_t screenX0 =
      originX + (direction > 0 ? x0 : SLEIGH_GROUP_WIDTH - 1 - x0);
  const int32_t screenX1 =
      originX + (direction > 0 ? x1 : SLEIGH_GROUP_WIDTH - 1 - x1);
  drawPixelLine(framebuffer, screenX0, originY + y0, screenX1, originY + y1,
                0b111);
}

void drawOutlineReindeer(uint8_t* framebuffer, int32_t originX,
                         int32_t originY, int8_t direction, int32_t x,
                         uint8_t runningFrame) {
  const int8_t stride = runningFrame ? 3 : -3;
  drawSleighLine(framebuffer, originX, originY, direction, x + 1, 25,
                 x + 3, 19);
  drawSleighLine(framebuffer, originX, originY, direction, x + 3, 19,
                 x + 9, 16);
  drawSleighLine(framebuffer, originX, originY, direction, x + 9, 16,
                 x + 19, 17);
  drawSleighLine(framebuffer, originX, originY, direction, x + 19, 17,
                 x + 25, 22);
  drawSleighLine(framebuffer, originX, originY, direction, x + 25, 22,
                 x + 24, 28);
  drawSleighLine(framebuffer, originX, originY, direction, x + 24, 28,
                 x + 19, 32);
  drawSleighLine(framebuffer, originX, originY, direction, x + 19, 32,
                 x + 5, 32);
  drawSleighLine(framebuffer, originX, originY, direction, x + 5, 32,
                 x + 1, 25);

  drawSleighLine(framebuffer, originX, originY, direction, x + 18, 19,
                 x + 24, 12);
  drawSleighLine(framebuffer, originX, originY, direction, x + 24, 12,
                 x + 29, 9);
  drawSleighLine(framebuffer, originX, originY, direction, x + 29, 9,
                 x + 37, 10);
  drawSleighLine(framebuffer, originX, originY, direction, x + 37, 10,
                 x + 41, 15);
  drawSleighLine(framebuffer, originX, originY, direction, x + 41, 15,
                 x + 38, 19);
  drawSleighLine(framebuffer, originX, originY, direction, x + 38, 19,
                 x + 30, 20);
  drawSleighLine(framebuffer, originX, originY, direction, x + 30, 20,
                 x + 24, 16);
  drawSleighLine(framebuffer, originX, originY, direction, x + 24, 16,
                 x + 18, 19);
  drawSleighLine(framebuffer, originX, originY, direction, x + 28, 10,
                 x + 26, 5);
  drawSleighLine(framebuffer, originX, originY, direction, x + 26, 5,
                 x + 22, 2);
  drawSleighLine(framebuffer, originX, originY, direction, x + 26, 5,
                 x + 30, 1);
  drawSleighLine(framebuffer, originX, originY, direction, x + 34, 10,
                 x + 34, 5);
  drawSleighLine(framebuffer, originX, originY, direction, x + 34, 5,
                 x + 39, 2);
  drawSleighLine(framebuffer, originX, originY, direction, x + 34, 5,
                 x + 30, 1);
  drawSleighLine(framebuffer, originX, originY, direction, x + 29, 13,
                 x + 31, 13);
  drawSleighLine(framebuffer, originX, originY, direction, x + 39, 15,
                 x + 44, 16);

  drawSleighLine(framebuffer, originX, originY, direction, x + 7, 31,
                 x + 7 + stride, 39);
  drawSleighLine(framebuffer, originX, originY, direction, x + 7 + stride, 39,
                 x + 7 + stride * 2, 45);
  drawSleighLine(framebuffer, originX, originY, direction, x + 12, 32,
                 x + 12 - stride, 39);
  drawSleighLine(framebuffer, originX, originY, direction, x + 12 - stride,
                 39, x + 12 - stride * 2, 45);
  drawSleighLine(framebuffer, originX, originY, direction, x + 19, 31,
                 x + 19 - stride, 39);
  drawSleighLine(framebuffer, originX, originY, direction, x + 19 - stride,
                 39, x + 19 - stride * 2, 45);
  drawSleighLine(framebuffer, originX, originY, direction, x + 23, 29,
                 x + 23 + stride, 37);
  drawSleighLine(framebuffer, originX, originY, direction, x + 23 + stride,
                 37, x + 23 + stride * 2, 45);
  drawSleighLine(framebuffer, originX, originY, direction, x + 5 + stride * 2,
                 45, x + 10 + stride * 2, 45);
  drawSleighLine(framebuffer, originX, originY, direction,
                 x + 10 - stride * 2, 45, x + 15 - stride * 2, 45);
  drawSleighLine(framebuffer, originX, originY, direction,
                 x + 16 - stride * 2, 45, x + 21 - stride * 2, 45);
  drawSleighLine(framebuffer, originX, originY, direction,
                 x + 20 + stride * 2, 45, x + 25 + stride * 2, 45);
}

void drawSantaSleigh(uint8_t* framebuffer, int32_t x, uint32_t elapsedMs,
                    int8_t direction) {
  constexpr int32_t y = 2;
  const uint8_t gallopFrame = (elapsedMs / 140) & 1;
  const int8_t bounce = gallopFrame ? 1 : 0;

  drawSleighLine(framebuffer, x, y, direction, 7, 30, 48, 30);
  drawSleighLine(framebuffer, x, y, direction, 48, 30, 69, 34);
  drawSleighLine(framebuffer, x, y, direction, 69, 34, 78, 42);
  drawSleighLine(framebuffer, x, y, direction, 78, 42, 72, 49);
  drawSleighLine(framebuffer, x, y, direction, 72, 49, 18, 49);
  drawSleighLine(framebuffer, x, y, direction, 18, 49, 7, 43);
  drawSleighLine(framebuffer, x, y, direction, 7, 43, 7, 30);
  drawSleighLine(framebuffer, x, y, direction, 14, 35, 57, 35);
  drawSleighLine(framebuffer, x, y, direction, 14, 35, 19, 45);
  drawSleighLine(framebuffer, x, y, direction, 19, 45, 68, 45);
  drawSleighLine(framebuffer, x, y, direction, 2, 52, 63, 52);
  drawSleighLine(framebuffer, x, y, direction, 63, 52, 77, 55);
  drawSleighLine(framebuffer, x, y, direction, 77, 55, 82, 52);
  drawSleighLine(framebuffer, x, y, direction, 82, 52, 78, 48);
  drawSleighLine(framebuffer, x, y, direction, 73, 52, 69, 56);
  drawSleighLine(framebuffer, x, y, direction, 69, 56, 20, 57);
  drawSleighLine(framebuffer, x, y, direction, 20, 57, 7, 55);
  drawSleighLine(framebuffer, x, y, direction, 7, 55, 2, 52);
  drawSleighLine(framebuffer, x, y, direction, 2, 52, 0, 55);
  drawSleighLine(framebuffer, x, y, direction, 0, 55, 4, 59);
  drawSleighLine(framebuffer, x, y, direction, 4, 59, 12, 57);
  drawSleighLine(framebuffer, x, y, direction, 12, 57, 15, 53);

  drawSleighLine(framebuffer, x, y, direction, 21, 31 + bounce, 22, 23);
  drawSleighLine(framebuffer, x, y, direction, 22, 23, 28, 19);
  drawSleighLine(framebuffer, x, y, direction, 28, 19, 35, 20);
  drawSleighLine(framebuffer, x, y, direction, 35, 20, 39, 25);
  drawSleighLine(framebuffer, x, y, direction, 39, 25, 36, 31);
  drawSleighLine(framebuffer, x, y, direction, 36, 31, 27, 33);
  drawSleighLine(framebuffer, x, y, direction, 27, 33, 21, 31 + bounce);
  drawSleighLine(framebuffer, x, y, direction, 23, 24, 34, 24);
  drawSleighLine(framebuffer, x, y, direction, 20, 31, 38, 31);
  drawSleighLine(framebuffer, x, y, direction, 20, 31, 16, 43);
  drawSleighLine(framebuffer, x, y, direction, 16, 43, 38, 43);
  drawSleighLine(framebuffer, x, y, direction, 38, 43, 43, 31);
  drawSleighLine(framebuffer, x, y, direction, 21, 34, 13, 39);
  drawSleighLine(framebuffer, x, y, direction, 13, 39, 18, 42);
  drawSleighLine(framebuffer, x, y, direction, 33, 33, 41, 38);
  drawSleighLine(framebuffer, x, y, direction, 41, 38, 38, 42);
  drawSleighLine(framebuffer, x, y, direction, 24, 20, 21, 16);
  drawSleighLine(framebuffer, x, y, direction, 21, 16, 23, 11);
  drawSleighLine(framebuffer, x, y, direction, 23, 11, 34, 10);
  drawSleighLine(framebuffer, x, y, direction, 34, 10, 39, 15);
  drawSleighLine(framebuffer, x, y, direction, 39, 15, 35, 20);
  drawSleighLine(framebuffer, x, y, direction, 21, 16, 26, 12);
  drawSleighLine(framebuffer, x, y, direction, 26, 12, 35, 12);
  drawSleighLine(framebuffer, x, y, direction, 20, 15, 26, 7);
  drawSleighLine(framebuffer, x, y, direction, 26, 7, 35, 9);
  drawSleighLine(framebuffer, x, y, direction, 20, 15, 17, 12);
  drawSleighLine(framebuffer, x, y, direction, 17, 12, 21, 11);
  drawSleighLine(framebuffer, x, y, direction, 21, 11, 24, 13);
  drawSleighLine(framebuffer, x, y, direction, 29, 16, 31, 16);
  drawSleighLine(framebuffer, x, y, direction, 24, 22, 34, 22);

  drawSleighLine(framebuffer, x, y, direction, 68, 27, 111, 24);
  drawSleighLine(framebuffer, x, y, direction, 69, 31, 111, 30);
  drawSleighLine(framebuffer, x, y, direction, 111, 24, 158, 22);
  drawSleighLine(framebuffer, x, y, direction, 111, 30, 158, 29);
  drawOutlineReindeer(framebuffer, x, y, direction, 108, gallopFrame);
  drawOutlineReindeer(framebuffer, x, y, direction, 155, gallopFrame ^ 1);
}

void drawOutlinedRect(uint8_t* framebuffer, int32_t x, int32_t y,
                      uint8_t width, uint8_t height, uint8_t color) {
  fillRect(framebuffer, x, y, width, 1, color);
  fillRect(framebuffer, x, y + height - 1, width, 1, color);
  fillRect(framebuffer, x, y, 1, height, color);
  fillRect(framebuffer, x + width - 1, y, 1, height, color);
}

void drawChristmasHouse(uint8_t* framebuffer, int32_t x, uint8_t width,
                       uint8_t bodyHeight, int32_t truckX) {
  constexpr int32_t groundY = 55;
  constexpr uint8_t sideWidth = 6;
  constexpr uint8_t lightColors[4] = {0b001, 0b010, 0b011, 0b111};
  constexpr uint8_t facadeColors[6] = {
      0b001, 0b010, 0b011, 0b100, 0b101, 0b110};
  const uint8_t frontWidth = width - sideWidth;
  const int32_t frontTop = groundY - bodyHeight;
  const int32_t roofY = frontTop - 8;
  const int32_t roofPeakX = x + frontWidth / 2;
  const int32_t frontRightX = x + frontWidth - 1;
  const int32_t backRightX = x + width - 1;
  const int32_t backRoofPeakX = roofPeakX + sideWidth;
  const int32_t backRoofPeakY = roofY - 2;
  const int32_t backRoofEaveY = frontTop + 3;

  drawPixelLine(framebuffer, roofPeakX, roofY, x, frontTop, 0b111);
  drawPixelLine(framebuffer, roofPeakX, roofY, frontRightX, frontTop, 0b111);
  drawPixelLine(framebuffer, roofPeakX, roofY, backRoofPeakX,
                backRoofPeakY, 0b111);
  drawPixelLine(framebuffer, frontRightX, frontTop, backRightX,
                backRoofEaveY, 0b111);
  drawPixelLine(framebuffer, backRoofPeakX, backRoofPeakY, backRightX,
                backRoofEaveY, 0b111);
  drawPixelLine(framebuffer, x, frontTop, frontRightX, frontTop, 0b111);

  drawPixelLine(framebuffer, x, frontTop, x, groundY - 1, 0b111);
  drawPixelLine(framebuffer, x, groundY - 1, frontRightX, groundY - 1,
                0b111);
  drawPixelLine(framebuffer, frontRightX, frontTop, frontRightX,
                groundY - 1, 0b111);
  drawPixelLine(framebuffer, frontRightX, groundY - 1, backRightX,
                groundY - 1, 0b111);
  drawPixelLine(framebuffer, backRightX, groundY - 1, backRightX,
                backRoofEaveY, 0b111);

  int32_t revealProgress =
      x + width - 1 + HOUSE_LIGHT_LEAD_PX - truckX;
  if (revealProgress < 0) {
    revealProgress = 0;
  } else if (revealProgress > HOUSE_LIGHT_LEAD_PX) {
    revealProgress = HOUSE_LIGHT_LEAD_PX;
  }
  const int32_t frontRevealWidth =
      revealProgress * (frontWidth - 2) / HOUSE_LIGHT_LEAD_PX;
  const int32_t sideRevealWidth =
      revealProgress * (sideWidth - 2) / HOUSE_LIGHT_LEAD_PX;
  const int32_t firstLitFrontX =
      x + frontWidth - 1 - frontRevealWidth;
  const int32_t firstLitSideX = x + width - 1 - sideRevealWidth;

  for (uint8_t row = 0; row < 8; ++row) {
    const int32_t roofHalfWidth = frontWidth / 2 * row / 7;
    const int32_t roofLeftX = roofPeakX - roofHalfWidth;
    const int32_t roofRightX =
        min(roofPeakX + roofHalfWidth, frontRightX);
    for (int32_t pixelX = roofLeftX + 1; pixelX < roofRightX; ++pixelX) {
      if (pixelX >= firstLitFrontX) {
        setPixel(framebuffer, pixelX, roofY + row, 0b001);
      }
    }

    const int32_t sideRoofStartX = roofPeakX + row;
    for (uint8_t column = 1; column < sideWidth + 1; ++column) {
      const int32_t pixelX = sideRoofStartX + column;
      if (pixelX >= firstLitSideX && pixelX < backRightX) {
        setPixel(framebuffer, pixelX, roofY + row, 0b001);
      }
    }
  }

  for (uint8_t row = 1; row < bodyHeight - 1; ++row) {
    for (uint8_t column = 1; column < frontWidth - 1; ++column) {
      const int32_t pixelX = x + column;
      if (pixelX >= firstLitFrontX) {
        const uint8_t colorIndex = (column + row) % 6;
        setPixel(framebuffer, pixelX, frontTop + row,
                 facadeColors[colorIndex]);
      }
    }
    for (uint8_t column = 1; column < sideWidth - 1; ++column) {
      const int32_t pixelX = x + frontWidth + column;
      if (pixelX >= firstLitSideX) {
        setPixel(framebuffer, pixelX, frontTop + row,
                 facadeColors[(column + row + 2) % 6]);
      }
    }
  }

  const int32_t doorX = x + frontWidth / 2 - 2;
  drawOutlinedRect(framebuffer, doorX, groundY - 9, 4, 9, 0b111);
  const int32_t windowY = frontTop + 4;
  const int32_t leftWindowX = x + 4;
  const int32_t rightWindowX = x + frontWidth - 8;
  const int32_t sideWindowX = x + frontWidth + 1;
  drawOutlinedRect(framebuffer, leftWindowX, windowY, 4, 6, 0b111);
  drawOutlinedRect(framebuffer, rightWindowX, windowY, 4, 6, 0b111);
  drawOutlinedRect(framebuffer, sideWindowX, frontTop + 6, 4, 5, 0b111);
  drawPixelLine(framebuffer, x + 2, frontTop - 1,
                x + frontWidth - 2, frontTop - 1, 0b111);

  for (uint8_t row = 1; row < 5; ++row) {
    for (uint8_t column = 1; column < 3; ++column) {
      const int32_t leftPixelX = leftWindowX + column;
      const int32_t rightPixelX = rightWindowX + column;
      setPixel(framebuffer, leftPixelX, windowY + row,
               leftPixelX >= firstLitFrontX ? 0b011 : 0b000);
      setPixel(framebuffer, rightPixelX, windowY + row,
               rightPixelX >= firstLitFrontX ? 0b011 : 0b000);
    }
  }
  for (uint8_t row = 1; row < 4; ++row) {
    for (uint8_t column = 1; column < 3; ++column) {
      const int32_t pixelX = sideWindowX + column;
      setPixel(framebuffer, pixelX, frontTop + 6 + row,
               pixelX >= firstLitSideX ? 0b011 : 0b000);
    }
  }

  for (uint8_t bulb = 0; bulb < 6; ++bulb) {
    const int32_t bulbX = x + 2 + bulb * 4;
    if (bulbX >= firstLitFrontX && bulbX < x + frontWidth - 1) {
      setPixel(framebuffer, bulbX, frontTop - 1, lightColors[bulb % 4]);
    }
  }
  for (uint8_t bulb = 0; bulb < 4; ++bulb) {
    const int32_t bulbX = x + frontWidth + 1 + (bulb % 3);
    const int32_t bulbY = frontTop + 8 + bulb * 4;
    if (bulbX >= firstLitSideX && bulbY < groundY - 2) {
      setPixel(framebuffer, bulbX, bulbY, lightColors[(bulb + 2) % 4]);
    }
  }

  const int32_t wreathX = doorX;
  const int32_t wreathY = groundY - 8;
  const int32_t wreathPixelX[5] = {
      wreathX + 1, wreathX, wreathX + 2, wreathX + 1, wreathX + 1};
  const int32_t wreathPixelY[5] = {
      wreathY, wreathY + 1, wreathY + 1, wreathY + 2, wreathY + 1};
  constexpr uint8_t wreathColors[5] = {
      0b010, 0b010, 0b010, 0b010, 0b100};
  for (uint8_t pixel = 0; pixel < 5; ++pixel) {
    if (wreathPixelX[pixel] >= firstLitFrontX) {
      setPixel(framebuffer, wreathPixelX[pixel], wreathPixelY[pixel],
               wreathColors[pixel]);
    }
  }
}

void drawStreetDecorations(uint8_t* framebuffer, int32_t truckX) {
  constexpr uint8_t lightColors[4] = {0b100, 0b010, 0b011, 0b111};
  for (uint16_t light = 0; light < WIDTH / 8; ++light) {
    const int32_t x = light * 8;
    const int32_t y = 10 + (light % 3);
    const bool lightsOn = truckX <= x + HOUSE_LIGHT_LEAD_PX;
    setPixel(framebuffer, x, y, lightsOn ? lightColors[light % 4] : 0b001);
  }
  for (uint16_t lampX = 130; lampX < WIDTH; lampX += 128) {
    fillRect(framebuffer, lampX, 23, 2, 31, 0b111);
    fillRect(framebuffer, lampX - 6, 22, 14, 2, 0b111);
    if (truckX <= lampX + HOUSE_LIGHT_LEAD_PX) {
      fillRect(framebuffer, lampX - 4, 24, 10, 4, 0b011);
    }
  }
}

void drawTruckWheel(uint8_t* framebuffer, int32_t centerX, int32_t centerY,
                    uint32_t elapsedMs) {
  constexpr int8_t radius = 4;
  for (int8_t y = -radius; y <= radius; ++y) {
    for (int8_t x = -radius; x <= radius; ++x) {
      if (x * x + y * y <= radius * radius) {
        setPixel(framebuffer, centerX + x, centerY + y, 0b011);
      }
    }
  }

  constexpr int8_t directionX[8] = {1, 1, 0, -1, -1, -1, 0, 1};
  constexpr int8_t directionY[8] = {0, 1, 1, 1, 0, -1, -1, -1};
  const uint8_t rotation = (elapsedMs / 120) & 7;
  for (uint8_t spoke = 0; spoke < 4; ++spoke) {
    const uint8_t direction = (rotation + spoke * 2) & 7;
    for (int8_t distance = 1; distance <= 3; ++distance) {
      const int32_t x = directionX[direction] * distance;
      const int32_t y = directionY[direction] * distance;
      if (x * x + y * y <= radius * radius) {
        setPixel(framebuffer, centerX + x, centerY + y, 0b000);
      }
    }
  }
  setPixel(framebuffer, centerX, centerY, 0b000);
}

void drawCocaColaTruck(uint8_t* framebuffer, int32_t x, uint32_t elapsedMs) {
  constexpr int32_t truckY = 31;
  fillRect(framebuffer, x + 32, truckY, 76, 19, 0b001);
  fillRect(framebuffer, x + 35, truckY + 3, 70, 2, 0b111);
  fillRect(framebuffer, x + 35, truckY + 14, 70, 2, 0b111);

  fillRect(framebuffer, x, truckY + 8, 30, 12, 0b001);
  fillRect(framebuffer, x + 7, truckY + 2, 20, 10, 0b001);
  fillRect(framebuffer, x + 10, truckY + 4, 13, 6, 0b110);
  fillRect(framebuffer, x + 1, truckY + 16, 5, 2, 0b111);

  const char logo[] = "COCA COLA";
  int32_t logoX = x + 43;
  for (const char* character = logo; *character != '\0'; ++character) {
    const uint8_t* glyph = glyphFor(*character);
    for (uint8_t row = 0; row < 7; ++row) {
      for (uint8_t column = 0; column < 5; ++column) {
        if ((glyph[row] & (1u << (4 - column))) != 0) {
          setPixel(framebuffer, logoX + column, truckY + 6 + row, 0b111);
        }
      }
    }
    logoX += 6;
  }

  drawTruckWheel(framebuffer, x + 14, truckY + 20, elapsedMs);
  drawTruckWheel(framebuffer, x + 82, truckY + 20, elapsedMs);
  drawTruckWheel(framebuffer, x + 102, truckY + 20, elapsedMs);
}

void renderCocaColaAnimation(uint8_t* framebuffer, uint32_t elapsedMs) {
  memset(framebuffer, 0, WIDTH * HEIGHT);
  fillRect(framebuffer, 0, 55, WIDTH, 9, 0b101);
  for (uint16_t x = 0; x < WIDTH; x += 24) {
    fillRect(framebuffer, x, 59, 12, 1, 0b111);
  }

  const uint32_t movementDuration =
      COCA_COLA_ANIMATION_DURATION_MS - TRUCK_START_DELAY_MS;
  const uint32_t movementTime =
      elapsedMs > TRUCK_START_DELAY_MS ? elapsedMs - TRUCK_START_DELAY_MS : 0;
  constexpr int32_t truckStartX = WIDTH - 40;
  const uint32_t travelDistance = truckStartX + TRUCK_WIDTH;
  const int32_t truckX =
      truckStartX -
      static_cast<int32_t>((movementTime * travelDistance) / movementDuration);

  constexpr uint8_t houseWidths[4] = {28, 30, 28, 26};
  constexpr uint8_t houseHeights[4] = {19, 16, 17, 20};
  uint8_t houseIndex = 0;
  for (uint16_t houseX = 0; houseX < WIDTH; houseX += 36, ++houseIndex) {
    const uint8_t style = houseIndex % 4;
    drawChristmasHouse(framebuffer, houseX, houseWidths[style],
                       houseHeights[style], truckX);
  }
  drawStreetDecorations(framebuffer, truckX);
  drawSnowfall(framebuffer, elapsedMs);
  drawCocaColaTruck(framebuffer, truckX, elapsedMs);
}

int32_t bouncingPosition(uint32_t elapsedMs, uint32_t initialPhase,
                         uint8_t millisecondsPerPixel, int32_t minimum,
                         int32_t maximum) {
  const uint32_t range = maximum - minimum;
  const uint32_t period = range * 2;
  const uint32_t phase =
      (initialPhase + elapsedMs / millisecondsPerPixel) % period;
  const uint32_t offset = phase <= range ? phase : period - phase;
  return minimum + offset;
}

void drawFivePointStar(uint8_t* framebuffer, int32_t centerX, int32_t centerY,
                       uint8_t color) {
  constexpr int8_t starX[10] = {0, 2, 8, 3, 5, 0, -5, -3, -8, -2};
  constexpr int8_t starY[10] = {-8, -2, -2, 1, 8, 4, 8, 1, -2, -2};

  for (int8_t y = -8; y <= 8; ++y) {
    for (int8_t x = -8; x <= 8; ++x) {
      bool inside = false;
      for (uint8_t point = 0, previous = 9; point < 10; previous = point++) {
        const int8_t x1 = starX[point];
        const int8_t y1 = starY[point];
        const int8_t x2 = starX[previous];
        const int8_t y2 = starY[previous];
        if ((y1 > y) != (y2 > y)) {
          const int16_t edgeY = y2 - y1;
          const int16_t intersectionNumerator =
              x1 * edgeY + (y - y1) * (x2 - x1);
          if ((edgeY > 0 && x * edgeY < intersectionNumerator) ||
              (edgeY < 0 && x * edgeY > intersectionNumerator)) {
            inside = !inside;
          }
        }
      }
      if (inside) {
        setPixel(framebuffer, centerX + x, centerY + y, color);
      }
    }
  }
}

void renderStarAnimation(uint8_t* framebuffer, uint32_t elapsedMs) {
  constexpr uint8_t starColors[7] = {
      0b001, 0b010, 0b100, 0b011, 0b101, 0b110, 0b111};
  memset(framebuffer, 0, WIDTH * HEIGHT);
  drawSnowfall(framebuffer, elapsedMs);

  for (uint8_t star = 0; star < 6; ++star) {
    const int32_t centerX = bouncingPosition(
        elapsedMs, star * 79, 24 + (star % 5) * 7, 8, WIDTH - 9);
    const int32_t centerY = bouncingPosition(
        elapsedMs, star * 31, 35 + (star * 3 % 6) * 7, 8, HEIGHT - 9);
    const uint8_t color = starColors[(elapsedMs / 400 + star * 3) % 7];
    drawFivePointStar(framebuffer, centerX, centerY, color);
  }
}

void drawBall(uint8_t* framebuffer, int32_t centerX, int32_t centerY,
              uint8_t radius, uint8_t color) {
  for (int8_t y = -radius; y <= radius; ++y) {
    for (int8_t x = -radius; x <= radius; ++x) {
      if (x * x + y * y <= radius * radius) {
        setPixel(framebuffer, centerX + x, centerY + y, color);
      }
    }
  }
  setPixel(framebuffer, centerX - 1, centerY - 1, 0b111);
}

void renderBallAnimation(uint8_t* framebuffer, uint32_t elapsedMs) {
  constexpr uint8_t ballColors[7] = {
      0b001, 0b010, 0b100, 0b011, 0b101, 0b110, 0b111};
  memset(framebuffer, 0, WIDTH * HEIGHT);

  for (uint8_t ball = 0; ball < BALL_COUNT; ++ball) {
    const uint8_t radius = 2 + ball % 4;
    const int32_t x = bouncingPosition(
        elapsedMs, ball * 79, 24 + (ball % 5) * 7, radius,
        WIDTH - radius - 1);
    const int32_t y = bouncingPosition(
        elapsedMs, ball * 31, 35 + (ball * 3 % 6) * 7, radius,
        HEIGHT - radius - 1);
    const uint8_t color =
        ballColors[(elapsedMs / 400 + ball * 3) % 7];
    drawBall(framebuffer, x, y, radius, color);
  }
}

void startBreakoutGame() {
  for (uint8_t brick = 0; brick < BREAKOUT_BRICK_COUNT; ++brick) {
    breakoutBricks[brick] = true;
  }
  breakoutBricksRemaining = BREAKOUT_BRICK_COUNT;
  breakoutPaddleX = (WIDTH - BREAKOUT_PADDLE_WIDTH) / 2;
  breakoutBallX = WIDTH / 2;
  breakoutBallY = BREAKOUT_PADDLE_Y - 4;
  breakoutBallVelocityX = 1;
  breakoutBallVelocityY = -1;
  breakoutPaddleBounceCount = 0;
}

int8_t findBreakoutAimVelocity(int16_t startX, int16_t startY) {
  constexpr int8_t candidateVelocities[4] = {-2, -1, 1, 2};
  uint8_t bestHitStep = HEIGHT;
  int8_t bestVelocity = 0;

  for (uint8_t candidate = 0; candidate < 4; ++candidate) {
    int16_t x = startX;
    int16_t y = startY;
    int8_t velocityX = candidateVelocities[candidate];

    for (uint8_t step = 1; step < HEIGHT; ++step) {
      int16_t nextX = x + velocityX;
      if (nextX < 1) {
        nextX = 1;
        velocityX = abs(velocityX);
      } else if (nextX >= WIDTH - 1) {
        nextX = WIDTH - 2;
        velocityX = -abs(velocityX);
      }

      const int16_t nextY = y - 1;
      if (nextY < 1) {
        break;
      }

      bool hitBrick = false;
      for (uint8_t row = 0; row < BREAKOUT_ROWS && !hitBrick; ++row) {
        const int16_t brickY = 4 + row * (BREAKOUT_BRICK_HEIGHT + 2);
        if (nextY < brickY || nextY >= brickY + BREAKOUT_BRICK_HEIGHT) {
          continue;
        }
        for (uint8_t column = 0; column < BREAKOUT_COLUMNS; ++column) {
          const uint8_t brick = row * BREAKOUT_COLUMNS + column;
          const int16_t brickX =
              BREAKOUT_GRID_LEFT +
              column * (BREAKOUT_BRICK_WIDTH + BREAKOUT_BRICK_GAP);
          if (breakoutBricks[brick] && nextX >= brickX &&
              nextX < brickX + BREAKOUT_BRICK_WIDTH) {
            hitBrick = true;
            break;
          }
        }
      }

      if (hitBrick) {
        if (step < bestHitStep) {
          bestHitStep = step;
          bestVelocity = candidateVelocities[candidate];
        }
        break;
      }

      x = nextX;
      y = nextY;
    }
  }

  return bestVelocity;
}

void advanceBreakoutGame() {
  constexpr int8_t paddleAimOffsets[8] = {0, -8, 8, -12, 12, -4, 4, 0};
  if (breakoutBricksRemaining == 0) {
    return;
  }

  int16_t paddleTarget =
      breakoutBallX - BREAKOUT_PADDLE_WIDTH / 2 +
      paddleAimOffsets[breakoutPaddleBounceCount & 7];
  const int16_t maxPaddleX = WIDTH - BREAKOUT_PADDLE_WIDTH;
  if (paddleTarget < 0) {
    paddleTarget = 0;
  } else if (paddleTarget > maxPaddleX) {
    paddleTarget = maxPaddleX;
  }
  if (breakoutPaddleX < paddleTarget) {
    breakoutPaddleX += 2;
    if (breakoutPaddleX > paddleTarget) {
      breakoutPaddleX = paddleTarget;
    }
  } else if (breakoutPaddleX > paddleTarget) {
    breakoutPaddleX -= 2;
    if (breakoutPaddleX < paddleTarget) {
      breakoutPaddleX = paddleTarget;
    }
  }

  int16_t nextX = breakoutBallX + breakoutBallVelocityX;
  int16_t nextY = breakoutBallY + breakoutBallVelocityY;
  if (nextX < 1) {
    nextX = 1;
    breakoutBallVelocityX = abs(breakoutBallVelocityX);
  } else if (nextX >= WIDTH - 1) {
    nextX = WIDTH - 2;
    breakoutBallVelocityX = -abs(breakoutBallVelocityX);
  }
  if (nextY < 1) {
    nextY = 1;
    breakoutBallVelocityY = abs(breakoutBallVelocityY);
  }

  if (breakoutBallVelocityY > 0 && nextY >= BREAKOUT_PADDLE_Y &&
      breakoutBallY < BREAKOUT_PADDLE_Y &&
      nextX >= breakoutPaddleX - 1 &&
      nextX <= breakoutPaddleX + BREAKOUT_PADDLE_WIDTH) {
    const int16_t paddleCenter = breakoutPaddleX + BREAKOUT_PADDLE_WIDTH / 2;
    const int16_t hitOffset = nextX - paddleCenter;
    const int8_t endgameVelocity =
        breakoutBricksRemaining <= 5
            ? findBreakoutAimVelocity(nextX, BREAKOUT_PADDLE_Y - 1)
            : 0;
    if (endgameVelocity != 0) {
      breakoutBallVelocityX = endgameVelocity;
    } else if (hitOffset < -8) {
      breakoutBallVelocityX = -2;
    } else if (hitOffset < -2) {
      breakoutBallVelocityX = -1;
    } else if (hitOffset > 8) {
      breakoutBallVelocityX = 2;
    } else if (hitOffset > 2) {
      breakoutBallVelocityX = 1;
    }
    breakoutBallVelocityY = -1;
    ++breakoutPaddleBounceCount;
    nextY = BREAKOUT_PADDLE_Y - 1;
  }

  bool brickHit = false;
  for (uint8_t row = 0; row < BREAKOUT_ROWS && !brickHit; ++row) {
    const int16_t brickY = 4 + row * (BREAKOUT_BRICK_HEIGHT + 2);
    if (nextY < brickY || nextY >= brickY + BREAKOUT_BRICK_HEIGHT) {
      continue;
    }
    for (uint8_t column = 0; column < BREAKOUT_COLUMNS; ++column) {
      const uint8_t brick = row * BREAKOUT_COLUMNS + column;
      const int16_t brickX =
          BREAKOUT_GRID_LEFT +
          column * (BREAKOUT_BRICK_WIDTH + BREAKOUT_BRICK_GAP);
      if (breakoutBricks[brick] && nextX >= brickX &&
          nextX < brickX + BREAKOUT_BRICK_WIDTH) {
        breakoutBricks[brick] = false;
        --breakoutBricksRemaining;
        breakoutBallVelocityY = -breakoutBallVelocityY;
        nextY = breakoutBallY + breakoutBallVelocityY;
        brickHit = true;
        break;
      }
    }
  }

  if (nextY >= HEIGHT - 1) {
    breakoutBallX = WIDTH / 2;
    breakoutBallY = BREAKOUT_PADDLE_Y - 2;
    breakoutBallVelocityX = 1;
    breakoutBallVelocityY = -1;
    return;
  }

  breakoutBallX = nextX;
  breakoutBallY = nextY;
}

void renderBreakoutGame(uint8_t* framebuffer) {
  constexpr uint8_t brickColors[BREAKOUT_ROWS] = {
      0b001, 0b011, 0b010, 0b100};
  memset(framebuffer, 0, WIDTH * HEIGHT);
  const uint8_t updatesPerFrame =
      breakoutBricksRemaining == 1
          ? 4
          : (breakoutBricksRemaining <= 5 ? 2 : 1);
  for (uint8_t update = 0; update < updatesPerFrame; ++update) {
    advanceBreakoutGame();
  }

  for (uint8_t row = 0; row < BREAKOUT_ROWS; ++row) {
    const int32_t brickY = 4 + row * (BREAKOUT_BRICK_HEIGHT + 2);
    for (uint8_t column = 0; column < BREAKOUT_COLUMNS; ++column) {
      const uint8_t brick = row * BREAKOUT_COLUMNS + column;
      if (!breakoutBricks[brick]) {
        continue;
      }
      const int32_t brickX =
          BREAKOUT_GRID_LEFT +
          column * (BREAKOUT_BRICK_WIDTH + BREAKOUT_BRICK_GAP);
      fillRect(framebuffer, brickX, brickY, BREAKOUT_BRICK_WIDTH,
               BREAKOUT_BRICK_HEIGHT, brickColors[row]);
      fillRect(framebuffer, brickX, brickY, BREAKOUT_BRICK_WIDTH, 1, 0b111);
    }
  }

  fillRect(framebuffer, breakoutPaddleX, BREAKOUT_PADDLE_Y,
           BREAKOUT_PADDLE_WIDTH, BREAKOUT_PADDLE_HEIGHT, 0b110);
  fillRect(framebuffer, breakoutPaddleX, BREAKOUT_PADDLE_Y,
           BREAKOUT_PADDLE_WIDTH, 1, 0b111);
  drawBall(framebuffer, breakoutBallX, breakoutBallY, 1, 0b111);
}

void renderHolidayAnimation(uint8_t* framebuffer, uint32_t elapsedMs) {
  memset(framebuffer, 0, WIDTH * HEIGHT);
  drawSnowfall(framebuffer, elapsedMs);
  drawChristmasTree(framebuffer);

  const int32_t travelDistance = WIDTH + SANTA_WIDTH;
  const int32_t distance =
      static_cast<int32_t>((elapsedMs * travelDistance) /
                           HOLIDAY_ANIMATION_DURATION_MS);
  const int32_t santaX = WIDTH - distance;
  const int32_t santaY = (HEIGHT - SANTA_HEIGHT) / 2;
  const uint8_t walkingFrame =
      (elapsedMs / SANTA_STEP_INTERVAL_MS) & 1;
  drawSanta(framebuffer, santaX, santaY, walkingFrame, elapsedMs);
}

void renderReindeerAnimation(uint8_t* framebuffer, uint32_t elapsedMs) {
  constexpr char endText[] = "KONIEC";
  constexpr int32_t textHeight = 38;
  constexpr int32_t glyphWidth = textHeight * 5 / 7;
  constexpr int32_t characterAdvance = textHeight * 6 / 7;
  constexpr int32_t textWidth =
      (sizeof(endText) - 2) * characterAdvance + glyphWidth;
  constexpr int32_t reindeerGap = 10;
  memset(framebuffer, 0, WIDTH * HEIGHT);
  drawSnowfall(framebuffer, elapsedMs);

  const int32_t finalReindeerX = WIDTH - REINDEER_WIDTH + 2;
  const uint32_t travelDistance = finalReindeerX + REINDEER_WIDTH;
  constexpr uint32_t crossingDuration =
      REINDEER_ANIMATION_DURATION_MS - REINDEER_END_HOLD_MS;
  const uint32_t crossingTime =
      elapsedMs < crossingDuration
          ? elapsedMs
          : crossingDuration;
  const int32_t reindeerX =
      -REINDEER_WIDTH +
      static_cast<int32_t>((crossingTime * travelDistance) /
                           crossingDuration);
  const uint8_t runningFrame =
      elapsedMs < crossingDuration ? (elapsedMs / 120) & 1 : 0;
  const int32_t reindeerY = 7 + runningFrame;
  const int32_t textX = reindeerX - textWidth - reindeerGap;
  const int32_t textY = reindeerY + 6;

  int32_t characterX = textX;
  for (const char* character = endText; *character != '\0'; ++character) {
    const uint8_t* glyph = glyphFor(*character);
    for (int32_t y = 0; y < textHeight; ++y) {
      const uint8_t sourceRow = y * 7 / textHeight;
      for (int32_t x = 0; x < glyphWidth; ++x) {
        const uint8_t sourceColumn = x * 5 / glyphWidth;
        if ((glyph[sourceRow] & (1u << (4 - sourceColumn))) != 0) {
          setPixel(framebuffer, characterX + x, textY + y, 0b111);
        }
      }
    }
    characterX += characterAdvance;
  }
  drawPixelLine(framebuffer, textX + textWidth, textY + textHeight / 2,
                reindeerX + 4, reindeerY + 27, 0b111);
  drawReindeer(framebuffer, reindeerX, reindeerY, runningFrame);
}

void renderSantaSleighAnimation(uint8_t* framebuffer, uint32_t elapsedMs) {
  memset(framebuffer, 0, WIDTH * HEIGHT);
  const uint32_t travelTime = elapsedMs % SLEIGH_ANIMATION_DURATION_MS;
  const uint32_t legTime = travelTime % SLEIGH_TRAVEL_LEG_MS;
  const int32_t travelDistance = WIDTH + SLEIGH_IMAGE_WIDTH;
  const bool movingRight = travelTime < SLEIGH_TRAVEL_LEG_MS;
  const int32_t x =
      movingRight
          ? -SLEIGH_IMAGE_WIDTH +
                static_cast<int32_t>((legTime * travelDistance) /
                                     SLEIGH_TRAVEL_LEG_MS)
          : WIDTH - static_cast<int32_t>((legTime * travelDistance) /
                                         SLEIGH_TRAVEL_LEG_MS);
  const int8_t gallopFrame = (elapsedMs / 140) & 1 ? 1 : -1;
  for (uint8_t y = 0; y < HEIGHT; ++y) {
    for (int32_t column = 0; column < SLEIGH_IMAGE_WIDTH; ++column) {
      const int32_t sourceColumn =
          movingRight ? column : SLEIGH_IMAGE_WIDTH - 1 - column;
      const uint8_t packedFill =
          SLEIGH_FILL_MAP[y][sourceColumn / 4];
      const uint8_t fillClass =
          (packedFill >> (6 - 2 * (sourceColumn % 4))) & 0x03;
      const bool outline =
          (SLEIGH_LINE_ART[y][sourceColumn / 32] &
           (1u << (31 - sourceColumn % 32))) != 0;
      const int8_t hoofOffset =
          y >= 38 && sourceColumn >= 120
              ? (sourceColumn < 188 ? gallopFrame : -gallopFrame)
              : 0;
      uint8_t color = 0;
      if (fillClass == 1 || fillClass == 2) {
        color = 0b001;
      } else if (fillClass == 3) {
        color = (sourceColumn + y) % 4 == 0 ? 0b011 : 0b001;
      }
      if (outline) {
        color = 0b111;
      }
      if (color != 0) {
        setPixel(framebuffer, x + column, y + hoofOffset, color);
      }
    }
  }
}

uint16_t panelPixelIndex(uint16_t chainX, uint8_t address, bool lowerRow) {
  constexpr uint8_t physicalBoxForSourceBox[8] = {2, 6, 0, 4, 3, 7, 1, 5};
  const uint8_t chainPanel = chainX / PANEL_SCAN_WIDTH;
  const uint8_t panelColumn = chainX % PANEL_SCAN_WIDTH;
  const uint8_t screenIndex =
      SCREEN_COUNT - 1 - chainPanel / SCREEN_PANEL_COUNT;
  const uint8_t panelInScreen = chainPanel % SCREEN_PANEL_COUNT;
  const bool lowerPanelRow = panelInScreen < PANEL_COLUMNS_PER_SCREEN;
  const uint8_t chainColumn = panelInScreen % PANEL_COLUMNS_PER_SCREEN;
  const uint8_t panelColumnIndex = chainColumn;
  const uint8_t panelRowIndex = lowerPanelRow ? 0 : 1;
  const uint8_t scanBlock = panelColumn / 32;
  const uint8_t scanBox = scanBlock * 2 + (lowerRow ? 1 : 0);
  const uint8_t boxIndex = physicalBoxForSourceBox[scanBox];
  const uint8_t physicalX =
      (boxIndex % 2) * 32 + (panelColumn % 32);
  const uint8_t physicalY = (boxIndex / 2) * ADDRESS_COUNT + address;
  const uint16_t x =
      (screenIndex == 0 ? SCREEN_WIDTH : 0) +
      panelColumnIndex * PANEL_WIDTH + physicalX;
  const uint8_t sourceY =
      panelRowIndex * PANEL_HEIGHT + SOURCE_ROW_FOR_DISPLAY_ROW[physicalY];
  return sourceY * WIDTH + x;
}

void initializeSourcePixelIndices() {
  for (uint8_t row = 0; row < ADDRESS_COUNT; ++row) {
    for (uint16_t chainX = 0; chainX < CHAIN_SCAN_WIDTH; ++chainX) {
      sourcePixelIndices[row][chainX][0] =
          panelPixelIndex(chainX, row, false);
      sourcePixelIndices[row][chainX][1] =
          panelPixelIndex(chainX, row, true);
    }
  }
}

uint8_t diagnosticColor(uint8_t address, bool blinkPhase) {
  constexpr uint8_t colors[7] = {0b001, 0b010, 0b100, 0b011,
                                 0b101, 0b110, 0b111};
  return address == 7 ? (blinkPhase ? 0b001 : 0b100) : colors[address];
}

void drawDiagnosticAddress(uint8_t* framebuffer, uint8_t address,
                           uint8_t color) {
  memset(framebuffer, 0, WIDTH * HEIGHT);
  if (address >= ADDRESS_COUNT) {
    return;
  }
  for (uint8_t row = address; row < HEIGHT; row += ADDRESS_COUNT) {
    for (uint16_t x = 0; x < WIDTH; ++x) {
      setPixel(framebuffer, x, row, color);
    }
  }
}

void drawRowMappingTest(uint8_t* framebuffer, uint8_t row) {
  memset(framebuffer, 0, WIDTH * HEIGHT);
  if (row >= HEIGHT) {
    return;
  }
  for (uint16_t x = 0; x < WIDTH; ++x) {
    setPixel(framebuffer, x, row, 0b111);
  }
}

void showRowMappingTest(uint8_t row) {
  while (!scanFrameComplete) {
    tight_loop_contents();
  }

  const uint8_t drawIndex = activeFramebuffer ^ 1;
  drawRowMappingTest(framebuffers[drawIndex], row);
  __dmb();
  activeFramebuffer = drawIndex;
  scanFrameComplete = false;
}

void showDiagnosticAddress(uint8_t address, bool blinkPhase) {
  while (!scanFrameComplete) {
    tight_loop_contents();
  }

  const uint8_t drawIndex = activeFramebuffer ^ 1;
  drawDiagnosticAddress(framebuffers[drawIndex], address,
                        diagnosticColor(address, blinkPhase));
  __dmb();
  activeFramebuffer = drawIndex;
  scanFrameComplete = false;
}

void scanMatrix() {
  const uint8_t bufferIndex = activeFramebuffer;
  const uint8_t* framebuffer = framebuffers[bufferIndex];

  for (uint8_t row = 0; row < ADDRESS_COUNT; ++row) {
    const absolute_time_t rowStart = get_absolute_time();
    gpio_put_masked(OUTPUT_ENABLE_MASK, OUTPUT_ENABLE_MASK);
    gpio_put_masked(ADDRESS_MASK, encodeAddress(row));

    for (uint16_t chainX = 0; chainX < CHAIN_SCAN_WIDTH; ++chainX) {
      const uint16_t* sourceIndices = sourcePixelIndices[row][chainX];
      const uint8_t upperPixel = framebuffer[sourceIndices[0]];
      const uint8_t lowerPixel = framebuffer[sourceIndices[1]];
      const uint32_t pixelData =
          ((upperPixel & 0b111) << 2) | ((lowerPixel & 0b111) << 5);
      gpio_put_masked(DATA_MASK, pixelData);
      gpio_put_masked(CLOCK_MASK, CLOCK_MASK);
      gpio_put_masked(CLOCK_MASK, 0);
    }

    gpio_put_masked(LATCH_MASK, LATCH_MASK);
    gpio_put_masked(LATCH_MASK, 0);
    gpio_put_masked(OUTPUT_ENABLE_MASK, 0);
    sleep_us(ROW_ON_TIME_US);
    gpio_put_masked(OUTPUT_ENABLE_MASK, OUTPUT_ENABLE_MASK);
    sleep_until(delayed_by_us(rowStart, ROW_SLOT_US));
  }
  scanFrameComplete = true;
}

void setup1() {
  gpio_init_mask(MATRIX_GPIO_MASK);
  gpio_set_dir_out_masked(MATRIX_GPIO_MASK);
  gpio_put_masked(MATRIX_GPIO_MASK, OUTPUT_ENABLE_MASK);
}

void loop1() {
  if (scanEnabled) {
    scanMatrix();
  } else {
    tight_loop_contents();
  }
}

void setup() {
  set_sys_clock_khz(CPU_CLOCK_KHZ, true);
  strncpy(scrollingText, SCROLLING_TEXT, sizeof(scrollingText) - 1);
  gpio_init(PS2_CLOCK_PIN);
  gpio_set_dir(PS2_CLOCK_PIN, GPIO_IN);
  gpio_pull_up(PS2_CLOCK_PIN);
  gpio_init(PS2_DATA_PIN);
  gpio_set_dir(PS2_DATA_PIN, GPIO_IN);
  gpio_pull_up(PS2_DATA_PIN);
  gpio_set_irq_enabled_with_callback(PS2_CLOCK_PIN, GPIO_IRQ_EDGE_FALL, true,
                                     &ps2ClockCallback);

  if (ROW_MAPPING_TEST_MODE) {
    drawRowMappingTest(framebuffers[0], 0);
    memcpy(framebuffers[1], framebuffers[0], WIDTH * HEIGHT);
  } else if (DIAGNOSTIC_MODE) {
    drawDiagnosticAddress(framebuffers[0], 0, diagnosticColor(0, false));
    memcpy(framebuffers[1], framebuffers[0], WIDTH * HEIGHT);
  }
  initializeSourcePixelIndices();
  __dmb();
  scanEnabled = true;
  delay(100);
}

void loop() {
  processPs2Keyboard();

  if (ROW_MAPPING_TEST_MODE) {
    static uint8_t row = 0;
    static uint32_t lastRowChange = millis();
    if (millis() - lastRowChange >= ROW_MAPPING_TEST_INTERVAL_MS) {
      row = (row + 1) % HEIGHT;
      lastRowChange = millis();
      showRowMappingTest(row);
    }
    delay(1);
    return;
  }

  if (DIAGNOSTIC_MODE) {
    static uint8_t address = 0;
    static uint32_t lastAddressChange = millis();
    static uint32_t lastPhaseChange = lastAddressChange;
    static bool blinkPhase = false;
    if (millis() - lastAddressChange >= DIAGNOSTIC_INTERVAL_MS) {
      address = (address + 1) % ADDRESS_COUNT;
      blinkPhase = false;
      lastAddressChange = millis();
      lastPhaseChange = lastAddressChange;
      showDiagnosticAddress(address, blinkPhase);
    } else if (address == 7 && millis() - lastPhaseChange >= 250) {
      blinkPhase = !blinkPhase;
      lastPhaseChange = millis();
      showDiagnosticAddress(address, blinkPhase);
    }
    delay(1);
    return;
  }

  static int32_t textX = WIDTH;
  static int32_t textWidth = measureTextWidth(SCROLLING_TEXT);
  if (pendingTextSubmit) {
    strncpy(scrollingText, inputText, sizeof(scrollingText) - 1);
    scrollingText[sizeof(scrollingText) - 1] = '\0';
    textWidth = measureTextWidth(scrollingText);
    textX = WIDTH;
    playbackScene = PlaybackScene::Text;
    pendingTextSubmit = false;
    inputTextChanged = false;
  }

  if (pendingColorRestart && !inputEditing) {
    textX = WIDTH;
    playbackScene = PlaybackScene::Text;
    pendingColorRestart = false;
  }

  if (pendingSceneNavigation != 0 && !inputEditing) {
    const int8_t sceneIndex = static_cast<int8_t>(playbackScene);
    const int8_t nextSceneIndex =
        (sceneIndex + pendingSceneNavigation + PLAYBACK_SCENE_COUNT) %
        PLAYBACK_SCENE_COUNT;
    playbackScene = static_cast<PlaybackScene>(nextSceneIndex);
    if (playbackScene == PlaybackScene::Text) {
      textX = WIDTH;
    } else {
      holidayAnimationStartMs = millis();
      if (playbackScene == PlaybackScene::Breakout) {
        startBreakoutGame();
      }
    }
    pendingSceneNavigation = 0;
  }

  if (playbackScene == PlaybackScene::Santa && !inputEditing &&
      millis() - holidayAnimationStartMs >=
          HOLIDAY_ANIMATION_DURATION_MS) {
    playbackScene = PlaybackScene::CocaColaTruck;
    holidayAnimationStartMs = millis();
  }

  if (playbackScene == PlaybackScene::CocaColaTruck && !inputEditing &&
      millis() - holidayAnimationStartMs >=
          COCA_COLA_ANIMATION_DURATION_MS) {
    playbackScene = PlaybackScene::Stars;
    holidayAnimationStartMs = millis();
  }

  if (playbackScene == PlaybackScene::Stars && !inputEditing &&
      millis() - holidayAnimationStartMs >= STAR_ANIMATION_DURATION_MS) {
    playbackScene = PlaybackScene::Balls;
    holidayAnimationStartMs = millis();
  }

  if (playbackScene == PlaybackScene::Balls && !inputEditing &&
      millis() - holidayAnimationStartMs >= BALL_ANIMATION_DURATION_MS) {
    playbackScene = PlaybackScene::Breakout;
    startBreakoutGame();
  }

  if (playbackScene == PlaybackScene::Breakout &&
      breakoutBricksRemaining == 0) {
    playbackScene = PlaybackScene::Reindeer;
    holidayAnimationStartMs = millis();
  }

  if (playbackScene == PlaybackScene::Reindeer && !inputEditing &&
      millis() - holidayAnimationStartMs >= REINDEER_ANIMATION_DURATION_MS) {
    playbackScene = PlaybackScene::SantaSleigh;
    holidayAnimationStartMs = millis();
  }

  if (playbackScene == PlaybackScene::SantaSleigh && !inputEditing &&
      millis() - holidayAnimationStartMs >= SLEIGH_ANIMATION_DURATION_MS) {
    playbackScene = PlaybackScene::Text;
    textX = WIDTH;
  }

  if (inputTextChanged && inputEditing) {
    textWidth = measureTextWidth(inputText);
    textX = WIDTH - textWidth;
    inputTextChanged = false;
  }

  while (!scanFrameComplete) {
    tight_loop_contents();
  }

  const uint8_t drawIndex = activeFramebuffer ^ 1;
  if (inputEditing) {
    renderText(framebuffers[drawIndex], inputText, textX, 0b010);
  } else if (playbackScene == PlaybackScene::Santa) {
    renderHolidayAnimation(framebuffers[drawIndex],
                           millis() - holidayAnimationStartMs);
  } else if (playbackScene == PlaybackScene::CocaColaTruck) {
    renderCocaColaAnimation(framebuffers[drawIndex],
                            millis() - holidayAnimationStartMs);
  } else if (playbackScene == PlaybackScene::Stars) {
    renderStarAnimation(framebuffers[drawIndex],
                        millis() - holidayAnimationStartMs);
  } else if (playbackScene == PlaybackScene::Balls) {
    renderBallAnimation(framebuffers[drawIndex],
                        millis() - holidayAnimationStartMs);
  } else if (playbackScene == PlaybackScene::Breakout) {
    renderBreakoutGame(framebuffers[drawIndex]);
  } else if (playbackScene == PlaybackScene::Reindeer) {
    renderReindeerAnimation(framebuffers[drawIndex],
                            millis() - holidayAnimationStartMs);
  } else if (playbackScene == PlaybackScene::SantaSleigh) {
    renderSantaSleighAnimation(framebuffers[drawIndex],
                               millis() - holidayAnimationStartMs);
  } else {
    renderText(framebuffers[drawIndex], scrollingText, textX,
               TEXT_COLORS[textColorIndex]);
  }
  __dmb();
  activeFramebuffer = drawIndex;
  scanFrameComplete = false;

  if (!inputEditing && playbackScene == PlaybackScene::Text) {
    --textX;
    if (textX < -textWidth) {
      playbackScene = PlaybackScene::Santa;
      holidayAnimationStartMs = millis();
      textColorIndex = (textColorIndex + 1) % TEXT_COLOR_COUNT;
    }
  }
  if (playbackScene != PlaybackScene::Text && !inputEditing) {
    delay(20);
  } else {
    const uint32_t scrollDelayUs =
        (BASE_SCROLL_DELAY_US * SCROLL_SPEED_DENOMINATORS[scrollSpeedIndex]) /
        SCROLL_SPEED_NUMERATORS[scrollSpeedIndex];
    delayMicroseconds(scrollDelayUs);
  }
}
