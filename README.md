# fetch-freebsd

This is the code for the fetch utility from FreeBSD Release 14.0.
It has been modified to allow building on Linux and Solaris 10u8, and is the version
of fetch used by [Ravenports](http://www.ravenports.com).

The cmake-based makefiles build and install:

  1. dynamically built fetch program (SSL libraries are statically linked by default)
  1. static libfetch library
  1. static libfetch library compiled with Position Independent Code
  1. dynamic libfetch library
  1. One man page for the fetch executable
  1. Another man page for the fetch library
    
## Name

    fetch -- retrieve a file by Uniform Resource Locator

## Description

    The fetch utility provides a command-line interface to the fetch(3)
    library.  Its purpose is to retrieve the file(s) pointed to by the URL(s)
    on the command line.

### Manual Page

[FreeBSD version](https://www.freebsd.org/cgi/man.cgi?fetch(1))

### Functional Upstream Difference

Until recently, fetch by default looked for ca_root_nss certificate files here:

  - /usr/local/etc/ssl/cert.pem
  - /etc/ssl/cert.pem

FreeBSD no longer does, but this code continues.
Note that the Ravenports specification file replaces "/usr/local" with the
defined installation prefix, so other users should make inline replacements
if /usr/local is unsuitable as well.
