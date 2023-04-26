// A very basic hypertext browser, implemented as a C++ class.
//
// Written by Graham Wheeler, 1994.
// (c) 1994, All Rights Reserved.
//
// This code is part of Gram's Commander v4.0. It is essentially a
// port of the gc v3.3 help system to C++. It also makes use of the
// new screen classes and function key classes, rather than drawing
// the Index/Next/Prev etc buttons on the screen directly.


#include <stdio.h>
#include <stdlib.h>
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
#include "hypertxt.h"

#define MAXLNLEN	256

const int HyperText::LinesPerScreen = 17; // maximum lines of info per screen
const int HyperText::RefsPerScreen  = 25; // maximum references per screen
const int HyperText::HistoryLength  = 32; // max links that can be undone
const int HyperText::MaxRefTextLen  = 82; // Space for a reference's text

#define SkipSpace(p) while (buff[p] && (buff[p] == ' ' || buff[p]=='\t')) p++
#define SkipPast(p,c) while (buff[p] && buff[p++] != c)
#define SkipTo(p,c) while (buff[p] && buff[p] != c) p++
#define SkipIdent(p) while (buff[p] && (isalnum(buff[p]) || buff[p]=='_')) p++

extern void fill(char *buf, int num, int ch);
extern char *ApplicationName;

#ifdef STANDALONE
void HyperCompile(char *fname);
#endif

/* Parse and print a topic entry heading */

char *HyperText::PrintTopic(int row, char *buf, int centerText)
{
    int start = 0, end = strlen(buf);
    while (buf[start] && buf[start++] != ','); // skip past the comma
    while (--end, end > start && buf[end] != '}');
    if (buf[end] == '}') buf[end] = '\0';
    //PutString(row, centerText ? ((80-(end-start))/2) : 8, &buf[start]);
    if (centerText)
    {
	char buf2[82];
	ReverseOn();
	sprintf(buf2, "%s Help", ApplicationName);
	fill(buf2,80,' ');
	PutString(0,0,buf2);
        PutString(0, ((80-(end-start))/2), &buf[start]);
	ReverseOff();
    }
    else PutString(row, 8, &buf[start]);
    return buf + start;
}

void HyperText::NextIndexPage()
{
    idxStart += LinesPerScreen;
    if (idxStart >= idxTotal)
    {
	idxStart -= LinesPerScreen;
	if (idxStart < 0)
	    idxStart = 0;
    }
    idxNow = idxStart;
    pagenum = 0;
}

void HyperText::PrevIndexPage()
{
    idxStart -= LinesPerScreen;
    if (idxStart < 0)
	idxStart = 0;
    idxNow = idxStart;
    pagenum = 0;
}

void HyperText::NextEntryPage()
{
    idxNow++;
    if (idxNow >= idxTotal)
	idxNow = idxTotal - 1;
    pagenum = 0;
}

void HyperText::PrevEntryPage()
{
    if (idxNow > 0)
	idxNow--;
    pagenum = 0;
}

void HyperText::ClearHistory()
{
    for (int p = 0; p < HistoryLength; p++)
	refHist[p] = 0;
}

void HyperText::LastRefPage() // retreat in history
{
    inIndex = 0;
    histNow -= 2;
    if (histNow < 0)
        histNow=0;
    idxNow = refHist[histNow];
    refCnt = refNow = pagenum = 0;
}

void HyperText::NextPage()
{
    if (inIndex) NextIndexPage();
    else NextEntryPage();
    refCnt = refNow = pagenum = 0;
}

void HyperText::PrevPage()
{
    if (inIndex) PrevIndexPage();
    else PrevEntryPage();
    refCnt = refNow = pagenum = 0;
}

void HyperText::EnterIndex()
{
    inIndex = 1;
    idxStart = idxNow - (idxNow % LinesPerScreen);
}

void HyperText::PaintIndex()
{
    char buff[MAXLNLEN];
    refCnt = 0;
    for (int r = 0, ri = idxStart; r<LinesPerScreen && ri<idxTotal; r++, ri++)
    {
	refIdx[refCnt] = ri;
	refRows[refCnt] = r + 3;
	refCols[refCnt] = 8;
	fseek(tfp, indexes[ri], 0);
	if (fgets(buff, MAXLNLEN, tfp) == 0) break;
	strcpy(refText[refCnt], PrintTopic(r + 3, buff, 0));
	refCnt++;
    }
}

