class zewClass(object):  
    num_count = 0
    def __init__(self,name):  
        self.name = name  
        zewClass.num_count += 1  
        print name,zewClass.num_count  
    def __del__(self):  
        zewClass.num_count -= 1  
        print "Del",self.name,zewClass.num_count  
    def test():  
        print "aa"  
  
aa = zewClass("Hello")  
bb = zewClass("World")  
cc = zewClass("aaaa")  
  
print "Over"  

