#ifndef Z_EN_WF_H
#define Z_EN_WF_H

#include "ultra64.h"
#include "global.h"

struct EnWf;

typedef struct EnWf {
    /* 0x0000 */ Actor actor;
    /* 0x014C */ char unk_14C[0x390];
} EnWf; // size = 0x04DC

#endif
