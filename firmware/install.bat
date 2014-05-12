cd default
make clean
make
avrdude -p atmega8 -c stk500v2 -P avrdoper  -U flash:w:"Nixie.hex":i
cd ..