#pragma once

namespace lib_input {
enum Key {
  kMouseLeft = 0,
  kMouseRight,
  kMouseMiddle,
  kMouse4,
  kMouse5,
  kMouse6,
  kMouse7,
  kMouse8,

  kSpace = 32,
  kApostrophe = 39, /* ' */
  kComma = 44,      /* , */
  kMinus,           /* - */
  kPeriod,          /* . */
  kSlash,           /* / */
  k0,
  k1,
  k2,
  k3,
  k4,
  k5,
  k6,
  k7,
  k8,
  k9,

  kSemicolon = 59, /* ; */

  kEqual = 61, /* = */

  kA = 65,
  kB,
  kC,
  kD,
  kE,
  kF,
  kG,
  kH,
  kI,
  kJ,
  kK,
  kL,
  kM,
  kN,
  kO,
  kP,
  kQ,
  kR,
  kS,
  kT,
  kU,
  kV,
  kW,
  kX,
  kY,
  kZ,
  kLeftBracket,  /* [ */
  kBackslash,    /* \ */
  kRightBracket, /* ] */

  kGraveAccent = 96, /* ` */

  kWorld1 = 161, /* non-US #1 */
  kWorld2,       /* non-US #2 */

  /* Function keys */
  kEscape = 256,
  kEnter,
  kTab,
  kBackspace,
  kInsert,
  kDelete,
  kRight,
  kLeft,
  kDown,
  kUp,
  kPageUp,
  kPageDown,
  kHome,
  kEnd,

  kCapsLock = 280,
  kScrollLock,
  kNumLock,
  kPrintScreen,
  kPause,

  kF1 = 290,
  kF2,
  kF3,
  kF4,
  kF5,
  kF6,
  kF7,
  kF8,
  kF9,
  kF10,
  kF11,
  kF12,
  kF13,
  kF14,
  kF15,
  kF16,
  kF17,
  kF18,
  kF19,
  kF20,
  kF21,
  kF22,
  kF23,
  kF24,
  kF25,

  kKp0 = 320,
  kKp1,
  kKp2,
  kKp3,
  kKp4,
  kKp5,
  kKp6,
  kKp7,
  kKp8,
  kKp9,
  kKpDecimal,
  kKpDivide,
  kKpMultiply,
  kKpSubtract,
  kKpAdd,
  kKpEnter,
  kKpEqual,

  kLeftShift = 340,
  kLeftControl,
  kLeftAlt,
  kLeftSuper,
  kRightShift,
  kRightControl,
  kRightAlt,
  kRightSuper,
  kMenu
};
}