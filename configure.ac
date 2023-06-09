# Initialize
AC_PREREQ([2.69])
AC_INIT([follow], [0.1], , , [https://github.com/aemsenhuber/follow])
AC_CONFIG_AUX_DIR([aux])
AM_INIT_AUTOMAKE([foreign -Wall -Werror])

# Enable non-standard functions and other goodies
AC_USE_SYSTEM_EXTENSIONS

# Standard checks
# ---------------

# Programs
AC_PROG_CC

# Language features
AC_C_RESTRICT

# Headers
AC_CHECK_HEADERS([unistd.h])
AC_CHECK_HEADERS([fcntl.h])
AC_CHECK_HEADERS([sys/param.h])

# Types
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T

# Functions
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_FUNC_FORK
AC_CHECK_FUNCS([memset])
AC_CHECK_FUNCS([setlocale])
AC_CHECK_FUNCS([strerror])
AC_CHECK_FUNCS([strndup])
AC_FUNC_STRTOD
AC_CHECK_FUNCS([gethostname])
AC_CHECK_FUNCS([localtime_r])
AC_CHECK_FUNCS([clock_gettime])
AC_CHECK_FUNCS([dup2])

# NCURSES
PKG_CHECK_MODULES(NCURSES, ncursesw >= 6.0)

# Output
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
