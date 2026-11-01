#ifndef KERNIX_CXX_H
#define KERNIX_CXX_H

#ifdef __cplusplus
extern "C" {
#endif

// Run global constructors (.init_array / .ctors). Call once from kmain.
void cxx_init(void);

#ifdef __cplusplus
}
#endif

#endif
