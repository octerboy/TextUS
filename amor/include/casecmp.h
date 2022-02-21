/*
 $Header: /export/home/octerboy/Tasks/textus/amor/include/casecmp.h    $
 $Date$
 $Revision$
*/

/* $NoKeywords: $ */

#if defined(_WIN32)
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#elif !defined(__linux__) && !defined(_AIX) && !defined(__hpux) && !defined(__sun) && !defined(__APPLE__)  && !defined(__FreeBSD__)  && !defined(__NetBSD__)  && !defined(__OpenBSD__)
#include <ctype.h>
static int strcasecmp(const char *s1, const char *s2)
{
	while (*s1 != '\0' && tolower(*s1) == tolower(*s2))
	{
		s1++;
		s2++;
	}

	return tolower(*(unsigned char *) s1) - tolower(*(unsigned char *) s2);
}

static int strncasecmp(const char *s1, const char *s2, unsigned int n)
{
	if (n == 0)
		return 0;

	while (n-- != 0 && tolower(*s1) == tolower(*s2))
	{
		if (n == 0 || *s1 == '\0' || *s2 == '\0')
			break;
		s1++;
		s2++;
	}

	return tolower(*(unsigned char *) s1) - tolower(*(unsigned char *) s2);
}
#endif

