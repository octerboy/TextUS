inline static int  hexit( char c )
{
    	if ( c >= '0' && c <= '9' )
		return c - '0';
    	if ( c >= 'a' && c <= 'f' )
		return c - 'a' + 10;
    	if ( c >= 'A' && c <= 'F' )
		return c - 'A' + 10;
    	return 0;           /* shouldn't happen, we're guarded by isxdigit() */
}

inline static void  strdecode( char* to, char* from )
{
	for ( ; *from != '\0'; ++to, ++from )
	{
		if ( from[0] == '%' && isxdigit( from[1] ) && isxdigit( from[2] ) )
	    	{
	    		*to = hexit( from[1] ) * 16 + hexit( from[2] );
	    		from += 2;
	    	}
		else if( from[0] == '+')
		{
			*to = ' ';
		}else
		    	*to = *from;
	}
    	*to = '\0';
}

