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

	awk -f- $THISDIR/http.errors > $TGT <<EOF
BEGIN {
  print "static struct fetcherr http_errlist[] = {"
}
END {
  print "    { -1, FETCH_UNKNOWN, \"Unknown HTTP error\" }"
  print "};"
}
/^[1-9]/ {
  msg=\$3
  for(x = 4; x <= NF; x++) {msg = msg " " \$x}
  print "    { " \$1 ", FETCH_" \$2 ", \"" msg "\" },"
}
EOF
	echo "Created $TGT"
}

gen_ftp()
{
	local TGT=$DEST/ftperr.h

	awk -f- $THISDIR/ftp.errors > $TGT <<EOF
BEGIN {
  print "static struct fetcherr ftp_errlist[] = {"
}
END {
  print "    { -1, FETCH_UNKNOWN, \"Unknown FTP error\" }"
  print "};"
}
/^[1-9]/ {
  msg=\$3
  for(x = 4; x <= NF; x++) {msg = msg " " \$x}
  print "    { " \$1 ", FETCH_" \$2 ", \"" msg "\" },"
}
EOF
	echo "Created $TGT"
}



if [ "$1" = "http" ]; then
	gen_http
elif [ "$1" = "ftp" ]; then
	gen_ftp
else
	echo "usage: pass either 'http' or 'ftp' as the first argument"
fi
