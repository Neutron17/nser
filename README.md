# C boilerplate project

## Contains:
 - ```arg.c,h```: Minimal getopt template
 - ```assrt.h```: Assert
 - ```exitCodes.h```: exit codes
 - ```global.h```: global definitions, included in every compile unit
 - ```log.c```,```log.h```: asynchronous logging
 - ```stdext.c```,```stdext.h```: extensions to the standard (strnlen, strrev, itoa, async io)
 - ```lt.c```,```lt.h```: lifetime functions, ```init()```: called first, ```cleanUp(int ret)```: called last
 - ```main.c```: main()

