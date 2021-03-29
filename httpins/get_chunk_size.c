	TEXTUS_LONG get_chunk_size(unsigned char *b)
	{
#ifndef Obtainc
#define Obtainc(s)   (s >= 'A' && s <='F' ? s-'A'+10 :(s >= 'a' && s <='f' ? s-'a'+10 : s-'0' ) )
#endif
    		TEXTUS_LONG chunksize = 0;
    		int chunkbits = sizeof(long) * 8;

		/* Skip leading zeros */
		while (*b == '0') ++b;

		while (isxdigit(*b) && (chunkbits > 0)) 
		{
        		int xvalue = 0;
	
            		xvalue = Obtainc(*b);

			chunksize = (chunksize << 4) | xvalue;
			chunkbits -= 4;
			++b;
		}

    		if (isxdigit(*b) && (chunkbits <= 0)) 
		{
        		/* overflow */
        		return -1;
		}
    		return chunksize;
	}
	
