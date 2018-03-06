/* Altered by Nick Jackson 3/3/18 */

#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "error.h"

enum
{
	MAXPOOL		= 4
};

#define nxt	u.s.bhn
#define fwd	u.s.bhf
#define back	u.s.bhb
#define prev	u.s.bhp

#define RESERVED	512*1024

struct Pool
{ // ---------- No modification needed -------------
	char*	name;
	int	pnum;
	ulong	maxsize;
	int	quanta;
	int	chunk;
	int	monitor;
	ulong	ressize;	/* restricted size */
	ulong	cursize;
	ulong	arenasize;
	ulong	hw;
	Lock	l;
	Bhdr*	root; // Modify?
	Bhdr*	chain; // Modify?
	ulong	nalloc;
	ulong	nfree;
	int	nbrk;
	int	lastfree;
	void	(*move)(void*, void*); // Modify?
};

void*	initbrk(ulong);

struct
{
	int	n;
	Pool	pool[MAXPOOL];
	/* Lock l; */
} table = {
	3,
	{
		{ "main",  0, 	32*1024*1024, 31,  512*1024, 0, 31*1024*1024 },
		{ "heap",  1, 	32*1024*1024, 31,  512*1024, 0, 31*1024*1024 },
		{ "image", 2,   64*1024*1024+256, 31, 4*1024*1024, 1, 63*1024*1024 },
	}
};

Pool*	mainmem = &table.pool[0];
Pool*	heapmem = &table.pool[1];
Pool*	imagmem = &table.pool[2];

static void _auditmemloc(char *, void *);
void (*auditmemloc)(char *, void *) = _auditmemloc;
static void _poolfault(void *, char *, ulong);
void (*poolfault)(void *, char *, ulong) = _poolfault;

/*	non tracing
 *
enum {
	Npadlong	= 0,
	MallocOffset = 0,
	ReallocOffset = 0
};
 *
 */

/* tracing */
enum {
	Npadlong	= 2,
	MallocOffset = 0,
	ReallocOffset = 1
};

enum {
	Monitor = 1
};

void	(*memmonitor)(int, ulong, ulong, ulong) = nil;
#define	MM(v,pc,base,size)	if(!Monitor || memmonitor==nil){} else memmonitor((v),(pc),(base),(size))

#define CKLEAK	0
int	ckleak;
#define	ML(v, sz, pc)	if(CKLEAK && ckleak && v){ if(sz) fprint(2, "%lux %lux %lux\n", (ulong)v, (ulong)sz, (ulong)pc); else fprint(2, "%lux\n", (ulong)v); }

int
memusehigh(void)
{
	return 	mainmem->cursize > mainmem->ressize ||
			heapmem->cursize > heapmem->ressize ||
			0 && imagmem->cursize > imagmem->ressize;
}

int
memlow(void)
{
	return heapmem->cursize > (heapmem->maxsize)/2;
}

int
poolsetsize(char *s, int size)
{
	int i;

	for(i = 0; i < table.n; i++) {
		if(strcmp(table.pool[i].name, s) == 0) {
			table.pool[i].maxsize = size;
			table.pool[i].ressize = size-RESERVED;
			if(size < RESERVED)
				panic("not enough memory");
			return 1;
		}
	}
	return 0;
}

void
poolimmutable(void *v)
{
	Bhdr *b;

	D2B(b, v);
	b->magic = MAGIC_I;
}

void
poolmutable(void *v)
{
	Bhdr *b;

	D2B(b, v);
	b->magic = MAGIC_A;
	((Heap*)v)->color = mutator;
}

char*
poolname(Pool *p)
{
	return p->name;
}

Bhdr*
poolchain(Pool *p)
{
	return p->chain;
}

void
pooldel(Pool *p, Bhdr *t)
{// -------- Marked for death ---------------
	print("Deleting from pool size:%d\n", t->size);
	Bhdr *tp;

	// Case 1
	if(t->prev == nil && p->root != t) {
		//print("del case 1\n");
		t->back->fwd = t->fwd;
		t->fwd->back = t->back;
		//print("Delete return\n");
		return;
	}
	
	// Cases 2.2 and 3.1
	if(t->fwd != t){
		//print("del cases 2.2 and 3.1\n");
		tp = t->fwd;
		if(t->prev == nil){
			p->root = tp;
		}
		tp->prev = t->prev;
		tp->nxt = t->nxt;
		t->back->fwd = t->fwd;
		tp->back = t->back;
		//print("Delete return\n");
		return;
	}
	
	// Case 3.2
	if(t->prev == nil){
		//print("del case 3.2\n");
		p->root = t->nxt;
		if(t->nxt != nil){
			t->nxt->prev = nil;
		}
		//print("Delete return\n");
		return;
	}
	
	// case 2.1
	//print("del case 2.1\n");
	t->prev->nxt = t->nxt;
	if(t->nxt != nil){
		t->nxt->prev = t->prev;
	}
	//print("Delete return\n");
}

