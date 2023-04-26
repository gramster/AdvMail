// config.h - the persistent config class
// Written by Graham Wheeler, 1994.
// (c) 1994 by Graham Wheeler, All Rights Reserved.
//
// A simple class for maintaining a persistent set of strings (names and
// values). This was used to restore various config options and variables
// in Gram's Commander 4.0. It has since been replaced by an extension to
// the GCScript Tcl language.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <sys/stat.h>

#ifndef __MSDOS__
#include <unistd.h>
#endif

#include "debug.h"
#include "config.h"

#define SYSTEMWIDE	1

//-----------------------------------------------------------------
// The search here is linear; if the number of config items becomes large,
// it should be changed to binary and the labels should be sorted.

PersistentConfig *config = 0;

void PersistentConfig::Set(char *label, char *value)
{
    for (int e = 0; e < eltcnt; e++)
	if (strcmp(labels[e], label)==0)
	{
	    if (values[e] && (flags[e] & SYSTEMWIDE)!=0)
		break;
	    delete [] values[e];
	    values[e] = new char[strlen(value)+1];
	    assert(values[e]);
	    strcpy(values[e], value);
	    break;
	}
}

// merge should be zero to trash existing config, one to load
// additions only, or two to load everything (overriding existing
// values).

void PersistentConfig::Load(char *fname, int merge)
{
    if (!merge)
    {
        for (int e = 0; e < eltcnt; e++)
	{
	    delete [] values[e];
	    values[e] = 0;
	}
    }
    FILE *fp = fopen(fname, "r");
    if (fp) 
    {
	while (!feof(fp))
	{
	    char buf[128], *tmp = buf;
	    if (fgets(buf, 128, fp) == 0) break;
	    if (buf[0] == '#') continue;
	    int l = strlen(buf);
	    while (l-->0 && isspace(buf[l])) buf[l] = '\0';
	    while (isalnum(*tmp) || (*tmp == '_')) ++tmp;
	    while (isspace(*tmp)) *tmp++ = '\0';
	    if (merge==2 || Get(buf)[0] == '\0')
		Set(buf, tmp);
	}
	fclose(fp);
    }
}

PersistentConfig::PersistentConfig(char *cname_in, char *items_in)
	: cname(cname_in)
{
    ConstructTrace("PersistentConfig");
    assert(items_in && items_in[0]);
    char *items = new char[strlen(items_in)+1];
    assert(items);
    strcpy(items, items_in);
    eltcnt = 1;
    for (int i = 0; items[i]; i++)
	if (items[i] == ':')
	    eltcnt++;
    labels = new char*[eltcnt];
    values = new char*[eltcnt];
    flags = new int[eltcnt];
    assert(labels && values && flags);
    if (items[0]=='*')
    {
	flags[0] = SYSTEMWIDE;
        labels[0] = items+1;
    }
    else
    {
	flags[0] = 0;
	labels[0] = items;
    }
    char *tmp = items;
    i = 1;
    while (i < eltcnt)
    {
        tmp = strchr(tmp, ':');
 	assert(tmp);
	*tmp++ = 0;
	if (tmp[0] == '*')
	{
	    flags[i] = SYSTEMWIDE;
	    labels[i++] = tmp+1;
	}
	else
	{
	    flags[i] = 0;
	    labels[i++] = tmp;
	}
    }
    for (int e = 0; e < eltcnt; e++)
	values[e] = 0;
#if __MSDOS__
    Load(cname, 0);
#else
    char fname[256];
    fname[0] = 0;
    char *omdir = getenv("OMRCPATH"); // somewhat presumptious!
    if (omdir && access(omdir,0) == 0)
	strcpy(fname, omdir);
    else
    {
    	strcpy(fname, "/etc/");
    	strcat(fname, cname);
    	if (access(fname, 0) != 0)
    	{
            strcpy(fname, "/usr/lib/");
            strcat(fname, cname);
    	    if (access(fname, 0) != 0)
	    {
            	strcpy(fname, "/usr/local/lib/");
            	strcat(fname, cname);
    	    	if (access(fname, 0) != 0)
		    fname[0] = 0;
	    }
        }
    }
    if (fname[0]) Load(fname, 0);
    char *home = getenv("HOME");
    if (home)
    {
	char fname2[256];
    	strcpy(fname2, getenv("HOME"));
    	strcat(fname2, "/.");
    	strcat(fname2, cname);
	if (access(fname2,0) == 0)
	{
    	    if (fname[0]==0)
		Load(fname2, 0);
	    else // compare modification times to get merge type
	    {
		int merge = 2;
		struct stat s1, s2;
		stat(fname, &s1);
		stat(fname2, &s2);
		if (s1.st_mtime >= s2.st_mtime)
		    merge = 1; // system-wide is newer than personal
		Load(fname2, merge);
	    }
	}
    }
#endif
}

void PersistentConfig::Save()
{
    char fname[256];
    strcpy(fname, getenv("HOME"));
    strcat(fname, "/.");
    strcat(fname, cname);
    FILE *fp = fopen(fname, "w");
    if (fp)
    {
	for (int e = 0; e < eltcnt; e++)
            if (values[e] && (flags[e]&SYSTEMWIDE)==0)
		fprintf(fp, "%-10s %-s\n", labels[e], values[e]);
    	fclose(fp);
    }
}

char *PersistentConfig::Get(char *label)
{
    for (int e = 0; e < eltcnt; e++)
	if (labels[e] && strcmp(labels[e], label)==0)
	    return values[e] ? values[e] : "";
    return "";
}

int PersistentConfig::IsSet(char *label)
{
    char *v = Get(label);
    if (v[0] == 'y' || v[0] == 'Y' || (isdigit(v[0]) && v[0] != '0'))
	return 1;
    return 0;
}

PersistentConfig::~PersistentConfig()
{
    DestructTrace("PersistentConfig");
    Save();
    for (int e = 0; e < eltcnt; e++)
	delete [] values[e];
    delete [] values;
    delete [] labels;
    delete [] items;
}

