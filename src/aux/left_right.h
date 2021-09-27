#ifndef LEFTRIGHT_H
#define LEFTRIGHT_H

#define RIGHT(iProc,nProc) ((iProc + nProc + 1) % nProc)
#define LEFT(iProc,nProc)  ((iProc + nProc - 1) % nProc)

#endif
