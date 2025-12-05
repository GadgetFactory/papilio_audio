/**
 * @file SIDPlayer.cpp
 * @brief SID file player implementation
 * 
 * Based on TinySID by:
 * - Tammo Hinrichs - 6510 and SID routines
 * - Rainer Sinsch - PSP TinySID
 * - Alvaro Lopes - ZPUino adaptation
 * 
 * @author GadgetFactory
 * @license GPL-3.0
 */

#include "SIDPlayer.h"
#include <string.h>

#if defined(ESP32) || defined(ESP8266)
#include <LittleFS.h>
#endif

// Addressing modes
#define MODE_IMP  0
#define MODE_IMM  1
#define MODE_ABS  2
#define MODE_ABSX 3
#define MODE_ABSY 4
#define MODE_ZP   6
#define MODE_ZPX  7
#define MODE_ZPY  8
#define MODE_IND  9
#define MODE_INDX 10
#define MODE_INDY 11
#define MODE_ACC  12
#define MODE_REL  13
#define MODE_XXX  14

// Opcodes
enum {
    OPadc, OPand, OPasl, OPbcc, OPbcs, OPbeq, OPbit, OPbmi, OPbne, OPbpl, OPbrk, OPbvc, OPbvs, OPclc,
    OPcld, OPcli, OPclv, OPcmp, OPcpx, OPcpy, OPdec, OPdex, OPdey, OPeor, OPinc, OPinx, OPiny, OPjmp,
    OPjsr, OPlda, OPldx, OPldy, OPlsr, OPnop, OPora, OPpha, OPphp, OPpla, OPplp, OProl, OPror, OPrti,
    OPrts, OPsbc, OPsec, OPsed, OPsei, OPsta, OPstx, OPsty, OPtax, OPtay, OPtsx, OPtxa, OPtxs, OPtya,
    OPxxx
};

static const uint8_t opcodes[256] = {
    OPbrk,OPora,OPxxx,OPxxx,OPxxx,OPora,OPasl,OPxxx,OPphp,OPora,OPasl,OPxxx,OPxxx,OPora,OPasl,OPxxx,
    OPbpl,OPora,OPxxx,OPxxx,OPxxx,OPora,OPasl,OPxxx,OPclc,OPora,OPxxx,OPxxx,OPxxx,OPora,OPasl,OPxxx,
    OPjsr,OPand,OPxxx,OPxxx,OPbit,OPand,OProl,OPxxx,OPplp,OPand,OProl,OPxxx,OPbit,OPand,OProl,OPxxx,
    OPbmi,OPand,OPxxx,OPxxx,OPxxx,OPand,OProl,OPxxx,OPsec,OPand,OPxxx,OPxxx,OPxxx,OPand,OProl,OPxxx,
    OPrti,OPeor,OPxxx,OPxxx,OPxxx,OPeor,OPlsr,OPxxx,OPpha,OPeor,OPlsr,OPxxx,OPjmp,OPeor,OPlsr,OPxxx,
    OPbvc,OPeor,OPxxx,OPxxx,OPxxx,OPeor,OPlsr,OPxxx,OPcli,OPeor,OPxxx,OPxxx,OPxxx,OPeor,OPlsr,OPxxx,
    OPrts,OPadc,OPxxx,OPxxx,OPxxx,OPadc,OPror,OPxxx,OPpla,OPadc,OPror,OPxxx,OPjmp,OPadc,OPror,OPxxx,
    OPbvs,OPadc,OPxxx,OPxxx,OPxxx,OPadc,OPror,OPxxx,OPsei,OPadc,OPxxx,OPxxx,OPxxx,OPadc,OPror,OPxxx,
    OPxxx,OPsta,OPxxx,OPxxx,OPsty,OPsta,OPstx,OPxxx,OPdey,OPxxx,OPtxa,OPxxx,OPsty,OPsta,OPstx,OPxxx,
    OPbcc,OPsta,OPxxx,OPxxx,OPsty,OPsta,OPstx,OPxxx,OPtya,OPsta,OPtxs,OPxxx,OPxxx,OPsta,OPxxx,OPxxx,
    OPldy,OPlda,OPldx,OPxxx,OPldy,OPlda,OPldx,OPxxx,OPtay,OPlda,OPtax,OPxxx,OPldy,OPlda,OPldx,OPxxx,
    OPbcs,OPlda,OPxxx,OPxxx,OPldy,OPlda,OPldx,OPxxx,OPclv,OPlda,OPtsx,OPxxx,OPldy,OPlda,OPldx,OPxxx,
    OPcpy,OPcmp,OPxxx,OPxxx,OPcpy,OPcmp,OPdec,OPxxx,OPiny,OPcmp,OPdex,OPxxx,OPcpy,OPcmp,OPdec,OPxxx,
    OPbne,OPcmp,OPxxx,OPxxx,OPxxx,OPcmp,OPdec,OPxxx,OPcld,OPcmp,OPxxx,OPxxx,OPxxx,OPcmp,OPdec,OPxxx,
    OPcpx,OPsbc,OPxxx,OPxxx,OPcpx,OPsbc,OPinc,OPxxx,OPinx,OPsbc,OPnop,OPxxx,OPcpx,OPsbc,OPinc,OPxxx,
    OPbeq,OPsbc,OPxxx,OPxxx,OPxxx,OPsbc,OPinc,OPxxx,OPsed,OPsbc,OPxxx,OPxxx,OPxxx,OPsbc,OPinc,OPxxx
};

