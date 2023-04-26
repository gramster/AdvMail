#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>

#if __MSDOS__
#else
#include <unistd.h>
#endif

#include "debug.h"
#include "ansicurs.h"
#include "config.h"
#include "screen.h"
#include "ed.h"
#include "hypertxt.h"

#ifdef NO_INLINES
#include "screen.inl"
#endif

extern char *ApplicationName;
extern char *ApplicationVersion;

// Useful utilities

void fill(char *buf, int num, int ch)
{
    int pos = strlen(buf);
    while (pos < num) buf[pos++] = ch;
    buf[pos] = 0;
}

// This should use locale info to format appropriately!!

char *FormatDate(long ticks)
{
    struct tm *tm = localtime((time_t*)&ticks);
    static char datestring[12];
    // European format
    sprintf(datestring, "%02d/%02d/%02d", tm->tm_mday, tm->tm_mon+1, tm->tm_year);
    // US format
    //sprintf(datestring, "%02d/%02d/%02d", tm->tm_mon+1, tm->tm_mday, tm->tm_year);
    return datestring;
}

char *FormatTime(long ticks)
{
    struct tm *tm = localtime((time_t*)&ticks);
    static char timestring[6];
    sprintf(timestring, "%02d:%02d", tm->tm_hour, tm->tm_min);
    return timestring;
}

char *FormatAmount(int cnt, char *itemname)
{
    static char buf[40];
    if (cnt==0)
	sprintf(buf, "No %ss", itemname);
    else if (cnt==1)
	sprintf(buf, "1 %s", itemname);
    else 
	sprintf(buf, "%d %ss", cnt, itemname);
    return buf;
}

//-----------------------------------------------------------------------
// Function key handler

void FKeySet::InitModeKey(int idx, int modenum, char *info_in)
{
    assert(info_in);
    assert(modenum>=0 && modenum<=1);
    assert(idx>=0 && idx<nkeys);
    int l = strlen(info_in);
    assert(l < (nlines*keywidth+nlines-1));
    char *info = new char[l+1];
    assert(info);
    strcpy(info, info_in);
    if (info[0]=='*') 
    {
	on_if_empty[idx][modenum] = 0;
	info++;
    }
    else on_if_empty[idx][modenum] = 1;
    char *split = strchr(info, '\n');
    if (split)
    {
	assert(nlines==2);
	*split++ = '\0';
    }
    if (info[0])
    {
	l = strlen(info);
	assert(l>1 && l<keywidth);
	label[idx][modenum][0] = new char[l+1];
	assert(label[idx][modenum][0]);
	strcpy(label[idx][modenum][0], info);
    }
    else label[idx][modenum][0] = 0;
    if (split && split[0])
    {
	l = strlen(split);
	assert(l>1 && l<keywidth);
	label[idx][modenum][1] = new char[l+1];
	assert(label[idx][modenum][1]);
	strcpy(label[idx][modenum][1], split);
    }
    else label[idx][modenum][1] = 0;
    delete [] info;
}

void FKeySet::InitKey(int idx, char *info)
{
    char buf[80];
    strcpy(buf, info?info:"");
    char *split = strchr(buf, ':');
    if (split != 0)
    {
	*split++ = '\0';
	InitModeKey(idx, 1, split);
    }
    else InitModeKey(idx, 1, buf);
    InitModeKey(idx, 0, buf);
}

FKeySet::FKeySet(int row_in, int nkeys_in, int nlines_in, int keywidth_in,
		 char *f1, char *f2, char *f3, char *f4,
	    	 char *f5, char *f6, char *f7, char *f8,
	    	 char *f9, char *f10, char *f11, char *f12)
    : isempty(1), mode(0), row(row_in), nkeys(nkeys_in),
	 nlines(nlines_in), keywidth(keywidth_in)
{
    InitKey(0, f1);
    InitKey(1, f2);
    InitKey(2, f3);
    InitKey(3, f4);
    InitKey(4, f5);
    InitKey(5, f6);
    InitKey(6, f7);
    InitKey(7, f8);
    if (nkeys >= 10)
    {
        InitKey(8, f9);
        InitKey(9, f10);
    }
    if (nkeys >= 12)
    {
        InitKey(10, f11);
        InitKey(11, f12);
    }
    mask[0] = mask[1] = 0xFFFF;
    ConstructTrace("FKeySet");
}

FKeySet::~FKeySet()
{
    for (int k = 0; k < nkeys; k++)
	for (int m = 0; m < 2; m++)
	    for (int l = 0; l < nlines; l++)
		delete [] label[k][m][l];
    DestructTrace("FKeySet");
}

void FKeySet::Enable(int modenum, int fkeynum, int setval)
{
    if (setval)
        mask[modenum] |= 1l << fkeynum;
    else
        mask[modenum] &= ~(1l << fkeynum);
}

void FKeySet::Disable(int modenum, int fkeynum)
{
    Enable(modenum, fkeynum, 0);
}

