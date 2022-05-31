/*
 * File: z_en_door.c
 * Overlay: ovl_En_Door
 * Description: Doors with handles
 */

#include "z_en_door.h"
#include "objects/gameplay_keep/gameplay_keep.h"
#include "objects/gameplay_field_keep/gameplay_field_keep.h"
#include "objects/object_hidan_objects/object_hidan_objects.h"
#include "objects/object_mizu_objects/object_mizu_objects.h"
#include "objects/object_haka_door/object_haka_door.h"

#define FLAGS ACTOR_FLAG_4

#define DOOR_AJAR_SLAM_RANGE 120.0f
#define DOOR_AJAR_OPEN_RANGE (2 * DOOR_AJAR_SLAM_RANGE)

#define DOOR_CHECK_RANGE 40.0f

void EnDoor_Init(Actor* thisx, PlayState* play);
void EnDoor_Destroy(Actor* thisx, PlayState* play);
void EnDoor_Update(Actor* thisx, PlayState* play);
void EnDoor_Draw(Actor* thisx, PlayState* play);

void EnDoor_SetupType(EnDoor* this, PlayState* play);
void EnDoor_Idle(EnDoor* this, PlayState* play);
void EnDoor_WaitForCheck(EnDoor* this, PlayState* play);
void EnDoor_Check(EnDoor* this, PlayState* play);
void EnDoor_AjarWait(EnDoor* this, PlayState* play);
void EnDoor_AjarOpen(EnDoor* this, PlayState* play);
void EnDoor_AjarClose(EnDoor* this, PlayState* play);
void EnDoor_Open(EnDoor* this, PlayState* play);

const ActorInit En_Door_InitVars = {
    ACTOR_EN_DOOR,
    ACTORCAT_DOOR,
    FLAGS,
    OBJECT_GAMEPLAY_KEEP,
    sizeof(EnDoor),
    (ActorFunc)EnDoor_Init,
    (ActorFunc)EnDoor_Destroy,
    (ActorFunc)EnDoor_Update,
    (ActorFunc)EnDoor_Draw,
};

/**
 * Controls which object and display lists to use in a given scene
 */
static EnDoorInfo sDoorInfo[] = {
    { SCENE_HIDAN, 1, OBJECT_HIDAN_OBJECTS },
    { SCENE_MIZUSIN, 2, OBJECT_MIZU_OBJECTS },
    { SCENE_HAKADAN, 3, OBJECT_HAKA_DOOR },
    { SCENE_HAKADANCH, 3, OBJECT_HAKA_DOOR },
    // KEEP objects should remain last and in this order
    { -1, 0, OBJECT_GAMEPLAY_KEEP },
    { -1, 4, OBJECT_GAMEPLAY_FIELD_KEEP },
};

static InitChainEntry sInitChain[] = {
    ICHAIN_U8(targetMode, 0, ICHAIN_CONTINUE),
    ICHAIN_F32(uncullZoneForward, 4000, ICHAIN_STOP),
};

static AnimationHeader* sDoorAnims[] = { &gDoor3Anim, &gDoor1Anim, &gDoor4Anim, &gDoor2Anim };

static u8 sDoorAnimOpenFrames[] = { 25, 25, 25, 25 };

static u8 sDoorAnimCloseFrames[] = { 60, 70, 60, 70 };

static Gfx* sDoorDLists[5][2] = {
    { gDoorLeftDL, gDoorRightDL },
    { gFireTempleDoorWithHandleFrontDL, gFireTempleDoorWithHandleBackDL },
    { gWaterTempleDoorLeftDL, gWaterTempleDoorRightDL },
    { object_haka_door_DL_0013B8, object_haka_door_DL_001420 },
    { gFieldDoor1DL, gFieldDoor2DL },
};