void HyperText::AddToHistory(int idx)
{
    if (inIndex) return;
    if (histNow == HistoryLength)
    {
	for (int p = 1; p < HistoryLength; p++)
	    refHist[p - 1] = refHist[p];
	refHist[HistoryLength - 1] = idx;
    }
    else
	refHist[histNow++] = idx;
}

int HyperText::PaintEntry()
{
    char buff[MAXLNLEN];
    fseek(tfp, indexes[idxNow], 0);
    if (fgets(buff, MAXLNLEN, tfp) == 0) return 0;
    PrintTopic(0, buff, 1);
    int r, p, q, c, hasmore, pg = 0;
showpage:
    refCnt = 0;
    hasmore = 1;
    for (r = 3;
	!feof(tfp) && r < (3+LinesPerScreen) && refCnt < RefsPerScreen;
	r++)
    {
retry:
	if (fgets(buff, MAXLNLEN, tfp) == 0)
	{
	    hasmore = 0;
	    break;
	}
	buff[strlen(buff) - 1] = '\0';	/* remove \n */
	if (r==3 && buff[0] == '\0') goto retry; // skip leading blank lines
	if (strncmp(buff, "@entry{", 7) == 0)
	{
	    hasmore = 0;
	    break;	/* next entry */
	}
	else if (strncmp(buff, ".np", 3) == 0)
	    break;
	if (strchr(buff, '@'))
	{
	    /* We have at least one reference. Process the line... */
	    p = q = 0;
	    c = 2;
	    for (;;)
	    {
	      NotRef:
		SkipTo(p, '@');
		if (!buff[p])
		{
		    PutString(r, c, buff + q);
		    c += strlen(buff+q);
		    break;
		}
		if (strncmp(buff + p + 1, "ref{", 4) != 0)
		{
		    p++;
		    goto NotRef;
		}
		buff[p++] = '\0';
		PutString(r, c, buff + q);
		c += strlen(buff + q);
		if (refCnt >= RefsPerScreen)
		    break;
		SkipPast(p, '{');
		SkipSpace(p);
		q = p;
		SkipIdent(p);
		if (!buff[p])
		    break;
		if (buff[p] != ',')
		{
		    buff[p++] = '\0';
		    SkipPast(p, ',');
		}
		else
		    buff[p++] = '\0';
		/* Find the entry... */
		int gotEntry = 0;
		for (int i = 0; i < idxTotal; i++)
		{
		    if (strcmp(namespace + nidx[i], buff + q) == 0)
		    {
			refIdx[refCnt] = i;
			refRows[refCnt] = r;
			refCols[refCnt] = c;
			gotEntry = 1;
			break;
		    }
		}
		SkipSpace(p);
		if (!buff[p])
		    break;
		q = p;
		SkipTo(p, '}');
		if (!buff[p])
		    break;
		buff[p++] = '\0';
		if (gotEntry)
		{
		    strcpy(refText[refCnt], buff + q);
		    refCnt++;
		}
		/* Turn on bold attribute and print the word... */
		ReverseOn();
		PutString(r, c, buff + q);
		ReverseOff();
		c += strlen(buff + q);
		q = p;
	    }
	}
	else
	{
	    PutString(r, 2, buff);
	    c = 2 + strlen(buff);
	}
	buff[0] = '\0';
	fill(buff, 80 - c, ' ');
	PutString(r,c,buff); /* clear that kosmik debris... */
    }
    if (hasmore) /* make sure that entry doesn't end anyway */
    {
	long pos = ftell(tfp);
	for (;;)
	{
	    if (fgets(buff, MAXLNLEN, tfp) == 0)
	    {
		hasmore = 0;
		break;
	    }
	    if (buff[0] != '\n')
	    {
		if (strncmp(buff, "@entry", 6) == 0)
		    hasmore = 0;
		break;
	    }
	}
	if (hasmore) fseek(tfp, pos, 0);
    }
    buff[0] = '\0';
    fill(buff, 80, ' ');
    while (r < (LinesPerScreen+5)) PutString(r++,0,buff);
    if (hasmore)
    {
	if (pg < pagenum)
	{
	    pg++;
	    //refNow = 0;
	    goto showpage;
	}
	ReverseOn();
	PutString((3+LinesPerScreen), 50, "Press Space for more...");
	ReverseOff();
	int ch = GetKeyPress();
        if (ch == ' ')
	{
	    refNow = 0;
	    pagenum++;
	    pg++;
	    goto showpage;
	}
	return ch;
    }
    return 0;
}

