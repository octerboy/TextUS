AMOROBJS=R531DeviceFunc.obj publicFunc.obj des.obj atr.obj

R531DeviceFunc.obj:R531DeviceFunc.cpp 
	$(CC) $(CFLAGS) R531DeviceFunc.cpp

publicFunc.obj:publicFunc.cpp 
	$(CC) $(CFLAGS) publicFunc.cpp

des.obj:DES.cpp 
	$(CC) $(CFLAGS) DES.cpp

atr.obj:ATR.cpp 
	$(CC) $(CFLAGS) ATR.cpp

clean:
	del *.obj
	del *.dll.*
	del t*.lib
	del *.exp