void EnDoor_Init(Actor* thisx, PlayState* play2) {
    PlayState* play = play2;
    EnDoor* this = (EnDoor*)thisx;
    EnDoorInfo* objectInfo;
    s32 i;
    s32 objBankIndex;
    f32 xOffset;
    f32 zOffset;

    objectInfo = &sDoorInfo[0];
    Actor_ProcessInitChain(&this->actor, sInitChain);
    SkelAnime_Init(play, &this->skelAnime, &gDoorSkel, &gDoor3Anim, this->jointTable, this->morphTable, 5);
    for (i = 0; i < ARRAY_COUNT(sDoorInfo) - 2; i++, objectInfo++) {
        if (play->sceneNum == objectInfo->sceneNum) {
            break;
        }
    }
    if (i >= ARRAY_COUNT(sDoorInfo) - 2 && Object_GetIndex(&play->objectCtx, OBJECT_GAMEPLAY_FIELD_KEEP) >= 0) {
        objectInfo++;
    }

    this->dListIndex = objectInfo->dListIndex;
    objBankIndex = Object_GetIndex(&play->objectCtx, objectInfo->objectId);
    if (objBankIndex < 0) {
        Actor_Kill(&this->actor);
        return;
    }

    this->requiredObjBankIndex = objBankIndex;
    this->dListIndex = objectInfo->dListIndex;
    if (this->actor.objBankIndex == this->requiredObjBankIndex) {
        EnDoor_SetupType(this, play);
    } else {
        this->actionFunc = EnDoor_SetupType;
    }

    // Double doors
    if (this->actor.params & 0x40) {
        EnDoor* other;

        xOffset = Math_CosS(this->actor.shape.rot.y) * 30.0f;
        zOffset = Math_SinS(this->actor.shape.rot.y) * 30.0f;
        other = (EnDoor*)Actor_SpawnAsChild(&play->actorCtx, &this->actor, play, ACTOR_EN_DOOR,
                                            this->actor.world.pos.x + xOffset, this->actor.world.pos.y,
                                            this->actor.world.pos.z - zOffset, 0, this->actor.shape.rot.y + 0x8000, 0,
                                            this->actor.params & ~0x40);
        if (other != NULL) {
            other->unk_192 = 1;
        }
        this->actor.world.pos.x -= xOffset;
        this->actor.world.pos.z += zOffset;
    }
    Actor_SetFocus(&this->actor, 70.0f);
}

void EnDoor_Destroy(Actor* thisx, PlayState* play) {
    TransitionActorEntry* transitionEntry;
    EnDoor* this = (EnDoor*)thisx;

    transitionEntry = &play->transiActorCtx.list[(u16)this->actor.params >> 0xA];
    if (transitionEntry->id < 0) {
        transitionEntry->id = -transitionEntry->id;
    }
}

void EnDoor_SetupType(EnDoor* this, PlayState* play) {
    s32 doorType;

    if (Object_IsLoaded(&play->objectCtx, this->requiredObjBankIndex)) {
        doorType = this->actor.params >> 7 & 7;
        this->actor.flags &= ~ACTOR_FLAG_4;
        this->actor.objBankIndex = this->requiredObjBankIndex;
        this->actionFunc = EnDoor_Idle;
        if (doorType == DOOR_EVENING) {
            doorType = (gSaveContext.dayTime > CLOCK_TIME(18, 0) && gSaveContext.dayTime < CLOCK_TIME(21, 0))
                           ? DOOR_SCENEEXIT
                           : DOOR_CHECKABLE;
        }
        this->actor.world.rot.y = 0x0000;
        if (doorType == DOOR_LOCKED) {
            if (!Flags_GetSwitch(play, this->actor.params & 0x3F)) {
                this->lockTimer = 10;
            }
        } else if (doorType == DOOR_AJAR) {
            if (Actor_WorldDistXZToActor(&this->actor, &GET_PLAYER(play)->actor) > DOOR_AJAR_SLAM_RANGE) {
                this->actionFunc = EnDoor_AjarWait;
                this->actor.world.rot.y = -0x1800;
            }
        } else if (doorType == DOOR_CHECKABLE) {
            this->actor.textId = (this->actor.params & 0x3F) + 0x0200;
            if (this->actor.textId == 0x0229 && !GET_EVENTCHKINF(EVENTCHKINF_14)) {
                // Talon's house door. If Talon has not been woken up at Hyrule Castle
                // this door should be openable at any time of day. Note that there is no
                // check for time of day as the scene setup for Lon Lon merely initializes
                // the door with a different text id between day and night setups
                doorType = DOOR_SCENEEXIT;
            } else {
                this->actionFunc = EnDoor_WaitForCheck;
                this->actor.flags |= ACTOR_FLAG_0 | ACTOR_FLAG_3 | ACTOR_FLAG_27;
            }
        }
        // Replace the door type it was loaded with by the new type
        this->actor.params = (this->actor.params & ~0x380) | (doorType << 7);
    }
}