int HyperText::PaintPage()
{
    int rtn = 0;
    ClearScreen();
    Screen::Paint();
    if (inIndex) PaintIndex();
    else rtn = PaintEntry();
    RefreshScreen();
    return rtn;
}

void HyperText::PrevReference()
{
    refNow--;
    if (refNow < 0)
        refNow = refCnt - 1;
}

void HyperText::NextReference()
{
    refNow++;
    if (refNow >= refCnt)
        refNow = 0;
}

void HyperText::FollowRef()
{
    idxNow = refIdx[refNow];
    inIndex = pagenum = refCnt = refNow = 0;
}

void HyperText::FindRefByLetter(int ch)
{
    if (refCnt)
    {
        int rf = 0, key;
        if (islower(ch))
	    ch = toupper(ch);
	for (rf = 0; rf < refCnt; rf++)
        {
	    int key = (int) refText[rf][0];
	    if (islower(key))
	        key = toupper(key);
	    if (key == ch)
	    {
	        refNow = rf;
	        break;
	    }
        }
    }
}

int HyperText::GetKeyPress()
{
    /* Show the current reference */
    if (refCnt)
    {
	ReverseOn(); BoldOn();
	PutString(refRows[refNow], refCols[refNow], refText[refNow]);
	BoldOff(); ReverseOff();
    }
    RefreshScreen();
    int key = GetKey();
    return key;
}

int HyperText::Run()
{
    int done = 0, key;
    if (namespace == 0) return 0;
    ClearScreen();
    ClearHistory();
    while (!done)
    {
	AddToHistory(idxNow);
	key = PaintPage();
	if (key == 0) key = GetKeyPress();
	switch (key)
	{
	case KB_LEFT:
	case KB_UP:
	    PrevReference();
	    break;
	case KB_RIGHT:
	case KB_DOWN:
	case '\t':
	    NextReference();
	    break;
	case KB_PGUP:
	    PrevPage();
	    break;
	case KB_PGDN:
	    NextPage();
	    break;
	case KB_FKEY_CODE(0):
	    EnterIndex();
	    break;
	case '\n':
	case '\r':
	case KB_FKEY_CODE(1):
	    FollowRef();
	    break;
	case KB_FKEY_CODE(2):
    	    if (histNow > 1)
		LastRefPage();
	    break;
	case KB_FKEY_CODE(3):
	    ShellOut();
	    break;
#ifdef STANDALONE
	case KB_FKEY_CODE(4):
	    char *inm = new char[strlen(namespace + nidx[idxNow])+1];
	    assert(inm);
	    strcpy(inm, namespace + nidx[idxNow]);
	    EditScreen *es = new EditScreen(this, helpfile);
	    assert(es);
	    es->Editor()->GotoByteAbs(indexes[idxNow]);
	    es->Editor()->SetWrap(0);
	    es->Run();
	    delete es;
	    // Recompile and reload
    	    HyperCompile(helpfile);
	    Load(inm);
	    delete [] inm;
	    break;
#endif
	case 27: // ESC - back out (only works with ESC Fkey maps disabled)
    	    if (histNow > 1)
		LastRefPage();
	    else done = 1;
	    break;
	case KB_FKEY_CODE(7): // cancel
	    done = 1;
	    break;
	default:
	    FindRefByLetter(key);
	    break;
	}
    }
    ClearScreen();
    return 0;
}

HyperText::HyperText(Screen *parent_in, char *fname, char *start)
	: histNow(0), inIndex(0), idxStart(0), refCnt(0), refNow(0),
	  pagenum(0), tfp(0), idxNow(0), idxTotal(0), namespace(0),
	  indexes(0), nidx(0),
	  Screen(parent_in, "Help", "Press Index for a list of topics")
{
    ConstructTrace("HyperTxt");
#ifdef STANDALONE
    fkeys = new FKeySet8by2(" Index", " Follow\nRefrnce", 
			    " Prev\nRefrnce", " Shell",
			    "  Edit", 0, 0, "  Done");
#else
    fkeys = new FKeySet8by2(" Index", " Follow\nRefrnce",
			    " Prev\nRefrnce", " Shell",
			    0, 0, 0, "  Done");
    (void)fname;
#endif
    assert(fkeys);
    helpfile = new char[strlen(fname)+1];
    refHist = new short[HistoryLength];
    refRows = new short[RefsPerScreen];
    refCols = new short[RefsPerScreen];
    refIdx = new short[RefsPerScreen];
    refText = new char*[RefsPerScreen];
    assert(helpfile && refHist && refRows && refCols && refIdx && refText);
    strcpy(helpfile, fname);
    for (int i = 0; i < RefsPerScreen; i++)
    {
        refText[i] = new char[MaxRefTextLen];
	assert(refText[i]);
    }
    Load(start);
}

