#ifndef _ZLIB_FGETS_H
#define _ZLIB_FGETS_H

/*
 zlib provides the equivalent of fopen and fread for gzipped files, but it's
 missing fgets.  This is a quick 'n dirty replacement.
*/
#include <zlib.h>


char *gzgets(char *str, int n, gzFile f)
{
	int count_read = 0;
	char *s = str;

	if (n == 0)
		return NULL;
	if (n == 1) {
		*s = '\0';
		return NULL;
	}

	while (1) {
		if (!gzread(f, s, 1))
			break;
		count_read++;
		if (count_read >= n-1) {
			*(s+1) = '\0';
			return str;
		}
		if (*s == '\n')
			break;
		s++;
	}

	if (!count_read)
		return NULL;
	*(s+1) = '\0';
	return str;
}

#endif
