dnl $Rev: 1050 $
dnl $Id: boost.m4 1050 2005-10-12 16:23:18Z sjoyeux $

AC_DEFUN([CLBS_CHECK_BOOST],
 [
  AC_SUBST(BOOST_CPPFLAGS)
  AC_SUBST(BOOST_LDFLAGS)

  AC_DEFINE([HAVE_BOOST], [], [If the Boost libraries are available])
  
  BOOST_CPPFLAGS=""
  BOOST_LDFLAGS=""

  BOOST_ROOT=""

dnl Extract the path name from a --with-boost=PATH argument
  AC_ARG_WITH(boost,
	AC_HELP_STRING([--with-boost=PATH],
	               [absolute path where the Boost C++ libraries reside]),
	[
	 if test "x$withval" = "x" ; then
	   BOOST_ROOT=""
	 else
	   BOOST_ROOT="$withval"
	   BOOST_TMP_CPPFLAGS="-I$BOOST_ROOT/include"
	   BOOST_TMP_LDFLAGS="-L$BOOST_ROOT/lib"
    	 fi
	])

dnl Checking for Boost headers
   CPPFLAGS_OLD=$CPPFLAGS
   CPPFLAGS="$CPPFLAGS $BOOST_TMP_CPPFLAGS"
   AC_LANG_SAVE
   AC_LANG_CPLUSPLUS
   AC_CHECK_HEADER([boost/version.hpp], [have_boost="yes"])
   AC_LANG_RESTORE
   CPPFLAGS=$CPPFLAGS_OLD 
   if test "x$have_boost" = "xyes" ; then 
     ifelse([$1], , :, [$1])
     BOOST_CPPFLAGS="$BOOST_TMP_CPPFLAGS"
     BOOST_LDFLAGS=$BOOST_TMP_LDFLAGS
     HAVE_BOOST=1
     AC_DEFINE(HAVE_BOOST)
   else
     ifelse([$2], , :, [$2])
   fi
 ]
)

AC_DEFUN([CLBS_BOOST_SUBLIB_DEFINE],
[
    AC_DEFINE(HAVE_BOOST_$2, [], [If the boost/$1 library is available])
    HAVE_BOOST_$2=1
    BOOST_$2_CPPFLAGS="$BOOST_CPPFLAGS"
    BOOST_$2_LDFLAGS="$BOOST_LDFLAGS ifelse([$3], [], [], -lboost_$3)"

    AC_SUBST(BOOST_$2_CPPFLAGS)
    AC_SUBST(BOOST_$2_LDFLAGS)
    AC_SUBST(HAVE_BOOST_$2)
])

dnl CLBS_BOOST_SUBLIB(libname, library, header, class, [if found], [if not found])
AC_DEFUN([CLBS_BOOST_SUBLIB],
[
  AC_REQUIRE([CLBS_CHECK_BOOST])
  AC_LANG_PUSH(C++)

  clbs_sv_$1_CPPFLAGS="$CPPFLAGS"
  clbs_sv_$1_LDFLAGS="$LDFLAGS"

  CPPFLAGS="$BOOST_CPPFLAGS $CPPFLAGS"
  AC_CHECK_HEADER([$3], [has_working_$1=yes], [has_working_$1=no])
    
  if test "$has_working_$1" = "yes"; then
    AC_MSG_CHECKING([for the Boost/$1 library])
    LDFLAGS="$BOOST_LDFLAGS ifelse([$2], [], [], -lboost_$2) $PTHREAD_LIBS $LDFLAGS"
    AC_LINK_IFELSE(
    [
      #include <$3>

      int main()
      {
        $4 test;
      }
    ], 
    [AC_MSG_RESULT([yes])], 
    [has_working_$1=no
     AC_MSG_RESULT([no])])
  fi

  CPPFLAGS="$clbs_sv_$1_CPPFLAGS"
  LDFLAGS="$clbs_sv_$1_LDFLAGS"
 
  if test "$has_working_$1" != "no"; then
    ifelse([$5], , , $5)
    CLBS_BOOST_SUBLIB_DEFINE($1, translit($1, 'a-z', 'A-Z'), [$2])
  ifelse([$6], [], [], [
  else 
    $6
  ])
  fi

  AC_LANG_POP
])

