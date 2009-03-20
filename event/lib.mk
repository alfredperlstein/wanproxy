.if !defined(TOPDIR)
.error "TOPDIR must be defined."
.endif

.PATH: ${TOPDIR}/event

SRCS+=	event_poll.cc
SRCS+=	event_system.cc
SRCS+=	timeout.cc
SRCS+=	timer.cc

OSNAME!=	uname -s

.if !defined(USE_POLL)
.if ${OSNAME} == "Darwin" || ${OSNAME} == "FreeBSD"
USE_POLL=	kqueue
.else
USE_POLL=	poll
.endif
.endif

.if ${OSNAME} == "Linux"
# Required for clock_gettime(3).
LDADD+=		-lrt
.endif

SRCS+=	event_poll_${USE_POLL}.cc

.if ${USE_POLL} == "kqueue"
CFLAGS+=-DUSE_POLL_KQUEUE
.elif ${USE_POLL} == "poll"
CFLAGS+=-DUSE_POLL_POLL
.else
.error "Unsupported poll mechanism."
.endif