void EnDoor_Idle(EnDoor* this, PlayState* play) {
    Player* player = GET_PLAYER(play);
    s32 doorType;
    Vec3f playerPosRelToDoor;
    s16 yawDiff;

    doorType = this->actor.params >> 7 & 7;
    func_8002DBD0(&this->actor, &playerPosRelToDoor, &player->actor.world.pos);
    if (this->playerIsOpening != 0) {
        this->actionFunc = EnDoor_Open;
        Animation_PlayOnceSetSpeed(&this->skelAnime, sDoorAnims[this->animStyle],
                                   (player->stateFlags1 & PLAYER_STATE1_27) ? 0.75f : 1.5f);
        if (this->lockTimer != 0) {
            gSaveContext.inventory.dungeonKeys[gSaveContext.mapIndex]--;
            Flags_SetSwitch(play, this->actor.params & 0x3F);
            Audio_PlayActorSound2(&this->actor, NA_SE_EV_CHAIN_KEY_UNLOCK);
        }
    } else if (!Player_InCsMode(play)) {
        if (fabsf(playerPosRelToDoor.y) < 20.0f && fabsf(playerPosRelToDoor.x) < 20.0f &&
            fabsf(playerPosRelToDoor.z) < 50.0f) {
            yawDiff = player->actor.shape.rot.y - this->actor.shape.rot.y;
            if (playerPosRelToDoor.z > 0.0f) {
                yawDiff = 0x8000 - yawDiff;
            }
            if (ABS(yawDiff) < 0x3000) {
                if (this->lockTimer != 0) {
                    if (gSaveContext.inventory.dungeonKeys[gSaveContext.mapIndex] <= 0) {
                        Player* player2 = GET_PLAYER(play);

                        player2->naviTextId = -0x203;
                        return;
                    } else {
                        player->doorTimer = 10;
                    }
                }
                player->doorType = (doorType == DOOR_AJAR) ? PLAYER_DOORTYPE_AJAR : PLAYER_DOORTYPE_HANDLE;
                player->doorDirection = (playerPosRelToDoor.z >= 0.0f) ? 1.0f : -1.0f;
                player->doorActor = &this->actor;
            }
        } else if (doorType == DOOR_AJAR && this->actor.xzDistToPlayer > DOOR_AJAR_OPEN_RANGE) {
            this->actionFunc = EnDoor_AjarOpen;
        }
    }
}

void EnDoor_WaitForCheck(EnDoor* this, PlayState* play) {
    if (Actor_ProcessTalkRequest(&this->actor, play)) {
        this->actionFunc = EnDoor_Check;
    } else {
        func_8002F2CC(&this->actor, play, DOOR_CHECK_RANGE);
    }
}

void EnDoor_Check(EnDoor* this, PlayState* play) {
    if (Actor_TextboxIsClosing(&this->actor, play)) {
        this->actionFunc = EnDoor_WaitForCheck;
    }
}

void EnDoor_AjarWait(EnDoor* this, PlayState* play) {
    if (this->actor.xzDistToPlayer < DOOR_AJAR_SLAM_RANGE) {
        this->actionFunc = EnDoor_AjarClose;
    }
}

void EnDoor_AjarOpen(EnDoor* this, PlayState* play) {
    if (this->actor.xzDistToPlayer < DOOR_AJAR_SLAM_RANGE) {
        this->actionFunc = EnDoor_AjarClose;
    } else if (Math_ScaledStepToS(&this->actor.world.rot.y, -0x1800, 0x100)) {
        this->actionFunc = EnDoor_AjarWait;
    }
}

void EnDoor_AjarClose(EnDoor* this, PlayState* play) {
    if (Math_ScaledStepToS(&this->actor.world.rot.y, 0, 0x700)) {
        this->actionFunc = EnDoor_Idle;
    }
}

