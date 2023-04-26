#ifndef _SCREEN_H
#define _SCREEN_H

#define LINES		24
#define COLS		80
#define FKEY_COLS	9

//--------------------------------------------------------------------
// Keycodes passed to HandleFKey methods

#define F1_PRI	0
#define F2_PRI	1
#define F3_PRI	2
#define F4_PRI	3
#define F5_PRI	4
#define F6_PRI	5
#define F7_PRI	6
#define F8_PRI	7
#define F9_PRI	8
#define F10_PRI	9
#define F11_PRI	10
#define F12_PRI	11

#define F1_SEC	12
#define F2_SEC	13
#define F3_SEC	14
#define F4_SEC	15
#define F5_SEC	16
#define F6_SEC	17
#define F7_SEC	18
#define F8_SEC	19
#define F9_SEC	20
#define F10_SEC	21
#define F11_SEC	22
#define F12_SEC	23

#define SIZE(array)	(sizeof(array)/sizeof(array[0]))

void fill(char *buf, int num, int ch);

char *FormatDate(long ticks);
char *FormatTime(long ticks);
char *FormatAmount(int cnt, char *itemname);

class Screen; // forward declaration

//---------------------------------------------------------------
// Simple text screen `widgets'
    
class TextLabel
{
    int		    r;
    int		    c;
    char *	    text;
public:
    TextLabel();
    void Init(int r_in, int c_in, char * text_in = 0);
    int Initialised();
    TextLabel(int r_in, int c_in, char * text_in);
    void Paint();
    char *Text();
    void Position(int &r_out, int &c_out);
    ~TextLabel();
};

class Field
{
protected:
    int		    r;
    int		    c;
    int		    w;
    virtual int  IsValid();
public:
    void Init(int r_in, int c_in, int w_in);
    int Initialised();
    Field(int r_in = -1, int c_in = -1, int w_in = -1);
    int Row();
    int Col();
    int Width();
    virtual ~Field();
    virtual void Paint(int active = 0) = 0;
    virtual int  Run() = 0;
    virtual int  IsToggle() = 0;
};

class DataField : public Field
{
protected:
    char *	    value;	// current value
    int		    echo;	// should the value be echoed? For password.
    char *	    exitchars;	// characters which exit this field
    int		    pos;	// current position
    int		    len;	// current length
    unsigned	    cancelfkeys;// bitmask of fkeys that bypass validation
    virtual int  IsValid();
public:
    void Init(int r_in, int c_in, int w_in, unsigned cancelfkeys_in = 0,
		     char * exitchars_in =  "\t\n\r", int echo_in = 1,
		     char * value_in = 0);
    DataField();
    DataField(int r_in, int c_in, int w_in, unsigned cancelfkeys_in = 0,
		 char * exitchars_in =  "\t\n\r", int echo_in = 1,
		 char * value_in = 0);
    char *Value();
    virtual ~DataField();
    virtual void Paint(int active = 0);
    virtual int  Run();
    virtual int  HandleKey(int ch);
    virtual int  IsToggle();
    void SetValue(char * val);
};

class ToggleField : public Field
{
protected:
    int		    cnt;
    int		    value;
    char **	    valueset;
    int		    nextchar;
    int		    prevchar;
    virtual int  IsValid();
public:
    void Init(int r_in, int c_in, int w_in,
		     char *valueset_in, int nextchar_in = KB_FKEY_CODE(3),
		     int prevchar_in = KB_FKEY_CODE(2));
    int Initialised();
    ToggleField();
    ToggleField(int r_in, int c_in, int w_in,
		 char *valueset_in, int nextchar_in = KB_FKEY_CODE(3),
		     int prevchar_in = KB_FKEY_CODE(2));
    virtual ~ToggleField();
    virtual void Paint(int active = 0);
    virtual int  Run();
    virtual int  IsToggle();
    virtual char *GetText();
    int Value();
    void SetValue(int val);
    virtual void NextValue();
    virtual void PrevValue();
};

class DateField: public DataField
{
    virtual int  IsValid();
public:
    DateField();
    DateField(int r_in, int c_in, int w_in, unsigned cancelfkeys_in = 0,
		 char * exitchars_in =  "\t\n\r", int echo_in = 1,
		 char * value_in = 0);
    virtual ~DateField();
};

