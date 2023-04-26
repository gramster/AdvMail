// A simple set of classes for text editing
// (c) 1994 by Graham Wheeler
// All Rights Reserved
//
// These classes separate the editor functionality from user interface
// concerns, with the exception of translation of special keys. They have
// been used to build text editors under MS-DOS, Windows 3.1, and UNIX
// systems. Originally written for the SDL*Design tool.
//
// This version has been pruned down. Multi-window support, block
// operations, language syntax formatting (Windows only), and undo/
// redo have been removed.
//
// TODO: replace the asserts with better error handling

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#if __MSDOS__
#include <io.h>
#else
#include <unistd.h>
#endif
#include <assert.h>

#include "debug.h"
#include "ansicurs.h"
#include "ed.h"

#ifndef TRUE
#define TRUE	1
#define FALSE	0
#endif

#ifndef max
#define min(a, b)	( ((a) < (b)) ? (a) : (b) )
#define max(a, b)	( ((a) > (b)) ? (a) : (b) )
#endif

//------------------------------------------------------------------
// option flags

int makebackups = 0;	// if this flag is set .BAK files are made upon saving
int softcursor = 1;	// indicates that the cursor is software-based and thus
			// cursor movement requires screen updating. It should
			// be zero only when hardware-based cursors are in use
int fancytext = 1;	// use bold font to distinguish reserved words, etc

//------------------------------------------------------------------
// Implicit `pad' characters at end of line

#if __MSDOS__
#define PAD	2		// CR+LF
#else
#define PAD	1		// LF
#endif

//-------------------------------------------------------------------
// Each line in an edit buffer is held in a TextLine

int TextLine::newID = 1;

void TextLine::Dump()
{
    fprintf(stderr, "  %3d %2d %s\n", id, length, text);
}

void TextLine::Text(char *text_in, int length_in)
{
    delete [] text;
    if (text_in == 0)
    {
	text = new char[1];
	assert(text);
	text[0] = 0;
	length = 0;
    }
    else
    {
	text = text_in;
	length = length_in;
	if (text && text[0] && length_in == 0)
	    length = strlen(text);
    }
    NewID();
}

//--------------------------------------------------------------
// Each file being edited is held within a doubly-linked list
// of TextLines, managed by a TextBuffer
// Note: Undo must be implemented here (or at least via here).
// Undo is per-file, not per-window. The current implementation
// is not very undo-friendly - an audit-trail mechanism will
// have to be done independently, giving the cursor positions
// and keys necessary to do the undo's.

TextBuffer::TextBuffer(char *fname)
	: root(0, &root, &root), filename(0)
{
    ConstructTrace("TextBuffer");
#if __MSDOS__
    if (fname == 0) fname ="NONAME.XXX";
#else
    assert(fname);
#endif
    if (Load(fname)<0)
    {
	// create an empty line
	TextLine *first = new TextLine(0, &root, &root);
	assert(first);
	root.Next(first);
	root.Prev(first);
        numlines = 1;
    }
}

void TextBuffer::Clear()
{
    TextLine *ln = root.Next();
    while (ln && ln != &root) // check against root needed for constructor
    {
	TextLine *tmp = ln;
	ln = Next(ln);
	Delete(tmp);
    }
    delete [] filename;
    filename = 0;
    filesize = 0l;
    numlines = 0;
    haschanged = 0;
}

TextBuffer::~TextBuffer()
{
    Clear();
    DestructTrace("TextBuffer");
}

