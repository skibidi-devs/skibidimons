#ifndef GUARD_BATTLE_RAID_H
#define GUARD_BATTLE_RAID_H

struct RaidType {
    u8 shield:1;
    u8 shockwave:1;
    u8 rules:1;
    u8 gimmick;
};

struct RaidData {
    u8 rank;
    u8 raidType;
    struct Pokemon* mon;
    struct Trainer* partners;
    u8 numPartners;
};

extern const struct RaidType gRaidTypes[NUM_RAID_TYPES];
extern struct RaidData* gRaidData;

// Functions
bool32 InitRaidData(void);
bool32 InitCustomRaidData(void);
void InitRaidBattleData(void);
bool32 IsRaidBoss(u16 battlerId);
u8 GetRaidBattleTransition(void);
void ApplyRaidHPMultiplier(u16 battlerId, struct Pokemon* mon);
bool32 ShouldRaidKickPlayer(void);
bool32 UpdateRaidShield(void);
u16 GetShieldDamageRequired(u16 hp, u16 maxHP);
u16 GetShieldDamageReduction(void);
void RaidBarrier_SetVisibilities(u32 healthboxId, bool32 invisible);

#endif
