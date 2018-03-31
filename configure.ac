AC_PREREQ([2.69])
AC_INIT([Arctracker], [0.2.0], [richardwild76@gmail.com])
AM_INIT_AUTOMAKE([foreign])
AC_CONFIG_SRCDIR([play_mod.c])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
AC_CHECK_LIB(asound, snd_pcm_writei)
AC_CHECK_LIB(artsc, arts_write)

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h string.h sys/ioctl.h sys/soundcard.h sys/unistd.h sys/fcntl.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_BIGENDIAN

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
