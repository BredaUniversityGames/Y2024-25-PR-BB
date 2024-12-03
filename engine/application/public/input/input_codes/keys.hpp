#pragma once
#include <cstdint>

// Modified from SDL_keycode.h

enum class KeyboardCode : uint32_t
{
    eUNKNOWN = 0x00000000u /* 0 */
    ,
    eRETURN = 0x0000000du /* '\r' */
    ,
    eESCAPE = 0x0000001bu /* '\x1B' */
    ,
    eBACKSPACE = 0x00000008u /* '\b' */
    ,
    eTAB = 0x00000009u /* '\t' */
    ,
    eSPACE = 0x00000020u /* ' ' */
    ,
    eEXCLAIM = 0x00000021u /* '!' */
    ,
    eDBLAPOSTROPHE = 0x00000022u /* '"' */
    ,
    eHASH = 0x00000023u /* '#' */
    ,
    eDOLLAR = 0x00000024u /* '$' */
    ,
    ePERCENT = 0x00000025u /* '%' */
    ,
    eAMPERSAND = 0x00000026u /* '&' */
    ,
    eAPOSTROPHE = 0x00000027u /* '\'' */
    ,
    eLEFTPAREN = 0x00000028u /* '(' */
    ,
    eRIGHTPAREN = 0x00000029u /* ')' */
    ,
    eASTERISK = 0x0000002au /* '*' */
    ,
    ePLUS = 0x0000002bu /* '+' */
    ,
    eCOMMA = 0x0000002cu /* ',' */
    ,
    eMINUS = 0x0000002du /* '-' */
    ,
    ePERIOD = 0x0000002eu /* '.' */
    ,
    eSLASH = 0x0000002fu /* '/' */
    ,
    e0 = 0x00000030u /* '0' */
    ,
    e1 = 0x00000031u /* '1' */
    ,
    e2 = 0x00000032u /* '2' */
    ,
    e3 = 0x00000033u /* '3' */
    ,
    e4 = 0x00000034u /* '4' */
    ,
    e5 = 0x00000035u /* '5' */
    ,
    e6 = 0x00000036u /* '6' */
    ,
    e7 = 0x00000037u /* '7' */
    ,
    e8 = 0x00000038u /* '8' */
    ,
    e9 = 0x00000039u /* '9' */
    ,
    eCOLON = 0x0000003au /* ':' */
    ,
    eSEMICOLON = 0x0000003bu /* ';' */
    ,
    eLESS = 0x0000003cu /* '<' */
    ,
    eEQUALS = 0x0000003du /* '=' */
    ,
    eGREATER = 0x0000003eu /* '>' */
    ,
    eQUESTION = 0x0000003fu /* '?' */
    ,
    eAT = 0x00000040u /* '@' */
    ,
    eLEFTBRACKET = 0x0000005bu /* '[' */
    ,
    eBACKSLASH = 0x0000005cu /* '\\' */
    ,
    eRIGHTBRACKET = 0x0000005du /* ']' */
    ,
    eCARET = 0x0000005eu /* '^' */
    ,
    eUNDERSCORE = 0x0000005fu /* '_' */
    ,
    eGRAVE = 0x00000060u /* '`' */
    ,
    eA = 0x00000061u /* 'a' */
    ,
    eB = 0x00000062u /* 'b' */
    ,
    eC = 0x00000063u /* 'c' */
    ,
    eD = 0x00000064u /* 'd' */
    ,
    eE = 0x00000065u /* 'e' */
    ,
    eF = 0x00000066u /* 'f' */
    ,
    eG = 0x00000067u /* 'g' */
    ,
    eH = 0x00000068u /* 'h' */
    ,
    eI = 0x00000069u /* 'i' */
    ,
    eJ = 0x0000006au /* 'j' */
    ,
    eK = 0x0000006bu /* 'k' */
    ,
    eL = 0x0000006cu /* 'l' */
    ,
    eM = 0x0000006du /* 'm' */
    ,
    eN = 0x0000006eu /* 'n' */
    ,
    eO = 0x0000006fu /* 'o' */
    ,
    eP = 0x00000070u /* 'p' */
    ,
    eQ = 0x00000071u /* 'q' */
    ,
    eR = 0x00000072u /* 'r' */
    ,
    eS = 0x00000073u /* 's' */
    ,
    eT = 0x00000074u /* 't' */
    ,
    eU = 0x00000075u /* 'u' */
    ,
    eV = 0x00000076u /* 'v' */
    ,
    eW = 0x00000077u /* 'w' */
    ,
    eX = 0x00000078u /* 'x' */
    ,
    eY = 0x00000079u /* 'y' */
    ,
    eZ = 0x0000007au /* 'z' */
    ,
    eLEFTBRACE = 0x0000007bu /* '{' */
    ,
    ePIPE = 0x0000007cu /* '|' */
    ,
    eRIGHTBRACE = 0x0000007du /* '}' */
    ,
    eTILDE = 0x0000007eu /* '~' */
    ,
    eDELETE = 0x0000007fu /* '\x7F' */
    ,
    ePLUSMINUS = 0x000000b1u /* '±' */
    ,
    eCAPSLOCK = 0x40000039u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_CAPSLOCK) */
    ,
    eF1 = 0x4000003au /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F1) */
    ,
    eF2 = 0x4000003bu /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F2) */
    ,
    eF3 = 0x4000003cu /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F3) */
    ,
    eF4 = 0x4000003du /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F4) */
    ,
    eF5 = 0x4000003eu /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F5) */
    ,
    eF6 = 0x4000003fu /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F6) */
    ,
    eF7 = 0x40000040u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F7) */
    ,
    eF8 = 0x40000041u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F8) */
    ,
    eF9 = 0x40000042u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F9) */
    ,
    eF10 = 0x40000043u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F10) */
    ,
    eF11 = 0x40000044u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F11) */
    ,
    eF12 = 0x40000045u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F12) */
    ,
    ePRINTSCREEN = 0x40000046u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_PRINTSCREEN) */
    ,
    eSCROLLLOCK = 0x40000047u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_SCROLLLOCK) */
    ,
    ePAUSE = 0x40000048u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_PAUSE) */
    ,
    eINSERT = 0x40000049u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_INSERT) */
    ,
    eHOME = 0x4000004au /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_HOME) */
    ,
    ePAGEUP = 0x4000004bu /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_PAGEUP) */
    ,
    eEND = 0x4000004du /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_END) */
    ,
    ePAGEDOWN = 0x4000004eu /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_PAGEDOWN) */
    ,
    eRIGHT = 0x4000004fu /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_RIGHT) */
    ,
    eLEFT = 0x40000050u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_LEFT) */
    ,
    eDOWN = 0x40000051u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_DOWN) */
    ,
    eUP = 0x40000052u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_UP) */
    ,
    eNUMLOCKCLEAR = 0x40000053u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_NUMLOCKCLEAR) */
    ,
    eKP_DIVIDE = 0x40000054u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_DIVIDE) */
    ,
    eKP_MULTIPLY = 0x40000055u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_MULTIPLY) */
    ,
    eKP_MINUS = 0x40000056u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_MINUS) */
    ,
    eKP_PLUS = 0x40000057u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_PLUS) */
    ,
    eKP_ENTER = 0x40000058u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_ENTER) */
    ,
    eKP_1 = 0x40000059u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_1) */
    ,
    eKP_2 = 0x4000005au /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_2) */
    ,
    eKP_3 = 0x4000005bu /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_3) */
    ,
    eKP_4 = 0x4000005cu /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_4) */
    ,
    eKP_5 = 0x4000005du /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_5) */
    ,
    eKP_6 = 0x4000005eu /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_6) */
    ,
    eKP_7 = 0x4000005fu /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_7) */
    ,
    eKP_8 = 0x40000060u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_8) */
    ,
    eKP_9 = 0x40000061u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_9) */
    ,
    eKP_0 = 0x40000062u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_0) */
    ,
    eKP_PERIOD = 0x40000063u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_PERIOD) */
    ,
    eAPPLICATION = 0x40000065u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_APPLICATION) */
    ,
    ePOWER = 0x40000066u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_POWER) */
    ,
    eKP_EQUALS = 0x40000067u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_EQUALS) */
    ,
    eF13 = 0x40000068u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F13) */
    ,
    eF14 = 0x40000069u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F14) */
    ,
    eF15 = 0x4000006au /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F15) */
    ,
    eF16 = 0x4000006bu /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F16) */
    ,
    eF17 = 0x4000006cu /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F17) */
    ,
    eF18 = 0x4000006du /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F18) */
    ,
    eF19 = 0x4000006eu /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F19) */
    ,
    eF20 = 0x4000006fu /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F20) */
    ,
    eF21 = 0x40000070u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F21) */
    ,
    eF22 = 0x40000071u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F22) */
    ,
    eF23 = 0x40000072u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F23) */
    ,
    eF24 = 0x40000073u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F24) */
    ,
    eEXECUTE = 0x40000074u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_EXECUTE) */
    ,
    eHELP = 0x40000075u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_HELP) */
    ,
    eMENU = 0x40000076u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_MENU) */
    ,
    eSELECT = 0x40000077u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_SELECT) */
    ,
    eSTOP = 0x40000078u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_STOP) */
    ,
    eAGAIN = 0x40000079u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_AGAIN) */
    ,
    eUNDO = 0x4000007au /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_UNDO) */
    ,
    eCUT = 0x4000007bu /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_CUT) */
    ,
    eCOPY = 0x4000007cu /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_COPY) */
    ,
    ePASTE = 0x4000007du /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_PASTE) */
    ,
    eFIND = 0x4000007eu /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_FIND) */
    ,
    eMUTE = 0x4000007fu /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_MUTE) */
    ,
    eVOLUMEUP = 0x40000080u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_VOLUMEUP) */
    ,
    eVOLUMEDOWN = 0x40000081u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_VOLUMEDOWN) */
    ,
    eKP_COMMA = 0x40000085u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_COMMA) */
    ,
    eKP_EQUALSAS400 = 0x40000086u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_EQUALSAS400) */
    ,
    eALTERASE = 0x40000099u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_ALTERASE) */
    ,
    eSYSREQ = 0x4000009au /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_SYSREQ) */
    ,
    eCANCEL = 0x4000009bu /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_CANCEL) */
    ,
    eCLEAR = 0x4000009cu /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_CLEAR) */
    ,
    ePRIOR = 0x4000009du /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_PRIOR) */
    ,
    eRETURN2 = 0x4000009eu /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_RETURN2) */
    ,
    eSEPARATOR = 0x4000009fu /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_SEPARATOR) */
    ,
    eOUT = 0x400000a0u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_OUT) */
    ,
    eOPER = 0x400000a1u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_OPER) */
    ,
    eCLEARAGAIN = 0x400000a2u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_CLEARAGAIN) */
    ,
    eCRSEL = 0x400000a3u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_CRSEL) */
    ,
    eEXSEL = 0x400000a4u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_EXSEL) */
    ,
    eKP_00 = 0x400000b0u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_00) */
    ,
    eKP_000 = 0x400000b1u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_000) */
    ,
    eTHOUSANDSSEPARATOR = 0x400000b2u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_THOUSANDSSEPARATOR) */
    ,
    eDECIMALSEPARATOR = 0x400000b3u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_DECIMALSEPARATOR) */
    ,
    eCURRENCYUNIT = 0x400000b4u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_CURRENCYUNIT) */
    ,
    eCURRENCYSUBUNIT = 0x400000b5u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_CURRENCYSUBUNIT) */
    ,
    eKP_LEFTPAREN = 0x400000b6u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_LEFTPAREN) */
    ,
    eKP_RIGHTPAREN = 0x400000b7u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_RIGHTPAREN) */
    ,
    eKP_LEFTBRACE = 0x400000b8u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_LEFTBRACE) */
    ,
    eKP_RIGHTBRACE = 0x400000b9u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_RIGHTBRACE) */
    ,
    eKP_TAB = 0x400000bau /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_TAB) */
    ,
    eKP_BACKSPACE = 0x400000bbu /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_BACKSPACE) */
    ,
    eKP_A = 0x400000bcu /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_A) */
    ,
    eKP_B = 0x400000bdu /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_B) */
    ,
    eKP_C = 0x400000beu /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_C) */
    ,
    eKP_D = 0x400000bfu /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_D) */
    ,
    eKP_E = 0x400000c0u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_E) */
    ,
    eKP_F = 0x400000c1u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_F) */
    ,
    eKP_XOR = 0x400000c2u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_XOR) */
    ,
    eKP_POWER = 0x400000c3u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_POWER) */
    ,
    eKP_PERCENT = 0x400000c4u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_PERCENT) */
    ,
    eKP_LESS = 0x400000c5u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_LESS) */
    ,
    eKP_GREATER = 0x400000c6u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_GREATER) */
    ,
    eKP_AMPERSAND = 0x400000c7u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_AMPERSAND) */
    ,
    eKP_DBLAMPERSAND = 0x400000c8u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_DBLAMPERSAND) */
    ,
    eKP_VERTICALBAR = 0x400000c9u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_VERTICALBAR) */
    ,
    eKP_DBLVERTICALBAR = 0x400000cau /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_DBLVERTICALBAR) */
    ,
    eKP_COLON = 0x400000cbu /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_COLON) */
    ,
    eKP_HASH = 0x400000ccu /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_HASH) */
    ,
    eKP_SPACE = 0x400000cdu /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_SPACE) */
    ,
    eKP_AT = 0x400000ceu /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_AT) */
    ,
    eKP_EXCLAM = 0x400000cfu /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_EXCLAM) */
    ,
    eKP_MEMSTORE = 0x400000d0u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_MEMSTORE) */
    ,
    eKP_MEMRECALL = 0x400000d1u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_MEMRECALL) */
    ,
    eKP_MEMCLEAR = 0x400000d2u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_MEMCLEAR) */
    ,
    eKP_MEMADD = 0x400000d3u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_MEMADD) */
    ,
    eKP_MEMSUBTRACT = 0x400000d4u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_MEMSUBTRACT) */
    ,
    eKP_MEMMULTIPLY = 0x400000d5u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_MEMMULTIPLY) */
    ,
    eKP_MEMDIVIDE = 0x400000d6u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_MEMDIVIDE) */
    ,
    eKP_PLUSMINUS = 0x400000d7u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_PLUSMINUS) */
    ,
    eKP_CLEAR = 0x400000d8u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_CLEAR) */
    ,
    eKP_CLEARENTRY = 0x400000d9u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_CLEARENTRY) */
    ,
    eKP_BINARY = 0x400000dau /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_BINARY) */
    ,
    eKP_OCTAL = 0x400000dbu /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_OCTAL) */
    ,
    eKP_DECIMAL = 0x400000dcu /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_DECIMAL) */
    ,
    eKP_HEXADECIMAL = 0x400000ddu /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_HEXADECIMAL) */
    ,
    eLCTRL = 0x400000e0u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_LCTRL) */
    ,
    eLSHIFT = 0x400000e1u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_LSHIFT) */
    ,
    eLALT = 0x400000e2u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_LALT) */
    ,
    eLGUI = 0x400000e3u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_LGUI) */
    ,
    eRCTRL = 0x400000e4u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_RCTRL) */
    ,
    eRSHIFT = 0x400000e5u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_RSHIFT) */
    ,
    eRALT = 0x400000e6u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_RALT) */
    ,
    eRGUI = 0x400000e7u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_RGUI) */
    ,
    eMODE = 0x40000101u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_MODE) */
    ,
    eSLEEP = 0x40000102u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_SLEEP) */
    ,
    eWAKE = 0x40000103u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_WAKE) */
    ,
    eCHANNEL_INCREMENT = 0x40000104u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_CHANNEL_INCREMENT) */
    ,
    eCHANNEL_DECREMENT = 0x40000105u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_CHANNEL_DECREMENT) */
    ,
    eMEDIA_PLAY = 0x40000106u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_MEDIA_PLAY) */
    ,
    eMEDIA_PAUSE = 0x40000107u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_MEDIA_PAUSE) */
    ,
    eMEDIA_RECORD = 0x40000108u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_MEDIA_RECORD) */
    ,
    eMEDIA_FAST_FORWARD = 0x40000109u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_MEDIA_FAST_FORWARD) */
    ,
    eMEDIA_REWIND = 0x4000010au /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_MEDIA_REWIND) */
    ,
    eMEDIA_NEXT_TRACK = 0x4000010bu /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_MEDIA_NEXT_TRACK) */
    ,
    eMEDIA_PREVIOUS_TRACK = 0x4000010cu /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_MEDIA_PREVIOUS_TRACK) */
    ,
    eMEDIA_STOP = 0x4000010du /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_MEDIA_STOP) */
    ,
    eMEDIA_EJECT = 0x4000010eu /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_MEDIA_EJECT) */
    ,
    eMEDIA_PLAY_PAUSE = 0x4000010fu /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_MEDIA_PLAY_PAUSE) */
    ,
    eMEDIA_SELECT = 0x40000110u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_MEDIA_SELECT) */
    ,
    eAC_NEW = 0x40000111u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_AC_NEW) */
    ,
    eAC_OPEN = 0x40000112u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_AC_OPEN) */
    ,
    eAC_CLOSE = 0x40000113u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_AC_CLOSE) */
    ,
    eAC_EXIT = 0x40000114u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_AC_EXIT) */
    ,
    eAC_SAVE = 0x40000115u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_AC_SAVE) */
    ,
    eAC_PRINT = 0x40000116u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_AC_PRINT) */
    ,
    eAC_PROPERTIES = 0x40000117u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_AC_PROPERTIES) */
    ,
    eAC_SEARCH = 0x40000118u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_AC_SEARCH) */
    ,
    eAC_HOME = 0x40000119u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_AC_HOME) */
    ,
    eAC_BACK = 0x4000011au /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_AC_BACK) */
    ,
    eAC_FORWARD = 0x4000011bu /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_AC_FORWARD) */
    ,
    eAC_STOP = 0x4000011cu /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_AC_STOP) */
    ,
    eAC_REFRESH = 0x4000011du /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_AC_REFRESH) */
    ,
    eAC_BOOKMARKS = 0x4000011eu /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_AC_BOOKMARKS) */
    ,
    eSOFTLEFT = 0x4000011fu /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_SOFTLEFT) */
    ,
    eSOFTRIGHT = 0x40000120u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_SOFTRIGHT) */
    ,
    eCALL = 0x40000121u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_CALL) */
    ,
    eENDCALL = 0x40000122u /* SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_ENDCALL) */
};