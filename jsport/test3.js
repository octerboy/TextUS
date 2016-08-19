function ignite()
{
	this.count = 0;
	print ( "test3 ... ignite  this.count " + this.count + " \n");
	print ( "test3 version " + version() + " \n");
}

function sponte(pius)
{
	print ("test3 here in sponte pius.ordo " + pius.ordo + " \n");
	return true;
}

function facio(pius)
{
	print ("test3 " + this.count + " here in facio pius.ordo " + pius.ordo + " \n" );
	var dic=new Object();
	b = new Pius();
	p0 = new Pius();
	p1 = new Pius();
	p2= new Pius();
	if ( pius.ordo == 200 && this.count > 0)
	{
		print("START_SESSION\n");
		b.ordo = 99;
		this.aptus_facio(b);
		this.pac0.set(1,100);
		this.pac0.set(2,"9981abcd\0");
		this.pac0.set(3,"I v mess");
		this.pac1.set(1,3499);
		print("my pac1(1) " + this.pac1.getInt(1) + "\n");
		p0.ordo = 200;
		p0.indic = new Object();
		p0.indic.pac0 = this.pac0;
		p0.indic.pac1 = this.pac1;
		this.aptus_facio(p0);
	}
	return true;
}

function clone()
{
	var neo = new Amor();
	neo.count = this.count + 1;
	neo.pac0 = this.alloc("packet");
	neo.pac0.maxium = 100;
	neo.pac1 = this.alloc("packet");
	neo.pac1.maxium = 100;
	return neo;
}
