#include "version.c"
extern "C" TEXTUS_AMOR_EXPORT void textus_get_version_1(char *scm_id, char *time_str, char *ver_no, int len) 
{
	textus_base_get_version(scm_id, time_str, ver_no, len);
}
