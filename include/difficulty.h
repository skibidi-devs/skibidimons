#ifndef GUARD_DIFFICULTY_H
#define GUARD_DIFFICULTY_H

u32 GetCurrentDifficultyLevel(void);
u32 GetBattlePartnerDifficultyLevel(u16);
u32 GetTrainerDifficultyLevel(u16);
void Script_IncreaseDifficulty(void);
void Script_DecreaseDifficulty(void);
void Script_SetDifficulty(u32);

enum DifficultyLevel
{
    DIFFICULTY_EASY,
    DIFFICULTY_NORMAL,
    DIFFICULTY_HARD,
    DIFFICULTY_COUNT,
};

#define DIFFICULTY_DEFAULT DIFFICULTY_NORMAL
#define DIFFICULTY_MIN 0
#define DIFFICULTY_MAX (DIFFICULTY_COUNT - 1)

#endif // GUARD_DIFFICULTY_H