class TimeField: public DataField
{
    virtual int  IsValid();
public:
    TimeField();
    TimeField(int r_in, int c_in, int w_in, unsigned cancelfkeys_in = 0,
		 char * exitchars_in =  "\t\n\r", int echo_in = 1,
		 char * value_in = 0);
    virtual ~TimeField();
};

class NumberField: public DataField
{
    int minv, maxv;
    virtual int  IsValid();
public:
    NumberField(int minv_in, int maxv_in);
    NumberField(int r_in, int c_in, int w_in, int minv_in, int maxv_in,
		 unsigned cancelfkeys_in = 0,
		 char * exitchars_in =  "\t\n\r", int echo_in = 1,
		 char * value_in = 0);
    virtual ~NumberField();
};

class FKeySet
{
    char   *(label[12][2][2]); // indexed by fkeynum, mode, line
    int	   on_if_empty[12][2];
    int	   mode;
    int    isempty;
    int	   nkeys;
    int	   nlines;
    int	   keywidth;
    int	   row;
    unsigned long mask[2];

    void InitModeKey(int idx, int modenum, char *info);
    void InitKey(int idx, char *info);
 public:
    FKeySet(int row, int nkeys, int nlines, int keywidth,
	    char *f1, char *f2, char *f3, char *f4,
	    char *f5=0, char *f6=0, char *f7=0, char *f8=0,
	    char *f9=0, char *f10=0, char *f11=0, char *f12=0);
    const void SetEmpty(int isempty_in);
    const int GetCommand(int ch); // returns -1 (no command), 0..7
	                          // (mode 0) or 8..15 (mode 1)
    void ToggleMode();
    void SetMode(int m);
    void Paint();
    void Enable(int modenum, int key, int setval = 1);
    void Disable(int modenum, int key);
    ~FKeySet();
};

class FKeySet8by2 : public FKeySet
{
 public:
    FKeySet8by2(char *f1, char *f2, char *f3, char *f4,
	    char *f5, char *f6, char *f7 = "  Help", char *f8 = "  Done")
	: FKeySet(GetLines()-2, 8, 2, 9, f1, f2, f3, f4, f5, f6, f7, f8)
    {
    }
};

class PopupPrompt
{
    FKeySet8by2 fkeys;
    DataField fld;
    char *help, *prompt;
    Screen *parent;
public:
    PopupPrompt(Screen *parent, char *prompt, char *help,
		char *f1lbl, char *f2lbl, char *f8lbl,
		int echo = 1);
    void Paint();
    int Run(char *buf, int rtn_on_enter = 1);
};
 
//-----------------------------------------------------------------
// Popup list boxes

class ListBoxPopup
{
protected:
    char	*prompt;
    char	**items;
    Screen	*owner;
    FKeySet     *fkeys;
    int		cnt;
    int		pos;
    int		row;
    int		col;
    int		width;
    int		maxitems;
    int		islast;
    int		pagenum;

    virtual void HandleKey(int ch, int &done, int &rtn);
    virtual void HandleFKey(int ch, int &done, int &rtn);
    char *PagePrompt();
    void Paint();
    void PageUp();
    void PageDown();
    virtual int NextPage() = 0;
    virtual int PrevPage() = 0;
    virtual void GetItems() = 0;
public:
    ListBoxPopup(Screen *owner_in, int row_in, int col_in,
		 int width_in, char *prompt_in, int maxitems_in);
    int Run();
    virtual ~ListBoxPopup();
};

//---------------------------------------------------------------

class Menu 
{
    int		    pos;
    int		    selection;
    int		    maxentries;
    int		    entrycnt;
    TextLabel	    *entries;
 public:
    Menu(int maxentries_in);
    void AddEntry(int num, int r, int c, char *text);
    void DelEntry(int num);
    char *GetEntry(int num = -1);
    void Up(int paint = 1);
    void Down(int paint = 1);
    int Current();
    void SetCurrent(int pos_in);
    int NumEntries();
    void Paint(int selected = -1); // selected used when re-ordering
    int Selection();
    void ClearSelection();
    void StartSelection();
    void GetSelection(int &first, int &last);
    ~Menu();
};