int TextBuffer::Load(char *fname)
{
    assert(fname);
    Clear();
    FILE *fp = fopen(fname, "r");
    filename = new char[strlen(fname)+1];
    assert(filename);
    strcpy(filename, fname);
    if (fp)
    {
	char buff[256], buff2[256];
	while (!feof(fp))
	{
	    buff[0] = '\0';
	    if (fgets(buff, 256, fp) == 0) break;
	    int l = strlen(buff);
	    // Strip newline
	    if (l > 0 && buff[l-1]=='\n') buff[--l] = '\0';
	    // Expand tabs
	    for (int i = 0, j = 0; i < l; i++)
	    {
		if (buff[i] == '\t')
		{
    		    int c = 8 - ( j % 8);
		    while (c--) buff2[j++] = ' ';
		}
		else buff2[j++] = buff[i];
	    }
	    buff2[j] = 0;
	    char *newtext = new char[j+1];
	    assert(newtext);
	    strcpy(newtext, buff2);
	    Append(Last(), newtext, j);
	}
	fclose(fp);
        filesize-=PAD; // undo last newline count
	return 0;
    }
    return -1;
}

void TextBuffer::Save()
{
    if (makebackups)
    {
	if (access(filename,0)==0)
	{
#if _MSDOS__
	    char backupname[80], drive[MAXDRIVE], directory[MAXDIR];
	    char basename[MAXFILE], extension[MAXEXT];
	    (void)fnsplit(filename,drive,directory,basename,extension);
	    fnmerge(backupname,drive,directory,basename,".BAK");
	    (void)rename(filename,backupname);
#else
	    // must still add this
#endif
	}
    }
    FILE *fp = fopen(filename, "w");
    if (fp)
    {
	TextLine *ln = root.Next();
	fprintf(fp, "%s", ln->Text());
	ln = Next(ln);
	while (ln)
	{
	    fprintf(fp, "\n%s", ln->Text());
	    ln = Next(ln);
	}
	fclose(fp);
    }
}

void TextBuffer::Insert(TextLine *now, char *text, int len)
{
    TextLine *nt = new TextLine(text, now->Prev(), now, len);
    assert(nt);
    nt->Prev()->Next(nt);
    now->Prev(nt);
    filesize += nt->Length() + PAD;
    numlines++;
    haschanged = 1;
}

void TextBuffer::Append(TextLine *now, char *text, int len)
{
    TextLine *nt = new TextLine(text, now, now->Next(), len);
    assert(nt);
    nt->Next()->Prev(nt);
    now->Next(nt);
    filesize += nt->Length() + PAD;
    numlines++;
    haschanged = 1;
}

void TextBuffer::Delete(TextLine *now)
{
    now->Prev()->Next(now->Next());
    now->Next()->Prev(now->Prev());
    filesize -= now->Length() + PAD;
    delete now;
    numlines--;
    haschanged = 1;
}

void TextBuffer::Replace(TextLine *now, char *text, int length)
{
    if (text && length == 0)
	length = strlen(text);
    filesize += length - now->Length();
    now->Text(text, length);
    haschanged = 1;
}

void TextBuffer::Dump()
{
    TextLine *f = First();
    while (f)
    {
	f->Dump();
	f = Next(f);
    }
}

//------------------------------------------------------------------
// The textbuffers are managed by a tbufmanager, which ensures
// that two tbufs for the same file never exist together.

TBufManager::TBufManager()
{
    ConstructTrace("TBufManager");
    for (int i = 0; i < MAX_TBUFS; i++)
    {
	tbufs[i] = 0;
	refs[i] = 0;
    }
}

TextBuffer *TBufManager::GetTextBuffer(char *fname)
{
    for (int i = 0; i < MAX_TBUFS; i++)
	if (tbufs[i] && strcmp(tbufs[i]->Name(), fname)==0)
	{
	    refs[i]++;
	    return tbufs[i];
	}
    for (i = 0; i < MAX_TBUFS; i++)
	if (tbufs[i] == 0)
	{
	    refs[i] = 1;
	    tbufs[i] = new TextBuffer(fname);
	    assert(tbufs[i]);
	    return tbufs[i];
	}
    return 0; // failed
}

int TBufManager::ReleaseTextBuffer(char *fname)
{
    for (int i = 0; i < MAX_TBUFS; i++)
	if (tbufs[i] && strcmp(tbufs[i]->Name(), fname)==0)
	{
	    if (--refs[i] == 0)
	    {
		tbufs[i] = 0;
		return 1;
	    }
	    break;
	}
    return 0;
}

