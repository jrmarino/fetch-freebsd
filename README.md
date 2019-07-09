# fetch-freebsd

This is the code for the fetch utility from FreeBSD Release 12.0.
It has been modified to allow building on Linux and Solaris 10u8, and is the version
of fetch used by [Ravenports](https://www.ravenports.com).

The BSD-style makefile build and installs:

  1. statically built fetch program
  1. static libfetch library
  1. dynamic libfetch library
  1. a single manpage
    
## Name

    fetch -- retrieve a file by Uniform Resource Locator

## Description

    The fetch utility provides a command-line interface to the fetch(3)
    library.  Its purpose is to retrieve the file(s) pointed to by the URL(s)
    on the command line.