class Screen
{
protected:
    char *	    title;
    char *	    prompt;
    int		    numlabels;
    FKeySet	    *fkeys;
    TextLabel	    *labels;
    Screen	    *parent;
    void SetTitle(char *s);
    int AddLabel(int r_in, int c_in, char *text_in);
    void SetLabel(int l_in, int r_in, int c_in, char *text_in);
    void ToggleFKeys();
    int Confirm(char *title_in, char *prompt_in, char *f1lbl, char *f8lbl);
    virtual Screen *HandleKey(int ch, int &done);
    virtual Screen *HandleFKey(int ch, int &done) = 0;
    //friend class FilePopup;
public:
    char *GetPrompt();
    Screen(Screen *parent_in, char * title_in, char * prompt_in,
	   int nlabels_in=0);
    virtual void DrawTitle(char *prompt = 0);
    void ShellOut();
    void Help(char *entry);
    virtual void Paint();
    virtual int Run();
    virtual void RefreshContents();
    virtual ~Screen();
};

class FormScreen : public Screen
{
    int		    nextchar;
    int		    prevchar;
protected:
    int		    fnow;
    int		    canedit;
    int		    numfields;
    Field	    **fields;
    int AddToggle(int r_in, int c_in, int w_in, char *valueset_in);
    virtual Screen *HandleKey(int ch, int &done);
    virtual Screen *HandleFKey(int ch, int &done) = 0;
    void LeftField();
    void RightField();
    void UpField();
    void DownField();
    int AddField(Field *fld);
    void LoadMultiField(int ffnum, int nf, char *val);
    char *GetMultiField(int ffnum, int nf);
public:
    FormScreen(Screen *parent_in, char * title_in, char * prompt_in,
	   int nlabels_in=0, int nfields_in=0, 
	   int nextchar_in = 0, int prevchar_in = 0, int canedit_in = 1);
    virtual void DrawTitle(char *prompt = 0);
    virtual void Paint();
    virtual int Run();
    virtual ~FormScreen();
};

class PageableScreen : public FormScreen
{
protected:
    int		    msglines; // number of items/lines per page
    int		    pagenum;  // current page number (from 0)
    int		    is_lastpage;
    virtual void GetItems() = 0;
    virtual int PrevPage() = 0;
    virtual int NextPage() = 0;
    void PageUp();
    void PageDown();
    void ShowPagePrompt(int pg, int islast);
    virtual Screen *HandleKey(int ch, int &done);
    virtual Screen *HandleFKey(int ch, int &done) = 0;
public:
    PageableScreen(Screen *parent_in, char *title_in, char *prompt_in,
		   int nlabels_in, int nfields_in, int msglines_in);
    virtual void DrawTitle(char *prompt = 0);
    virtual void Paint();
    virtual int Run();
    virtual ~PageableScreen();    
    virtual void RefreshContents();
};

class TextEditor;

class EditScreen : public Screen
{
    TextEditor *editor;
    int filechanged;
 protected:
    virtual Screen *HandleKey(int ch, int &done);
    virtual Screen *HandleFKey(int fkeynum, int &done);
public:    
    EditScreen(Screen *parent_in, char *fname_in);
    virtual void Paint();
    int HasFileChanged();
    virtual ~EditScreen();
    TextEditor *Editor();
};

class EditWin
{
    TextEditor *editor;
    int rr, cc, w, h, hasborder;
public:
    EditWin(int row, int col, int width, int height, char *fname = 0,
		int border = 1);
    int Load(char *fname);
    void Save(char *fname = 0);
    int HandleKey(int ch);
    void Paint(int paintall = 0);
    ~EditWin();
};

class MultiEditScreen : public Screen
{
 protected:
    EditWin **window; // crude for now
    int numwins;
    int winnow; // window with keyboard focus
    virtual Screen *HandleKey(int ch, int &done);
    virtual Screen *HandleFKey(int fkeynum, int &done);
public:    
    MultiEditScreen(Screen *parent_in, int nfiles, char **files);
    virtual void Paint();
    virtual ~MultiEditScreen();
};

#ifndef INLINE
#ifdef NO_INLINES
#define INLINE
#else
#define INLINE inline
#endif
#endif

#ifndef NO_INLINES
#include "screen.inl"
#endif

#endif
