#
# Derived from FreeBSD src/sys/conf/kern.pre.mk
#

include ${TOP_DIR}/network/uinet/mk/compiler.mk

MACHINE_CPUARCH:= $(shell uname -m)

# Convert Mac OS X name to FreeBSD one.
ifeq (${MACHINE_CPUARCH},x86_64)
MACHINE_CPUARCH=	amd64
endif

AWK?=		awk

ifdef DEBUG
_MINUS_O=	-O
CTFFLAGS+=	-g
else
ifeq (${MACHINE_CPUARCH},powerpc)
_MINUS_O=	-O	# gcc miscompiles some code at -O2
else
_MINUS_O=	-O2
endif
endif
ifeq (${MACHINE_CPUARCH},amd64)
ifneq (${COMPILER_TYPE},clang)
COPTFLAGS?=-O2 -frename-registers -pipe
else
COPTFLAGS?=-O2 -pipe
endif
else
COPTFLAGS?=${_MINUS_O} -pipe
endif

ifneq ($(filter -O2 -O3 -Os,${COPTFLAGS}),) 
ifeq ($(filter -fno-strict-aliasing,${COPTFLAGS}),)
COPTFLAGS+= -fno-strict-aliasing
endif
endif

ifndef NO_CPU_COPTFLAGS
COPTFLAGS+= ${_CPUCFLAGS}
endif
C_DIALECT= -std=c99
NOSTDINC= -nostdinc

INCLUDES= ${NOSTDINC} ${INCLMAGIC} -I. -I$S

# This hack lets us use the OpenBSD altq code without spamming a new
# include path into contrib'ed source files.
INCLUDES+= -I$S/contrib/altq

CFLAGS=	${COPTFLAGS} ${C_DIALECT} ${DEBUG} ${CWARNFLAGS}
CFLAGS+= ${INCLUDES} -D_KERNEL -DHAVE_KERNEL_OPTION_HEADERS -include opt_global.h
ifneq (${COMPILER_TYPE},clang)
CFLAGS+= -fno-common -finline-limit=${INLINE_LIMIT}
ifneq (${MACHINE_CPUARCH},mips)
CFLAGS+= --param inline-unit-growth=100
CFLAGS+= --param large-function-growth=1000
else
# XXX Actually a gross hack just for Octeon because of the Simple Executive.
CFLAGS+= --param inline-unit-growth=10000
CFLAGS+= --param large-function-growth=100000
CFLAGS+= --param max-inline-insns-single=10000
endif
endif
WERROR?= -Werror

# XXX LOCORE means "don't declare C stuff" not "for locore.s".
ASM_CFLAGS= -x assembler-with-cpp -DLOCORE ${CFLAGS}

ifeq (${COMPILER_TYPE},clang)
CLANG_NO_IAS= -no-integrated-as
endif

DEFINED_PROF=	${PROF}

# Put configuration-specific C flags last (except for ${PROF}) so that they
# can override the others.
CFLAGS+=	${CONF_CFLAGS}

# Optional linting. This can be overridden in /etc/make.conf.
LINTFLAGS=	${LINTOBJKERNFLAGS}

NORMAL_C= ${CC} -c ${CFLAGS} ${WERROR} ${PROF} $<
NORMAL_S= ${CC} -c ${ASM_CFLAGS} ${WERROR} $<
PROFILE_C= ${CC} -c ${CFLAGS} ${WERROR} $<
NORMAL_C_NOWERROR= ${CC} -c ${CFLAGS} ${PROF} $<

NORMAL_M= ${AWK} -f $S/tools/makeobjops.awk $< -c ; \
	  ${CC} -c ${CFLAGS} ${WERROR} ${PROF} $*.c

GEN_CFILES= $S/$M/$M/genassym.c ${MFILES:T:S/.m$/.c/}
SYSTEM_CFILES= config.c env.c hints.c vnode_if.c
SYSTEM_DEP= Makefile ${SYSTEM_OBJS}
SYSTEM_OBJS= locore.o ${MDOBJS} ${OBJS}
SYSTEM_OBJS+= ${SYSTEM_CFILES:.c=.o}
SYSTEM_OBJS+= hack.So
SYSTEM_LD= @${LD} -Bdynamic -T ${LDSCRIPT} ${LDFLAGS} --no-warn-mismatch \
	-warn-common -export-dynamic -dynamic-linker /red/herring \
	-o ${.TARGET} -X ${SYSTEM_OBJS} vers.o
SYSTEM_LD_TAIL= @${OBJCOPY} --strip-symbol gcc2_compiled. ${.TARGET} ; \
	${SIZE} ${.TARGET} ; chmod 755 ${.TARGET}
SYSTEM_DEP+= ${LDSCRIPT}