static const uint8_t modes[256] = {
    MODE_IMP,MODE_INDX,MODE_XXX,MODE_XXX,MODE_ZP,MODE_ZP,MODE_ZP,MODE_XXX,MODE_IMP,MODE_IMM,MODE_ACC,MODE_XXX,MODE_ABS,MODE_ABS,MODE_ABS,MODE_XXX,
    MODE_REL,MODE_INDY,MODE_XXX,MODE_XXX,MODE_XXX,MODE_ZPX,MODE_ZPX,MODE_XXX,MODE_IMP,MODE_ABSY,MODE_XXX,MODE_XXX,MODE_XXX,MODE_ABSX,MODE_ABSX,MODE_XXX,
    MODE_ABS,MODE_INDX,MODE_XXX,MODE_XXX,MODE_ZP,MODE_ZP,MODE_ZP,MODE_XXX,MODE_IMP,MODE_IMM,MODE_ACC,MODE_XXX,MODE_ABS,MODE_ABS,MODE_ABS,MODE_XXX,
    MODE_REL,MODE_INDY,MODE_XXX,MODE_XXX,MODE_XXX,MODE_ZPX,MODE_ZPX,MODE_XXX,MODE_IMP,MODE_ABSY,MODE_XXX,MODE_XXX,MODE_XXX,MODE_ABSX,MODE_ABSX,MODE_XXX,
    MODE_IMP,MODE_INDX,MODE_XXX,MODE_XXX,MODE_ZP,MODE_ZP,MODE_ZP,MODE_XXX,MODE_IMP,MODE_IMM,MODE_ACC,MODE_XXX,MODE_ABS,MODE_ABS,MODE_ABS,MODE_XXX,
    MODE_REL,MODE_INDY,MODE_XXX,MODE_XXX,MODE_XXX,MODE_ZPX,MODE_ZPX,MODE_XXX,MODE_IMP,MODE_ABSY,MODE_XXX,MODE_XXX,MODE_XXX,MODE_ABSX,MODE_ABSX,MODE_XXX,
    MODE_IMP,MODE_INDX,MODE_XXX,MODE_XXX,MODE_ZP,MODE_ZP,MODE_ZP,MODE_XXX,MODE_IMP,MODE_IMM,MODE_ACC,MODE_XXX,MODE_IND,MODE_ABS,MODE_ABS,MODE_XXX,
    MODE_REL,MODE_INDY,MODE_XXX,MODE_XXX,MODE_XXX,MODE_ZPX,MODE_ZPX,MODE_XXX,MODE_IMP,MODE_ABSY,MODE_XXX,MODE_XXX,MODE_XXX,MODE_ABSX,MODE_ABSX,MODE_XXX,
    MODE_XXX,MODE_INDX,MODE_XXX,MODE_XXX,MODE_ZP,MODE_ZP,MODE_ZP,MODE_XXX,MODE_IMP,MODE_XXX,MODE_IMP,MODE_XXX,MODE_ABS,MODE_ABS,MODE_ABS,MODE_XXX,
    MODE_REL,MODE_INDY,MODE_XXX,MODE_XXX,MODE_ZPX,MODE_ZPX,MODE_ZPY,MODE_XXX,MODE_IMP,MODE_ABSY,MODE_IMP,MODE_XXX,MODE_XXX,MODE_ABSX,MODE_XXX,MODE_XXX,
    MODE_IMM,MODE_INDX,MODE_IMM,MODE_XXX,MODE_ZP,MODE_ZP,MODE_ZP,MODE_XXX,MODE_IMP,MODE_IMM,MODE_IMP,MODE_XXX,MODE_ABS,MODE_ABS,MODE_ABS,MODE_XXX,
    MODE_REL,MODE_INDY,MODE_XXX,MODE_XXX,MODE_ZPX,MODE_ZPX,MODE_ZPY,MODE_XXX,MODE_IMP,MODE_ABSY,MODE_IMP,MODE_XXX,MODE_ABSX,MODE_ABSX,MODE_ABSY,MODE_XXX,
    MODE_IMM,MODE_INDX,MODE_XXX,MODE_XXX,MODE_ZP,MODE_ZP,MODE_ZP,MODE_XXX,MODE_IMP,MODE_IMM,MODE_IMP,MODE_XXX,MODE_ABS,MODE_ABS,MODE_ABS,MODE_XXX,
    MODE_REL,MODE_INDY,MODE_XXX,MODE_XXX,MODE_XXX,MODE_ZPX,MODE_ZPX,MODE_XXX,MODE_IMP,MODE_ABSY,MODE_XXX,MODE_XXX,MODE_XXX,MODE_ABSX,MODE_ABSX,MODE_XXX,
    MODE_IMM,MODE_INDX,MODE_XXX,MODE_XXX,MODE_ZP,MODE_ZP,MODE_ZP,MODE_XXX,MODE_IMP,MODE_IMM,MODE_IMP,MODE_XXX,MODE_ABS,MODE_ABS,MODE_ABS,MODE_XXX,
    MODE_REL,MODE_INDY,MODE_XXX,MODE_XXX,MODE_XXX,MODE_ZPX,MODE_ZPX,MODE_XXX,MODE_IMP,MODE_ABSY,MODE_XXX,MODE_XXX,MODE_XXX,MODE_ABSX,MODE_ABSX,MODE_XXX
};