TBufManager::~TBufManager()
{
    DestructTrace("TBufManager");
}

TBufManager buffermanager;

//------------------------------------------------------------------
// Each editor window maintains a pointer to a TextBuffer for the
// file being edited, and a TextLine within that.

TextEditor::TextEditor(char *fname, int width_in, int height_in,
				int readonly_in)
    : buffer(0), line(0), fetchline(0), linenow(0), colnow(0),
      markline(0), markcol(0), marktype(0), prefcol(0),
      width(width_in), height(height_in), first(0),
      left(0), lastleft(0), fetchnum(0), mustredo(0),
      cursormoved(0), autowrap(0), tabsize(8),
      readonly(readonly_in)
{
    // attach to existing TextBuffer if there is one
    ConstructTrace("TextEditor");
    buffer = buffermanager.GetTextBuffer(fname);
    line = buffer->First();
    for (int i = 0; i < MAX_HEIGHT; i++)
	IDs[i] = 0;
}

TextEditor::~TextEditor()
{
    if (buffermanager.ReleaseTextBuffer(buffer->Name())) // last instance?
    {
	if (!readonly && buffer->Changed())
	{
	    // Ask if we must save. This has been removed as it is
	    // now considered the responsibility of the application 
	    // to do this before deleting the TextEditor. 
#ifdef TEST
	    // hax for now
	    printf("File %s has been changed. Save? (y/n) ",
		   buffer->Name());
	    fflush(stdout);
	    for (;;)
	    {
		char c = getchar();
		if (c=='Y' || c=='y')
		{
		    Save();
		    break;
		}
		if (c=='N' || c=='n')
		    break;
	    }
#endif
	}
	delete buffer;
    }
    DestructTrace("TextEditor");
}

//------------------------------------------------------------------
// Cursor movement around file

void TextEditor::Point(int ln, int canUndo) // move within list
{
    (void) canUndo;
    if (ln > linenow)
	while (ln != linenow)
	{
	    line = line->Next();
	    linenow++;
	}
    else
	while (ln != linenow)
	{
	    line = line->Prev();
	    linenow--;
	}
}

void TextEditor::SetCursor(int ln, int col) // normalise location and view
{
    cursormoved = 0;
    ln = max(0, min(ln, Lines()-1));
    if (ln != Line())
    {
	if (softcursor) line->NewID();
	Point(ln);
	if (softcursor) line->NewID();
	cursormoved = 1;
    }
    col = max(0, min(col, Length()));
    if (col != colnow)
    {
	colnow = col;
	if (softcursor) line->NewID();
	cursormoved = 1;
    }
    
    // make sure window view includes line
    
    if (Line() < first)
	first = Line();
    else if (Line() >= (first + height))
	first = Line() - height + 1;
    
    // make sure column is in view
    
    if (Column() < left)
	left = Column();
    else if (Column() >= (left + width))
	left = Column() - width + 1;
}

void TextEditor::GotoLineAbs(int ln, int canUndo)
{
    (void)canUndo;
    SetCursor(ln, prefcol);
}

void TextEditor::GotoByteAbs(long offset)
{
    SetCursor(0, 0);
    while (offset > 0l && Line() < Lines())
    {
	int len = Length();
	if (offset <= len)
	    GotoColAbs(offset);
	else
	    Down();
	offset -= len+PAD;
    }
}

void TextEditor::GotoColAbs(int col)
{
    if (col<0 && Line()>0L)
    {
	GotoLineRel(-1, 1);
	col = Length();
    }
    else if (col>Length() && Line()<(Lines()-1))
    {
	GotoLineRel(+1, 1);
	col = 0;
    }
    SetCursor(Line(), prefcol=col);
}

#define is_letter(c)	((c >= 'a' && c <='z') || (c>='A' && c<='Z'))
#define is_digit(c)	((c) >= '0' && (c) <= '9')

void TextEditor::LeftWord()
{
    char c;
    Left();
    do
    {
	Left();
	c = Text()[Column()];
    }
    while (is_letter(c) && !AtStart());
    if (!AtStart())
	Right();
}

