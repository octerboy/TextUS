class NewClass(object):
    num_count = 0
    def __init__(self,name):
        self.name = name
        self.__class__.num_count += 1
        print name,self.__class__.num_count
    def __del__(self):
        self.__class__.num_count -= 1
        print "Del",self.name,self.__class__.num_count
    def test():
        print "aa"

aa = NewClass("Hello")
bb = NewClass("World")
cc = NewClass("aaaa")

print "Over"
