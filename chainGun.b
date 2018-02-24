implement ChainGun;

include "draw.m";
include "sys.m";
	sys: Sys;

ChainGun: module
{
	init: fn (ctxt: ref Draw->Context, argv: list of string);
};

init(ctxt: ref Draw->Context, argv: list of string)
{
	for(;;)
	{
		spawn bullet();
		spawn bullet();
		spawn bullet();
	}
}

bullet()
{
	sys = load Sys Sys->PATH;
	sys->print("bullet pid: %d\n", sys->pctl(0, nil));
}
