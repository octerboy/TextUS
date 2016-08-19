function ignite()
{
	this.name = 0;
	this.count = 0;
	var ps;
	ps = new Pius();
	ps.ordo = 200;
	ps.indic = new Object();
	ps.indic.pac0 = this.alloc("packet");
	ps.indic.pac0.maxium = 100;
	ps.indic.pac0.set(10,"9988aa");
	ps.indic.pac1 = this.alloc("packet");
	ps.indic.pac1.maxium = 99;
	ps.indic.pac1.set(10,"777aaa");
	this.ps = ps;
}

function sponte(pius)
{
}

function facio(pius)
{
	var con;
	if ( pius.ordo == 101 )
	{
		this.recv_buf = pius.indic.tb0;
		this.aptus_facio(this.ps);
	}
	if ( pius.ordo == 102 )
	{
		con = this.recv_buf.peek(2000);
		print(con);
	}
	return true;
}

function clone()
{
	var neo = new Amor();
	this.count = this.count + 1;
	neo.name = this.count;
	neo.ps = this.ps;
	return neo;
}
