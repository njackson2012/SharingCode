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
radfsread(Chan *c, void *a, long n, vlong offset){
	char* buff;
	ulong pth;
	pth = c->qid.path;
	
}

Dev radfsdevtab = {
	'R',
	"radfs",

	devinit,     // done
	radfsattach, // done
	radfswalk,   // done
	radfsstat,   // done
	radfsopen,   // done
	radfscreate,
	radfsclose,  // done
	radfsread,
	devbread,    // done
	radfswrite,
	devbwrite,   // done
	devwstat     // done
};