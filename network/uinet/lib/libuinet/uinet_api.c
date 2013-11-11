/*
 * Copyright (c) 2013 Patrick Kelsey. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */



#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/sockio.h>
#include <sys/uio.h>

#include <net/if.h>
#include <netinet/in.h>
#include <netinet/in_promisc.h>

#include <arpa/inet.h>

#include <pthread.h>

#include "uinet_api.h"


extern struct thread *uinet_thread_alloc(struct proc *p);


int
uinet_initialize_thread(void)
{
	struct thread *td;

	if (NULL == pcurthread) {
		td = uinet_thread_alloc(NULL);
		if (NULL == td)
			return (ENOMEM);
		
		pcurthread = td;
		pcurthread->td_proc = &proc0;
		pcurthread->td_wchan = pthread_self();
	}

	return (0);
}


void
uinet_finalize_thread(void)
{
	struct thread *td = pcurthread;
	
	free(td, M_TEMP);
}


uinet_in_addr_t
uinet_inet_addr(const char *cp)
{
	return (uinet_in_addr_t)inet_addr(cp);
}


char *
uinet_inet_ntoa(struct uinet_in_addr in, char *buf, unsigned int size)
{
	return inet_ntoa_r(*((struct in_addr *)&in), buf, size); 
}


int
uinet_interface_up(const char *canonical_name, unsigned int qno)
{
	struct socket *cfg_so;
	struct thread *td = curthread;
	struct ifreq ifr;
	int error;
	char ifname[IF_NAMESIZE];

	error = socreate(PF_INET, &cfg_so, SOCK_DGRAM, 0, td->td_ucred, td);
	if (0 != error) {
		printf("Socket creation failed (%d)\n", error);
		return (1);
	}

	snprintf(ifname, sizeof(ifname), "%s:%u", canonical_name, qno);
	strcpy(ifr.ifr_name, ifname);

	
	/* set interface to UP */

	error = ifioctl(cfg_so, SIOCGIFFLAGS, (caddr_t)&ifr, td);
	if (0 != error) {
		printf("SSIOCGIFFLAGS failed %d\n", error);
		return (1);
	}

	ifr.ifr_flags |= IFF_UP;
	ifr.ifr_flagshigh |= (IFF_PPROMISC | IFF_PROMISCINET) >> 16;
	error = ifioctl(cfg_so, SIOCSIFFLAGS, (caddr_t)&ifr, td);
	if (0 != error) {
		printf("SSIOCSIFFLAGS failed %d\n", error);
		return (1);
	}

	soclose(cfg_so);

	return (0);
	
}


int
uinet_mac_aton(const char *macstr, uint8_t *macout)
{

	unsigned int i;
	const char *p;
	char *endp;

	if ((NULL == macstr) || (macstr[0] == '\0')) {
		memset(macout, 0, ETHER_ADDR_LEN);
		return (0);
	}

	p = macstr;
	for (i = 0; i < ETHER_ADDR_LEN; i++) {
		macout[i] = strtoul(p, &endp, 16);
		if ((endp != &p[2]) ||					/* two hex digits */
		    ((i < ETHER_ADDR_LEN - 1) && (*endp != ':')) ||	/* followed by ':', unless last pair */
		    ((i == ETHER_ADDR_LEN - 1) && (*endp != '\0'))) {	/* followed by '\0', if last pair */
			return (1);
		}
		p = endp + 1;
	}

	return (0);
}


int
uinet_make_socket_promiscuous(struct uinet_socket *so, unsigned int fib)
{
	struct socket *so_internal = (struct socket *)so;
	unsigned int optval, optlen;
	int error;

	optlen = sizeof(optval);

	optval = 1;
	if ((error = so_setsockopt(so_internal, SOL_SOCKET, SO_PROMISC, &optval, optlen)))
		goto out;
	
	optval = fib;
	if ((error = so_setsockopt(so_internal, SOL_SOCKET, SO_SETFIB, &optval, optlen)))
		goto out;

	optval = 1;
	if ((error = so_setsockopt(so_internal, SOL_SOCKET, SO_REUSEPORT, &optval, optlen)))
		goto out;
	
	optval = 1;
	if ((error = so_setsockopt(so_internal, IPPROTO_IP, IP_BINDANY, &optval, optlen)))
		goto out;

out:
	return (error);
}


