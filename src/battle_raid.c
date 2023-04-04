#include "global.h"
#include "battle.h"
#include "battle_anim.h"
#include "battle_interface.h"
#include "battle_raid.h"
#include "battle_scripts.h"
#include "battle_setup.h"
#include "battle_transition.h"
#include "data.h"
#include "event_data.h"
#include "malloc.h"
#include "pokemon.h"
#include "sprite.h"
#include "constants/battle_raid.h"
#include "constants/battle_string_ids.h"
#include "constants/items.h"
#include "constants/moves.h"

// TODO:
//  - Switch-in abilities reapplying after Max Mindstorm?
//  - Battle partner fainting and switching breaking after Max Flare?
//  - Raid barrier sprite alignment.
//  - Raid rank disappears in Intro UI after a Raid has occurred.

// Settings for each Raid Type.
const struct RaidType gRaidTypes[NUM_RAID_TYPES] = {
    [RAID_TYPE_MAX] = {
        .shield = RAID_GEN_8,
        .shockwave = RAID_GEN_8,
        .rules = RAID_GEN_8,
        .gimmick = GIMMICK_DYNAMAX,
    },
    [RAID_TYPE_TERA] = {
        .shield = RAID_GEN_9,
        .shockwave = RAID_GEN_9,
        .rules = RAID_GEN_9,
        .gimmick = GIMMICK_TERA,
    },
    [RAID_TYPE_MEGA] = {
        .shield = RAID_GEN_8,
        .shockwave = RAID_GEN_8,
        .rules = RAID_GEN_8,
        .gimmick = GIMMICK_MEGA,
    },
};

// Rank-based HP Multipliers
static const u16 sGen8RaidHPMultipliers[] = {
    [RAID_RANK_1] = UQ_4_12(1.4),
    [RAID_RANK_2] = UQ_4_12(1.6),
    [RAID_RANK_3] = UQ_4_12(1.9),
    [RAID_RANK_4] = UQ_4_12(2.5),
    [RAID_RANK_5] = UQ_4_12(3.0),
    [RAID_RANK_6] = UQ_4_12(3.0),
    [RAID_RANK_7] = UQ_4_12(3.0),
};

static const u16 sGen9RaidHPMultipliers[] = {
    [RAID_RANK_1] = UQ_4_12(5.0),
    [RAID_RANK_2] = UQ_4_12(5.0),
    [RAID_RANK_3] = UQ_4_12(8.0),
    [RAID_RANK_4] = UQ_4_12(12.0),
    [RAID_RANK_5] = UQ_4_12(20.0),
    [RAID_RANK_6] = UQ_4_12(25.0),
    [RAID_RANK_7] = UQ_4_12(30.0),
};

EWRAM_DATA struct RaidData* gRaidData = NULL;

// forward declarations
static u32 CreateRaidBarrierSprite(u8 index);
static void CreateAllRaidBarrierSprites(void);
static void DestroyRaidBarrierSprite(u8 index);

// Sets the data for the Raid being loaded from the map information.
bool32 InitRaidData(void)
{
    // Initialize fields if needed.
    if (gRaidData == NULL)
        gRaidData = AllocZeroed(sizeof(struct RaidData));
    if (gRaidData->mon == NULL)
        gRaidData->mon = AllocZeroed(sizeof(struct Pokemon));
    if (gRaidData->partners == NULL) // TODO: use numPartners or set it here
        gRaidData->partners = AllocZeroed(3 * sizeof(struct Trainer));

    // TODO: Generate the Pokemon, rank, and partners based on a seed.
    //       The above would be selected from a list based off map sections.
    gRaidData->rank = 6;
    CreateMon(gRaidData->mon, SPECIES_SALAMENCE, 50, USE_RANDOM_IVS, FALSE, 0, OT_ID_PLAYER_ID, 0);

    return TRUE;
}

