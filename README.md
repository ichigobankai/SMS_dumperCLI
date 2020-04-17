# SMS_dumperCLI
SMS Dumper CLI version

Usage:
smsdumper [read|write] [mapper] [file if write|size if read (kB)] [old pcb]
  
mapper can be:
  
  * 1: Sega Master System compatible
  * 2: Codemasters
  * 3: 4 Pak All Action
  * 4: Korean 1MB
  * 5: Korean X-in-1
  * 6: Korean MSX 8Kb
  * 7: Korean Jagun
  * 8: SG1000 Taiwan MSX
  * 9: Sega Master System EXT

-----

Compile on Mac :

gcc -L /usr/local/Cellar/libusb/1.0.23/lib/ -I /usr/local/include/libusb-1.0 -o smsdumper smsdumper.c -lusb-1.0

(adjust path accordingly to your configuration)

* use LIBSUB library