dnl CLBS_BOOST_THREADS([if-found], [if-not-found])
AC_DEFUN([CLBS_BOOST_THREAD],
[
  AC_REQUIRE([CLBS_CHECK_BOOST])
  AC_REQUIRE([APR_PTHREADS_CHECK])

  has_working_bthreads=yes
  if test "x$pthreads_working" != "xyes"; then
    AC_MSG_FAILURE([POSIX threads not available])
    has_working_bthreads=no
  fi

  if test "$has_working_bthreads" = "yes"; then
    clbs_sv_CPPFLAGS="$CPPFLAGS"
    clbs_sv_LDFLAGS="$LDFLAGS"
    CPPFLAGS="$PTHREAD_CFLAGS $CPPFLAGS"
    LDFLAGS="$PTHREAD_LIBS $LDFLAGS"
    CLBS_BOOST_SUBLIB(thread, [thread], [boost/thread/mutex.hpp], [boost::mutex], [], [has_working_bthreads=no])
    CPPFLAGS=$clbs_sv_CPPFLAGS
    LDFLAGS=$clbs_sv_LDFLAGS
  fi

  if test "$has_working_bthreads" != "no"; then
    ifelse([$1], , , $1)
    BOOST_THREAD_CXXFLAGS="$PTHREAD_CXXFLAGS"
    BOOST_THREAD_LDFLAGS="$BOOST_THREAD_LDFLAGS $PTHREAD_LIBS"
    AC_SUBST(BOOST_THREAD_CXXFLAGS)
    AC_SUBST(BOOST_THREAD_LDFLAGS)
  ifelse([$2], [], [], [
  else
    $2
  ])
  fi
])

dnl CLBS_BOOST_REGEX(if-found, if-not-found)
AC_DEFUN([CLBS_BOOST_REGEX], 
[ CLBS_BOOST_SUBLIB(regex, [regex], [boost/regex.hpp], [boost::regex], [$1], [$2]) ])
AC_DEFUN([CLBS_BOOST_FILESYSTEM], 
[ CLBS_BOOST_SUBLIB(filesystem, [filesystem], [boost/filesystem/path.hpp], [boost::filesystem::path], [$1], [$2]) ])

AC_DEFUN([CLBS_BOOST_TEST], [
    AC_CHECK_HEADER([boost/test/unit_test.hpp], [has_working_test=yes], [has_working_test=no])
    if test "$has_working_test" = "yes"; then
        AC_MSG_CHECKING([for a working boost/unit_test framework])
        clbs_sv_CPPFLAGS=$CPPFLAGS
        clbs_sv_LDFLAGS=$LDFLAGS
        
        CPPFLAGS="$BOOST_TEST_CPPFLAGS $CPPFLAGS"
        LDFLAGS="$BOOST_TEST_LDFLAGS -lboost_unit_test_framework $LDFLAGS"

        AC_LANG_PUSH(C++)
        AC_LINK_IFELSE([
          #include <boost/test/unit_test.hpp>

          boost::unit_test::test_suite*
          init_unit_test_suite(int, char**) { return 0; }

          int main()
          {
            boost::unit_test::test_suite* ts_check = BOOST_TEST_SUITE( "" );
          }
        ], [has_working_test=yes], [has_working_test=no])

        CPPFLAGS=$clbs_sv_CPPFLAGS
        LDFLAGS=$clbs_sv_LDFLAGS
        AC_LANG_POP

        if test "$has_working_test" = "yes"; then
           AC_MSG_RESULT([yes])
           CLBS_BOOST_SUBLIB_DEFINE(test, TEST)
           ifelse([$1], [], [], [$1])
        else
           AC_MSG_RESULT([no])
           ifelse([$2], [], [], [$2])
        fi
    fi
])