void
pooladd(Pool *p, Bhdr *q)
{ // ------------- Marked for death ---------------
	print("Adding to pool size:%d\n", q->size);
	int size;
	Bhdr *tp, *t, *temp;

	q->magic = MAGIC_F;

	q->prev = nil;
	q->fwd = q;
	q->back = q;

	t = p->root;
	if(t == nil) {
		p->root = q;
		//print("Add return\n");
		return;
	}

	size = q->size;
	
	if(t->size > size){
		p->root = q;
		t->prev = q;
		q->nxt = t;
		//print("Add return\n");
		return;
	}
	
	tp = nil;
	
	while(t != nil){
		if(t->size == size){
			q->back = t->back;
			q->back->fwd = q;
			q->fwd = t;
			t->back = q;
			//print("Add return\n");
			return;
		}
		if(t->size > size){
			tp->nxt = q;
			q->nxt = t;
			t->prev = q;
			//print("Add return\n");
			return;
		}
		tp = t;
		t = t->nxt;
	}
	
	tp->nxt = q;
	q->prev = tp;
}

static void*
dopoolalloc(Pool *p, ulong asize, ulong pc)
{ // ------------------ Marked for slight modification ---------------
	print("allocating from pool\n");
	Bhdr *q, *t;
	int alloc, ldr, ns, frag;
	int osize, size;

	if(asize >= 1024*1024*1024){	/* for sanity and to avoid overflow */
		//print("Alloc return\n");
		return nil;
	}
	size = asize;
	osize = size;
	size = (size + BHDRSIZE + p->quanta) & ~(p->quanta);
	
	print("dead1\n");
	
	lock(&p->l);
	p->nalloc++;

	t = p->root;
	q = nil;
	print("dead1.1\n");
	while(t != nil) {
		if(t->size == size) {
			print("dead1.2\n");
			t = t->fwd;
			pooldel(p, t);
			t->magic = MAGIC_A;
			p->cursize += t->size;
			if(p->cursize > p->hw)
				p->hw = p->cursize;
			unlock(&p->l);
			if(p->monitor)
				MM(p->pnum, pc, (ulong)B2D(t), size);
			//print("Alloc return\n");
			return B2D(t);
		}
		print("dead1.3\n");
		if(size < t->size){
			q = t;
		}
		t = t->nxt;
		print("dead1.4\n");
	}
	
	print("dead2\n");
	
	if(q != nil) {
		pooldel(p, q);
		q->magic = MAGIC_A;
		frag = q->size - size;
		
		print("dead3\n");
		
		if(frag < (size>>2) && frag < 0x8000) {
			p->cursize += q->size;
			if(p->cursize > p->hw)
				p->hw = p->cursize;
			unlock(&p->l);
			if(p->monitor)
				MM(p->pnum, pc, (ulong)B2D(q), size);
			//print("Alloc return\n");
			return B2D(q);
		}
		/* Split */
		print("dead4\n");
		
		ns = q->size - size;
		q->size = size;
		B2T(q)->hdr = q;
		t = B2NB(q);
		t->size = ns;
		B2T(t)->hdr = t;
		pooladd(p, t);
		p->cursize += q->size;
		
		print("dead5\n");
		
		if(p->cursize > p->hw){
			print("dead5.1\n");
			p->hw = p->cursize;
		}
		print("dead5.2\n");
		unlock(&p->l);
		print("dead5.3\n");
		if(p->monitor){
			print("dead5.4\n");
			MM(p->pnum, pc, (ulong)B2D(q), size);
		}
		print("Alloc return\n");
		return B2D(q);
	}

	ns = p->chunk;
	if(size > ns)
		ns = size;
	ldr = p->quanta+1;
	
	print("dead6\n");
	
	alloc = ns+ldr+ldr;
	p->arenasize += alloc;
	if(p->arenasize > p->maxsize) {
		p->arenasize -= alloc;
		ns = p->maxsize-p->arenasize-ldr-ldr;
		ns &= ~p->quanta;
		
		print("dead7\n");
		
		if (ns < size) {
			if(poolcompact(p)) {
				unlock(&p->l);
				//print("Alloc return\n");
				return poolalloc(p, osize);
			}

			unlock(&p->l);
			print("arena %s too large: size %d cursize %lud arenasize %lud maxsize %lud\n",
			 p->name, size, p->cursize, p->arenasize, p->maxsize);
			//print("Alloc return\n");
			return nil;
		}
		alloc = ns+ldr+ldr;
		p->arenasize += alloc;
	}

	print("dead8\n");
	
	p->nbrk++;
	t = (Bhdr *)sbrk(alloc);
	if(t == (void*)-1) {
		p->nbrk--;
		unlock(&p->l);
		//print("Alloc return\n");
		return nil;
	}
	/* Double alignment */
	print("dead9\n");
	
	t = (Bhdr *)(((ulong)t + 7) & ~7);

	if(p->chain != nil && (char*)t-(char*)B2LIMIT(p->chain)-ldr == 0){
		/* can merge chains */
		if(0)print("merging chains %p and %p in %s\n", p->chain, t, p->name);
		q = B2LIMIT(p->chain);
		q->magic = MAGIC_A;
		q->size = alloc;
		B2T(q)->hdr = q;
		t = B2NB(q);
		t->magic = MAGIC_E;
		p->chain->csize += alloc;
		p->cursize += alloc;
		unlock(&p->l);
		poolfree(p, B2D(q));		/* for backward merge */
		//print("Alloc return\n");
		return poolalloc(p, osize);
	}
	
	print("dead10\n");
	
	t->magic = MAGIC_E;		/* Make a leader */
	t->size = ldr;
	t->csize = ns+ldr;
	t->clink = p->chain;
	p->chain = t;
	B2T(t)->hdr = t;
	t = B2NB(t);

	t->magic = MAGIC_A;		/* Make the block we are going to return */
	t->size = size;
	B2T(t)->hdr = t;
	q = t;

	ns -= size;			/* Free the rest */
	if(ns > 0) {
		
		print("dead11\n");
		
		q = B2NB(t);
		q->size = ns;
		B2T(q)->hdr = q;
		pooladd(p, q);
	}
	
	print("dead12\n");
	
	B2NB(q)->magic = MAGIC_E;	/* Mark the end of the chunk */

	p->cursize += t->size;
	if(p->cursize > p->hw)
		p->hw = p->cursize;
	unlock(&p->l);
	if(p->monitor)
		MM(p->pnum, pc, (ulong)B2D(t), size);
	
	print("Alloc return\n");
	
	return B2D(t);
}