void TextEditor::RightWord()
{
    char c;
    Right();
    while (c = Text()[Column()], is_letter(c) && !AtEnd())
	Right();
}

//-----------------------------------------------------------
// Character insertion

void TextEditor::InsertChar(char c)
{
    if (readonly) return;
    if (autowrap && Column()>(width-5) && c != '\n')
    {
	if (c == ' ')
	{
	    InsertCR();
	    return;
	}
	else
	{
	    int cnt = 0;
	    // Move back to last space before the wrap column
	    while (Column()>(width-5))
	    {
		Left();
		cnt++;
	    }
	    while (Column()>0 && Text()[Column()] != ' ')
	    {
		Left();
		cnt++;
	    }
	    if (Column()>0)
	    {
		Delete();
	        InsertCR();
	    }
	    // restore cursor
	    while (--cnt > 0) Right();
	}
    }
    int oldlen = Length();
    char *oldtext = Text();
    char *s = new char [oldlen+2];
    assert(s);
    strncpy(s, oldtext, (unsigned)Column());
    s[Column()]=c;
    strncpy(s+Column()+1, oldtext+Column(),(unsigned)(oldlen-Column()));
    s[oldlen+1] = 0;
    Replace(s, oldlen+1);
    Right();
}

void TextEditor::Tab()	// make spaces for now
{
    if (readonly) return;
    int i = tabsize - ( Column() % tabsize);
    while (i--)
	InsertChar(' ');
}

void TextEditor::InsertCR() // splits a line at current column
{
    if (readonly) return;
    char *oldtext = Text();
    int tlen = Length() - Column();
    assert(tlen >= 0);
    char *newtext1 = new char[Column()+1];
    assert(newtext1);
    strncpy(newtext1, oldtext, Column());
    newtext1[Column()] = 0;
    char *newtext2 = new char[tlen+1];
    assert(newtext2);
    strncpy(newtext2, oldtext+Column(), tlen);
    newtext2[tlen] = 0;
    Replace(newtext1, Column());
    Append(newtext2, tlen);
    Down();
    GotoColAbs(0);
}

void TextEditor::InsertStr(char *str)
{
    if (readonly) return;
    int slen = strlen(str);
    int len = Length() + slen;
    char *newtext = new char[len+1];
    assert(newtext);
    strncpy(newtext, Text(), Column());
    strcpy(newtext+Column(), str);
    strcpy(newtext+Column()+slen, Text()+Column());
    Replace(newtext, len);
}

//-------------------------------------------------------------------
// Character deletion

void TextEditor::Join() // deletion of a newline
{
    if (readonly) return;
    if (Line() > 0)
    {
	char *txt2 = Text();
	int len = Length(), len2;
	Up();
	len += (len2 = Length());
	char *newtext = new char[len+1];
	assert(newtext);
	strcpy(newtext, Text());
	strcat(newtext, txt2);
	Replace(newtext, len);
	buffer->Delete(line->Next());
	SetCursor(Line(), prefcol = len2);
    }
}

void TextEditor::CutCols(int startcol, int endcol, int cutToClipboard)
{
    if (readonly) return;
    if (startcol>=Length())
    {
//	if (cutToClipboard) CopyCols(0,0); /* Create an empty scrap line */
	return;
    }
    if (endcol==(-1) || endcol>=Length())
	endcol=Length()-1;
    int len = endcol-startcol+1;
//  if (cutToClipboard) CopyCols(startcol,endcol);
    char *newtext = new char[Length() - len + 1];
    assert(newtext);
    strncpy(newtext, Text(), (unsigned)startcol);
    strncpy(newtext+startcol, Text()+endcol+1, (unsigned)(Length()-endcol-1));
    newtext[Length()-len] = 0;
    Replace(newtext, Length() - len);
    GotoColAbs(startcol);
}

