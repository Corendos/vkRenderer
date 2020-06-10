#ifndef __CG_RANDOM_H__
#define __CG_RANDOM_H__

void init_random();
void init_random(u64 seed);

u32 randlim(u32 limit);
u32 randrange(u32 a, u32 b);
ConstString random_temp_word(TemporaryMemory* memory, u64 len);

#endif //CG_RANDOM_H
