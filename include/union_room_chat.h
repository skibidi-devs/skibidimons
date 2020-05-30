#ifndef GUARD_UNION_ROOM_CHAT_H
#define GUARD_UNION_ROOM_CHAT_H

enum
{
    UNION_ROOM_KB_PAGE_UPPER,
    UNION_ROOM_KB_PAGE_LOWER,
    UNION_ROOM_KB_PAGE_EMOJI,
    UNION_ROOM_KB_PAGE_COUNT
};

#define UNION_ROOM_KB_ROW_COUNT 10


void EnterUnionRoomChat(void);
void copy_strings_to_sav1(void);

#endif // GUARD_UNION_ROOM_CHAT_H
