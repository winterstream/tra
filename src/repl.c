#include "tra.h"

static uchar rbuf[100];
Buf*
replread(Replica *r)
{
	uchar hdr[4];
	int n, nn;
	Buf *b, *bb;

	qlock(&r->rlk);
	if(r->err){
	Error:
		werrstr("%s", r->err);
		qunlock(&r->rlk);
		return nil;
	}

	if(tcanread(r->rfd) < 4){
		qunlock(&r->rlk);
		replflush(r);
		qlock(&r->rlk);
		if(r->err)
			goto Error;
	}

	if(treadn(r->rfd, hdr, 4) != 4){
		r->err = "eof reading input";
		goto Error;
	}

	n = LONG(hdr);
	if(n < 6){
		r->err = "short rpc packet";
		goto Error;
	}

	if(n > MAXPACKET){
		memmove(rbuf, hdr, 4);
		n = tread(r->rfd, rbuf+4, sizeof rbuf-5);
		rbuf[n] = 0;
		fprint(2, "IMPLAUSIBLE %s\n", rbuf); 
		r->err = "implausible rpc packet";
		goto Error;
	}

	if(tcanread(r->rfd) < n)
		replflush(r);

	b = mkbuf(nil, n);
	if(treadn(r->rfd, b->p, n) != n){
		r->err = "eof reading input";
		goto Error;
	}

	// dbg(DbgRpc, "replread %.*H\n", (int)(b->ep-b->p), b->p);
	if(r->inflate){
		inzrpctot += b->ep - b->p;
		nn = readbufl(b);
		if(nn > MAXPACKET){
			r->err = "implausible rpc packet";
			goto Error;
		}
		bb = mkbuf(nil, nn);
		// dbg(DbgRpc, "inflate %.*H\n", (int)(b->ep-b->p), b->p);
		if(inflateblock(r->inflate, bb->p, nn, b->p, b->ep-b->p) != nn){
			r->err = "error decompressing block";
			goto Error;
		}
		free(b);
		b = bb;
	}
	inrpctot += b->ep - b->p;
	qunlock(&r->rlk);
	return b;
}

int
replwrite(Replica *r, Buf *b)
{
	int n, nn;
	uchar hdr[8];
	Buf *bb;

	qlock(&r->wlk);
	n = b->ep - b->p;
	bb = nil;
	outrpctot += n;
	if(r->deflate){
		// dbg(DbgRpc, "deflate %.*H\n", (int)(b->ep-b->p), b->p);
		bb = mkbuf(nil, n+128);
		writebufl(bb, n);
		nn = deflateblock(r->deflate, bb->p, bb->ep-bb->p, b->p, n);
		if(nn < 0){
			r->err = "error compressing block";
			qunlock(&r->wlk);
			return -1;
		}
		bb->ep = bb->p+nn;
		bb->p -= 4;
		b = bb;
		// dbg(DbgRpc, " => %.*H\n", (int)(b->ep-b->p), b->p);
		n = b->ep - b->p;
		outzrpctot += n;
	}
	PLONG(hdr, n);
	if(twrite(r->wfd, hdr, 4) != 4
	|| twrite(r->wfd, b->p, n) != n){
		free(bb);
fprint(2, "write error\n");
		r->err = "write error";
		qunlock(&r->wlk);
		return -1;
	}
	free(bb);
	qunlock(&r->wlk);
	return 0;
}

int
replflush(Replica *r)
{
dbg(DbgRpc, "replflush\n");
	qlock(&r->wlk);
	if(twflush(r->wfd) < 0){
		r->err = "write error";
		qunlock(&r->wlk);
		return -1;
	}
	qunlock(&r->wlk);
	return 0;
}

void
replclose(Replica *r)
{
	tclose(r->rfd);
	if(r->rfd != r->wfd)
		tclose(r->wfd);
	free(r);
}

Replica*
fd2replica(int fd0, int fd1)
{
	Replica *repl;

	repl = emalloc(sizeof(Replica));
	repl->tagrend.l = &repl->lk;
	repl->rfd = topen(fd0);
	repl->wfd = topen(fd1);
	return repl;
}

/* BUG: the sysfatals here and in _dialreplica are antisocial */
Replica*
dialreplica(char *name)
{
	Replica *r;

	r = _dialreplica(name);
	if(banner(r, name) < 0)
		sysfatal("%s banner: %r", name);
	r->name = estrdup(name);
	if((r->sysname = rpcmeta(r, "sysname")) == nil)
		sysfatal("%s sysname: %r", name);
	return r;
}
