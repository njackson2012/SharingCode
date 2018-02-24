implement Bunny;

include "draw.m";
include "sys.m";
	sys: Sys;

Bunny: module
{
	init: fn (ctxt: ref Draw->Context, argv: list of string);
};

init(ctxt: ref Draw->Context, argv: list of string)
{
	mate();
}

mate()
{
	spawn bunny();
}

bunny()
{
	sys = load Sys Sys->PATH;
	mate();
	mate();
	sys->print("bunny pid: %d\n", sys->pctl(0, nil));
}