void EnDoor_Open(EnDoor* this, PlayState* play) {
    s32 i;
    s32 numEffects;

    if (DECR(this->lockTimer) == 0) {
        if (SkelAnime_Update(&this->skelAnime)) {
            this->actionFunc = EnDoor_Idle;
            this->playerIsOpening = 0;
        } else if (Animation_OnFrame(&this->skelAnime, sDoorAnimOpenFrames[this->animStyle])) {
            Audio_PlayActorSound2(&this->actor, (play->sceneNum == SCENE_HAKADAN || play->sceneNum == SCENE_HAKADANCH ||
                                                 play->sceneNum == SCENE_HIDAN)
                                                    ? NA_SE_EV_IRON_DOOR_OPEN
                                                    : NA_SE_OC_DOOR_OPEN);
            if (this->skelAnime.playSpeed < 1.5f) {
                numEffects = (s32)(Rand_ZeroOne() * 30.0f) + 50;
                for (i = 0; i < numEffects; i++) {
                    EffectSsBubble_Spawn(play, &this->actor.world.pos, 60.0f, 100.0f, 50.0f, 0.15f);
                }
            }
        } else if (Animation_OnFrame(&this->skelAnime, sDoorAnimCloseFrames[this->animStyle])) {
            Audio_PlayActorSound2(&this->actor, (play->sceneNum == SCENE_HAKADAN || play->sceneNum == SCENE_HAKADANCH ||
                                                 play->sceneNum == SCENE_HIDAN)
                                                    ? NA_SE_EV_IRON_DOOR_CLOSE
                                                    : NA_SE_EV_DOOR_CLOSE);
        }
    }
}

void EnDoor_Update(Actor* thisx, PlayState* play) {
    EnDoor* this = (EnDoor*)thisx;

    this->actionFunc(this, play);
}

s32 EnDoor_OverrideLimbDraw(PlayState* play, s32 limbIndex, Gfx** dList, Vec3f* pos, Vec3s* rot, void* thisx) {
    s32 pad;
    TransitionActorEntry* transitionEntry;
    Gfx** doorDLists;
    s32 pad2;
    s16 rotDiff;
    s32 doorDListIndex;
    EnDoor* this = (EnDoor*)thisx;

    if (limbIndex == 4) {
        doorDLists = sDoorDLists[this->dListIndex];
        transitionEntry = &play->transiActorCtx.list[(u16)this->actor.params >> 0xA];
        rot->z += this->actor.world.rot.y;
        if ((play->roomCtx.prevRoom.num >= 0) || (transitionEntry->sides[0].room == transitionEntry->sides[1].room)) {
            rotDiff = ((this->actor.shape.rot.y + this->skelAnime.jointTable[3].z) + rot->z) -
                      Math_Vec3f_Yaw(&play->view.eye, &this->actor.world.pos);
            *dList = (ABS(rotDiff) < 0x4000) ? doorDLists[0] : doorDLists[1];
        } else {
            doorDListIndex = this->unk_192;
            if (transitionEntry->sides[0].room != this->actor.room) {
                doorDListIndex ^= 1;
            }
            *dList = doorDLists[doorDListIndex];
        }
    }
    return false;
}

void EnDoor_Draw(Actor* thisx, PlayState* play) {
    EnDoor* this = (EnDoor*)thisx;

    if (this->actor.objBankIndex == this->requiredObjBankIndex) {
        OPEN_DISPS(play->state.gfxCtx, "../z_en_door.c", 910);

        func_80093D18(play->state.gfxCtx);
        SkelAnime_DrawOpa(play, this->skelAnime.skeleton, this->skelAnime.jointTable, EnDoor_OverrideLimbDraw, NULL,
                          &this->actor);
        if (this->actor.world.rot.y != 0) {
            if (1) {}
            if (this->actor.world.rot.y > 0) {
                gSPDisplayList(POLY_OPA_DISP++, gDoorRightDL);
            } else {
                gSPDisplayList(POLY_OPA_DISP++, gDoorLeftDL);
            }
        }
        if (this->lockTimer != 0) {
            Actor_DrawDoorLock(play, this->lockTimer, DOORLOCK_NORMAL);
        }

        CLOSE_DISPS(play->state.gfxCtx, "../z_en_door.c", 941);
    }
}
