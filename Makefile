# fetch, libfetch.a, libfetch.so.1

SRCS=		fetch.c common.c ftp.c http.c file.c ftperr.h httperr.h
CSRC=		${SRCS:M*.c}
OBJS=		${CSRC:.c=.o}
SOBJS=		${CSRC:.c=.So}
PREFIX?=	/usr/local
MANPREFIX?=	${PREFIX}/man
CFLAGS=		-DINET6 -DFTP_COMBINE_CWDS
CFLAGS+=	-I${PREFIX}/include -I.
LDADD=		-lssl -lcrypto

RP_LIBDIR=	${PREFIX}/libressl/lib
RP_SSL=		${RP_LIBDIR}/libssl.a
RP_CRYPTO=	${RP_LIBDIR}/libcrypto.a
RP_INCLUDE=	-I${PREFIX}/libressl/include
REG_LIBDIR?=	${PREFIX}/lib
REG_SSL?=	${REG_LIBDIR}/libssl.a
REG_CRYPTO?=	${REG_LIBDIR}/libcrypto.a
REG_INCLUDE?=	-I${PREFIX}/include/openssl

.if defined(RAVENADM)
CFLAGS+=	${RP_INCLUDE}
LIBS=		${RP_SSL} ${RP_CRYPTO}
LDFLAGS=	-L${RP_LIBDIR} -Wl,-rpath,${RP_LIBDIR}
.else
CFLAGS+=	${REG_INCLUDE}
LIBS=		${REG_SSL} ${REG_CRYPTO}
LDFLAGS=	-L${REG_LIBDIR} -Wl,-rpath,${REG_LIBDIR}
.endif
		
PICFLAG=	-fpic
LIB=		fetch
LIB_MAJOR?=	1	# currently so.4 on DF and so.6 on FreeBSD
LIB_STATIC=	lib${LIB}.a
LIB_SHARED=	lib${LIB}.so.${LIB_MAJOR}
LIB_LINK=	lib${LIB}.so
LIB_PIC=	lib${LIB}_pic.a

.SUFFIXES:	.o .c
.SUFFIXES:	.So .c

.if "${OPSYS}" == "Linux"
SRCS+=		funopen.c funopen.h
LDFLAGS+=	-lpthread
TSORT=		tsort 2>/dev/null
.elif "${OPSYS}" == "SunOS"
SRCS+=		estream.c estream.h
SRCS+=		estream-printf.c estream-printf.h
LDFLAGS+=	-lresolv -lsocket
TSORT=		tsort 2>/dev/null
.else
TSORT=		tsort -q
.endif

all: fetch

library: ${LIB_SHARED} ${LIB_STATIC} ${LIB_PIC}

clean:
	rm -f ${OBJS} fetch ftperr.h httperr.h
	rm -f ${SOBJS} ${LIB_SHARED} ${LIB_STATIC} ${LIB_PIC}

.c.o:
	${CC} -c ${.IMPSRC} ${CFLAGS} -o ${.TARGET}

.c.So:
	${CC} ${PICFLAG} -DPIC ${CFLAGS} -c ${.IMPSRC} -o ${.TARGET}

ftp.c: ftperr.h 
http.c: httperr.h

fetch:	${OBJS} main.c
	${CC} -o fetch main.c ${OBJS} ${CFLAGS} ${LIBS} ${LDFLAGS}

ftperr.h: ftp.errors
	@echo "static struct fetcherr ftp_errlist[] = {" > ${.TARGET}
	@cat ${.CURDIR}/ftp.errors \
	  | grep -v ^# \
	  | sort \
	  | while read NUM CAT STRING; do \
	    echo "    { $${NUM}, FETCH_$${CAT}, \"$${STRING}\" },"; \
	  done >> ${.TARGET}
	@echo "    { -1, FETCH_UNKNOWN, \"Unknown FTP error\" }" >> ${.TARGET}
	@echo "};" >> ${.TARGET}
	@echo "Created ${.TARGET}"

httperr.h: http.errors
	@echo "static struct fetcherr http_errlist[] = {" > ${.TARGET}
	@cat ${.CURDIR}/http.errors \
	  | grep -v ^# \
	  | sort \
	  | while read NUM CAT STRING; do \
	    echo "    { $${NUM}, FETCH_$${CAT}, \"$${STRING}\" },"; \
	  done >> ${.TARGET}
	@echo "    { -1, FETCH_UNKNOWN, \"Unknown HTTP error\" }" >> ${.TARGET}
	@echo "};" >> ${.TARGET}
	@echo "Created ${.TARGET}"

${LIB_SHARED}: ${SOBJS}
	@${ECHO} building shared ${LIB} library
	${CC_LINK} ${LDFLAGS} -shared \
	    -o ${.TARGET} -Wl,-soname,${.TARGET} \
	    `lorder ${SOBJS} | ${TSORT}` ${LDADD}

${LIB_STATIC}: ${OBJS}
	${AR} cq ${.TARGET} ${.ALLSRC}

${LIB_PIC}: ${SOBJS}
	${AR} cq ${.TARGET} ${.ALLSRC}

install:
	${BSD_INSTALL_PROGRAM} fetch ${DESTDIR}${PREFIX}/bin/
	${BSD_INSTALL_MAN} fetch.1 ${DESTDIR}${MANPREFIX}/man/man1/

install-library:
	${BSD_INSTALL_DATA} fetch.h fetch-estream.h ${DESTDIR}${PREFIX}/include
	${BSD_INSTALL_DATA} ${LIB_STATIC} ${LIB_PIC} ${DESTDIR}${PREFIX}/lib
	${BSD_INSTALL_LIB} ${LIB_SHARED} ${DESTDIR}${PREFIX}/lib
	ln -s ${LIB_SHARED} ${DESTDIR}${PREFIX}/lib/${LIB_LINK}
	${BSD_INSTALL_MAN} fetch.3 ${DESTDIR}${MANPREFIX}/man/man3/