void *
poolalloc(Pool *p, ulong asize)
{
	Prog *prog;

	if(p->cursize > p->ressize && (prog = currun()) != nil && prog->flags&Prestricted)
		return nil;
	return dopoolalloc(p, asize, getcallerpc(&p));
}

void
poolfree(Pool *p, void *v)
{
	Bhdr *b, *c;
	extern Bhdr *ptr;

	D2B(b, v);
	if(p->monitor)
		MM(p->pnum|(1<<8), getcallerpc(&p), (ulong)v, b->size);

	lock(&p->l);
	p->nfree++;
	p->cursize -= b->size;
	c = B2NB(b);
	if(c->magic == MAGIC_F) {	/* Join forward */
		if(c == ptr)
			ptr = b;
		pooldel(p, c);
		c->magic = 0;
		b->size += c->size;
		B2T(b)->hdr = b;
	}

	c = B2PT(b)->hdr;
	if(c->magic == MAGIC_F) {	/* Join backward */
		if(b == ptr)
			ptr = c;
		pooldel(p, c);
		b->magic = 0;
		c->size += b->size;
		b = c;
		B2T(b)->hdr = b;
	}
	pooladd(p, b);
	unlock(&p->l);
}

void *
poolrealloc(Pool *p, void *v, ulong size)
{ // --------------- Leave this alone -----------
	Bhdr *b;
	void *nv;
	int osize;

	if(size >= 1024*1024*1024)	/* for sanity and to avoid overflow */
		return nil;
	if(size == 0){
		poolfree(p, v);
		return nil;
	}
	SET(osize);
	if(v != nil){
		lock(&p->l);
		D2B(b, v);
		osize = b->size - BHDRSIZE;
		unlock(&p->l);
		if(osize >= size)
			return v;
	}
	nv = poolalloc(p, size);
	if(nv != nil && v != nil){
		memmove(nv, v, osize);
		poolfree(p, v);
	}
	return nv;
}