int
uinet_setl2info(struct uinet_socket *so, const uint8_t *local_mac, const uint8_t *foreign_mac,
		const uint32_t *tag_stack, const uint32_t mask, int stack_depth)
{
	struct socket *so_internal = (struct socket *)so;
	struct in_l2info l2i;
	struct in_l2tagstack *ts = &l2i.inl2i_tagstack;
	int error = 0;

	memset(&l2i, 0, sizeof(l2i));

	if (local_mac) memcpy(l2i.inl2i_local_addr, local_mac, ETHER_ADDR_LEN);
	if (foreign_mac) memcpy(l2i.inl2i_foreign_addr, foreign_mac, ETHER_ADDR_LEN);

	if (stack_depth < 0) {
		l2i.inl2i_flags |= INL2I_TAG_ANY;
	} else if (stack_depth > 0) {
		ts->inl2t_cnt = stack_depth;
		ts->inl2t_mask = mask;
		memcpy(ts->inl2t_tags, tag_stack, sizeof(uint32_t) * stack_depth);
	}

	error = so_setsockopt(so_internal, SOL_SOCKET, SO_L2INFO, &l2i, sizeof(l2i));

	return (error);
}


struct uinet_socket *
uinet_soaccept(struct uinet_socket *listener, struct uinet_sockaddr **nam)
{
	struct socket *listener_internal = (struct socket *)listener;
	struct socket *so;
	int error;

	*nam = NULL;

	ACCEPT_LOCK();
	if (TAILQ_EMPTY(&listener_internal->so_comp)) {
		ACCEPT_UNLOCK();
		return (NULL);
	}

	so = TAILQ_FIRST(&listener_internal->so_comp);
	KASSERT(!(so->so_qstate & SQ_INCOMP), ("uinet_soaccept: so_qstate SQ_INCOMP"));
	KASSERT(so->so_qstate & SQ_COMP, ("uinet_soaccept: so_qstate not SQ_COMP"));

	/*
	 * Before changing the flags on the socket, we have to bump the
	 * reference count.  Otherwise, if the protocol calls sofree(),
	 * the socket will be released due to a zero refcount.
	 */
	SOCK_LOCK(so);			/* soref() and so_state update */
	soref(so);			/* socket came from sonewconn() with an so_count of 0 */

	TAILQ_REMOVE(&listener_internal->so_comp, so, so_list);
	listener_internal->so_qlen--;
	so->so_state |= (listener_internal->so_state & SS_NBIO);
	so->so_qstate &= ~SQ_COMP;
	so->so_head = NULL;

	SOCK_UNLOCK(so);
	ACCEPT_UNLOCK();

	error = soaccept(so, (struct sockaddr **)nam);
	if (error) {
		soclose(so);
		return (NULL);
	}

	return ((struct uinet_socket *)so);
}


int
uinet_sobind(struct uinet_socket *so, struct uinet_sockaddr *nam)
{
	return sobind((struct socket *)so, (struct sockaddr *)nam, curthread);
}


void
uinet_soclose(struct uinet_socket *so)
{
	soclose((struct socket *)so);
}


int
uinet_soconnect(struct uinet_socket *so, struct uinet_sockaddr *nam)
{
	return soconnect((struct socket *)so, (struct sockaddr *)nam, curthread);
}


int
uinet_socreate(int dom, struct uinet_socket **aso, int type, int proto)
{
	struct thread *td = curthread;

	return socreate(dom, (struct socket **)aso, type, proto, td->td_ucred, td);
}


void
uinet_sogetconninfo(struct uinet_socket *so, struct uinet_in_conninfo *inc)
{
	struct socket *so_internal = (struct socket *)so;

	memcpy(inc, &sotoinpcb(so_internal)->inp_inc, sizeof(struct uinet_in_conninfo));
}


int
uinet_sogetsockopt(struct uinet_socket *so, int level, int optname, void *optval,
		   unsigned int *optlen)
{
	size_t local_optlen;
	int result;

	result = so_getsockopt((struct socket *)so, level, optname, optval, &local_optlen);
	*optlen = local_optlen;

	return (result);
}


short
uinet_sogetstate(struct uinet_socket *so)
{
	struct socket *so_internal = (struct socket *)so;

	return (so_internal->so_state);
}


int
uinet_solisten(struct uinet_socket *so, int backlog)
{
	return solisten((struct socket *)so, backlog, curthread);
}


