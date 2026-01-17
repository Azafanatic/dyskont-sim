#!/usr/bin/bash

if [ -d "build" ]; then
  rm -rf build
fi

if [ -d "locale" ]; then
  rm -rf locale
fi

#xgettext --keyword=_ --language=C --add-comments src/*.c -o messages.pot

#msginit --input=messages.pot --locale=pl --output=pl.po
#msginit --input=messages.pot --locale=en --output=en.po

#msgmerge --update pl.po messages.pot
#msgmerge --update en.po messages.pot

msgfmt --output-file=pl.mo pl.po
msgfmt --output-file=en.mo en.po
mkdir -p locale/pl/LC_MESSAGES
mkdir -p locale/en/LC_MESSAGES
mv pl.mo locale/pl/LC_MESSAGES/messages.mo
mv en.mo locale/en/LC_MESSAGES/messages.mo

mkdir build
cd build
cp -r ../locale ./locale
cmake ..
make

./sim 57600 5760
