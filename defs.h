/* Copyright (c) 1994 Sun Wu, Udi Manber, Burra Gopal.  All Rights Reserved. */
/* Must be the same as those defined in compress/defs.h */
#define SIGNATURE_LEN	16
#define TC_LITTLE_ENDIAN 1
#define TC_BIG_ENDIAN	0
#define TC_EASYSEARCH	0x1
#define TC_UNTILNEWLINE	0x2
#define TC_REMOVE	0x4
#define TC_OVERWRITE	0x8
#define TC_RECURSIVE	0x10
#define TC_ERRORMSGS	0x20
#define TC_SILENT	0x40
#define TC_NOPROMPT	0x80
#define TC_FILENAMESONSTDIN 0x100
#define COMP_SUFFIX	".CZ"
#define DEF_FREQ_FILE	".glimpse_quick"
#define DEF_HASH_FILE	".glimpse_compress"
#define DEF_STRING_FILE	".glimpse_uncompress"
