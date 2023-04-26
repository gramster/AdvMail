// Scan the debug file for constructtrace and destructtrace messages,
// and report any memory leaks

#include <stdio.h>
#include <string.h>
#include <assert.h>

#define MAXOBJ	1000

struct objrecord
{
    long addr;
    char *name;
};

objrecord records[MAXOBJ];

void Init()
{
    for (int i = 0; i < MAXOBJ; i++)
    {
	records[i].name = 0;
	records[i].addr = 0;
    }
}

int FindEntry(char *name, long addr) // linear search :-(
{
    assert(addr != 0l);
    for (int i = 0; i < MAXOBJ; i++)
	if (records[i].addr == addr)
	    if (strcmp(records[i].name, name) == 0)
	    return i;
    return -1;
}

int FindSpace() // linear search :-(
{
    for (int i = 0; i < MAXOBJ; i++)
	if (records[i].addr == 0l)
	    return i;
    return -1;
}

void DoLine(FILE *fp)
{
    char buf[256], oname[256], op[256];
    long addr;
    if (fgets(buf, 256, fp) == 0) return;
    buf[strlen(buf)-1] = 0; // strip \n
    if (sscanf(buf, "%lX %s %s", &addr, oname, op)==3)
    {
	int idx = FindEntry(oname, addr);
	if (strcmp(op, "Constructed") == 0)
	{
	    if (idx >= 0) 
		fprintf(stderr, "Bad construct of %s at %lX!\n", oname, addr);
	    else
	    {
		idx = FindSpace();
		if (idx < 0)
		    fprintf(stderr, "Out of space!\n");
		else
		{
		    records[idx].addr = addr;
		    delete [] records[idx].name;
		    records[idx].name = new char[strlen(oname)+1];
		    strcpy(records[idx].name, oname);
		}
	    }
	}
	else if (strcmp(op, "Destructed") == 0)
	{
	    if (idx < 0) 
		fprintf(stderr, "Bad destroy of %s at %lX!\n", oname, addr);
	    else
		records[idx].addr = 0;
	}
    }
}

void Report()
{
    for (int i = 0; i < MAXOBJ; i++)
    {
	if (records[i].addr!=0)
	    fprintf(stdout, "Leak of %s at %lX\n",
		records[i].name, records[i].addr);
	delete [] records[i].name;
    }
}

void main(int argc, char *argv[])
{
    if (argc != 2)
    {
	fprintf(stderr, "Useage: findleak <debug trace file>\n");
	exit(-1);
    }
    FILE *fp = fopen(argv[1], "r");
    if (fp == 0)
    {
	fprintf(stderr, "Failed to open file %s\n", argv[1]);
	exit(-1);
    }
    Init();
    while (!feof(fp))
	DoLine(fp);
    fclose(fp);
    Report();
}

