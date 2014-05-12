#/bin/sh
cd default
make clean && make && avrdude -p m8 -P avrdoper -U flash:w:minixie.hex -c stk500v2
