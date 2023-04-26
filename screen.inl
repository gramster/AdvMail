#ifndef _SCREEN_INL
#define _SCREEN_INL
//---------------------------------------------------------------------
// inlines

INLINE const void FKeySet::SetEmpty(int isempty_in)
{
    isempty = isempty_in;
    if (isempty) mode = 0; // enforce sanity
    Paint();
}

INLINE void FKeySet::ToggleMode()
{
    mode = 1 - mode;
    Paint();
}

INLINE void FKeySet::SetMode(int m)
{
    mode = m;
    Paint();
}

INLINE void TextLabel::Init(int r_in, int c_in, char * text_in)
{
    r = r_in;
    c = c_in;
    delete [] text;
    if (text_in)
    {
        text = new char [strlen(text_in)+1];
	if (text) strcpy(text, text_in);
    } else text = 0;
}

INLINE int TextLabel::Initialised()
{
    return (r>=0);
}

INLINE TextLabel::TextLabel()
	: text(0)
{
    ConstructTrace("TextLabel");
    Init(-1, -1, NULL);
}

INLINE TextLabel::TextLabel(int r_in, int c_in, char * text_in)
	: text(0)
{
    ConstructTrace("TextLabel");
    Init(r_in, c_in, text_in);
} 

INLINE void TextLabel::Paint()
{
    if (r>=0) PutString(r, c, text);
}

INLINE char *TextLabel::Text()
{
    return text;
}

INLINE void TextLabel::Position(int &r_out, int &c_out)
{
    r_out = r;
    c_out = c;
}

INLINE TextLabel::~TextLabel()
{
    delete [] text;
    DestructTrace("TextLabel");
}

//-------------------------------------------------------------------------

INLINE void Field::Init(int r_in, int c_in, int w_in)
{
    r = r_in;
    c = c_in;
    w = w_in;
}

INLINE Field::Field(int r_in, int c_in, int w_in)
    : r(r_in), c(c_in), w(w_in)
{
    ConstructTrace("Field");
}

INLINE int Field::Initialised()
{
    return (r>=0);
}

INLINE int Field::Row()
{
    return r;
}

INLINE int Field::Col()
{
    return c;
}

INLINE int Field::Width()
{
    return w;
}

INLINE int Field::IsValid()
{
    return 1;
}

//-----------------------------------------------------------------------

INLINE void DataField::SetValue(char *val)
{
    delete [] value;
    value = new char[strlen(val)+1];
    if (value)
	strcpy(value, val);
}

INLINE void DataField::Init(int r_in, int c_in, int w_in,
			    unsigned cancelfkeys_in, char * exitchars_in,
			    int echo_in, char * value_in)
{
    Field::Init(r_in, c_in, w_in);
    echo = echo_in;
    cancelfkeys = cancelfkeys_in;
    if (exitchars_in)
    {
	exitchars = new char[strlen(exitchars_in)+1];
	if (exitchars)
	    strcpy(exitchars, exitchars_in);
    }
    else exitchars = 0;
    if (value_in)
    {
    	value = new char[strlen(value_in)+1];
	if (value)
	    strcpy(value, value_in);
    }
    else value = 0;
}

INLINE DataField::DataField()
	: Field()
{
    ConstructTrace("DataField");
    Init(-1, -1, -1, 0);
}

INLINE DataField::DataField(int r_in, int c_in, int w_in,
			    unsigned cancelfkeys_in, char * exitchars_in,
			    int echo_in, char * value_in)
	: Field()
{
    ConstructTrace("DataField");
    Init(r_in, c_in, w_in, cancelfkeys_in, exitchars_in, echo_in, value_in);
} 

INLINE char *DataField::Value() 
{
    return value;
}

INLINE DateField::DateField()
    : DataField()
{
    ConstructTrace("DateField");
}

