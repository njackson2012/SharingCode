#include	"dat.h"
#include	"fns.h"
#include	"error.h"

enum {
	Qdir = 0,
	Qrad,
	Qradctl
};

Dirtab radtab[] = {
	".",		{Qdir, 0, QTDIR},	0,	0555,
	"radctl",	{Qradctl},	0,	0666,
};

static Chan*
radfsattach(char *spec){
	return devattach('R', spec);
}

static Walkqid*
radfswalk(Chan *c, Chan *nc, char **name, int nname){
	return devwalk(c, nc, name, nname, radtab, nelem(radtab), devgen);
}

static int
radfsstat(Chan *c, uchar *db, int n){
	return devstat(c, db, n, radtab, nelem(radtab), devgen);
}

static Chan*
radfsopen(Chan *c, int omode){
	return devopen(c, omode, radtab, nelem(radtab), devgen);
}

static void	 
radfsclose(Chan *c){
	USED(c);
}

static long
radfsread(Chan *c, void *buf, long n, vlong offset){
	ulong len;
	uchar *data;

	if(c->qid.type & QTDIR)
		return devdirread(c, buf, n, radtab, nelem(radtab), devgen);
	len = nelem(radtab);
	if(offset < 0 || offset >= len)
		return 0;
	if(offset+n > len)
		n = len - offset;
	data = radtab;
	memmove(buf, data+offset, n);
	return n;
}


Dev radfsdevtab = {
	'R',
	"radfs",

	devinit,     // done
	radfsattach, // done
	radfswalk,   // done
	radfsstat,   // done
	radfsopen,   // done
	devcreate,   // done
	radfsclose,  // done
	radfsread,   // done
	devbread,    // done
	devbwrite,   // done
	devwstat     // done
};