// Helper variables for CPU emulation
static uint8_t bval;
static uint16_t wval;

SIDPlayer::SIDPlayer(SID6581* sid) : 
    _sid(sid), _playing(false), _fileLoaded(false), _timerTick(false),
    _loadAddr(0), _initAddr(0), _playAddr(0), _numSongs(1), _currentSong(0),
    _a(0), _x(0), _y(0), _s(0xFF), _p(0), _pc(0), _cycles(0)
{
    memset(_title, 0, sizeof(_title));
    memset(_author, 0, sizeof(_author));
    memset(_copyright, 0, sizeof(_copyright));
    memset(_memory, 0, sizeof(_memory));
}

void SIDPlayer::begin() {
    _sid->begin();
    _sid->reset();
    cpuReset();
}

uint8_t SIDPlayer::getMem(uint16_t addr) {
    if (addr == 0xdd0d) _memory[addr] = 0;
    return _memory[addr];
}

void SIDPlayer::setMem(uint16_t addr, uint8_t value) {
    // Intercept SID writes ($D400-$D7FF)
    if ((addr & 0xfc00) == 0xd400) {
        _sid->writeReg(addr & 31, value);
    }
    _memory[addr] = value;
}

inline void setFlags(uint8_t& p, uint8_t flag, bool cond) {
    if (cond) p |= flag;
    else p &= ~flag;
}

inline void SIDPlayer::branch(bool condition) {
    int8_t dist = (int8_t)getAddr(MODE_IMM);
    uint16_t newpc = _pc + dist;
    if (condition) {
        _cycles += ((_pc & 0x100) != (newpc & 0x100)) ? 2 : 1;
        _pc = newpc;
    }
}

