#ifndef __HASH_H__
#define __HASH_H__

u64 hash(const char* str);
u64 hash(ConstString str);
u64 hash(const u8* data, u32 count);

#endif
