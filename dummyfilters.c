/* Copyright (c) 1994 Burra Gopal, Udi Manber.  All Rights Reserved. */

/* bgopal: used if search in compressed text files is not being performed */
/* Always say could not be compressed */
int
quick_tcompress()
{
	return 0;
}

/* Always say could not be uncompressed */
int
quick_tuncompress()
{
	return 0;
}

/* Always return uncompressible */
int
tuncompressible()
{
	return 0;
}

/* Always return uncompressible */
int
tuncompressible_filename()
{
	return 0;
}

/* Always return uncompressible */
int
tuncompressible_file()
{
	return 0;
}

/* Always return uncompressible */
int
tuncompressible_fp()
{
	return 0;
}

int
exists_tcompressed_word()
{
	return -1;
}

unsigned char *
forward_tcompressed_word(begin, end, delim, len, outtail, flags)
unsigned char *begin, *end, *delim;
int len, outtail, flags;
{
	return begin;
}

unsigned char *
backward_tcompressed_word(end, begin, delim, len, outtail, flags)
unsigned char *begin, *end, *delim;
int len, outtail, flags;
{
	return end;
}

int
tcompress_file()
{
	return 0;
}

int
tuncompress_file()
{
	return 0;
}

int
initialize_tcompress()
{
	return 0;
}

int
initialize_tuncompress()
{
	return 0;
}

int
initialize_common()
{
	return 0;
}

int
uninitialize_tuncompress()
{
	return 0;
}

int
compute_dictionary()
{
	return 0;
}

int
uninitialize_common()
{
	return 0;
}

int
uninitialize_tcompress()
{
	return 0;
}

int usemalloc = 0;

int
set_usemalloc()
{
	return 0;
}

int
unset_usemalloc()
{
	return 0;
}