uint8_t SIDPlayer::getAddr(uint8_t mode) {
    uint16_t ad, ad2;
    switch (mode) {
        case MODE_IMP:
            _cycles += 2;
            return 0;
        case MODE_IMM:
            _cycles += 2;
            return getMem(_pc++);
        case MODE_ABS:
            _cycles += 4;
            ad = getMem(_pc++);
            ad |= getMem(_pc++) << 8;
            return getMem(ad);
        case MODE_ABSX:
            _cycles += 4;
            ad = getMem(_pc++);
            ad |= 256 * getMem(_pc++);
            ad2 = ad + _x;
            if ((ad2 & 0xff00) != (ad & 0xff00)) _cycles++;
            return getMem(ad2);
        case MODE_ABSY:
            _cycles += 4;
            ad = getMem(_pc++);
            ad |= 256 * getMem(_pc++);
            ad2 = ad + _y;
            if ((ad2 & 0xff00) != (ad & 0xff00)) _cycles++;
            return getMem(ad2);
        case MODE_ZP:
            _cycles += 3;
            ad = getMem(_pc++);
            return getMem(ad);
        case MODE_ZPX:
            _cycles += 4;
            ad = getMem(_pc++);
            ad += _x;
            return getMem(ad & 0xff);
        case MODE_ZPY:
            _cycles += 4;
            ad = getMem(_pc++);
            ad += _y;
            return getMem(ad & 0xff);
        case MODE_INDX:
            _cycles += 6;
            ad = getMem(_pc++);
            ad += _x;
            ad2 = getMem(ad & 0xff);
            ad++;
            ad2 |= getMem(ad & 0xff) << 8;
            return getMem(ad2);
        case MODE_INDY:
            _cycles += 5;
            ad = getMem(_pc++);
            ad2 = getMem(ad);
            ad2 |= getMem((ad + 1) & 0xff) << 8;
            ad = ad2 + _y;
            if ((ad2 & 0xff00) != (ad & 0xff00)) _cycles++;
            return getMem(ad);
        case MODE_ACC:
            _cycles += 2;
            return _a;
    }
    return 0;
}

void SIDPlayer::setAddr(uint8_t mode, uint8_t val) {
    uint16_t ad, ad2;
    switch (mode) {
        case MODE_ABS:
            _cycles += 2;
            ad = getMem(_pc - 2);
            ad |= 256 * getMem(_pc - 1);
            setMem(ad, val);
            return;
        case MODE_ABSX:
            _cycles += 3;
            ad = getMem(_pc - 2);
            ad |= 256 * getMem(_pc - 1);
            ad2 = ad + _x;
            if ((ad2 & 0xff00) != (ad & 0xff00)) _cycles--;
            setMem(ad2, val);
            return;
        case MODE_ZP:
            _cycles += 2;
            ad = getMem(_pc - 1);
            setMem(ad, val);
            return;
        case MODE_ZPX:
            _cycles += 2;
            ad = getMem(_pc - 1);
            ad += _x;
            setMem(ad & 0xff, val);
            return;
        case MODE_ACC:
            _a = val;
            return;
    }
}

void SIDPlayer::putAddr(uint8_t mode, uint8_t val) {
    uint16_t ad, ad2;
    switch (mode) {
        case MODE_ABS:
            _cycles += 4;
            ad = getMem(_pc++);
            ad |= getMem(_pc++) << 8;
            setMem(ad, val);
            return;
        case MODE_ABSX:
            _cycles += 4;
            ad = getMem(_pc++);
            ad |= getMem(_pc++) << 8;
            ad2 = ad + _x;
            setMem(ad2, val);
            return;
        case MODE_ABSY:
            _cycles += 4;
            ad = getMem(_pc++);
            ad |= getMem(_pc++) << 8;
            ad2 = ad + _y;
            if ((ad2 & 0xff00) != (ad & 0xff00)) _cycles++;
            setMem(ad2, val);
            return;
        case MODE_ZP:
            _cycles += 3;
            ad = getMem(_pc++);
            setMem(ad, val);
            return;
        case MODE_ZPX:
            _cycles += 4;
            ad = getMem(_pc++);
            ad += _x;
            setMem(ad & 0xff, val);
            return;
        case MODE_ZPY:
            _cycles += 4;
            ad = getMem(_pc++);
            ad += _y;
            setMem(ad & 0xff, val);
            return;
        case MODE_INDX:
            _cycles += 6;
            ad = getMem(_pc++);
            ad += _x;
            ad2 = getMem(ad & 0xff);
            ad++;
            ad2 |= getMem(ad & 0xff) << 8;
            setMem(ad2, val);
            return;
        case MODE_INDY:
            _cycles += 5;
            ad = getMem(_pc++);
            ad2 = getMem(ad);
            ad2 |= getMem((ad + 1) & 0xff) << 8;
            ad = ad2 + _y;
            setMem(ad, val);
            return;
        case MODE_ACC:
            _cycles += 2;
            _a = val;
            return;
    }
}

