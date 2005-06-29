# $Id$


if UNIT_TESTS_YES

bin_PROGRAMS = tmwserv_test

tmwserv_test_SOURCES = testsmain.cpp \
					   testaccount.h \
					   testaccount.cpp \
					   testcipher.h \
					   testcipher.cpp \
					   testdataprovider.h \
					   testdataprovider.cpp \
					   testrecordset.h \
					   testrecordset.cpp \
					   teststorage.h \
					   teststorage.cpp

tmwserv_test_CXXFLAGS = -Wall $(CPPUNIT_CXXFLAGS)

tmwserv_test_LDFLAGS = $(CPPUNIT_LIBS)

tmwserv_test_LDADD = ../tmwserv-account.o \
					 ../tmwserv-cipher.o \
					 ../tmwserv-dalstorage.o \
					 ../tmwserv-dataprovider.o \
					 ../tmwserv-dataproviderfactory.o \
					 ../tmwserv-logger.o \
					 ../tmwserv-object.o \
					 ../tmwserv-being.o \
					 ../tmwserv-recordset.o \
					 ../tmwserv-storage.o

if BUILD_MYSQL
tmwserv_test_LDADD += ../tmwserv-mysqldataprovider.o
endif

if BUILD_POSTGRESQL
tmwserv_test_LDADD += ../tmwserv-pqdataprovider.o
endif

if BUILD_SQLITE
tmwserv_test_LDADD += ../tmwserv-sqlitedataprovider.o
endif

endif