ulong
poolmsize(Pool *p, void *v)
{
	Bhdr *b;
	ulong size;

	if(v == nil)
		return 0;
	lock(&p->l);
	D2B(b, v);
	size = b->size - BHDRSIZE;
	unlock(&p->l);
	return size;
}

static ulong
poolmax(Pool *p)
{// ----------- Marked for death ------------
	Bhdr *t;
	ulong size;

	lock(&p->l);
	size = p->maxsize - p->cursize;
	t = p->root;
	if(t != nil) {
		size = t->size;
		t = t->nxt;
	}
	if(size >= BHDRSIZE)
		size -= BHDRSIZE;
	unlock(&p->l);
	return size;
}

ulong
poolmaxsize(void)
{
	int i;
	ulong total;

	total = 0;
	for(i = 0; i < nelem(table.pool); i++)
		total += table.pool[i].maxsize;
	return total;
}

int
poolread(char *va, int count, ulong offset)
{
	Pool *p;
	int n, i, signed_off;

	n = 0;
	signed_off = offset;
	for(i = 0; i < table.n; i++) {
		p = &table.pool[i];
		n += snprint(va+n, count-n, "%11lud %11lud %11lud %11lud %11lud %11d %11lud %s\n",
			p->cursize,
			p->maxsize,
			p->hw,
			p->nalloc,
			p->nfree,
			p->nbrk,
			poolmax(p),
			p->name);

		if(signed_off > 0) {
			signed_off -= n;
			if(signed_off < 0) {
				memmove(va, va+n+signed_off, -signed_off);
				n = -signed_off;
			}
			else
				n = 0;
		}

	}
	return n;
}

void*
smalloc(size_t size)
{
	void *v;

	for(;;){
		v = malloc(size);
		if(v != nil)
			break;
		if(0)
			print("smalloc waiting from %lux\n", getcallerpc(&size));
		osenter();
		osmillisleep(100);
		osleave();
	}
	setmalloctag(v, getcallerpc(&size));
	setrealloctag(v, 0);
	return v;
}

void*
kmalloc(size_t size)
{
	void *v;

	v = dopoolalloc(mainmem, size+Npadlong*sizeof(ulong), getcallerpc(&size));
	if(v != nil){
		ML(v, size, getcallerpc(&size));
		if(Npadlong){
			v = (ulong*)v+Npadlong;
			setmalloctag(v, getcallerpc(&size));
			setrealloctag(v, 0);
		}
		memset(v, 0, size);
		MM(0, getcallerpc(&size), (ulong)v, size);
	}
	return v;
}



void*
malloc(size_t size)
{
	void *v;

	v = poolalloc(mainmem, size+Npadlong*sizeof(ulong));
	if(v != nil){
		ML(v, size, getcallerpc(&size));
		if(Npadlong){
			v = (ulong*)v+Npadlong;
			setmalloctag(v, getcallerpc(&size));
			setrealloctag(v, 0);
		}
		memset(v, 0, size);
		MM(0, getcallerpc(&size), (ulong)v, size);
	} else 
		print("malloc failed from %lux\n", getcallerpc(&size));
	return v;
}

void*
mallocz(ulong size, int clr)
{
	void *v;

	v = poolalloc(mainmem, size+Npadlong*sizeof(ulong));
	if(v != nil){
		ML(v, size, getcallerpc(&size));
		if(Npadlong){
			v = (ulong*)v+Npadlong;
			setmalloctag(v, getcallerpc(&size));
			setrealloctag(v, 0);
		}
		if(clr)
			memset(v, 0, size);
		MM(0, getcallerpc(&size), (ulong)v, size);
	} else 
		print("mallocz failed from %lux\n", getcallerpc(&size));
	return v;
}

void
free(void *v)
{
	Bhdr *b;

	if(v != nil) {
		if(Npadlong)
			v = (ulong*)v-Npadlong;
		D2B(b, v);
		ML(v, 0, 0);
		MM(1<<8|0, getcallerpc(&v), (ulong)((ulong*)v+Npadlong), b->size);
		poolfree(mainmem, v);
	}
}