void SIDPlayer::cpuReset() {
    _a = _x = _y = 0;
    _p = 0;
    _s = 255;
    _pc = getMem(0xfffc) | (getMem(0xfffd) << 8);
}

void SIDPlayer::cpuJsr(uint16_t addr, uint8_t acc) {
    _a = acc;
    _x = 0;
    _y = 0;
    _p = 0;
    _s = 255;
    _pc = addr;
    
    // Push return address (0x0000 - 1)
    setMem(0x100 + _s--, 0x00);
    setMem(0x100 + _s--, 0x00);
    
    // Execute until RTS back to 0x0000
    while (_pc) {
        cpuStep();
    }
}

bool SIDPlayer::cpuStep() {
    uint8_t opc = getMem(_pc++);
    uint8_t cmd = opcodes[opc];
    uint8_t addr = modes[opc];
    int c;
    
    _cycles = 0;
    
    switch (cmd) {
        case OPadc:
            wval = (uint16_t)_a + getAddr(addr) + ((_p & FLAG_C) ? 1 : 0);
            setFlags(_p, FLAG_C, wval & 0x100);
            _a = (uint8_t)wval;
            setFlags(_p, FLAG_Z, !_a);
            setFlags(_p, FLAG_N, _a & 0x80);
            setFlags(_p, FLAG_V, (!!(_p & FLAG_C)) ^ (!!(_p & FLAG_N)));
            break;
        case OPand:
            bval = getAddr(addr);
            _a &= bval;
            setFlags(_p, FLAG_Z, !_a);
            setFlags(_p, FLAG_N, _a & 0x80);
            break;
        case OPasl:
            wval = getAddr(addr);
            wval <<= 1;
            setAddr(addr, (uint8_t)wval);
            setFlags(_p, FLAG_Z, !wval);
            setFlags(_p, FLAG_N, wval & 0x80);
            setFlags(_p, FLAG_C, wval & 0x100);
            break;
        case OPbcc:
            branch(!(_p & FLAG_C));
            break;
        case OPbcs:
            branch(_p & FLAG_C);
            break;
        case OPbne:
            branch(!(_p & FLAG_Z));
            break;
        case OPbeq:
            branch(_p & FLAG_Z);
            break;
        case OPbpl:
            branch(!(_p & FLAG_N));
            break;
        case OPbmi:
            branch(_p & FLAG_N);
            break;
        case OPbvc:
            branch(!(_p & FLAG_V));
            break;
        case OPbvs:
            branch(_p & FLAG_V);
            break;
        case OPbit:
            bval = getAddr(addr);
            setFlags(_p, FLAG_Z, !(_a & bval));
            setFlags(_p, FLAG_N, bval & 0x80);
            setFlags(_p, FLAG_V, bval & 0x40);
            break;
        case OPbrk:
            setMem(0x100 + _s--, _pc & 0xff);
            setMem(0x100 + _s--, _pc >> 8);
            setMem(0x100 + _s--, _p);
            setFlags(_p, FLAG_B, 1);
            _pc = getMem(0xfffe) | (getMem(0xffff) << 8);
            _cycles += 7;
            break;
        case OPclc:
            _cycles += 2;
            setFlags(_p, FLAG_C, 0);
            break;
        case OPcld:
            _cycles += 2;
            setFlags(_p, FLAG_D, 0);
            break;
        case OPcli:
            _cycles += 2;
            setFlags(_p, FLAG_I, 0);
            break;
        case OPclv:
            _cycles += 2;
            setFlags(_p, FLAG_V, 0);
            break;
        case OPcmp:
            bval = getAddr(addr);
            wval = (uint16_t)_a - bval;
            setFlags(_p, FLAG_Z, !wval);
            setFlags(_p, FLAG_N, wval & 0x80);
            setFlags(_p, FLAG_C, _a >= bval);
            break;
        case OPcpx:
            bval = getAddr(addr);
            wval = (uint16_t)_x - bval;
            setFlags(_p, FLAG_Z, !wval);
            setFlags(_p, FLAG_N, wval & 0x80);
            setFlags(_p, FLAG_C, _x >= bval);
            break;
        case OPcpy:
            bval = getAddr(addr);
            wval = (uint16_t)_y - bval;
            setFlags(_p, FLAG_Z, !wval);
            setFlags(_p, FLAG_N, wval & 0x80);
            setFlags(_p, FLAG_C, _y >= bval);
            break;
        case OPdec:
            bval = getAddr(addr);
            bval--;
            setAddr(addr, bval);
            setFlags(_p, FLAG_Z, !bval);
            setFlags(_p, FLAG_N, bval & 0x80);
            break;
        case OPdex:
            _cycles += 2;
            _x--;
            setFlags(_p, FLAG_Z, !_x);
            setFlags(_p, FLAG_N, _x & 0x80);
            break;
        case OPdey:
            _cycles += 2;
            _y--;
            setFlags(_p, FLAG_Z, !_y);
            setFlags(_p, FLAG_N, _y & 0x80);
            break;
        case OPeor:
            bval = getAddr(addr);
            _a ^= bval;
            setFlags(_p, FLAG_Z, !_a);
            setFlags(_p, FLAG_N, _a & 0x80);
            break;
        case OPinc:
            bval = getAddr(addr);
            bval++;
            setAddr(addr, bval);
            setFlags(_p, FLAG_Z, !bval);
            setFlags(_p, FLAG_N, bval & 0x80);
            break;
        case OPinx:
            _cycles += 2;
            _x++;
            setFlags(_p, FLAG_Z, !_x);
            setFlags(_p, FLAG_N, _x & 0x80);
            break;
        case OPiny:
            _cycles += 2;
            _y++;
            setFlags(_p, FLAG_Z, !_y);
            setFlags(_p, FLAG_N, _y & 0x80);
            break;
        case OPjmp:
            _cycles += 3;
            wval = getMem(_pc++);
            wval |= 256 * getMem(_pc++);
            if (addr == MODE_ABS) {
                _pc = wval;
            } else if (addr == MODE_IND) {
                _pc = getMem(wval);
                _pc |= 256 * getMem(wval + 1);
                _cycles += 2;
            }
            break;
        case OPjsr:
            _cycles += 6;
            setMem(0x100 + _s--, (_pc + 1) & 0xff);
            setMem(0x100 + _s--, (_pc + 1) >> 8);
            wval = getMem(_pc++);
            wval |= 256 * getMem(_pc++);
            _pc = wval;
            break;
        case OPlda:
            _a = getAddr(addr);
            setFlags(_p, FLAG_Z, !_a);
            setFlags(_p, FLAG_N, _a & 0x80);
            break;
        case OPldx:
            _x = getAddr(addr);
            setFlags(_p, FLAG_Z, !_x);
            setFlags(_p, FLAG_N, _x & 0x80);
            break;
        case OPldy:
            _y = getAddr(addr);
            setFlags(_p, FLAG_Z, !_y);
            setFlags(_p, FLAG_N, _y & 0x80);
            break;
        case OPlsr:
            bval = getAddr(addr);
            wval = (uint8_t)bval;
            wval >>= 1;
            setAddr(addr, (uint8_t)wval);
            setFlags(_p, FLAG_Z, !wval);
            setFlags(_p, FLAG_N, wval & 0x80);
            setFlags(_p, FLAG_C, bval & 1);
            break;
        case OPnop:
            _cycles += 2;
            break;
        case OPora:
            bval = getAddr(addr);
            _a |= bval;
            setFlags(_p, FLAG_Z, !_a);
            setFlags(_p, FLAG_N, _a & 0x80);
            break;
        case OPpha:
            setMem(0x100 + _s--, _a);
            _cycles += 3;
            break;
        case OPphp:
            setMem(0x100 + _s--, _p);
            _cycles += 3;
            break;
        case OPpla:
            _a = getMem(0x100 + ++_s);
            setFlags(_p, FLAG_Z, !_a);
            setFlags(_p, FLAG_N, _a & 0x80);
            _cycles += 4;
            break;
        case OPplp:
            _p = getMem(0x100 + ++_s);
            _cycles += 4;
            break;
        case OProl:
            bval = getAddr(addr);
            c = !!(_p & FLAG_C);
            setFlags(_p, FLAG_C, bval & 0x80);
            bval <<= 1;
            bval |= c;
            setAddr(addr, bval);
            setFlags(_p, FLAG_N, bval & 0x80);
            setFlags(_p, FLAG_Z, !bval);
            break;
        case OPror:
            bval = getAddr(addr);
            c = !!(_p & FLAG_C);
            setFlags(_p, FLAG_C, bval & 1);
            bval >>= 1;
            bval |= 128 * c;
            setAddr(addr, bval);
            setFlags(_p, FLAG_N, bval & 0x80);
            setFlags(_p, FLAG_Z, !bval);
            break;
        case OPrti:
            _p = getMem(0x100 + ++_s);
            // Fall through
        case OPrts:
            wval = getMem(0x100 + ++_s) << 8;
            wval |= getMem(0x100 + ++_s);
            _pc = wval;
            _cycles += 6;
            break;
        case OPsbc:
            bval = getAddr(addr) ^ 0xff;
            wval = (uint16_t)_a + bval + ((_p & FLAG_C) ? 1 : 0);
            setFlags(_p, FLAG_C, wval & 0x100);
            _a = (uint8_t)wval;
            setFlags(_p, FLAG_Z, !_a);
            setFlags(_p, FLAG_N, _a > 127);
            setFlags(_p, FLAG_V, (!!(_p & FLAG_C)) ^ (!!(_p & FLAG_N)));
            break;
        case OPsec:
            _cycles += 2;
            setFlags(_p, FLAG_C, 1);
            break;
        case OPsed:
            _cycles += 2;
            setFlags(_p, FLAG_D, 1);
            break;
        case OPsei:
            _cycles += 2;
            setFlags(_p, FLAG_I, 1);
            break;
        case OPsta:
            putAddr(addr, _a);
            break;
        case OPstx:
            putAddr(addr, _x);
            break;
        case OPsty:
            putAddr(addr, _y);
            break;
        case OPtax:
            _cycles += 2;
            _x = _a;
            setFlags(_p, FLAG_Z, !_x);
            setFlags(_p, FLAG_N, _x & 0x80);
            break;
        case OPtay:
            _cycles += 2;
            _y = _a;
            setFlags(_p, FLAG_Z, !_y);
            setFlags(_p, FLAG_N, _y & 0x80);
            break;
        case OPtsx:
            _cycles += 2;
            _x = _s;
            setFlags(_p, FLAG_Z, !_x);
            setFlags(_p, FLAG_N, _x & 0x80);
            break;
        case OPtxa:
            _cycles += 2;
            _a = _x;
            setFlags(_p, FLAG_Z, !_a);
            setFlags(_p, FLAG_N, _a & 0x80);
            break;
        case OPtxs:
            _cycles += 2;
            _s = _x;
            break;
        case OPtya:
            _cycles += 2;
            _a = _y;
            setFlags(_p, FLAG_Z, !_a);
            setFlags(_p, FLAG_N, _a & 0x80);
            break;
        default:
            // Unknown opcode
            break;
    }
    
    return true;
}

