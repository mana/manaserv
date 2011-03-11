#!/bin/sh

# This script creates (and updates) a Qt Creator project file, for developing
# manaserv using the Qt Creator IDE.

echo "[General]" > manaserv.creator
echo "#define SQLITE_SUPPORT 1" > manaserv.config
echo "src
/usr/include/libxml2" > manaserv.includes

git ls-files \*.cpp \
             \*.h \
             \*.txt \
             \*.xml \
             \*.pro \
             \*.lua \
    > manaserv.files
