import textor
import binascii
class zewClass(textor.Amor):  
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
    def facio(self, pius):  
#	print self
	print "type of pius" , type(pius), " ordo=" , pius.ordo , " subor=", pius.subor
#	if not isnull(pius.indic) :
	print  "type of pius.indic " , type(pius.indic)

	if pius.ordo == textor._MAIN_PARA:
		print "pius.ordo == MAIN_PARA "
		for j in pius.indic :
			print j, " ",
		print " "

	if pius.ordo == textor._SET_TBUF:
		print "pius.ordo == SET_TBUF "
		print "type of pius" , type(pius), " type of pius.indic " , type(pius.indic)
		a = pius.indic[0]
		b = pius.indic[1]
		print a
		print b
		a.input("1234234")
		print a.getbytes()

	if pius.ordo == textor._SET_UNIPAC:
		print "pius.ordo == SET_UNIPAC "
		print "type of pius" , type(pius), " type of pius.indic " , type(pius.indic)
		a = pius.indic[0]
		b = pius.indic[1]
		print a
		print b
		a.set(1,"abixyhxx")
		print a.getbytes(1)

	if pius.ordo == textor._WebSock_Start :
		print "WebSocket Start ", pius.indic

#	self.test("99988")
#	print "type aptus_facio ", type(self.aptus_facio)
#	print "type aptus.facio ", type(aptus.facio)
	print "type textor.error ************ ", type(textor.error)
	ta = textor.TBuffer(64)
	tb = textor.TBuffer()
	print tb
	tb.input("1234234")
	print "type textor.tb----------- ", type(tb), " getbytes ", tb.getbytes(),  " getstr ", tb.get()
	tq = textor.Packet(128)
	tp = textor.Packet(128)
	print tp
	print "tp.set(1) return ", tp.set(1, "good example")	, " MAIN_PARA ", textor._MAIN_PARA
	#textor._MAIN_PARA = 3
	tp.set(2,"abc")	
	tp.set(2,"abc", "xyz")	
#	tp.set(2,"abc", "xyz", "aa")	
	print "get textor.tp----------- ", type(tp), "  get2 ", tp.get(2), " get1 ", tp.get(1)
	apt = textor.Amor()
	print apt
	tb.input("1234") 
	print "tb content ", tb.get()
#	tb.input(tp) 
	tb.input(bytearray("12\x0034"))
	print "tb content ", binascii.b2a_hex(tb.getbytes())
#	ps = textor.Pius(11,11)
	ps = textor.Pius(11)
	print "ps.ordo -- =", ps.ordo
	ps.subor = -2
	ps.ordo = textor._SET_TBUF
	print "ps.ordo ++=  =", ps.ordo
	print ps 
	ps.indic = [ta, tb]
	self.aptus_facio(ps)
	self.aptus_facio(ps)
	pss = textor.Pius(11)
	pss.ordo = textor._SET_UNIPAC
	pss.subor = -2
	pss.indic = [tq, tp]
#	pss.indic = [ta, tb]
	self.aptus_facio(pss)
	self.aptus_facio( textor.Pius(textor._LOG_ALERT))
	self.aptus_log_bug("Eample for debug")
	print bytearray("9999aba---------")
#	alist = ['phy', 'chem', 199, 2000]
#	print 'list len ', len(alist)
	return True

#bb = zewClass()
#bb.aptus_facio(bb, 94, -1)
#aa = zewClass()
#aa.test("9999988")
#aa.facio(aa, 94, -1)
#def testa():
#	aa = zewClass()
#	aa.test("9999988")