// Sets the data for the Raid being loaded from set variables.
bool32 InitCustomRaidData(void)
{
    // Initialize fields if needed.
    if (gRaidData == NULL)
        gRaidData = AllocZeroed(sizeof(struct RaidData));
    if (gRaidData->mon == NULL)
        gRaidData->mon = AllocZeroed(sizeof(struct Pokemon));
    if (gRaidData->partners == NULL) // TODO: use numPartners or set it here
        gRaidData->partners = AllocZeroed(3 * sizeof(struct Trainer));

    gRaidData->raidType = gSpecialVar_0x8001;
    gRaidData->rank = gSpecialVar_0x8002;
    CreateMon(&gEnemyParty[0], gSpecialVar_0x8003, 50, USE_RANDOM_IVS, FALSE, 0, OT_ID_PLAYER_ID, 0);
    gRaidData->mon = &gEnemyParty[0];

    return TRUE;
}

// Sets up the RaidBattleData struct in gBattleStruct.
void InitRaidBattleData(void)
{
    u8 i;

    gBattleStruct->raid.shield = 0;
    gBattleStruct->raid.nextShield = 50; // TODO: calculate
    gBattleStruct->raid.shieldsRemaining = 0; // TODO: calculate
    gBattleStruct->raid.state |= RAID_INTRO_COMPLETE;
	gBattleStruct->raid.energy = B_POSITION_PLAYER_LEFT;

    // Zeroes sprite IDs for Gen 8-style shield.
    for (i = 0; i < MAX_BARRIER_COUNT; i++)
        gBattleStruct->raid.barrierSpriteIds[i] = MAX_SPRITES;

    // TODO: Some Raids start off with a shield at the beginning.
    // if (should do shield)
    //    do shield.

    // Update HP Multiplier.
    RecalcBattlerStats(GetBattlerAtPosition(B_POSITION_OPPONENT_LEFT), &gEnemyParty[0]);
}

// Returns whether the target is a Raid Boss. Includes battle type flag check.
bool32 IsRaidBoss(u16 battlerId)
{
    return (gBattleTypeFlags & BATTLE_TYPE_RAID) && GetBattlerPosition(battlerId) == B_POSITION_OPPONENT_LEFT;
}

// Returns the battle transition ID for the Raid battle.
u8 GetRaidBattleTransition(void)
{
    if (gRaidData->raidType == RAID_TYPE_TERA)
        return B_TRANSITION_TERA_RAID;
    else
        return B_TRANSITION_MAX_RAID;
}

// Applies the HP multiplier for Raid Bosses.
void ApplyRaidHPMultiplier(u16 battlerId, struct Pokemon* mon)
{
    u16 mult;
    if (gRaidTypes[gRaidData->raidType].shield == RAID_GEN_8)
        mult = sGen8RaidHPMultipliers[gRaidData->rank];
    else
        mult = sGen9RaidHPMultipliers[gRaidData->rank];

    if (GetMonData(mon, MON_DATA_SPECIES) != SPECIES_SHEDINJA) {
        u16 hp = UQ_4_12_TO_INT((GetMonData(mon, MON_DATA_HP) * mult) + UQ_4_12_ROUND);
        u16 maxHP = UQ_4_12_TO_INT((GetMonData(mon, MON_DATA_MAX_HP) * mult) + UQ_4_12_ROUND);
        SetMonData(mon, MON_DATA_HP, &hp);
        SetMonData(mon, MON_DATA_MAX_HP, &maxHP);
    }
}

