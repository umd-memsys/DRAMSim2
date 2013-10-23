AC_INIT([DRAMSim2], [2.4.0], [dramninjas@gmail.com])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CPP

# Checks for header files.
AC_CHECK_HEADERS([stdint.h stdlib.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T
AC_TYPE_UINT64_T

# Checks for library functions.
AC_FUNC_MALLOC

# The old DRAMSim2 used to generate the files to the build directory -- keep that as the default behavior
AC_PREFIX_DEFAULT($PWD)

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