const int FKeySet::GetCommand(int ch) // returns -1 (no command), 0..nkeys-1
	                          // (mode 0) or nkeys..2*nkeys-1 (mode 1)
{
    int key = KB_FKEY_NUM(ch);
    if (key<0 || key>=nkeys)
	return -1;
    if (isempty && !on_if_empty[key][mode])
	return -1;
    if (label[key][mode][0][0] == 0)
	return -1;
    if ((mask[mode] & (1l << key)) == 0)
	return -1;
    return (key + 12 * mode);
}

void FKeySet::Paint()
{
    int key, oldr, oldc, ocr, occ;
    GetCursor(&oldr,&oldc);
    GetClip(&ocr,&occ);
    SetClip(0,0); // allow drawing on whole screen
    ReverseOn();
    for (key = 0; key < nkeys; key++)
    {
	int c = key*keywidth;
	if (key>=(nkeys/2))
	    c += 80 - nkeys * keywidth;
	for (int r = 0; r < nlines; r++)
	{
	    char buf[32], *s = 0;
	    if ((mask[mode] & (1l<<key)) != 0 &&
		(!isempty || on_if_empty[key][mode]))
		s = label[key][mode][r];
	    if (s) strcpy(buf, s);
	    else buf[0] = 0;
	    fill(buf, keywidth-1, ' ');
	    PutString(row+r,c,buf);
	}
    }
    ReverseOff();
    SetCursor(oldr,oldc);
    SetClip(ocr,occ);
}

//-------------------------------------------------------------------------

Field::~Field()
{
    DestructTrace("Field");
}

//-------------------------------------------------------------------------

int DataField::IsToggle()
{
    return 0;
}

int DataField::IsValid()
{
    return 1;
}

void DataField::Paint(int active)
{
    if (r>=0)
    {
        char buf[82];
        if (echo) 
            strcpy(buf, value);
        else
            buf[0]=0;
	if (active < 2)
	{
	    ReverseOn();
	    if (active) BoldOn();
	}
        fill(buf, w, ' ');
        PutString(r, c, buf);
	if (active < 2)
	{
	    if (active) BoldOff();
	    ReverseOff();
	}
    }
}

int DataField::HandleKey(int ch)
{
    char buf[80];
    switch (ch) 
    {
    case KB_LEFT:
        if (pos > 0) pos--;
	break;
    case KB_RIGHT:
	if (pos <= len)
	    pos++;
	break;
    case KB_HOME:
	pos = 0;
	break;
    case KB_END:
	pos = len+1;
	break;
    case KB_DELETE:
	if (pos < len)
	{
	    if (pos>0)
	    {
		strncpy(buf, value, pos);
		buf[pos]=0;
	    }
	    else buf[0] = 0;
	    if (value[pos+1]) strcat(buf, value+pos+1);
            else buf[pos] = 0;
	    SetValue(buf);
	    len--;
	}
	break;
    case 25: // ^Y deletes value
	SetValue("");
	len = pos = 0;
	break;
    case KB_BACKSPACE:
	if (pos > 0)
	{
	    strncpy(buf, value, pos-1);
	    if (value[pos]) strcpy(buf+pos-1, value+pos);
	    else buf[pos-1]=0;
	    SetValue(buf);
	    len--;
	    pos--;
	}
	break;
    default:
        if (ch < 32 || ch > 126 || strchr(exitchars, ch) != NULL)
        {
	    if ((IS_FKEY(ch) && ((1<<KB_FKEY_NUM(ch)) & cancelfkeys) != 0) ||
		IsValid())
	        return ch; // done
	    else break;
        }
	if (len==w) break; // field full
	if (pos > 0)
	{
	    strncpy(buf, value, pos);
	    buf[pos]=ch;
	    if (value[pos]) strcpy(buf+pos+1, value+pos);
	    else buf[pos+1] = 0;
	}
	else
	{
	    buf[0] = ch;
	    strcpy(buf+1, value);
	}
	SetValue(buf);
	pos++;
	len++;
	break;
    }
    return 0;
}

int DataField::Run()
{
    int ch = 0;
    if (value==0)
    {
	value = new char[1];
	assert(value);
	value[0] = len = pos = 0;
    }
    else len = pos = strlen(value);
    while (ch == 0)
    {
	Paint(1);
	SetCursor(r, c+pos);
	ch = HandleKey(GetKey());
    }
    Paint();
    return ch;
}

DataField::~DataField()
{
    delete [] exitchars;
    delete [] value;
    DestructTrace("DataField");
}

//---------------------------------------------------------------
// we don't check leap years properly yet

