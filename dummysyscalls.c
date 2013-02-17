
/* These functions have been added here so that agrep/cast binaries will work independent of glimpse */

int
my_open(name, flags, mode)
	char	*name;
	int	flags, mode;
{
	return open(name, flags, mode);
}

FILE *
my_fopen(name, flags)
	char	*name;
	char	*flags;
{
	return fopen(name, flags);
}

int
my_lstat(name, buf)
	char	*name;
	struct stat *buf;
{
	return lstat(name, buf);
}

int
my_stat(name, buf)
	char	*name;
	struct stat *buf;
{
	return stat(name, buf);
}

int
special_get_name(name, len, temp)
	char	*name;
	int	len;
	char	*temp;
{
	strcpy(temp, name);
	return 0;
}

