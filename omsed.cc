#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#if !__MSDOS__
#include <unistd.h>
#endif

#include "debug.h"
#include "ansicurs.h"
#include "ed.h"
#include "screen.h"

char *ApplicationName = "OMS/Edit";
char *ApplicationVersion = "v0.1";
FILE *debugfp = 0;

int main(int argc, char *argv[] )
{
    StartCurses();
    MultiEditScreen *es = new MultiEditScreen(0, argc-1, (argc>1)? &argv[1] : 0);
    es->Run();
    delete es;
    StopCurses();
    return 0;
}


