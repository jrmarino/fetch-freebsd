#!/bin/sh

if [ -z "$2" ]; then
	DEST="."
else
	DEST="$2"
fi

DPATH=$(dirname $0)
THISDIR=$(cd ${DPATH} && pwd -P)

gen_http()
{
	local TGT=$DEST/httperr.h
	local EDATA=$DEST/http.errors

	cp $THISDIR/http.errors $EDATA
	echo "static struct fetcherr http_errlist[] = {" > $TGT
	grep -v ^\# $EDATA \
		| sort \
		| while read -r NUM CAT STRING; do \
			echo "    { $NUM, FETCH_$CAT, \"$STRING\" },"; \
		done >> $TGT
	echo "    { -1, FETCH_UNKNOWN, \"Unknown HTTP error\" }" >> $TGT
	echo "};" >> $TGT
	echo "Created $TGT"
}

gen_ftp()
{
	local TGT=$DEST/ftperr.h
	local EDATA=$DEST/ftp.errors

	cp $THISDIR/ftp.errors $EDATA
	echo "static struct fetcherr ftp_errlist[] = {" > $TGT
	grep -v ^\# $EDATA \
		| sort \
		| while read -r NUM CAT STRING; do \
			echo "    { $NUM, FETCH_$CAT, \"$STRING\" },"; \
		done >> $TGT
	echo "    { -1, FETCH_UNKNOWN, \"Unknown FTP error\" }" >> $TGT
	echo "};" >> $TGT
	echo "Created $TGT"
}



if [ "$1" = "http" ]; then
	gen_http
elif [ "$1" = "ftp" ]; then
	gen_ftp
else
	echo "usage: pass either 'http' or 'ftp' as the first argument"
fi
