/*  MAKE BINARY ROM IMAGE  */

OPTIONS RESULTS
ADDRESS COMMAND

'VERSION >T:1 RLOBJ:RL.'
dummy1 = open('1','T:1','read')
RLVersion = RIGHT(readln('1'),4)
dummy1 = close('1')


MakeBinCmd = "xr -v -eRLOBJ:RL. -s$80000000 -z$200000 -k -y -xRLOBJ:RL" || RLVersion || ".sfc" || '0A'X
'ECHO ' || MakeBinCmd
MakeBinCmd
MakeBinCmd = "smake xb"
'ECHO ' || MakeBinCmd
MakeBinCmd
MakeBinCmd = "COPY RLOBJ:RL5934.sfc hd:compbi"
'ECHO ' || MakeBinCmd
MakeBinCmd
MakeBinCmd = "COPY RLOBJ:XB.bin hd:compbi"
'ECHO ' || MakeBinCmd
MakeBinCmd
MakeBinCmd
MakeBinCmd = "COPY HD:source/RL.SEC hd:compbi"
'ECHO ' || MakeBinCmd
MakeBinCmd
MakeBinCmd
MakeBinCmd = "COPY HD:source/RL.BNK hd:compbi"
'ECHO ' || MakeBinCmd
MakeBinCmd
MakeBinCmd = "COPY HD:source/XB.SEC hd:compbi"
'ECHO ' || MakeBinCmd
MakeBinCmd
MakeBinCmd = "COPY HD:source/XB.BNK hd:compbi"
'ECHO ' || MakeBinCmd
MakeBinCmd
EXIT
