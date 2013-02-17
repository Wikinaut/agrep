/* Copyright (c) 1994 Sun Wu, Udi Manber, Burra Gopal.  All Rights Reserved. */

/*
 * Definitions in this file will be visible throughout glimpse source code.
 * Any global flags or macros can should be defined here.
 */

#if defined(__NeXT__)
#define getcwd(buf,size) getwd(buf)   /* NB: unchecked target size--could overflow; BG: Ok since buffers are usually >= 256B */
#define S_ISREG(mode)   (((mode) & (_S_IFMT)) == (_S_IFREG))
#define S_ISDIR(mode)   (((mode) & (_S_IFMT)) == (_S_IFDIR))
#endif
