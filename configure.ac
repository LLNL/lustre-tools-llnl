#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_INIT
X_AC_META
AC_CONFIG_AUX_DIR([config])
AC_CONFIG_SRCDIR([src/lpurge.c])
AC_CONFIG_HEADER([config.h])

AC_CANONICAL_HOST
AC_CANONICAL_SYSTEM

# Check for system capabilities
AC_SYS_LARGEFILE

# Automake support
AM_INIT_AUTOMAKE([$PACKAGE], [$VERSION], [check-news dist-bzip2 gnu no-define])
AM_MAINTAINER_MODE

# Checks for programs.
AC_PROG_CC

# Check for headers
AC_CHECK_HEADERS([lustre/lustre_user.h])

AC_CONFIG_FILES([Makefile
                 doc/Makefile
                 doc/man/Makefile
                 src/Makefile
                 scripts/Makefile])

AC_OUTPUT