bool SIDPlayer::parseSIDHeader(const uint8_t* data, size_t length) {
    if (length < 0x7C) return false;
    
    // Check magic
    if (data[0] != 'P' && data[0] != 'R') return false;
    if (data[1] != 'S') return false;
    if (data[2] != 'I') return false;
    if (data[3] != 'D') return false;
    
    // Get header size
    uint8_t dataOffset = data[7];
    
    // Get addresses (big-endian in PSID format)
    _loadAddr = (data[8] << 8) | data[9];
    _initAddr = (data[10] << 8) | data[11];
    _playAddr = (data[12] << 8) | data[13];
    
    // Number of songs
    _numSongs = data[0x0F];
    _currentSong = data[0x11] - 1;
    
    // If load address is 0, get it from data
    if (_loadAddr == 0) {
        _loadAddr = data[dataOffset] | (data[dataOffset + 1] << 8);
    }
    
    // Copy strings (null-terminated, max 32 chars)
    memcpy(_title, &data[0x16], 32);
    _title[32] = '\0';
    memcpy(_author, &data[0x36], 32);
    _author[32] = '\0';
    memcpy(_copyright, &data[0x56], 32);
    _copyright[32] = '\0';
    
    // Clear memory and load data
    memset(_memory, 0, sizeof(_memory));
    
    // Load actual address from file
    uint16_t actualLoadAddr = data[dataOffset] | (data[dataOffset + 1] << 8);
    
    // Copy program data to memory
    size_t dataSize = length - dataOffset - 2;
    memcpy(&_memory[actualLoadAddr], &data[dataOffset + 2], dataSize);
    
    return true;
}

