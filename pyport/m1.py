class zewClass:  
    num_count = 0
    def __init__(self):  
        zewClass.num_count += 1  
        print zewClass.num_count  
    def __del__(self):  
        zewClass.num_count -= 1  
        print "Del",zewClass.num_count  
    def test(self,str):  
	print str
        print "tt\n"  
  

