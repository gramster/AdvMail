// A very basic hypertext browser, implemented as a C++ class.
//
// Written by Graham Wheeler, 1994.
// (c) 1994, All Rights Reserved.
//
// This code is part of Gram's Commander v4.0. It is essentially a
// port of the gc v3.3 help system to C++. It also makes use of the
// new screen classes and function key classes, rather than drawing
// the Index/Next/Prev etc buttons on the screen directly.

#ifndef _HYPERTEXT_H
#define _HYPERTEXT_H

class HyperText : public Screen // inherit the function key painting code
{
    static const int LinesPerScreen;	// maximum lines of info per screen
    static const int RefsPerScreen;	// maximum references per screen
    static const int HistoryLength;	// max links that can be undone
    static const int MaxRefTextLen;	// Space for a reference's text

    FILE *tfp;			// help text file pointer
    int idxStart,		// first index on screen
	idxNow,			// current index on screen
	idxTotal;		// total number of indices on screen
    short refNow, refCnt;	// current and total number of links
    short *refHist,		// circular buffer of visited links
	histNow;		// current location in history buffer
    int inIndex;		// are we displaying the index?
    int pagenum;		// page number (from 0) of displayed page
    short *refRows;		// current hot link locations on screen
    short *refCols;		// ""
    short *refIdx;		// indices of these hot links
    char **refText;		// names of these links
    char *namespace;		// space for storing link names
    long *indexes;		// space for storing link indices (offsets)
    short *nidx;		// offsets of names in namespace
    char *helpfile;		// help file name; used in standalone version

    char *PrintTopic(int row, char *buf, int centerText);
    void NextIndexPage();
    void PrevIndexPage();
    void NextEntryPage();
    void PrevEntryPage();
    void NextPage();
    void PrevPage();
    void AddToHistory(int idx);
    void ClearHistory();
    void LastRefPage();
    void EnterIndex();
    void PaintIndex();
    int PaintEntry();
    int PaintPage();
    void PrevReference();
    void NextReference();
    void FollowRef();
    void FindRefByLetter(int ch);
    int  GetKeyPress();
    // Key handling is done from Run... should fix this
    virtual Screen *HandleFKey(int key, int &done)
    { done = 0; (void)key; return 0; }
    void Load(char *start);
public:
    HyperText(Screen *parent, char *fname, char *start);
    ~HyperText();
    virtual int Run();
};

#endif