int
uinet_soreceive(struct uinet_socket *so, struct uinet_sockaddr **psa, struct uinet_uio *uio, int *flagsp)
{
	struct iovec iov[uio->uio_iovcnt];
	struct uio uio_internal;
	int i;
	int result;

	for (i = 0; i < uio->uio_iovcnt; i++) {
		iov[i].iov_base = uio->uio_iov[i].iov_base;
		iov[i].iov_len = uio->uio_iov[i].iov_len;
	}
	uio_internal.uio_iov = iov;
	uio_internal.uio_iovcnt = uio->uio_iovcnt;
	uio_internal.uio_offset = uio->uio_offset;
	uio_internal.uio_resid = uio->uio_resid;
	uio_internal.uio_segflg = UIO_SYSSPACE;
	uio_internal.uio_rw = UIO_READ;
	uio_internal.uio_td = curthread;
	
	result = soreceive((struct socket *)so, (struct sockaddr **)psa, &uio_internal, NULL, NULL, flagsp);

	uio->uio_resid = uio_internal.uio_resid;

	return (result);
}


void
uinet_sosetnonblocking(struct uinet_socket *so, unsigned int nonblocking)
{
	struct socket *so_internal = (struct socket *)so;

	if (nonblocking) {
		so_internal->so_state |= SS_NBIO;
	} else {
		so_internal->so_state &= ~SS_NBIO;
	}

}


int
uinet_sosetsockopt(struct uinet_socket *so, int level, int optname, void *optval,
		   unsigned int optlen)
{
	return so_setsockopt((struct socket *)so, level, optname, optval, optlen);
}


int
uinet_sosend(struct uinet_socket *so, struct uinet_sockaddr *addr, struct uinet_uio *uio, int flags)
{
	struct iovec iov[uio->uio_iovcnt];
	struct uio uio_internal;
	int i;
	int result;

	for (i = 0; i < uio->uio_iovcnt; i++) {
		iov[i].iov_base = uio->uio_iov[i].iov_base;
		iov[i].iov_len = uio->uio_iov[i].iov_len;
	}
	uio_internal.uio_iov = iov;
	uio_internal.uio_iovcnt = uio->uio_iovcnt;
	uio_internal.uio_offset = uio->uio_offset;
	uio_internal.uio_resid = uio->uio_resid;
	uio_internal.uio_segflg = UIO_SYSSPACE;
	uio_internal.uio_rw = UIO_WRITE;
	uio_internal.uio_td = curthread;
	
	result = sosend((struct socket *)so, (struct sockaddr *)addr, &uio_internal, NULL, NULL, flags, curthread);

	uio->uio_resid = uio_internal.uio_resid;

	return (result);
}


int
uinet_soshutdown(struct uinet_socket *so, int how)
{
	return soshutdown((struct socket *)so, how);
}


int
uinet_sogetpeeraddr(struct uinet_socket *so, struct uinet_sockaddr **sa)
{
	struct socket *so_internal = (struct socket *)so;

	*sa = NULL;
	return (*so_internal->so_proto->pr_usrreqs->pru_peeraddr)(so_internal, (struct sockaddr **)sa);
}


int
uinet_sogetsockaddr(struct uinet_socket *so, struct uinet_sockaddr **sa)
{
	struct socket *so_internal = (struct socket *)so;

	*sa = NULL;
	return (*so_internal->so_proto->pr_usrreqs->pru_sockaddr)(so_internal, (struct sockaddr **)sa);
}


void
uinet_free_sockaddr(struct uinet_sockaddr *sa)
{
	free(sa, M_SONAME);
}


void
uinet_soupcall_set(struct uinet_socket *so, int which,
		   int (*func)(struct uinet_socket *, void *, int), void *arg)
{
	struct socket *so_internal = (struct socket *)so;
	struct sockbuf *sb;

	switch(which) {
	case UINET_SO_RCV:
		sb = &so_internal->so_rcv;
		break;
	case UINET_SO_SND:
		sb = &so_internal->so_snd;
		break;
	default:
		return;
	}

	SOCKBUF_LOCK(sb);
	soupcall_set(so_internal, which, (int (*)(struct socket *, void *, int))func, arg);
	SOCKBUF_UNLOCK(sb);
}


void
uinet_soupcall_clear(struct uinet_socket *so, int which)
{
	struct socket *so_internal = (struct socket *)so;
	struct sockbuf *sb;

	switch(which) {
	case UINET_SO_RCV:
		sb = &so_internal->so_rcv;
		break;
	case UINET_SO_SND:
		sb = &so_internal->so_snd;
		break;
	default:
		return;
	}

	SOCKBUF_LOCK(sb);
	soupcall_clear(so_internal, which);
	SOCKBUF_UNLOCK(sb);

}


