copy ..\patch\gamepatch.s temp1.s
asm658 /zd /l temp1.s,gamepatch.obj,,gamepatch.lst > errs.
copy ..\patch\xbcomm.s temp2.s
asm658 /zd /l temp2.s,xbcomm.obj,,xbcomm.lst >> errs.
copy xxxpatch.s temp3.s
asm658 /m /zd /l temp3.s,xxxpatch.obj,,xxxpatch.lst >> errs.
psylink /i @xxxpatch.lnk,temp.cpe,xxxpatch.sym >> errs.
psylink /m /p /o$e00210 @xxxpatch.lnk,xxxpatch.bin,xxxpatch.sym,xxxpatch.map >> errs.
copy temp.cpe xxxpatch.cpe
del temp*.*
del *.obj