// Updates Raid Storm state and returns whether battle should end.
bool32 ShouldRaidKickPlayer(void)
{
    // Gen 8-style raids are capped at 10 turns.
    if (gRaidTypes[gRaidData->raidType].rules == RAID_GEN_8)
    {
        switch (gBattleResults.battleTurnCounter)
        {
            case RAID_STORM_TURNS_LEVEL_1:
                gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_GETTING_STRONGER;
                BattleScriptExecute(BattleScript_MaxRaidStormBrews);
                break;
            case RAID_STORM_TURNS_LEVEL_2:
                gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_GETTING_STRONGER;
                BattleScriptExecute(BattleScript_MaxRaidStormBrews);
                break;
            case RAID_STORM_TURNS_LEVEL_3:
                gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_GETTING_EVEN_STRONGER;
                BattleScriptExecute(BattleScript_MaxRaidStormBrews);
                break;
            case RAID_STORM_TURNS_MAX:
                gBattleCommunication[MULTIUSE_STATE] = RAID_GEN_8;
                gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_GETTING_TOO_STRONG;
                BattleScriptExecute(BattleScript_RaidDefeat);
                return TRUE;
        }
    }
    // Gen 9-style raids are capped at 7.5 minutes.
    else if (gBattleStruct->battleTimer >= RAID_STORM_TIMER_MAX)
    {
        gBattleCommunication[MULTIUSE_STATE] = RAID_GEN_9;
        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_PKMN_RELEASED_ENERGY;
        BattleScriptExecute(BattleScript_RaidDefeat);
        return TRUE;
    }
    else if (gBattleStruct->battleTimer >= RAID_STORM_TIMER_WARNING)
    {
        BattleScriptExecute(BattleScript_TeraRaidTimerLow);
    }
    return FALSE;
}

// Returns the number of shields to produce, or the amount of HP to protect.
static u16 GetShieldAmount(void)
{
    // TODO: Actually calculate this.
    return 3;
}

static u16 GetNextShieldThreshold(void)
{
    // TODO: Actually calculate this.
    return 0;
}

// Updates the state of the Raid shield (set up, clearing, or breaking individual barriers).
bool32 UpdateRaidShield(void)
{
    if (gBattleStruct->raid.state & RAID_CREATE_SHIELD)
    {
        gBattleStruct->raid.state &= ~RAID_CREATE_SHIELD;
        gBattlerTarget = B_POSITION_OPPONENT_LEFT;

        // Set up shield data.
        gBattleStruct->raid.shield = GetShieldAmount();
        gBattleStruct->raid.nextShield = GetNextShieldThreshold();
        if (gRaidTypes[gRaidData->raidType].shield == RAID_GEN_8)
            CreateAllRaidBarrierSprites();

        // Play animation and message.
        BattleScriptPushCursor();
        gBattlescriptCurrInstr = BattleScript_RaidShieldAppeared;
        return TRUE;
    }
    else if (gBattleStruct->raid.state & RAID_BREAK_SHIELD && gBattleMoveDamage > 0)
    {
        gBattleStruct->raid.state &= ~RAID_BREAK_SHIELD;
        // Destroy an extra barrier with a Max Move.
        // TODO: Tera STAB moves will probably break 2 barriers, too.
        if (IsMaxMove(gLastUsedMove) && gBattleStruct->raid.shield > 1)
        {
            gBattleStruct->raid.shield--;
            DestroyRaidBarrierSprite(gBattleStruct->raid.shield);
        }
        if (gBattleStruct->raid.shield > 0)
        {
            gBattleStruct->raid.shield--;
            DestroyRaidBarrierSprite(gBattleStruct->raid.shield);
        }

        BattleScriptPushCursor();
        if (gBattleStruct->raid.shield == 0)
            gBattlescriptCurrInstr = BattleScript_RaidShieldDisappeared;
        else
            gBattlescriptCurrInstr = BattleScript_RaidBarrierBroken;
        return TRUE;
    }
    return FALSE;
}

// Returns the amount of damage required to make a Raid Boss produce a shield.
u16 GetShieldDamageRequired(u16 hp, u16 maxHP)
{
    u16 threshold = (gBattleStruct->raid.nextShield * maxHP) / 100;
    if (gBattleStruct->raid.nextShield != 0 && hp > threshold)
        return hp - threshold;
    else
        return 0xFFFF; // don't make a shield
}

// Returns the damage reduction applied by a Raid Boss's shield.
u16 GetShieldDamageReduction(void)
{
    // Gen 8-style shields reduce damage by a constant 95%.
    if (gRaidTypes[gRaidData->raidType].shield == RAID_GEN_8)
    {
        return UQ_4_12(1-0.95);
    }
}

// SHIELD SPRITE DATA:
static const u16 sRaidBarrierGfx[] = INCBIN_U16("graphics/battle_interface/raid_barrier.4bpp");
static const u16 sRaidBarrierPal[] = INCBIN_U16("graphics/battle_interface/misc_indicator.gbapal");

