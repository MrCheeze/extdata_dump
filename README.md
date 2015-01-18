3ds extdata dump & restore tool
============

Allows you to decrypt, edit, and then recrypt 3ds extra data. For use with Ninjhax.

Precompiled download: https://dl.dropboxusercontent.com/u/183608682/extdata_dump.zip

----

When run, you'll be presented with three options:

* Dump all extdata to sd card
* Dump extdata specified in config
* Restore extdata specified in config

I recommend just dumping all extdata to begin with, and browsing the dumps to see what you can find. There's a fair bit of documentation that might help you navigate the dumps [over on the 3dbrew wiki](http://3dbrew.org/wiki/Extdata#SD_Extdata).

If you want to restore an edited file (or just want to dump a single file instead of all of them), you'll have to edit config.txt. I've included a few examples in there that should make the format clear.

----

Finally, a technical note: the 3ds actually has three different types of extra data. Shared extdata is stored on the 3DS itself, and it usually relevant to multiple games (e.g. the Play Coin counter). "User" and "boss" extdata are both stored on the SD card, encrypted, and are associated with a specific game. Generally, Spotpass data goes into boss and everything else goes into user. This tool can access user and shared extdata, but boss extdata is likely impossible.
