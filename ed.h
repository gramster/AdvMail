// A simple set of classes for text editing
// (c) 1994 by Graham Wheeler
// All Rights Reserved
//
// These classes separate the editor functionality from user interface
// concerns, with the exception of translation of special keys. They have
// been used to build text editors under MS-DOS, Windows 3.1, and UNIX
// systems. Originally written for the SDL*Design tool.
//

#ifndef __ED_H
#define __ED_H

//-------------------------------------------------------------------
// Each line in an edit buffer is held in a TextLine

class TextLine	// line in a buffer
{
    static int newID;
    int id;
    int length;
    char *text;
    TextLine *next, *prev;
public:
    void Text(char *text_in, int length_in = 0);
    TextLine(char *text_in = 0, TextLine *prev_in = 0,
	     TextLine *next_in = 0, int length_in = 0)
    {
	text = 0;
	prev = prev_in;
	next = next_in;
	Text(text_in, length_in);
    }
    ~TextLine()
    {
	delete [] text;
    }
    void Next(TextLine *next_in)
    {
	next = next_in;
    }
    void Prev(TextLine *prev_in)
    {
	prev = prev_in;
    }
    TextLine *Next()
    {
	return next;
    }
    TextLine *Prev()
    {
	return prev;
    }
    int ID()
    {
	return id;
    }
    char *Text()
    {
	return text;
    }
    int Length()
    {
	return length;
    }
    void NewID()
    {
	id = newID++;
    }
    void Dump();
};

//--------------------------------------------------------------
// The file being edited is held within a doubly-linked list
// of TextLines, managed by a TextBuffer

class TextBuffer // buffer manager
{
    TextLine root;
    long filesize;
    int numlines;
    int haschanged;
    char *filename;
public:
    TextBuffer(char *fname = 0);
    void Clear();
    ~TextBuffer();
    TextLine *First()
    {
	return root.Next();
    }
    TextLine *Last()
    {
	return root.Prev();
    }
    TextLine *Next(TextLine *now)
    {
	if (now->Next() == &root)
	    return 0;
	else
	    return now->Next();
    }
    TextLine *Prev(TextLine *now)
    {
	if (now->Prev() == &root)
	    return 0;
	else
	    return now->Prev();
    }
    void Insert(TextLine *now, char *text, int len = 0);
    void Append(TextLine *now, char *text, int len = 0);
    void Delete(TextLine *now);
    void Replace(TextLine *now, char *text, int length = 0);
    long Size()
    {
	return filesize;
    }
    int Lines()
    {
	return numlines;
    }
    int Changed()
    {
	return haschanged;
    }
    int Load(char *fname);
    void Save();
    char *Name()
    {
	return filename;
    }
    void Dump();
};

//------------------------------------------------------------------
// The textbuffers are managed by a tbufmanager, which ensures
// that two tbufs for the same file never exist together.

#define MAX_TBUFS	32

class TBufManager
{
    TextBuffer	*tbufs[MAX_TBUFS];
    int		refs[MAX_TBUFS];
public:
    TBufManager();
    TextBuffer *GetTextBuffer(char *fname);
    int ReleaseTextBuffer(char *fname);
    ~TBufManager();
};

//------------------------------------------------------------------
// Each editor window maintains a pointer to a TextBuffer for the
// file being edited, and a TextLine within that.

#define MAX_HEIGHT	64

class TextEditor
{
    TextBuffer     *buffer;
    TextLine       *line, *fetchline;
    int		linenow,
                colnow,
                markline,
		markcol,
		marktype,
		prefcol,
		width,	// width of window
		height,	// height of window
		first,	// first line displayed in window
		left,	// leftmost column displayed in window
		lastleft,
		fetchnum,
		mustredo,
                cursormoved,
		autowrap,
		tabsize,
		readonly,
                IDs[MAX_HEIGHT];

public:
    int Length()
    {
	return line->Length();
    }
    char *Text()
    {
	return line->Text();
    }
    int Lines()
    {
	return buffer->Lines();
    }
    long Size()
    {
	return buffer->Size();
    }
    char *Name()
    {
        return buffer->Name();
    }
    int Changed()
    {
	return buffer->Changed();
    }
    void Replace(char *txt, int len = 0)
    {
	if (!readonly) buffer->Replace(line, txt, len);
    }
    void Append(char *txt, int len = 0)
    {
	if (!readonly) buffer->Append(line, txt, len);
    }
    void Insert(char *txt, int len = 0)
    {
	if (!readonly) buffer->Insert(line, txt, len);
    }
    int Line()
    {
	return linenow;
    }
    int Column()
    {
	return colnow;
    }
    void SetWrap(int v)
    {
	autowrap = v;
    }
    void SetTabs(int tabstop)
    {
	tabsize = tabstop;
    }
    void Point(int ln, int canUndo = 1);
    void SetCursor(int ln, int col);
    
    // We provide both relative and absolute methods; this may be a
    // possible optimisation for efficiency later.
	    
    void GotoLineAbs(int ln, int canUndo = 1);
    void GotoLineRel(int cnt, int canUndo = 1)
    {
	GotoLineAbs(Line() + cnt, canUndo);
    }
    void GotoColAbs(int col);
    void GotoColRel(int cnt)
    {
	GotoColAbs(Column() + cnt);
    }
    int AtStart()
    {
	return Line()==0 && Column()==0;
    }
    int AtEnd()
    {
	return Line()==(Lines()-1) && Column() == Length();
    }
    void Up()
    {
	GotoLineRel(-1);
    }
    void Down()
    {
	GotoLineRel(+1);
    }
    void Left()
    {
	GotoColRel(-1);
    }
    void LeftWord();
    void Right()
    {
	GotoColRel(+1);
    }
    void RightWord();
    void Home()
    {
	if (Column() == 0)
	    if (Line() == first)
		SetCursor(0,0); // go to top of file
	    else
		GotoLineAbs(first);
	else
	    GotoColAbs(0);
    }		
    void End	()
    {
	GotoColAbs(line->Length());
    }
    void PageUp()
    {
	if (Line() != first)
	    GotoLineAbs(first);
	else GotoLineRel(- height + 1);
    }
    void PageDown()
    {
	if (Line() != (first + height - 1))
	    GotoLineAbs(first + height - 1);
	else GotoLineRel(height-1);
    }
    void SetMark(int typ = 0)
    {
	markline = linenow;
	markcol = colnow;
	marktype = typ;
    }
    void GotoLine(int ln)
    {
	SetCursor(ln, prefcol);
    }
    void GotoByteAbs(long offset);
    void Tab();
    void InsertCR();
    void InsertChar(char c);
    void InsertStr(char *str);
    void Join();
    void CutCols(int startcol, int endcol, int canUndo = 1);
    void DeleteBlock();
    void Backspace();
    void Delete();
    void DeleteLine();
    void Undo();
    void Redo();
    void DisplaySize(int lns, int cols)
    {
	height = lns;
	width = cols;
    }
    void Prepare(int redoall = 0);
    char *Fetch(int &cursorpos);
    int CursorMoved()
    {
	return cursormoved;
    }
    TextEditor(char *fname, int width_in, int height_in, int readonly = 0);
    ~TextEditor();
    void Save()
    {
	if (!readonly) buffer->Save();
    }
    void Load(char *fname)
    {
	buffer->Load(fname);
    }
    int  HandleKey(int ke);
    void Dump();
    friend class EditScreen;
    friend class HyperText;
};

#endif /* __ED_H */