INLINE DateField::DateField(int r_in, int c_in, int w_in,
			    unsigned cancelfkeys_in, char * exitchars_in,
			    int echo_in, char *value_in)
    : DataField(r_in,c_in, w_in, cancelfkeys_in, exitchars_in, echo_in, value_in)
{
    ConstructTrace("DateField");
}

INLINE TimeField::TimeField()
    : DataField()
{
    ConstructTrace("TimeField");
}

INLINE TimeField::TimeField(int r_in, int c_in, int w_in,
			unsigned cancelfkeys_in, char * exitchars_in,
			int echo_in, char *value_in)
    : DataField(r_in,c_in, w_in, cancelfkeys_in, exitchars_in, echo_in, value_in)
{
    ConstructTrace("TimeField");
}

INLINE NumberField::NumberField(int minv_in, int maxv_in)
    : DataField(), minv(minv_in), maxv(maxv_in)
{
    ConstructTrace("NumberField");
}

INLINE NumberField::NumberField(int r_in, int c_in, int w_in, int minv_in,
				int maxv_in, unsigned cancelfkeys_in,
		 		char * exitchars_in, int echo_in,
				char *value_in)
    : DataField(r_in,c_in, w_in, cancelfkeys_in, exitchars_in, echo_in, value_in),
	minv(minv_in), maxv(maxv_in)
{
    ConstructTrace("NumberField");
}

//---------------------------------------------------------------------

INLINE ToggleField::ToggleField()
    : Field(), valueset(0), cnt(0)
{
    ConstructTrace("ToggleField");
    Init(-1,-1,-1,0,0,0);
}

INLINE ToggleField::ToggleField(int r_in, int c_in, int w_in,
		 char *valueset_in, int nextchar_in, int prevchar_in)
    : Field(), valueset(0), cnt(0)
{
    ConstructTrace("ToggleField");
    Init(r_in, c_in, w_in, valueset_in, nextchar_in, prevchar_in);
}

INLINE void ToggleField::NextValue()
{
    value = (value+1)%cnt;
}

INLINE void ToggleField::PrevValue()
{
    value = (value+cnt-1)%cnt;
}

INLINE int ToggleField::Value()
{
    return value;
}

INLINE void ToggleField::SetValue(int val)
{
    value = val;
}

//--------------------------------------------------------------------

INLINE Menu::Menu(int maxentries_in)
    : maxentries(maxentries_in), pos(0), entrycnt(0), selection(-1)
{
    ConstructTrace("Menu");
    entries = new TextLabel[maxentries];
    assert(entries);
}

INLINE void Menu::AddEntry(int num, int r, int c, char *text)
{
    assert(num>=0 && num<maxentries);
    if (!entries[num].Initialised())
        entrycnt++;
    entries[num].Init(r,c,text);
}

INLINE void Menu::DelEntry(int num)
{
    assert(num>=0 && num<maxentries);
    if (entries[num].Initialised())
    {
	if (pos == num && pos > 0) --pos;
    	entries[num].Init(-1,-1,0);
	entrycnt--;
    }
}

INLINE char *Menu::GetEntry(int num)
{
    if (num<0) num = pos;
    return entries[num].Text();    
}

INLINE int Menu::NumEntries()
{
    return entrycnt;
}

INLINE int Menu::Current()
{
    return pos;
}

INLINE void Menu::SetCurrent(int pos_in)
{
    if (pos_in < entrycnt)
	pos = pos_in;
    else
	pos = entrycnt-1;
}

INLINE int Menu::Selection()
{
    return selection;
}

INLINE void Menu::ClearSelection()
{
    selection = -1;
}

INLINE void Menu::StartSelection()
{
    selection = pos;
}

INLINE Menu::~Menu()
{
    delete [] entries;
    DestructTrace("Menu");
}

//--------------------------------------------------------------

INLINE void Screen::SetTitle(char *s)
{
    title = s;
}

INLINE char *Screen::GetPrompt()
{
    return prompt;
}

INLINE TextEditor *EditScreen::Editor()
{
    return editor;
}

#endif

