AC_INIT([DRAMSim2], [2.4.0], [dramninjas@gmail.com])
AM_INIT_AUTOMAKE([1.9.6 foreign])

LT_INIT([shared])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([config.h])
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

# check for google perftools 
has_gperf=0
saved_LIBS = "$LIBS"
AC_CHECK_HEADERS([google/profiler.h], [], [has_gperf=0])
AC_CHECK_LIB([profiler], [ProfilerStart], [], [has_gperf=0])
AS_IF([test "$has_gperf" = "1"], 
		[AC_MSG_NOTICE([Found gperf!])
		 AC_DEFINE([HAS_GPERF], [1], [1 if found gperf library])
		 ], 
		 [LIBS="$saved_LIBS"])

AC_OUTPUT
