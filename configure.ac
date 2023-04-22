# Initialize
AC_PREREQ([2.69])
AC_INIT([follow], [0.1])
AC_CONFIG_AUX_DIR([aux])
AM_INIT_AUTOMAKE([foreign -Wall -Werror])

# Enable non-standard functions and other goodies
AC_USE_SYSTEM_EXTENSIONS

# Standard checks
# ---------------

# Programs
AC_PROG_CC

# Headers
AC_CHECK_HEADERS([stdlib.h])
AC_CHECK_HEADERS([string.h])
AC_CHECK_HEADERS([unistd.h])
AC_CHECK_HEADERS([fcntl.h])

# Types
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T

# Functions
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_FUNC_FORK
AC_CHECK_FUNCS([memset])
AC_CHECK_FUNCS([strndup])
AC_CHECK_FUNCS([strtol])
AC_CHECK_FUNCS([gethostname])
AC_CHECK_FUNCS([localtime_r])
AC_CHECK_FUNCS([dup2])

# NCURSES
PKG_CHECK_MODULES(NC, ncurses >= 6.0)

# Output
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
