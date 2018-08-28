import textus
#import m
#class zewClass(object):  
class zewClass(textus.Amor):  
    num_count = 0
    def __init__(self):  
        zewClass.num_count += 1  
#        print "init.... count", zewClass.num_count  
    def __del__(self):  
        zewClass.num_count -= 1  
#        print "Del---- count", zewClass.num_count  
    def test(self,str):  
	print str
	print self
#	aptus.facio()
	print "tt\n"  
    def facio(self, ordo, subor):  
	print self
	print "ordo=" , ordo , " subor=", subor
	print "type self ", type(self), "type ordo ", type(ordo), " subor type " , type(subor)
	self.test("99988")
#	print "type aptus_facio ", type(self.aptus_facio)
#	print "type aptus.facio ", type(aptus.facio)
	print "type textus.error ************ ", type(textus.error)
	tb = textus.TBuffer()
	print tb
	print "type textus.tb----------- ", type(tb)
	apt = textus.Amor()
	print apt
	tb.input("1234")
	tb.input(bytearray("1234"))
	self.aptus_facio(94, 1)
#	self.aptus_facio(self, 94, -1)
	print bytearray("9999aba---------")

#bb = zewClass()
#bb.aptus_facio(bb, 94, -1)
#aa = zewClass()
#aa.test("9999988")
#aa.facio(aa, 94, -1)
#def testa():
#	aa = zewClass()
#	aa.test("9999988")