static int
uinet_api_synfilter_callback(struct inpcb *inp, void *inst_arg, struct syn_filter_cbarg *arg)
{
	struct uinet_api_synfilter_ctx *ctx = inst_arg;
	
	return (ctx->callback((struct uinet_socket *)inp->inp_socket, ctx->arg, arg));
}

static void *
uinet_api_synfilter_ctor(struct inpcb *inp, char *arg)
{
	void *result;
	memcpy(&result, arg, sizeof(result));
	return result;
}


static void
uinet_api_synfilter_dtor(struct inpcb *inp, void *arg)
{
	free(arg, M_DEVBUF);
}


static struct syn_filter synf_uinet_api = {
	"uinet_api",
	uinet_api_synfilter_callback,
	uinet_api_synfilter_ctor,
	uinet_api_synfilter_dtor,
};

static moduledata_t synf_uinet_api_mod = {
	"uinet_api_synf",
	syn_filter_generic_mod_event,
	&synf_uinet_api
};

DECLARE_MODULE(synf_uinet_api, synf_uinet_api_mod, SI_SUB_DRIVERS, SI_ORDER_MIDDLE);


void *
uinet_synfilter_deferral_alloc(struct uinet_socket *so, uinet_api_synfilter_cookie_t cookie)
{
	struct syn_filter_cbarg *cbarg = cookie;
	struct syn_filter_cbarg *result;
	
	result = malloc(sizeof(*result), M_DEVBUF, M_WAITOK);
	*result = *cbarg;

	return result;
}


void
uinet_synfilter_deferral_deliver(struct uinet_socket *so, void *deferral, int decision)
{
	struct socket *so_internal = (struct socket *)so;
	struct syn_filter_cbarg *cbarg = deferral;

	cbarg->decision = decision;
	so_setsockopt(so_internal, IPPROTO_IP, IP_SYNFILTER_RESULT, cbarg, sizeof(*cbarg));

	free(deferral, M_DEVBUF);
}


void
uinet_synfilter_get_conninfo(uinet_api_synfilter_cookie_t cookie, struct uinet_in_conninfo *inc)
{
	struct syn_filter_cbarg *cbarg = cookie;
	memcpy(inc, &cbarg->inc, sizeof(struct uinet_in_conninfo));
}


void
uinet_synfilter_get_l2info(uinet_api_synfilter_cookie_t cookie, struct uinet_in_l2info *l2i)
{
	struct syn_filter_cbarg *cbarg = cookie;
	
	memcpy(l2i->inl2i_local_addr, cbarg->l2i->inl2i_local_addr, UINET_IN_L2INFO_ADDR_MAX);
	memcpy(l2i->inl2i_foreign_addr, cbarg->l2i->inl2i_foreign_addr, UINET_IN_L2INFO_ADDR_MAX);
	l2i->inl2i_cnt = cbarg->l2i->inl2i_tagstack.inl2t_cnt;
	l2i->inl2i_mask = cbarg->l2i->inl2i_tagstack.inl2t_mask;

	if (l2i->inl2i_cnt)
		memcpy(l2i->inl2i_tags, cbarg->l2i->inl2i_tagstack.inl2t_tags, l2i->inl2i_cnt * sizeof(l2i->inl2i_tags[0]));
}


int
uinet_synfilter_install(struct uinet_socket *so, uinet_api_synfilter_callback_t callback, void *arg)
{
	struct socket *so_internal = (struct socket *)so;
	struct uinet_api_synfilter_ctx *ctx;
	struct syn_filter_optarg synf;
	int error = 0;

	ctx = malloc(sizeof(*ctx), M_DEVBUF, M_WAITOK);
	ctx->callback = callback;
	ctx->arg = arg;

	memset(&synf, 0, sizeof(synf));
	strlcpy(synf.sfa_name, synf_uinet_api.synf_name, SYNF_NAME_MAX);
	memcpy(synf.sfa_arg, &ctx, sizeof(ctx));

	if ((error = so_setsockopt(so_internal, IPPROTO_IP, IP_SYNFILTER, &synf, sizeof(synf)))) {
		free(ctx, M_DEVBUF);
	}

	return (error);
}





