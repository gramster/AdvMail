//----------------------------------------------------------
// Simple Hypertext compiler for Gram's Commander
// (c) 1994 by Graham Wheeler, All Rights Reserved

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#define MAXLINELEN 256

void Fatal(int lnum, char *msg)
{
    fprintf(stderr, "Fatal error");
    if (lnum >0) fprintf(stderr, " on line %d", lnum);
    fprintf(stderr, ": %s\n", msg);
    exit(-1);
}

void HyperCompile(char *fname)
{
    char buf[MAXLINELEN];
    FILE *ifp, *ofp;
    ifp = fopen(fname, "r");
    if (ifp == NULL)
    {
	fprintf(stderr, "Cannot open %s for input!\n", fname);
	exit(-1);
    }
    char iname[256];
    strcpy(iname, fname);
    for (char *sptr = iname+strlen(iname); sptr>=iname && *sptr != '.'; sptr--);
    if (sptr >= iname) *sptr = '\0';
    strcat(iname, ".idx");
    ofp = fopen(iname, "w");
    if (ofp == NULL)
    {
	fprintf(stderr, "Cannot open %s for output!\n", iname);
	fclose(ifp);
	exit(-1);
    }
    /* Find out how much space is needed */
    int namespacesize = 0, numentries = 0, linenum = 0;
    while (!feof(ifp))
    {
	if (fgets(buf, MAXLINELEN, ifp)==0) break;
	linenum++;
	if (strncmp(buf, "@entry", 6) == 0)
	{
	    if (buf[6] != '{') Fatal(linenum, "{ expected in @entry");
	    char *ep = strchr(buf, ',');
	    if (ep == 0) Fatal(linenum, ", expected in @entry");
	    numentries++;
	    namespacesize += ep - (buf+6);
	}
    }
    fseek(ifp, 0, 0);
    printf("Allocating %d bytes for names, %d bytes for indices\n", 
	namespacesize, 4 * numentries);
    char *namespace = new char [namespacesize];
    long *indexes = new long[numentries];
    if (namespace == 0 || indexes == 0)
	Fatal(-1, "Cannot allocate memory");
    linenum = 0;
    long pos;
    int icnt = 0, ncnt = 0;
    while (!feof(ifp))
    {
	pos = ftell(ifp);
	linenum++;
	if (fgets(buf, MAXLINELEN, ifp) == 0) break;
	if (strncmp(buf, "@entry{", 7) == 0)
	{
	    int pf = 7, pb; // front and back pointers
	    while (isspace(buf[pf])) pf++;
	    pb = pf;
	    while (isalnum(buf[pb]) || buf[pb] == '_') pb++;
	    if (pf == pb || buf[pb] != ',' || buf[pb] == '\0')
		Fatal(linenum, "Bad @entry");
	    buf[pb] = '\0';
	    indexes[icnt++] = pos;
	    assert(icnt <= numentries);
	    assert((ncnt + pb - pf + 1) <= namespacesize);
	    strcpy(namespace + ncnt, buf + pf);
	    ncnt += pb - pf + 1;
	}
    }
    fprintf(ofp, "%05d Indices\n%05d Namespace\n", icnt, ncnt);
    int p, q;
    for (p = q = 0; p < icnt; p++, q += strlen(namespace + q) + 1)
	fprintf(ofp, "%-16s %05ld\n", namespace + q, indexes[p]);
    fclose(ofp);
    fclose(ifp);
    delete [] indexes;
    delete [] namespace;
    // very kludgy way of sorting the result; seeing as the hc shouldn't
	// be run very often it doesn't really matter...
    //sprintf(buf, "sort < %s > OMS_HC.TMP", iname);
    //system(buf);
    //sprintf(buf, "mv OMS_HC.TMP %s", iname);
    //system(buf);
}

int main(int argc, char *argv[])
{
    if (argc != 2)
	Fatal(-1, "Useage: omshc <Hypertext Input File>\n");
    HyperCompile(argv[1]);
    return 0;
}

