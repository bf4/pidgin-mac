#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(pidgin-twitter, 0.8.4, yaz@honeyplanet.jp)
AC_CONFIG_SRCDIR([main.c])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
PKG_CHECK_MODULES(GTK, [gtk+-2.0 >= 2.12.0], , [
        AC_MSG_RESULT(no)
        AC_MSG_ERROR([

You must have gtk+-2.0 >= 2.12.0 development headers installed to build.
])])

PKG_CHECK_MODULES(GLIB, [glib-2.0 >= 2.16.0], , [
        AC_MSG_RESULT(no)
        AC_MSG_ERROR([

You must have glib >= 2.16.0 development headers installed to build.
])])

GLIB_CFLAGS=`pkg-config --cflags glib-2.0 2> /dev/null`
GLIB_LIBS=`pkg-config --libs glib-2.0 2> /dev/null`
GLIB_LIB_DIR=`pkg-config --variable=lib_dir glib-2.0 2> /dev/null`
AC_SUBST(GLIB_CFLAGS)
AC_SUBST(GLIB_LIBS)
AC_SUBST(GLIB_LIB_DIR)

PKG_CHECK_MODULES(PIDGIN, [pidgin >= 2.4.0], , [
        AC_MSG_RESULT(no)
        AC_MSG_ERROR([

You must have pidgin >= 2.4.0 development headers installed to build.
])])
PIDGIN_CFLAGS=`pkg-config --cflags pidgin 2> /dev/null`
PIDGIN_LIBS=`pkg-config --libs pidgin 2> /dev/null`
if test x"$prefix" = x"NONE" ; then
	PIDGIN_PREFIX=`pkg-config --variable=prefix pidgin 2> /dev/null`
else
	PIDGIN_PREFIX=$prefix
fi
AC_SUBST(PIDGIN_CFLAGS)
AC_SUBST(PIDGIN_LIBS)
AC_SUBST(PIDGIN_PREFIX)

PKG_CHECK_MODULES(XML, [libxml-2.0 >= 2.6.27], , [
        AC_MSG_RESULT(no)
        AC_MSG_ERROR([

You must have libxml2 >= 2.6.27 installed to build.
])])

XML_CFLAGS=`pkg-config --cflags libxml-2.0 2> /dev/null`
XML_LIBS=`pkg-config --libs libxml-2.0 2> /dev/null`
XML_LIB_DIR=`pkg-config --variable=lib_dir libxml-2.0 2> /dev/null`
AC_SUBST(XML_CFLAGS)
AC_SUBST(XML_LIBS)
AC_SUBST(XML_LIB_DIR)

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([stdlib.h string.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST

# Checks for library functions.
AC_CHECK_FUNCS([strstr])

AC_OUTPUT(Makefile)