void TextEditor::DeleteLine()
{
    if (readonly) return;
    int prefline = Line();
    if (Lines() > 1)
    {
	TextLine *tmp = line;
	// move to a different line before deleting current
	if (Line() == 0)
	{
	    Down();
	    // kludge, else SetCursor gets confused. This should be done
	    // after the buffer->Delete below, but may as well be done here
	    linenow = 0;
	}
	else Up();
	// and delete old line
	buffer->Delete(tmp);
    }
    else // only line in file; just empty it
    {
	buffer->Replace(line, 0, 0);
	colnow = 0;
    }
    SetCursor(prefline, prefcol = colnow);
}

void TextEditor::DeleteBlock()
{
    if (Column() < Length())
	CutCols(Column(), Column(), 0);
}

void TextEditor::Backspace()
{
    if (Column() == 0)
	Join();
    else
	CutCols(Column()-1,Column()-1, 0);
}

void TextEditor::Delete()
{
    if (Column() >= Length() && Line() < (Lines()-1)
		/* && MarkType==ED_NOMARK */)
    {
	Down();
	Join();
    }
    else DeleteBlock();
}

//---------------------------------------------------------------
// Must still be done...

void TextEditor::Undo()
{
}

void TextEditor::Redo()
{
}

//---------------------------------------------------------------
// hooks to get the stuff to display

void TextEditor::Prepare(int redoall)
{
    int cnt = Line() - first;
    fetchline = line;
    while (cnt-- > 0)
	fetchline = buffer->Prev(fetchline);
    fetchnum = 0;
    mustredo = (redoall || (lastleft != left));
    lastleft = left;
}

char *TextEditor::Fetch(int &cursorpos)
{
    char *rtn = "";
    cursorpos = -1;
    if (fetchline == 0)
    {
	if (mustredo || IDs[fetchnum])
	    IDs[fetchnum] = 0;
    }
    else
    {
	if (mustredo || IDs[fetchnum] != fetchline->ID())
	{
	    if (left < fetchline->Length())
	        rtn = fetchline->Text() + left;
	    IDs[fetchnum] = fetchline->ID();
	}
	fetchline = buffer->Next(fetchline);
    }
    if ((first + fetchnum) == Line())
	cursorpos = Column() - left;
    fetchnum++;
    return rtn;
}

//---------------------------------------------------------------

int TextEditor::HandleKey(int ke)
{
    SetCursor(Line(), Column());	// normalise; needed if editing
					// has occurred in other windows
    switch(ke)
    {
	// Hard-coded keys
    case '\t':			Tab();		break;
    case '\n':			InsertCR();	break;
    case KB_BACKSPACE:		Backspace();	break;
    case KB_UP:			Up();		break;
    case KB_DOWN:		Down();		break;
    case KB_LEFT:		Left();		break;
    case KB_RIGHT:		Right();	break;
    case KB_HOME:		Home();		break; // ^A
    case KB_END:		End();		break; // ^X
    case KB_PGUP:		PageUp();	break;
    case KB_PGDN:		PageDown();	break;
    case KB_DELETE:		Delete();	break;

    // These should be bindable

    case 1:			Home();		break; // ^A
    case 24:			End();		break; // ^X
    case 14:			RightWord();	break; // ^N
    case 16:			LeftWord();	break; // ^P
    case 25:			DeleteLine();	break; // ^Y

    default:
        if (isprint(ke))
	    InsertChar(ke);
//	else for (cmd=_LeftWord;cmd<_NumKeyDefs;cmd++)
//	    if (rtn == Key[(int)cmd])
//	    {
//		(*(Func[(int)(ed_op = cmd)]))();
//		break;
//	    }
    }
//Dump();
    return 0;
}

void TextEditor::Dump()
{
	fprintf(stderr, "line = %d   col = %d   prefcol = %d  first = %d  left = %d\n",
		linenow, colnow, prefcol, first, left);
	fprintf(stderr, "lines = %d   size = %ld   length = %d\n",
		Lines(), Size(), Length());
	fprintf(stderr, "text = %.*s@%s\n\nBuffer:\n", Column(), Text(),
		Text()+Column());
	buffer->Dump();
}