void*
realloc(void *v, size_t size)
{
	void *nv;

	if(size == 0)
		return malloc(size);	/* temporary change until realloc calls can be checked */
	if(v != nil)
		v = (ulong*)v-Npadlong;
	if(Npadlong!=0 && size!=0)
		size += Npadlong*sizeof(ulong);
	nv = poolrealloc(mainmem, v, size);
	ML(v, 0, 0);
	ML(nv, size, getcallerpc(&v));
	if(nv != nil) {
		nv = (ulong*)nv+Npadlong;
		setrealloctag(nv, getcallerpc(&v));
		if(v == nil)
			setmalloctag(v, getcallerpc(&v));
	} else 
		print("realloc failed from %lux\n", getcallerpc(&v));
	return nv;
}

void
setmalloctag(void *v, ulong pc)
{
	ulong *u;

	USED(v);
	USED(pc);
	if(Npadlong <= MallocOffset || v == nil)
		return;
	u = v;
	u[-Npadlong+MallocOffset] = pc;
}

ulong
getmalloctag(void *v)
{
	USED(v);
	if(Npadlong <= MallocOffset)
		return ~0;
	return ((ulong*)v)[-Npadlong+MallocOffset];
}

void
setrealloctag(void *v, ulong pc)
{
	ulong *u;

	USED(v);
	USED(pc);
	if(Npadlong <= ReallocOffset || v == nil)
		return;
	u = v;
	u[-Npadlong+ReallocOffset] = pc;
}

ulong
getrealloctag(void *v)
{
	USED(v);
	if(Npadlong <= ReallocOffset)
		return ((ulong*)v)[-Npadlong+ReallocOffset];
	return ~0;
}

ulong
msize(void *v)
{
	if(v == nil)
		return 0;
	return poolmsize(mainmem, (ulong*)v-Npadlong)-Npadlong*sizeof(ulong);
}

void*
calloc(size_t n, size_t szelem)
{
	return malloc(n*szelem);
}

/*
void
pooldump(Pool *p)
{
	Bhdr *b, *base, *limit, *ptr;

	b = p->chain;
	if(b == nil)
		return;
	base = b;
	ptr = b;
	limit = B2LIMIT(b);

	while(base != nil) {
		print("\tbase #%.8lux ptr #%.8lux", base, ptr);
		if(ptr->magic == MAGIC_A || ptr->magic == MAGIC_I)
			print("\tA%.5d\n", ptr->size);
		else if(ptr->magic == MAGIC_E)
			print("\tE\tL#%.8lux\tS#%.8lux\n", ptr->clink, ptr->csize);
		else
			print("\tF%.5d\tL#%.8lux\tR#%.8lux\tF#%.8lux\tP#%.8lux\tT#%.8lux\n",
				ptr->size, ptr->left, ptr->right, ptr->fwd, ptr->prev, ptr->parent);
		ptr = B2NB(ptr);
		if(ptr >= limit) {
			print("link to #%.8lux\n", base->clink);
			base = base->clink;
			if(base == nil)
				break;
			ptr = base;
			limit = B2LIMIT(base);
		}
	}
}
*/

void
poolsetcompact(Pool *p, void (*move)(void*, void*))
{
	p->move = move;
}

int
poolcompact(Pool *pool)
{
	Bhdr *base, *limit, *ptr, *end, *next;
	int compacted, nb;

	if(pool->move == nil || pool->lastfree == pool->nfree)
		return 0;

	pool->lastfree = pool->nfree;

	base = pool->chain;
	ptr = B2NB(base);	/* First Block in arena has clink */
	limit = B2LIMIT(base);
	compacted = 0;

	pool->root = nil;
	end = ptr;
	while(base != nil) {
		next = B2NB(ptr);
		if(ptr->magic == MAGIC_A || ptr->magic == MAGIC_I) {
			if(ptr != end) {
				memmove(end, ptr, ptr->size);
				pool->move(B2D(ptr), B2D(end));
				compacted = 1;
			}
			end = B2NB(end);
		}
		if(next >= limit) {
			nb = (uchar*)limit - (uchar*)end;
			if(nb > 0){
				if(nb < pool->quanta+1){
					print("poolcompact: leftover too small\n");
					abort();
				}
				end->size = nb;
				B2T(end)->hdr = end;
				pooladd(pool, end);
			}
			base = base->clink;
			if(base == nil)
				break;
			ptr = B2NB(base);
			end = ptr;	/* could do better by copying between chains */
			limit = B2LIMIT(base);
		} else
			ptr = next;
	}

	return compacted;
}