static const struct SpriteSheet sSpriteSheet_RaidBarrier =
{
    sRaidBarrierGfx, sizeof(sRaidBarrierGfx), TAG_RAID_BARRIER_TILE
};

// Raid Barriers share a palette with the Alpha, Omega, and Dynamax indicators.
static const struct SpritePalette sSpritePalette_RaidBarrier =
{
    sRaidBarrierPal, TAG_MISC_INDICATOR_PAL
};

static const struct OamData sOamData_RaidBarrier =
{
    .y = 0,
    .affineMode = 0,
    .objMode = 0,
    .mosaic = 0,
    .bpp = 0,
    .shape = SPRITE_SHAPE(16x16),
    .x = 0,
    .matrixNum = 0,
    .size = SPRITE_SIZE(16x16),
    .tileNum = 0,
    .priority = 1,
    .paletteNum = 0,
    .affineParam = 0,
};

static const s8 sBarrierPosition[2] = {48, 10};

// Sync up barrier sprites with healthbox.
static void SpriteCb_RaidBarrier(struct Sprite *sprite)
{
    u8 healthboxSpriteId = gBattleSpritesDataPtr->battleBars[GetBattlerAtPosition(B_POSITION_OPPONENT_LEFT)].healthboxSpriteId;
    sprite->y2 = gSprites[healthboxSpriteId].y2;
}

static const struct SpriteTemplate sSpriteTemplate_RaidBarrier =
{
    .tileTag = TAG_RAID_BARRIER_TILE,
    .paletteTag = TAG_MISC_INDICATOR_PAL,
    .oam = &sOamData_RaidBarrier,
    .anims = gDummySpriteAnimTable,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCb_RaidBarrier,
};

#define tBattler    data[0]
#define tHide       data[1]
#define hOther_IndicatorSpriteId data[6]

static u32 CreateRaidBarrierSprite(u8 index)
{
    u32 spriteId, position;
    s16 x, y;

    GetBattlerHealthboxCoords(GetBattlerAtPosition(B_POSITION_OPPONENT_LEFT), &x, &y);
    x += sBarrierPosition[0] - (index * 10);
    y += sBarrierPosition[1];

   	LoadSpritePalette(&sSpritePalette_RaidBarrier);
   	LoadSpriteSheet(&sSpriteSheet_RaidBarrier);
    spriteId = CreateSprite(&sSpriteTemplate_RaidBarrier, x, y, 0);

    gSprites[spriteId].tBattler = GetBattlerAtPosition(B_POSITION_OPPONENT_LEFT);
    return spriteId;
}

// Draws all Raid barriers onto the Raid Boss's healthbox.
static void CreateAllRaidBarrierSprites(void)
{
    u8 i;
    for (i = 0; i < gBattleStruct->raid.shield; i++)
    {
        if (gBattleStruct->raid.barrierSpriteIds[i] == MAX_SPRITES)
            gBattleStruct->raid.barrierSpriteIds[i] = CreateRaidBarrierSprite(i);
    }
}

// Destroys one Raid barrier sprite.
static void DestroyRaidBarrierSprite(u8 index)
{
    if (gBattleStruct->raid.barrierSpriteIds[index] != MAX_SPRITES)
    {
        DestroySprite(&gSprites[gBattleStruct->raid.barrierSpriteIds[index]]);
        gBattleStruct->raid.barrierSpriteIds[index] = MAX_SPRITES;
    }

    if (index == 0)
    {
        // Don't free palette because it is shared.
        // FreeSpritePaletteByTag(TAG_MISC_INDICATOR_PAL);
        FreeSpriteTilesByTag(TAG_RAID_BARRIER_TILE);
    }
}

#define hMain_Battler data[6]

void RaidBarrier_SetVisibilities(u32 healthboxId, bool32 invisible)
{
    u32 i;
    for (i = 0; i < gBattleStruct->raid.shield; i++)
    {
        gSprites[gBattleStruct->raid.barrierSpriteIds[i]].invisible = invisible;
    }
}

#undef hMain_Battler