void HyperText::Load(char *start)
{
    FILE *ifp;
    int icnt, ncnt;
    short p, q;
    char iname[256];

    delete [] namespace;
    delete [] indexes;
    delete [] nidx;
    if (tfp) fclose(tfp);
    tfp = fopen(helpfile, "r");
    if (tfp == 0)
    {
	Debug1("Cannot open help text file %s for input!\n", helpfile);
	return;
    }
    strcpy(iname, helpfile);
    for (char *sptr = iname+strlen(iname); sptr>=iname && *sptr != '.'; sptr--);
    if (sptr >= iname) *sptr = '\0';
    strcat(iname, ".idx");
    ifp = fopen(iname, "r");
    if (ifp == 0)
    {
	Debug1("Cannot open help index file %s for input!\n", iname);
	fclose(ifp);
	return;
    }
    fscanf(ifp, "%d Indices", &icnt);
    fscanf(ifp, "%d Namespace", &ncnt);
    namespace = new char [ncnt];
    indexes = new long[icnt];
    nidx = new short[icnt];
    if (namespace == 0 || indexes == 0 || nidx == 0)
    {
	delete [] namespace;
	delete [] indexes;
	delete [] nidx;
	fclose(ifp);
	fclose(tfp);
	namespace = 0;
	return;
    }
    for (p = q = 0; p < icnt; p++, q += strlen(namespace + q) + 1)
    {
	nidx[p] = q;
	fscanf(ifp, "%s %ld", namespace + q, &indexes[p]);
	if (strcmp(namespace + q, start) == 0)
	    idxNow = p;
    }
    if (start[0] == '\0')
	idxNow = 0;
    idxTotal = icnt;
    fclose(ifp);
}

HyperText::~HyperText()
{
    delete [] nidx;
    delete [] indexes;
    delete [] namespace;
    delete [] helpfile;
    delete [] refHist;
    delete [] refRows;
    delete [] refCols;
    delete [] refIdx;
    for (int i = 0; i < RefsPerScreen; i++)
        delete [] refText[i];
    delete [] refText;
    fclose(tfp);
    DestructTrace("HyperTxt");
}

#ifdef STANDALONE

char *ApplicationName = "OMS";
char *ApplicationVersion = "v1.0";
FILE *debugfp = 0;

void Fatal(int lnum, char *msg)
{
    fprintf(stderr, "Fatal error");
    if (lnum >0) fprintf(stderr, " on line %d", lnum);
    fprintf(stderr, ": %s\n", msg);
    exit(-1);
}

void HyperCompile(char *fname)
{
    char buf[MAXLNLEN];
    FILE *ifp, *ofp;
    ifp = fopen(fname, "r");
    if (ifp == 0)
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
    if (ofp == 0)
    {
	fprintf(stderr, "Cannot open %s for output!\n", iname);
	fclose(ifp);
	exit(-1);
    }
    /* Find out how much space is needed */
    int namespacesize = 0, numentries = 0, linenum = 0;
    while (!feof(ifp))
    {
	if (fgets(buf, MAXLNLEN, ifp)==0) break;
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
	if (fgets(buf, MAXLNLEN, ifp) == 0) break;
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
}


int main(int argc, char *argv[] )
{
    if (argc < 2 || argc > 3)
    {
        fprintf(stderr, "Useage: browse <Hypertext file name> [<entry ref>]\n");
	exit(-1);
    }
    StartCurses();
    HyperText *h;
    if (argc==2)
	h = new HyperText(0, argv[1], "");
    else
	h = new HyperText(0, argv[1], argv[2]);
    assert(h);
    (void)h->Run();
    delete h;
    StopCurses();
    return 0;
}

#endif