static void
_poolfault(void *v, char *msg, ulong c)
{
	auditmemloc(msg, v);
	panic("%s %lux (from %lux/%lux)", msg, v, getcallerpc(&v), c);
}

static void
dumpvl(char *msg, ulong *v, int n)
{
	int i, l;

	l = print("%s at %p: ", msg, v);
	for (i = 0; i < n; i++) {
		if (l >= 60) {
			print("\n");
			l = print("    %p: ", v);
		}
		l += print(" %lux", *v++);
	}
	print("\n");
}

static void
corrupted(char *str, char *msg, Pool *p, Bhdr *b, void *v)
{
	print("%s(%p): pool %s CORRUPT: %s at %p'%lud(magic=%lux)\n",
		str, v, p->name, msg, b, b->size, b->magic);
	dumpvl("bad Bhdr", (ulong *)((ulong)b & ~3)-4, 10);
}

static void
_auditmemloc(char *str, void *v)
{
	Pool *p;
	Bhdr *bc, *ec, *b, *nb, *fb = nil;
	char *fmsg, *msg;
	ulong fsz;

	SET(fsz);
	SET(fmsg);
	for (p = &table.pool[0]; p < &table.pool[nelem(table.pool)]; p++) {
		lock(&p->l);
		for (bc = p->chain; bc != nil; bc = bc->clink) {
			if (bc->magic != MAGIC_E) {
				unlock(&p->l);
				corrupted(str, "chain hdr!=MAGIC_E", p, bc, v);
				goto nextpool;
			}
			ec = B2LIMIT(bc);
			if (((Bhdr*)v >= bc) && ((Bhdr*)v < ec))
				goto found;
		}
		unlock(&p->l);
nextpool:	;
	}
	print("%s: %p not in pools\n", str, v);
	return;

found:
	for (b = bc; b < ec; b = nb) {
		switch(b->magic) {
		case MAGIC_F:
			msg = "free blk";
			break;
		case MAGIC_I:
			msg = "immutable block";
			break;
		case MAGIC_A:
			msg = "block";
			break;
		default:
			if (b == bc && b->magic == MAGIC_E) {
				msg = "pool hdr";
				break;
			}
			unlock(&p->l);
			corrupted(str, "bad magic", p, b, v);
			goto badchunk;
		}
		if (b->size <= 0 || (b->size & p->quanta)) {
			unlock(&p->l);
			corrupted(str, "bad size", p, b, v);
			goto badchunk;
		}
		if (fb != nil)
			break;
		nb = B2NB(b);
		if ((Bhdr*)v < nb) {
			fb = b;
			fsz = b->size;
			fmsg = msg;
		}
	}
	unlock(&p->l);
	if (b >= ec) {
		if (b > ec)
			corrupted(str, "chain size mismatch", p, b, v);
		else if (b->magic != MAGIC_E)
			corrupted(str, "chain end!=MAGIC_E", p, b, v);
	}
badchunk:
	if (fb != nil) {
		print("%s: %p in %s:", str, v, p->name);
		if (fb == v)
			print(" is %s '%lux\n", fmsg, fsz);
		else
			print(" in %s at %p'%lux\n", fmsg, fb, fsz);
		dumpvl("area", (ulong *)((ulong)v & ~3)-4, 20);
	}
}

char *
poolaudit(char*(*audit)(int, Bhdr *))
{
	Pool *p;
	Bhdr *bc, *ec, *b;
	char *r = nil;

	for (p = &table.pool[0]; p < &table.pool[nelem(table.pool)]; p++) {
		lock(&p->l);
		for (bc = p->chain; bc != nil; bc = bc->clink) {
			if (bc->magic != MAGIC_E) {
				unlock(&p->l);
				return "bad chain hdr";
			}
			ec = B2LIMIT(bc);
			for (b = bc; b < ec; b = B2NB(b)) {
				if (b->size <= 0 || (b->size & p->quanta))
					r = "bad size in bhdr";
				else
					switch(b->magic) {
					case MAGIC_E:
						if (b != bc) {
							r = "unexpected MAGIC_E";
							break;
						}
					case MAGIC_F:
					case MAGIC_A:
					case MAGIC_I:
						r = audit(p->pnum, b);
						break;
					default:
						r = "bad magic";
					}
				if (r != nil) {
					unlock(&p->l);
					return r;
				}
			}
			if (b != ec || b->magic != MAGIC_E) {
				unlock(&p->l);
				return "bad chain ending";
			}
		}
		unlock(&p->l);
	}
	return r;
}
