/*
 * iterates through a 0-terminated string of items separated by 'del's.
 * white space around 'del' is considered to be a part of 'del'
 * like strtok, but preserves the source, and can iterate several strings at once
 *
 * returns true if next item is found.
 * init pos with NULL to start iteration.
 */
#include <string.h>
#include <ctype.h>
#include <assert.h>
//#include <glob.h>
#define xisspace(x) isspace((unsigned char)(x))

//typedef unsigned int size_t;
/* returns the number of leading white spaces in str; handy in skipping ws */
size_t xcountws(const char *str)
{
    size_t count = 0;
    if (str) {
	while (xisspace(*str)) {
	    str++;
	    count++;
	}
    }
    return count;
}

extern "C" int strListGetItem(const char * str, char del, const char **item, int *ilen, const char **pos)
{
    int len;
    static char delim[2][3] =
    {
	{'"', 0, 0},
	{'"', '\\', 0}};
    int quoted = 0;
    delim[0][1] = del;
    assert(str && item && pos);
    if (*pos) {
	if (!**pos)		/* end of string */
	    return 0;
	else
	    (*pos)++;
    } else {
	*pos = str;
	if (!*pos)
	    return 0;
    }

    /* skip leading ws (ltrim) */
    *pos += xcountws(*pos);
    *item = *pos;		/* remember item's start */
    /* find next delimiter */
    do {
	*pos += strcspn(*pos, delim[quoted]);
	if (**pos == del)
	    break;
	if (**pos == '"') {
	    quoted = !quoted;
	    *pos += 1;
	}
	if (quoted && **pos == '\\') {
	    *pos += 1;
	    if (**pos)
		*pos += 1;
	}
    } while (**pos);
    len = static_cast<int>(*pos - *item);		/* *pos points to del or '\0' */
    /* rtrim */
    while (len > 0 && xisspace((*item)[len - 1]))
	len--;
    if (ilen)
	*ilen = len;
    return len > 0;
}

