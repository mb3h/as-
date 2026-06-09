#ifndef MEMTOUL_H_INCLUDED__
#define MEMTOUL_H_INCLUDED__
#include <stddef.h> // size_t

unsigned long memtoul (const void *nptr, size_t n, void **_endptr, int base);

#endif //ndef MEMTOUL_H_INCLUDED__