bool SIDPlayer::loadFromMemory(const uint8_t* data, size_t length, uint8_t subSong) {
    if (!parseSIDHeader(data, length)) {
        return false;
    }
    
    _currentSong = subSong;
    if (_currentSong >= _numSongs) _currentSong = 0;
    
    // Reset and initialize
    _sid->reset();
    cpuReset();
    
    // Call init routine with song number in A
    cpuJsr(_initAddr, _currentSong);
    
    // If play address is 0, get it from IRQ vector
    if (_playAddr == 0) {
        _playAddr = (_memory[0x0315] << 8) + _memory[0x0314];
    }
    
    _fileLoaded = true;
    return true;
}

bool SIDPlayer::loadFile(const char* filename, uint8_t subSong) {
#if defined(ESP32) || defined(ESP8266)
    File file = LittleFS.open(filename, "r");
    if (!file) {
        Serial.print("Failed to open SID file: ");
        Serial.println(filename);
        return false;
    }
    
    size_t fileSize = file.size();
    if (fileSize > 65536) {
        Serial.println("SID file too large");
        file.close();
        return false;
    }
    
    // Allocate buffer for file data
    uint8_t* buffer = (uint8_t*)malloc(fileSize);
    if (!buffer) {
        Serial.println("Failed to allocate memory for SID file");
        file.close();
        return false;
    }
    
    // Read file into buffer
    size_t bytesRead = file.read(buffer, fileSize);
    file.close();
    
    if (bytesRead != fileSize) {
        Serial.println("Failed to read complete SID file");
        free(buffer);
        return false;
    }
    
    // Load from the buffer
    bool result = loadFromMemory(buffer, fileSize, subSong);
    
    // Free the buffer (data has been copied to _memory)
    free(buffer);
    
    return result;
#else
    // Non-ESP platforms not supported for file loading
    return false;
#endif
}

void SIDPlayer::play(bool play) {
    if (!_fileLoaded && play) {
        return;
    }
    _playing = play;
}

bool SIDPlayer::isPlaying() {
    return _playing;
}

void SIDPlayer::timerCallback() {
    _timerTick = true;
}

void SIDPlayer::update() {
    if (_playing && _timerTick) {
        cpuJsr(_playAddr, 0);
        _timerTick = false;
    }
}

const char* SIDPlayer::getTitle() {
    return _title;
}

const char* SIDPlayer::getAuthor() {
    return _author;
}

const char* SIDPlayer::getCopyright() {
    return _copyright;
}

uint8_t SIDPlayer::getNumSongs() {
    return _numSongs;
}

uint8_t SIDPlayer::getCurrentSong() {
    return _currentSong;
}

void SIDPlayer::nextSong() {
    if (_currentSong < _numSongs - 1) {
        _currentSong++;
        cpuReset();
        cpuJsr(_initAddr, _currentSong);
    }
}

void SIDPlayer::prevSong() {
    if (_currentSong > 0) {
        _currentSong--;
        cpuReset();
        cpuJsr(_initAddr, _currentSong);
    }
}
