//--------------------------------------------------------------------
// The Main Program for the AdvMail/Remote client.
//
// Written by Graham Wheeler, June 1995.
// (c) 1995 Open Mind Solutions.
// All Rights Reserved.
//--------------------------------------------------------------------
     
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <assert.h>

#include "ansicurs.h"
#include "omutil.h"
#include "mail.h"
#include "screens1.h"
#include "config.h"
#include "agent.h"

PersistentConfig *config = 0;
FILE *debugfp = 0;
char *ApplicationName = "Advancemail/Remote";
char *ApplicationVersion = "1.11";

/**************************************************************/

void ShutDown()
{
    QuitCurses();
    delete config;
    if (debugfp) fclose(debugfp);
}

static void FatalError(char *err)
{
    ShutDown();
    fprintf(stderr, "Fatal error: %s\n", err);
    exit(-1);
}

int main(int argc, char *argv[])
{
    FatalErrorHandler = FatalError;
    if (argc ==2)
    {
	if (strcmp(argv[1], "-D")==0) // debug
	    debugfp = fopen("agent.dbg", "w");
	else if (strcmp(argv[1], "-r")==0) // record
	    StartRecord("agent.rec");
	else if (strcmp(argv[1], "-p")==0) // replay
	    StartReplay("agent.rec");
	else if (strcmp(argv[1], "-b")==0) // batch
	{
	    StartReplay("agent.rec");
	    StartCapture("agent.out");
	}
    }
    config = new PersistentConfig("omsrc", "server:user:password:*print:"
					   "poll:filetypes:links:*helpfile:"
					   "*maxaddresses");
    assert(config);
    StartCurses();
    SetClip(-3, 0); // write-protect bottom lines
    LoginScreen *ls = new LoginScreen;
    assert(ls);
    int rtn = ls->Run();
    ls->DrawTitle("Terminating AdvanceMail/Remote");
    delete ls;
    ShutDown();
    return rtn;
}


