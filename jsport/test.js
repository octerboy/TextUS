function ignite()
{
	this.name = 0;
	this.count = 0;
	print ( " this.name " + this.name + " \n");
	print ( "version " + version() + " \n");
}

function sponte(pius)
{
	i = 0;
	print ("here in sponte pius.ordo\n" + pius.ordo);
	print ( "who " + i );
}

function facio(pius)
{
	a = 0;
	b = new Pius();
	print ("name ", this.name, " facio ordo " , pius.ordo , "\n");
	if ( pius.ordo == 200 )
	{
		var mystr = "abcd\099";
		print("mystr len " + mystr.length + "\n");
		print("pac0.maxium " + pius.indic.pac0.maxium +"\n");
		print("pac1.maxium " + pius.indic.pac1.maxium +"\n");
		pius.indic.pac0.set(1,mystr);
		print("pac1.fld[1].length=" + pius.indic.pac0.get(1).length + "\n");
		print("pac0.fld[10] " + pius.indic.pac0.get(10) + "\n");
		print("pac1.fld[10] " + pius.indic.pac1.get(10) + "\n");
	}
	pius.ordo = 999;
	this.aptus_sponte(pius);
	return true;
}

function clone()
{
	var neo = new Amor();
	this.count = this.count + 1;
	neo.name = this.count;
	return neo;
}