int mdays[] = { 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

int DateField::IsValid()
{
    int m, d, y;
    // Will have to change if FormatDate changes!!
    if (value == 0 || sscanf(value, "%d/%d/%d", &d, &m, &y) != 3)
	return 0;
    if (m<1 || m > 12) return 0;
    if (d<1 || d > mdays[m-1]) return 0;
    if (y<95) return 0;
    return 1;
}

DateField::~DateField()
{
    DestructTrace("DateField");
}

int TimeField::IsValid()
{
    int h, m;
    if (value == 0 || sscanf(value, "%d:%d", &h, &m) != 2)
	return 0;
    if (h<0 || h > 23) return 0;
    if (m<0 || m > 59) return 0;
    return 1;
}

TimeField::~TimeField()
{
    DestructTrace("TimeField");
}

int NumberField::IsValid()
{
    int v;
    if (value == 0 || sscanf(value, "%d%*s", &v) != 1 || v<minv || v>maxv)
    {
	char buf[82];
	strcpy(buf, "Invalid number or out of range value!");
	fill(buf,80,' ');
	ReverseOn();
	PutString(1,0,buf);
	ReverseOff();
	RefreshScreen();
	return 0;
    }
    return 1;
}

NumberField::~NumberField()
{
    DestructTrace("NumberField");
}

//---------------------------------------------------------------

int ToggleField::IsValid()
{
    return Field::IsValid();
}

int ToggleField::IsToggle()
{
    return 1;
}

void ToggleField::Init(int r_in, int c_in, int w_in,
	     char *valueset_in, int nextchar_in, int prevchar_in)
{
    r = r_in;
    c = c_in;
    w = w_in;
    nextchar = nextchar_in;
    prevchar = prevchar_in;
    value = 0;
    // init cnt and valueset.
    for (int c = 0; c<cnt; c++)
	delete [] valueset[c];
    delete [] valueset;
    cnt = 0;
    valueset = 0;
    char *s = valueset_in;
    if (s && s[0])
    {
	while (s)
        {
	    cnt++;
	    s = strchr(s+1, '@');
	    if (s) s++;
        }
        valueset = new (char*[cnt]);
	assert(valueset);
	s = valueset_in;
	char *buf = new char[strlen(valueset_in)+1];
	if (buf) 
	{
	    strcpy(buf, valueset_in);
	    for (c = strlen(buf); --c >=0;)
	        if (buf[c] == '@') buf[c] = '\0';
	    s = buf;
	    for (c = 0; c <cnt; c++)
	    {
	        valueset[c] = new char[strlen(s)+1];
		assert(valueset[c]);
	        strcpy(valueset[c], s);
	        s = s + strlen(s) + 1;
	    }
	    delete [] buf;
	}
	else cnt = 0;
    }
    Debug1("Toggle has %d possible values", cnt);
    if (cnt>0)
    {
	Debug1("Value 0 is %s", valueset[0]);
	Debug2("Value %d is %s", cnt-1, valueset[cnt-1]);
    }
}

ToggleField::~ToggleField()
{
    Debug1("Deleting toggle with %d values", cnt);
    if (valueset)
	for (int v = 0; v < cnt; v++)
	    delete [] valueset[v];
    delete [] valueset;
    DestructTrace("ToggleField");
}

char *ToggleField::GetText()
{
    return valueset[value];
}

void ToggleField::Paint(int active)
{
    if (r>=0)
    {
        char buf[82];
        strcpy(buf, GetText());
	buf[w] = '\0';
        fill(buf, w, ' ');
	ReverseOn();
	if (active) BoldOn();
        PutString(r, c, buf);
	ReverseOff();
	if (active) BoldOff();
    }
}

int ToggleField::Run()
{
    for (;;)
    {
	Paint(1);
	int ch = GetKey();
	if (ch == ' ' || ch == nextchar)
	    NextValue();
	else if (ch == prevchar)
	    PrevValue();
	else if (ch < 32 || ch > 127)
	    return ch;
    }
}

//--------------------------------------------------------------

PopupPrompt::PopupPrompt(Screen *parent_in, char *prompt_in, char *help_in,
	char *f1lbl, char *f2lbl,
	char *f8lbl, int echo)
	: fkeys(f1lbl,f2lbl,0,0,0,0,"  Help", f8lbl), 
    	  fld(2,0,80,0,"\n\t\r", echo, 0), help(help_in), parent(parent_in),
	  prompt(prompt_in)
{
    char buf[82];
    strcpy(buf, prompt);
    fill(buf,80,' ');
    ReverseOn();
    PutString(1,0,buf);
    ReverseOff();
    ConstructTrace("PopupPrompt");
}

void PopupPrompt::Paint()
{
    char buf[82];
    strcpy(buf, prompt);
    fill(buf,80,' ');
    ReverseOn();
    PutString(1,0,buf);
    ReverseOff();
    fkeys.Paint();
}

int PopupPrompt::Run(char *buf, int rtn_on_enter)
{
    fld.SetValue(buf);
    Paint();
    for (;;)
    {
	int ch = fld.Run();
	int fkey = fkeys.GetCommand(ch);
	if (fkey == 6) // help
	{
	    parent->Help(help);
	    Paint();
	}
	else if ((rtn_on_enter && ch == '\n') || fkey>=0)
	{
	    strcpy(buf, fld.Value());
	    return ch;
	}
    }
}

//---------------------------------------------------------------------

void Menu::GetSelection(int &first, int &last)
{
    if (selection >= 0)
	if (selection > pos)
	{
	    first = pos;
	    last = selection;
	}
	else
	{
	    first = selection;
	    last = pos;
	}
    else
    {
	first = last = pos;
    }
}

void Menu::Paint(int selected) // selected is used when re-ordering
{
    int first, last;
    GetSelection(first, last);
    for (int e = 0; e < maxentries; e++)
    {
	if (!entries[e].Initialised()) continue;
	if (e>= first && e <= last)
	{
            BoldOn();
            ReverseOn();
            entries[pos].Paint();
            ReverseOff();
            BoldOff();
	}
	else entries[e].Paint();
    }
    if (selected >= 0 && entries[selected].Initialised())
    {
        ReverseOn();
        entries[selected].Paint();
        ReverseOff();
    }
    if (entries[pos].Initialised())
    {
        int r, c;
        entries[pos].Position(r, c);
        SetCursor(r, c);
    }
}

void Menu::Up(int paint)
{
    if (entrycnt)
    {
	do
	{
	    pos = (pos+maxentries-1)%maxentries;
	} while (!entries[pos].Initialised());
    	if (paint) Paint();
    }
}

void Menu::Down(int paint)
{
    if (entrycnt)
    {
	do
	{
	    pos = (pos+1)%maxentries;
	} while (!entries[pos].Initialised());
        if (paint) Paint();
    }
}

//-----------------------------------------------------------------------
// List box popups

ListBoxPopup::ListBoxPopup(Screen *owner_in, int row_in, int col_in,
	int width_in, char *prompt_in, int maxitems_in)
    : row(row_in), col(col_in), width(width_in), fkeys(0), pos(0),
      cnt(0), owner(owner_in), maxitems(maxitems_in),
      pagenum(0), islast(1)
{
    prompt = prompt_in ? new char[strlen(prompt_in)+1] : 0;
    if (prompt) strcpy(prompt, prompt_in);
    items = new char*[maxitems];
    assert(items);
    for (int i = 0; i < maxitems; i++)
	items[i] = 0;
    ConstructTrace("ListBoxPopup");
}

char *ListBoxPopup::PagePrompt()
{
    if (islast)
        return (pagenum>0) ? "    PgUp for more" : "";
    else
	return (pagenum>0) ? "    PgUp/PgDn for more" : "    PgDn for more";
}

void ListBoxPopup::PageUp()
{
    if (pagenum>0)
    {
	int lcnt = PrevPage();
	if (lcnt>0)
	{
	    pagenum--;
	    GetItems();
	}
	islast =  (lcnt < maxitems);
	Paint();
    }
}

void ListBoxPopup::PageDown()
{
    if (!islast)
    {
	int lcnt = NextPage();
	if (lcnt > 0)
	{
	    pagenum++;
	    GetItems();
	}
	islast =  (lcnt < maxitems);
	Paint();
    }
    else Beep();
}

void ListBoxPopup::HandleKey(int ch, int &done, int &rtn)
{
    (void)done;
    (void)rtn;
    switch(ch)
    {
    case KB_UP:
	pos = (pos+cnt-1)%cnt;
	break;
    case KB_DOWN:
	pos = (pos+1)%cnt;
	break;
    case KB_PGUP:
	PageUp();
	break;
    case KB_PGDN:
	PageDown();
	break;
    default:
	Beep();
    }
}

void ListBoxPopup::HandleFKey(int ch, int &done, int &rtn)
{
    (void)rtn;
    (void)done;
    Beep();
}

void ListBoxPopup::Paint()
{
    char buf[82];
    strcpy(buf, prompt);
    fill(buf, 80, ' ');
    ReverseOn();
    PutString(1,0,buf);
    buf[0] = '\0';
    fill(buf, width, ' ');
    PutString(row, col, buf);
    ReverseOff();
    for (int r = 0; r < cnt; r++)
    {
    	ReverseOn();
	PutString(r+row+1, col, " ");
	PutString(r+row+1, col+width-1, " ");
    	ReverseOff();
	strncpy(buf, items[r], width-3);
	buf[width-3] = '\0';
	fill(buf,width-2,' ');
	if (pos == r)
	{
    	    ReverseOn();
	    PutString(r+row+1, col+1, buf);
    	    ReverseOff();
	}
	else PutString(r+row+1, col+1, buf);
    }
    if (pagenum>0)
    {
	buf[0] = 0;
        fill(buf,width-2,' ');
        for (; r < maxitems; r++)
	{
    	    PutString(r+row+1, col+1, buf);
    	    ReverseOn();
    	    PutString(r+row+1, col, " ");
    	    PutString(r+row+1, col+width-1, " ");
    	    ReverseOff();
	}
    }
    strcpy(buf, PagePrompt());
    fill(buf,width-2,' ');
    PutString(r+row+1, col+1, buf);
    ReverseOn();
    PutString(r+row+1, col, " ");
    PutString(r+row+1, col+width-1, " ");
    buf[0] = 0;
    fill(buf,width,' ');
    PutString(r+row+2, col, buf);
    ReverseOff();
    if (fkeys) fkeys->Paint();
}

int ListBoxPopup::Run()
{
    int done = 0, rtn = -1;
    while (done == 0)
    {
	Paint();
	int ch = GetKey();
	if (IS_FKEY(ch))
	{
	    if (fkeys->GetCommand(ch) >= 0)
		HandleFKey(ch, done, rtn);
	    else Beep();
	}
	else HandleKey(ch, done, rtn);
    }
    return rtn;
}

ListBoxPopup::~ListBoxPopup()
{
    for (int i = 0; i < cnt; i++)
	delete [] items[i];
    delete [] prompt;
    delete fkeys;
    delete [] items;
    DestructTrace("ListBoxPopup");
}

//-------------------------------------------------------------------------

void Screen::Help(char *entry)
{
    char *hf = config->Get("helpfile");
    if (hf && hf[0] && access(hf,0)==0)
    {
        HyperText *h = new HyperText(0, hf, entry);
	assert(h);
    	h->Run();
    	delete h;
	ClearScreen();
    	Paint();
    }
    else DrawTitle("No help file!");
}

void Screen::DrawTitle(char *prompt_in)
{
    char buf[COLS+1];
    int r, c;
    GetCursor(&r,&c);
    strcpy(buf, ApplicationName);
    strcat(buf, "(");
    strcat(buf, ApplicationVersion);
    strcat(buf, ")");
    fill(buf, COLS, ' ');
    PutString(0,0,buf);
    if (title) CenterText(0, title);
    char *prmpt = 0;
    if (prompt_in) prmpt = prompt_in;
    else if (prompt) prmpt = prompt;
    strcpy(buf, prmpt);
    fill(buf, COLS, ' ');
    ReverseOn();
    PutString(1,0,buf);
    ReverseOff();
    SetCursor(r,c);
}

Screen::Screen(Screen *parent_in, char *title_in, char *prompt_in,
	       int nlabels_in)
    : title(title_in), prompt(prompt_in), parent(parent_in),
      numlabels(nlabels_in), fkeys(0)
{
    if (numlabels>0) labels = new TextLabel[numlabels];
    else labels = 0;
    ClearScreen();
    SetOrigin(0,0);
    ConstructTrace("Screen");
}

int Screen::AddLabel(int r_in, int c_in, char *text_in)
{
    int l;
    for (l = 0; l < numlabels; l++)
    {
	if (!labels[l].Initialised())
	{
	    labels[l].Init(r_in, c_in, text_in);
	    return l;
	}
    }
    assert(l < numlabels);
    return 0; // unreachable
}

void Screen::SetLabel(int l_in, int r_in, int c_in, char *text_in)
{
    assert(l_in>=0 && l_in<numlabels);
    labels[l_in].Init(r_in, c_in, text_in);
}

void Screen::ToggleFKeys()
{
    fkeys->ToggleMode();
    fkeys->Paint();
}

void Screen::Paint()
{
    int i;
    DrawTitle(prompt);
    if (fkeys) fkeys->Paint();
    for (i = 0; i < numlabels; i++)
	labels[i].Paint();
    SetCursor(0,0);
}

Screen::~Screen()
{
    delete fkeys;
    delete [] labels;
    ClearScreen();
    SetOrigin(0,0);
    DestructTrace("Screen");
}

int Screen::Confirm(char *title_in, char *prompt_in, char *f1lbl, char *f8lbl)
{
    FKeySet *oldfkeys = fkeys;
    DrawTitle(prompt_in);
    if (title_in)
    {
	char buf[COLS+2];
	strcpy(buf, title_in);
	fill(buf, COLS, ' ');
	ReverseOn(); BoldOn();
	PutString(0,0,buf);
	BoldOff(); ReverseOff();
    }
    fkeys = new FKeySet8by2(f1lbl,0,0,0,0,0,0,f8lbl);
    if (fkeys) fkeys->Paint();
    int rtn = 0;
    for (;;)
    {
        int ch = GetKey();
        if (ch == (KB_F0+7))
	    break;
        else if (ch == KB_F0)
	{
	    rtn = 1;
	    break;
	}
    }
    delete fkeys;
    fkeys = oldfkeys;
    if (fkeys) fkeys->Paint();
    return rtn;
}

int Screen::Run()
{
    int done = 0;
    Paint();
    while (!done)
    {
        int ch = GetKey();
	int fkey = (fkeys ? fkeys->GetCommand(ch) : -1);
	Screen *s = 0;
	if (fkey>=0)
	    s = HandleFKey(fkey, done);
	else
	    s = HandleKey(ch, done);
	if (s)
	{
	    int rtn = s->Run();
	    delete s;
	    if (rtn < 0) return -1;
	    Paint();
	}
    }
    return 0;
}

void Screen::ShellOut()
{
    ClearScreen();
    StopCurses();
    printf("Type \"exit\" to return to %s.\n", ApplicationName);
    char *sh = getenv("SHELL");
    system(sh?sh:"sh");
    StartCurses();
    ClearScreen();
    Paint();
}

Screen *Screen::HandleKey(int ch, int &done)
{
    (void)ch;
    Beep();
    done = 0;
    return 0;
}

void Screen::RefreshContents()
{
    if (this->parent)
	this->parent->RefreshContents();
}

//----------------------------------------------------------------

int FormScreen::AddToggle(int r_in, int c_in, int w_in, char *valueset_in)
{
    ToggleField *tf = new ToggleField(r_in, c_in, w_in, valueset_in, nextchar, prevchar);
    assert(tf);
    return AddField(tf);
}

int FormScreen::AddField(Field *fld)
{
    int f;
    assert(fld);
    for (f = 0; f < numfields; f++)
    {
	if (fields[f]==0)
	{
	    fields[f] = fld;
	    return f;
	}
    }
    assert(f < numfields);
    return 0; // unreachable
}

void FormScreen::LoadMultiField(int ffnum, int nf, char *val)
{
    char buf[1024], *v = buf;
    strcpy(buf, val);
    int l = strlen(buf);
    for (int f = ffnum; f < (ffnum+nf); f++)
    {
	int w = fields[f]->Width();
	if (l < w)
	{
            ((DataField*)fields[f])->SetValue(v);
    	    for (++f; f < (ffnum+nf); f++)
        	((DataField*)fields[f])->SetValue("");
	}
        else
        {
	    char ch = v[w];
	    v[w] = '\0';
            ((DataField*)fields[f])->SetValue(v);
	    v[w] = ch;
	    l -= w;
	    v += w;
	}
    }
}

char *FormScreen::GetMultiField(int ffnum, int nf)
{
    char buf[1024];
    buf[0] = 0;
    for (int f = ffnum; f < (ffnum+nf); f++)
    {
        char *v = ((DataField*)fields[f])->Value();
	if (v) strcat(buf, v);
    }
    char *rtn = new char[strlen(buf)+1];
    if (rtn) strcpy(rtn, buf);
Debug1("GetMultiField returns \"%s\"", buf);
    return rtn;
}

void FormScreen::DrawTitle(char *prompt_in)
{
    Screen::DrawTitle(prompt_in);
}

void FormScreen::Paint()
{
    if (fields[fnow]->Row() > 19)
	SetOrigin(fields[fnow]->Row() - 19, 0);
    int r, c;
    GetOrigin(&r, &c);
    ClearScreen();
    if (r == 0) DrawTitle(prompt);
    if (IS_FKEY(nextchar) || IS_FKEY(prevchar))
    {
	int nk = KB_FKEY_NUM(nextchar);
	int pk = KB_FKEY_NUM(prevchar);
        int istog = fields[fnow]->IsToggle();
	if (nk>=0 && nk<8) fkeys->Enable(0,nk,istog);
	if (pk>=0 && pk<8) fkeys->Enable(0,pk,istog);
    }
    for (int i = 0; i < numlabels; i++)
	labels[i].Paint();
    for (i = 0; i < numfields; i++)
	fields[i]->Paint();
    if (fkeys)
    {
        SetOrigin(0, 0);
	fkeys->Paint();
        SetOrigin(r, c);
    }
    SetCursor(r,0);
}

Screen *FormScreen::HandleKey(int ch, int &done)
{
    switch(ch)
    {
	case KB_UP:
	    UpField();
	    break;
	case KB_DOWN:
	    DownField();
	    break;
	case KB_LEFT:
	    LeftField();
	    break;
	case KB_RIGHT:
	case '\t':
	case '\n':
	    RightField();
	    break;
	default:
    	    return Screen::HandleKey(ch,done);
    }
    return 0;
}

FormScreen::FormScreen(Screen *parent_in, char * title_in, char * prompt_in,
	   int nlabels_in, int nfields_in, int nextchar_in, int prevchar_in,
	   int canedit_in)
    : Screen(parent_in, title_in, prompt_in, nlabels_in),
	nextchar(nextchar_in),
	prevchar(prevchar_in),
	fnow(0),
	canedit(canedit_in),
	numfields(nfields_in)
{
    if (numfields>0)
    {
	fields = new (Field*[numfields]);
	assert(fields);
	for (int f = 0; f < numfields; f++)
	    fields[f] = 0;
    }
    else fields = 0;
    ConstructTrace("FormScreen");
}

// For these movement routines to work, fields must be created from
// top to bottom, left to right; i.e. reading order

void FormScreen::UpField()
{
    int f = fnow;
    int c = fields[f]->Col();
    for (;;)
    {
        f = (f+numfields-1)%numfields;
	if (f==fnow || fields[f]->Row() != fields[fnow]->Row())
	{
	    int r = fields[f]->Row(), bestf = f;
	    while (fields[f]->Row() == r && fields[f]->Col() >= c)
	    {
		bestf = f;
		f = (f+numfields-1)%numfields;
	    }
	    f = bestf;
	    break;
	}
    }
    fnow = f;
}

void FormScreen::DownField()
{
    int f = fnow;
    int c = fields[f]->Col();
    for (;;)
    {
        f = (f+1)%numfields;
	if (f==fnow || fields[f]->Row() != fields[fnow]->Row())
	{
	    int r = fields[f]->Row(), bestf = f;
	    while (fields[f]->Row() == r && fields[f]->Col() <= c)
	    {
		bestf = f;
		f = (f+1)%numfields;
	    }
	    f = bestf;
	    break;
	}
    }
    fnow = f;
}

void FormScreen::LeftField()
{
    fnow = (fnow+numfields-1)%numfields;
}

void FormScreen::RightField()
{
    fnow = (fnow+1)%numfields;
}

int FormScreen::Run()
{
    int done = 0;
    while (!done)
    {
        Paint();
        int ch = canedit ? fields[fnow]->Run() : GetKey();
	int fkey = (fkeys ? fkeys->GetCommand(ch) : -1);
	Screen *s = 0;
	if (fkey>=0)
	    s = HandleFKey(fkey, done);
	else
	    s = HandleKey(ch, done);
	if (s)
	{
	    int rtn = s->Run();
	    delete s;
	    if (rtn < 0) return -1;
	    Paint();
	}
    }
    return 0;
}

FormScreen::~FormScreen()
{
    SetOrigin(0,0);
    for (int f = 0; f < numfields; f++)
	delete fields[f];
    delete [] fields;
    DestructTrace("FormScreen");
}

//----------------------------------------------------------------

PageableScreen::PageableScreen(Screen *parent_in, char *title_in,
				char *prompt_in, int nlabels_in,
				int nfields_in, int msglines_in)
    : FormScreen(parent_in, title_in, prompt_in, nlabels_in, nfields_in),
      msglines(msglines_in), pagenum(0), is_lastpage(1)
{
    ConstructTrace("PageableScreen");
}

PageableScreen::~PageableScreen()
{
    DestructTrace("PageableScreen");
}
 
void PageableScreen::ShowPagePrompt(int pg, int islast)
{
    int ln = GetLines() - 4;
    if (islast)
        if (pg>0)
	    PutString(ln, 50, "     Press PgUp to see more");
        else
	    PutString(ln, 50, "                           ");
    else if (pg>0)
	PutString(ln, 50, "Press PgUp/PgDn to see more");
    else
	PutString(ln, 50, "     Press PgDn to see more");
}

int PageableScreen::Run()
{
    GetItems();
    return Screen::Run();
}
    
void PageableScreen::DrawTitle(char *prompt_in)
{
    Screen::DrawTitle(prompt_in);
}

void PageableScreen::Paint()
{
    Screen::Paint();
    ShowPagePrompt(pagenum, is_lastpage);
    SetCursor(0,0);
}

void PageableScreen::PageUp()
{
    if (pagenum>0)
    {
	int lcnt = PrevPage();
	if (lcnt>0)
	{
	    pagenum--;
	    GetItems();
	}
	is_lastpage =  (lcnt < msglines);
	ClearScreen();
	Paint();
    }
}

void PageableScreen::PageDown()
{
    if (!is_lastpage)
    {
	int lcnt = NextPage();
	if (lcnt > 0)
	{
	    pagenum++;
	    GetItems();
	}
	is_lastpage =  (lcnt < msglines);
	ClearScreen();
	Paint();
    }
    else Beep();
}

Screen *PageableScreen::HandleKey(int ch, int &done)
{
    done = 0;
    if (ch == KB_PGUP)
	PageUp();
    else if (ch == KB_PGDN)
	PageDown();
    else
	return Screen::HandleKey(ch, done);
    return 0;
}

void PageableScreen::RefreshContents()
{
    Screen::RefreshContents();
}

//----------------------------------------------------------------

EditScreen::EditScreen(Screen *parent_in, char *fname_in)
    : Screen(parent_in, "Edit", 
	"Make your changes and press Save Text when done.")
{
    ConstructTrace("EditScreen");
    editor = new TextEditor(fname_in,COLS,18);
    assert(editor);
    fkeys = new FKeySet8by2(" Cancel\nChanges", " Enable\nAutoWrap",
			" Disable\nAutoWrap", 0,
			0, " Start\n  Over",
			"  Help", "  Save\n  Text");
    assert(fkeys);
    fkeys->Disable(0, F2_PRI);
    editor->SetWrap(1);
    filechanged = 0;
    ConstructTrace("EditScreen");
}

void EditScreen::Paint()
{
    Screen::Paint();
    char buf[COLS+2];
    sprintf(buf, "Page %2d/%-2d     Line %3d/%-3d     Column %2d/%-2d         %5d Bytes",
	    editor->Line()/18+1, (editor->Lines()-1)/18+1,
	    editor->Line()+1, editor->Lines(),
	    editor->Column()+1, editor->Length()+1,
	    editor->Size());
    fill(buf, COLS, ' ');
    ReverseOn();
    PutString(2,0,buf);
    ReverseOff();
    editor->Prepare(1);
    int cursrow, curscol;
    for (int r = 0; r < 18; r++)
    {
	int cpos;
	char *txt = editor->Fetch(cpos);
	if (cpos != -1) 
	{
	    cursrow = r;
	    curscol = cpos;
	}
	strncpy(buf, txt?txt:"", COLS);
	buf[COLS] = '\0';
	fill(buf,COLS,' ');
	PutString(r+3,0,buf);
    }
    SetCursor(cursrow+3, curscol);
}

Screen *EditScreen::HandleFKey(int fkeynum, int &done)
{
    done = 0;
    switch(fkeynum)
    {
    case F1_PRI: // cancel
	if (editor->Changed())
	    done = Confirm("You have not saved the text.",
			   "Press Confirm to return without saving.",
			   "Confirm", " Cancel");
	else done = 1;
	break;
    case F2_PRI: // enable autowrap
	editor->SetWrap(1);
	fkeys->Enable(0,F3_PRI);
	fkeys->Disable(0,F2_PRI);
	Paint();
	break;
    case F3_PRI: // disable autowrap
	editor->SetWrap(0);
	fkeys->Enable(0,F2_PRI);
	fkeys->Disable(0,F3_PRI);
	Paint();
	break;
    case F6_PRI: // start over
	if (Confirm(0, "Press Confirm to clear all the text",
			"Confirm", " Cancel"))
	{
	    char fn[256];
	    strcpy(fn, editor->Name());
	    delete editor;
	    unlink(fn);
    	    editor = new TextEditor(fn,COLS,18);
	    assert(editor);
	    ClearScreen();
	}
	Paint();
	break;
    case F7_PRI:
	Help("EDITING");
	break;
    case F8_PRI:
	if (editor->Changed())
	{
	    editor->Save();
	    filechanged = 1;
	}
	done = 1;
	break;
    default:
	Beep();
    }
    return 0;
}

Screen *EditScreen::HandleKey(int ch, int &done)
{
    done = 0;
    editor->HandleKey(ch);
    Paint();
    return 0;
}

int EditScreen::HasFileChanged()
{
    return filechanged;
}

EditScreen::~EditScreen()
{
    DestructTrace("EditScreen");
    delete editor;
    DestructTrace("EditScreen");
}

//--------------------------------------------------------------------

EditWin::EditWin(int row, int col, int width, int height, char *fname,
	int border)
    : rr(row), cc(col), w(width), h(height), hasborder(border)
{
    editor = new TextEditor(fname,w-2*border,h-2*border);
    assert(editor);
    Paint(1);
    ConstructTrace("EditWin");
}

int EditWin::Load(char *fname)
{
    (void)fname;
//    return editor->Load(fname);
    return 0;
}

void EditWin::Save(char *fname)
{
    (void)fname;
    editor->Save(/*fname*/);
}

int EditWin::HandleKey(int ch)
{
    editor->HandleKey(ch);
    //Paint();
    return 0;
}

void EditWin::Paint(int paintall)
{
    char *buf = new char[w+2];
    assert(buf);
    SetOrigin(rr,cc);
    sprintf(buf, "Pg %2d/%-2d Ln %3d/%-3d Col %2d/%-2d  %5d Bytes    %s",
	    editor->Line()/h+1, (editor->Lines()-1)/h+1,
	    editor->Line()+1, editor->Lines(),
	    editor->Column()+1, editor->Length()+1,
	    editor->Size(), editor->Name());
    fill(buf, w, ' ');
    ReverseOn();
    PutString(0,0,buf);
    if (paintall && hasborder)
    {
	buf[0] = '\0';
        fill(buf, w, ' ');
        PutString(h-1,0,buf);
	buf[1] = '\0';
	for (int r = 1; r < (h-1); r++)
	{
            PutString(r,0,buf);
            PutString(r,w-1,buf);
	}
    }
    ReverseOff();
    editor->Prepare(1);
    int cursrow, curscol;
    for (int r = 0; r < (h-hasborder-1); r++)
    {
	int cpos;
	char *txt = editor->Fetch(cpos);
	if (cpos != -1) 
	{
	    cursrow = r;
	    curscol = cpos;
	}
	strncpy(buf, txt?txt:"", w);
	buf[w-2*hasborder] = '\0';
	fill(buf,w-2*hasborder,' ');
	PutString(r+1,hasborder,buf);
    }
    SetCursor(cursrow+1, curscol+hasborder);
    SetOrigin(0,0);
    delete [] buf;
}

EditWin::~EditWin()
{
    delete editor;
    DestructTrace("EditWin");
}

Screen *MultiEditScreen::HandleKey(int ch, int &done)
{
    window[winnow]->HandleKey(ch);
    Paint();
    done = 0;
    return 0;
}

Screen *MultiEditScreen::HandleFKey(int fkeynum, int &done)
{
    done = 0;
    if (fkeynum == F8_PRI)
	done = 1;
    return 0;
}

MultiEditScreen::MultiEditScreen(Screen *parent_in, int nfiles, char **files)
	: Screen(parent_in, "OMS Edit", "Remove me", 0), numwins(nfiles)
{
    window = new EditWin* [nfiles];
    assert(window);
    int rinc = 18 / nfiles, rnow = 2;
    for (int i = 0; i < numwins; i++, rnow += rinc)
    {
	window[i] = new EditWin(rnow, 0, 80, rinc, files[i]);
	assert(window[i]);
    }
    winnow = 0;
    fkeys = new FKeySet(24,8,1,9,0,0,0,0,0,0,0,"  Quit");
    assert(fkeys);
    ConstructTrace("MultiEditScreen");
}

void MultiEditScreen::Paint()
{
    Screen::Paint();
    for (int i = 0; i < numwins; i++)
	if ( i != winnow)
	    window[i]->Paint();
    window[winnow]->Paint();
}

MultiEditScreen::~MultiEditScreen()
{
    for (int i = 0; i < numwins; i++)
	delete window[i];
    delete [] window;
    DestructTrace("MultiEditScreen");
}

