// Base screen classes for AdvanceMail/Remote
// 
// Written by Graham Wheeler, June 1995.
// (c) 1995, Open Mind Solutions
// All Rights Reserved

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <assert.h>

#include "debug.h"
#include "ansicurs.h"
#include "mail.h"
#include "agent.h"
#include "config.h"
#include "screens0.h"
#include "hypertxt.h"

extern char *ServerError; // kludge to get this from omutil without #include

// Next two utilities are shared by the screen classes.

void OMDrawTitle(Screen *scr, char *prompt_in)
{
    scr->Screen::DrawTitle(prompt_in);
    char buf[COLS+1];
    int r, c;
    GetCursor(&r,&c);
    if (ServerError)
    {
	extern FILE *debugfp;
	if (debugfp) fprintf(debugfp, "Server error: %s\n", ServerError);
	if (strlen(ServerError)>COLS) ServerError[COLS]='\0';
    	strcpy(buf, ServerError);
	delete [] ServerError;
	ServerError = 0;
    	fill(buf, COLS, ' ');
	ReverseOn();
    	PutString(0,0,buf);
	ReverseOff();
    }
    if (mstore && mstore->NewMail()>0)
    {
        char *prmpt;
        if (prompt_in) prmpt = prompt_in;
        else prmpt = scr->GetPrompt();
	sprintf(buf, "New mail has arrived. %s", prmpt);
        fill(buf, COLS, ' ');
        ReverseOn();
        PutString(1,0,buf);
        ReverseOff();
    }
    SetCursor(r,c);
}

void OMShellOut(Screen *scr)
{
    scr->DrawTitle("Press Confirm to exit to the shell");
    FKeySet8by2 fks("Confirm\n  Exit",0,0,0,0,0,0," Cancel\n  Exit");
    fks.Paint();
    for (;;)
    {
	int ch = GetKey();
	if (ch == KB_FKEY_CODE(7))
	    break;
	if (ch == KB_FKEY_CODE(0))
	{
	    scr->ShellOut();
	    break;
	}
    }
    scr->Paint();
}

//-------------------------------------------------------------------
// Get the contents of a message in a local temporary file as text

char *Quote(FolderItem *msg)
{
    char *tn = tmpnam(0);
    if (tn)
    {
	char *quoted = new char[strlen(tn)+1];
	if (quoted)
	{
	    strcpy(quoted, tn);
	    if (msg->IsAtomic())
	        ((AtomicItem*)msg)->Export(quoted, 1167);
	    else
	        mstore->SaveAsText(msg->ItemNum(), quoted);
	    return quoted;
	}
    }
    return 0;
}

//----------------------------------------------------------------------

OMListBoxPopup::OMListBoxPopup(Screen *owner_in, char *prompt_in, char *hlp_in)
    : ListBoxPopup(owner_in, 2, 41, 39, prompt_in, 16),
      helpidx(hlp_in)
{
    ConstructTrace("OMListBoxPopup");
}

void OMListBoxPopup::HandleFKey(int ch, int &done, int &rtn)
{
    switch (ch)
    {
    case KB_FKEY_CODE(6): // help
    	char *hf = config->Get("helpfile");
	if (hf[0])
	{
    	    HyperText *h = new HyperText(0, hf, helpidx);
	    assert(h);
    	    h->Run();
    	    delete h;
	    ClearScreen();
    	    owner->Paint();
	}
	break;
    case KB_FKEY_CODE(7): // cancel
	done = 1;
    default:
	ListBoxPopup::HandleFKey(ch,done,rtn);
    }
}

void OMListBoxPopup::HandleKey(int ch, int &done, int &rtn)
{
    ListBoxPopup::HandleKey(ch, done, rtn);    
}

OMListBoxPopup::~OMListBoxPopup()
{
    DestructTrace("OMListBoxPopup");
}

//----------------------------------------------------------------------
// For the file popup, Run() returns -1 (cancel), 0 (copy), or 1 (move).
// The destination folder listref and itemref can be fetched by calling
// GetSelection(). The user must then do the move/copy
// before deleting the file popup (as this closes the destination
// folder).

FilePopup::FilePopup(Screen *owner_in)
    : OMListBoxPopup(owner_in,
         "Choose the folder in which to file this item and press Move or Copy",
	 "FILING"),
	possp(0)
{
    fkeys = new FKeySet8by2("Move To\n Folder", "Copy To\n Folder",
			" Create\n Folder", 0,
			"  Down\n Folder", "   Up\n Folder",
			"  Help", " Cancel\n  File");
    itemnums = new long[maxitems];
    posstk = new int[maxitems];
    pgstk = new int[maxitems];
    assert(fkeys && itemnums && posstk && pgstk);
    mstore->OpenSubfolder(4,maxitems); // should check return value!
    icnt = mstore->Folder()->FirstPage();
    GetItems();
    ConstructTrace("FilePopup");
}

void FilePopup::GetSelection(int &lref_out, long &inum_out)
{
    lref_out = listref_out;
    inum_out = itemnum_out;
}

int FilePopup::NextPage()
{
    return icnt = mstore->Folder()->NextPage();
}

int FilePopup::PrevPage()
{
    return icnt = mstore->Folder()->PrevPage();
}

void FilePopup::GetItems()
{
    Folder *f = (Folder*)mstore->Folder();
    for (int r = 0, chld = 1; r < maxitems && chld <= icnt; chld++)
    {
	FolderItem *fi = f->Child(pagenum*maxitems+chld);
	if (fi->IsFolder())
	{
	    delete [] items[r];
	    items[r] = new char[strlen(fi->Name())+1];
	    if (items[r])
	    {
		strcpy(items[r], fi->Name());
	    	itemnums[r] = fi->ItemNum();
	    	r++;
	    }
	}
    }
    cnt = r;
    fkeys->Enable(0, 0, cnt); // can we move?
    fkeys->Enable(0, 1, cnt); // can we copy?
    fkeys->Enable(0, 4, cnt); // can we go down?
    fkeys->Enable(0, 5, !mstore->IsSubfolder(4)); // can we go up?
    int sz = f->Size();
    islast = (cnt < maxitems);
    if (sz>0 && (pagenum+1)*maxitems>=sz) islast = 1;
}

void FilePopup::HandleKey(int ch, int &done, int &rtn)
{
    if (ch == '\n')
	HandleFKey(KB_FKEY_CODE(4), done, rtn);
    else
	OMListBoxPopup::HandleKey(ch, done, rtn);
}

void FilePopup::HandleFKey(int ch, int &done, int &rtn)
{
    FolderItem *f = mstore->Folder();
    int fref = f->Ref();
    switch (ch)
    {
    case KB_FKEY_CODE(0): // move to folder
	    listref_out = fref;
	    itemnum_out = itemnums[pos];
	    done = 1;
	    rtn = 1;
	    break;
    case KB_FKEY_CODE(1): // copy to folder
	    listref_out = fref;
	    itemnum_out = itemnums[pos];
	    done = 1;
	    rtn = 0;
	    break;
    case KB_FKEY_CODE(2): // create folder
	    int fnum, key;
    	    char buf[82];
    	    PopupPrompt *p = new
		PopupPrompt(owner,
			"Type the new folder name and press Move or Copy.",
			"CREATEFOLDER", "Move To\nFolder", "Copy To\nFolder",
			" Cancel\n  File", 1);
	    assert(p);
    	    buf[0] = '\0';
	    done = 1;
    	    if ((key = p->Run(buf,0)) != KB_FKEY_CODE(7))
    	    {
		int fnum= mstore->CreateFolder(fref,buf);
	    	if (fnum > 0)
	    	{
	            listref_out = fref;
	            itemnum_out = fnum;
		    if (key == KB_FKEY_CODE(0)) rtn  = 1; // move
		    else rtn = 0; // copy
		}
    	    }
	    break;
    case KB_FKEY_CODE(4): // down folder
	    if (possp < maxitems)
	    {
		posstk[possp] = pos;
		pgstk[possp] = pagenum;
	    }
	    possp++;
	    mstore->OpenFolder(itemnums[pos]);
	    pos = 0;
	    pagenum = 0;
	    islast = 1;
    	    icnt = mstore->Folder()->FirstPage();
	    GetItems();
	    ClearScreen();
    	    owner->Paint();
	    break;
    case KB_FKEY_CODE(5): // up folder
	    mstore->CloseFolder();
    	    icnt = mstore->Folder()->FirstPage();
	    assert(possp>0);
	    if (--possp < maxitems)
	    {
		pos = posstk[possp];
		pagenum = pgstk[possp];
	    }
	    else pos = pagenum = 0;
	    for (int pg = pagenum; --pg >= 0; )
    	        icnt = mstore->Folder()->NextPage();
	    GetItems();
	    ClearScreen();
    	    owner->Paint();
	    break;
    default:
	    OMListBoxPopup::HandleFKey(ch, done, rtn);
    }
}

FilePopup::~FilePopup()
{
    mstore->CloseSubfolder();
    owner->RefreshContents();
    ClearScreen();
    owner->Paint();
    delete [] itemnums;
    delete [] posstk;
    delete [] pgstk;
    DestructTrace("FilePopup");
}

OMScreen::OMScreen(Screen *parent_in, char * title_in, char * prompt_in,
	   int nlabels_in)
    : Screen(parent_in, title_in, prompt_in, nlabels_in)
{
    ConstructTrace("OMScreen");
}

Screen *OMScreen::HandleKey(int ch, int &done)
{
    if (ch == '!')
    {
	OMShellOut(this);
	return 0;
    }
    return Screen::HandleKey(ch, done);
}

void OMScreen::DrawTitle(char *prompt)
{
    OMDrawTitle(this, prompt);
}

void OMScreen::Paint()
{
    Screen::Paint();
}

int OMScreen::Run()
{
    return Screen::Run();
}

void OMScreen::RefreshContents()
{
    Screen::RefreshContents();
}

OMScreen::~OMScreen()
{
    DestructTrace("OMScreen");
}

//-----------------------------------------------------------------

OMFormScreen::OMFormScreen(Screen *parent_in, char * title_in, char * prompt_in,
	     int nlabels_in, int nfields_in, int nextchar_in, int prevchar_in,
	     int canedit_in)
    : FormScreen(parent_in, title_in, prompt_in, nlabels_in, nfields_in,
		 nextchar_in, prevchar_in, canedit_in)
{
    ConstructTrace("OMFormScreen");
}

Screen *OMFormScreen::HandleKey(int ch, int &done)
{
    if (ch == '!')
    {
	OMShellOut(this);
	return 0;
    }
    return FormScreen::HandleKey(ch, done);
}

void OMFormScreen::DrawTitle(char *prompt)
{
    OMDrawTitle(this, prompt);
}

void OMFormScreen::Paint()
{
    FormScreen::Paint();
}

int OMFormScreen::Run()
{
    return FormScreen::Run();
}

OMFormScreen::~OMFormScreen()
{
    DestructTrace("OMFormScreen");
}

//---------------------------------------------------------------

OMPageableScreen::OMPageableScreen(Screen *parent_in, char *title_in,
				   char *prompt_in, int nlabels_in,
				   int nfields_in, int msglines_in)
    : PageableScreen(parent_in, title_in, prompt_in, nlabels_in,
			nfields_in, msglines_in)
{
    ConstructTrace("OMPageableScreen");
}

int OMPageableScreen::Run()
{
    return PageableScreen::Run();
}

Screen *OMPageableScreen::HandleKey(int ch, int &done)
{
    if (ch == '!')
    {
	OMShellOut(this);
	return 0;
    }
    return PageableScreen::HandleKey(ch, done);
}

void OMPageableScreen::DrawTitle(char *prompt)
{
    OMDrawTitle(this, prompt);
}

void OMPageableScreen::Paint()
{
    PageableScreen::Paint();
}

void OMPageableScreen::RefreshContents()
{
    PageableScreen::RefreshContents();
}

void OMPageableScreen::Print(int listref, long item_num)
{
    char *prtcmd = config->Get("print");
    DrawTitle("Printing.");
#if 0 // the next line has the server do the print
    mstore->Print(mstore->Folder()->Ref(), item_num, prtcmd ? prtcmd : "lp -c");
#else // print locally
    if (prtcmd[0]==0) prtcmd = "lp -c";
    char *split = strchr(prtcmd, '!');
    char *tn = tmpnam(0);
    char cmd[80];
    if (mstore->SaveAsText(listref, item_num, tn) == 0)
    {
        if (split)
        {
	    strncpy(cmd, prtcmd, split-prtcmd);
	    strcpy(cmd+(split-prtcmd), tn);
	    strcat(cmd, split+1);
        }
        else
        {
	    strcpy(cmd, prtcmd);
	    strcat(cmd, " ");
	    strcat(cmd, tn);
        }
        strcat(cmd, " 2>&1 > ");
        strcat(cmd, getenv("HOME"));
        strcat(cmd, "/print.status");
        system(cmd);
        unlink(tn);
#endif
        DrawTitle("Print initiated. Choose a function or press Done.");
    }
    else Paint();
}

char *OMPageableScreen::GetQuotedPart(FolderItem *msg)
{
    if (Confirm("Include this message?",
		    "Press Quote if you wish to include the message",
		    " Quote", " Don't\n Quote"))
	return Quote(msg);
    return 0;
}

OMPageableScreen::~OMPageableScreen()
{
    DestructTrace("OMPageableScreen");
}

//----------------------------------------------------------------

OMCompositePageableScreen::OMCompositePageableScreen(Screen *parent_in,
	char *title_in, char *prompt_in, int nlabels_in, int nfields_in, 
	  int msglines_in, int fnum_in)
     : OMPageableScreen(parent_in,title_in,prompt_in,nlabels_in,
		nfields_in,msglines_in),
       fnum(fnum_in)
{
    ConstructTrace("OMCompositePageableScreen");
    if (fnum>0 && mstore->OpenFolder(fnum, msglines_in) == 0)
    {
	folder = (CompositeItem*)mstore->Folder();
        cnt = folder->Size();
        is_lastpage =(folder->FirstPage()<msglines || (cnt>0 && cnt<=msglines));
    }
    else
    {
	folder = 0;
	cnt = 0;
	is_lastpage = 1;
    }
}

int OMCompositePageableScreen::Run()
{
    if (folder) return OMPageableScreen::Run();
    else return 0;
}
    
int OMCompositePageableScreen::PrevPage()
{
    return folder->PrevPage();
}

int OMCompositePageableScreen::NextPage()
{
    return folder->NextPage();
}

void OMCompositePageableScreen::Paint()
{
    if (fkeys) fkeys->SetEmpty(cnt == 0);
    OMPageableScreen::Paint();
}

OMCompositePageableScreen::~OMCompositePageableScreen()
{
    if (folder) mstore->CloseFolder();
    DestructTrace("OMCompositePageableScreen");
}

Screen *OMCompositePageableScreen::HandleKey(int ch, int &done)
{
    Screen *rtn = OMPageableScreen::HandleKey(ch, done);
    if (cnt>0 && (pagenum+1)*msglines>=cnt) is_lastpage = 1;
    return rtn;
}

void OMCompositePageableScreen::RefreshContents()
{
    OMPageableScreen::RefreshContents();
}

//----------------------------------------------------------------------

OMPageableListScreen::OMPageableListScreen(Screen *parent_in, char *title_in,
	  char *prompt_in, int nlabels_in, int nfields_in, 
	  int msglines_in, int firstmsgln_in,
	  int fnum_in, char *emptyprompt_in)
     : OMCompositePageableScreen(parent_in, title_in,prompt_in,nlabels_in,
				nfields_in, msglines_in, fnum_in),
       	emptyprompt(emptyprompt_in),
	msgs(msglines_in),
	firstline(firstmsgln_in),
	itemnum(1),
	mustrefresh(0),
	selected(-1)
{
    ConstructTrace("OMPageableListScreen");
}

void OMPageableListScreen::Refresh(int oldpage, int olditemnum)
{
    if (folder)
    {
        int fnum = folder->ItemNum();
        mstore->CloseFolder();
        if (mstore->OpenFolder(fnum, msglines) == 0)
	    folder = (CompositeItem*)mstore->Folder();
	else
	    folder = 0;
        pagenum = is_lastpage = 0;
        if (folder)
        {
            cnt = folder->Size();
            is_lastpage = (folder->FirstPage() < msglines);
	    if (cnt>0 && (pagenum+1)*msglines>=cnt) is_lastpage = 1;
            while (pagenum < oldpage && !is_lastpage)
    	    {
    	        pagenum++;
        	is_lastpage = (folder->NextPage() < msglines);
	        if (cnt>0 && (pagenum+1)*msglines>=cnt) is_lastpage = 1;
    	    }
            GetItems();
            int downcnt = (olditemnum-1)%msglines;
            msgs.SetCurrent(downcnt);
            itemnum = pagenum * msglines + msgs.Current() + 1;
        }
        else
        {
    	    msgs.SetCurrent(cnt = 0);
    	    is_lastpage = 1;
            itemnum = 1;
        }
    }
    else
    {
        pagenum = cnt = 0;
        itemnum = 1;
        is_lastpage = 1;
    }
    UpdateCount();
}

void OMPageableListScreen::Paint()
{
    if (mustrefresh)
	Refresh(pagenum, pagenum*msglines + msgs.Current()+1);
    OMCompositePageableScreen::Paint();
    if (cnt) msgs.Paint();
    else DrawTitle(emptyprompt);
    mustrefresh = 0;
    SetCursor(0,0);
}

OMPageableListScreen::~OMPageableListScreen()
{
    DestructTrace("OMPageableListScreen");
}

void OMPageableListScreen::GetItems()
{
    if (folder)
    {
        CompositeItem *ci = (CompositeItem *)folder;
        FolderItem *fi;
        for (int m = 0; (fi = ci->Child(pagenum*msglines+m+1))!=0; m++)
	    msgs.AddEntry(m, firstline+m, 0, FormatItem(fi));
        while (m < msglines)
	    msgs.DelEntry(m++);
        msgs.SetCurrent(0);
        itemnum = pagenum * msglines + msgs.Current() + 1;
    }
}

Screen *OMPageableListScreen::HandleKey(int ch, int &done)
{
    Screen *rtn = 0;
    int oldinum = itemnum;
    done = 0;
    if (ch == KB_BACKSPACE)
	msgs.StartSelection();
    else if (ch == KB_UP)
	msgs.Up(0);
    else if (ch == KB_DOWN || ch == '\t')
	msgs.Down(0);
    else rtn = OMCompositePageableScreen::HandleKey(ch, done);
    itemnum = pagenum * msglines + msgs.Current() + 1;
    if (oldinum != itemnum) Paint();
    return rtn;
}

void OMPageableListScreen::GetRange(int &first, int &last)
{
    msgs.GetSelection(first, last);
    first += pagenum*msglines + 1; // cvt menu offset to item num
    last += pagenum*msglines + 1;
}

void OMPageableListScreen::DeleteChild()
{
    int first, last, fail = 0;
    if (cnt < 1) return;
    DrawTitle("Deleting.");
    GetRange(first, last);
    while (first <= last)
    {
        if (mstore->Delete(folder->Ref(), first++) == 0)
        {
	    if (itemnum >= cnt) itemnum--;
	    cnt--;
        }
	else fail++;
    }
    msgs.ClearSelection();
    Refresh(pagenum, itemnum);
    parent->RefreshContents();
    ClearScreen();
    Paint();
    if (fail==0)
	DrawTitle("Deleted. Choose a function or press Done.");
}

void OMPageableListScreen::Print(int list_ref, long item_num)
{
    int first, last;
    if (list_ref == 0)
	list_ref = mstore->Folder()->Ref();
    if (item_num == 0l) // default is to print current selection
	GetRange(first, last);
    else first = last = itemnum;
    while (first <= last)
        OMPageableScreen::Print(list_ref, first++);
    msgs.ClearSelection();
}

void OMPageableListScreen::ReorderFolder()
{
    if (cnt < 2) return;
    selected = itemnum;
    FKeySet8by2 fks("Perform\nRe-order",0,0,0,0,0,0,"Cancel\nRe-order");
    for (;;)
    {
	int snum = -1;
	if (((selected-1) / msglines) == pagenum)
	    snum = (selected-1) % msglines;
        DrawTitle("Move the cursor to the new position and press Perform Re-order.");
        fks.Paint();
	msgs.Paint(snum);
	switch (GetKey())
	{
	case KB_UP:
	    msgs.Up(0);
	    break;
	case KB_DOWN:
	    msgs.Down(0);
	    break;
	case KB_PGUP:
	    PageUp();
	    break;
	case KB_PGDN:
	    PageDown();
	    break;
	case KB_FKEY_CODE(0):
    	    itemnum = pagenum * msglines + msgs.Current() + 1;
	    if (itemnum > selected)
	    {
	        if (mstore->ReorderFolder(folder->Ref(),selected, itemnum)==0)
    	            RefreshContents();
	    }
	    else if (itemnum < selected)
	    {
	        if (mstore->ReorderFolder(folder->Ref(),selected, itemnum-1)==0)
    	            RefreshContents();
	    }
	    // fall thru...
	case KB_FKEY_CODE(7):
    	    ClearScreen();
    	    Paint();
	    return;
	}
    }
}

void OMPageableListScreen::RefreshContents()
{
    mustrefresh = 1; // refresh deferred until paint
    OMCompositePageableScreen::RefreshContents();
}

int OMPageableListScreen::InWaste()
{
    // kludge to check if in waste basket; is there a better way?
    for (FolderItem *fnow = folder; fnow; fnow = fnow->Parent())
    {
	if (strcmp(fnow->Name(), "WASTE BASKET")==0)
	    return 1;
    }
    return 0;
}

void OMPageableListScreen::UpdateCount()
{
     // default is no update
}

void OMPageableListScreen::CreateFolder()
{
    char buf[82];
    PopupPrompt *p =
        new PopupPrompt(this,
		"Type the name of the new folder and press Perform Create.",
		"CREATEFOLDER", "Perform\n Create", 0, " Cancel\n Create", 1);
    assert(p);
    buf[0] = '\0';
    if (p->Run(buf) != KB_FKEY_CODE(7))
    {
	if (mstore->CreateFolder(folder->Ref(), buf) > 0)
            RefreshContents();
    }
    delete p;
    ClearScreen();
    Paint();
}

void OMPageableListScreen::CreateArea()
{
    char buf[82];
    PopupPrompt *p =
        new PopupPrompt(this,
		"Type the name of the new area and press Perform Create.",
		"BBAREA", "Perform\n Create", 0, " Cancel\n Create", 1);
    assert(p);
    buf[0] = '\0';
    if (p->Run(buf) != KB_FKEY_CODE(7))
    {
	if (mstore->CreateArea(folder->Ref(), buf) > 0)
            RefreshContents();
    }
    delete p;
    ClearScreen();
    Paint();
}

void OMPageableListScreen::FileItem()
{
    if (cnt < 1) return;
    int srclistref = folder->Ref();
    int destlistref;
    long destitemnum;
    FilePopup *fpop = new FilePopup(this);
    assert(fpop);
    int cmd = fpop->Run();
    fpop->GetSelection(destlistref, destitemnum);
    int first, last;
    GetRange(first,last);
    if (cmd == 0) // copy?
    {
	while (first <= last &&
	       mstore->CopyItem(srclistref,first++,destlistref,destitemnum)==0);
        delete fpop;
    }
    else if (cmd == 1)
    {
	while (first <= last &&
	       mstore->MoveItem(srclistref,first++,destlistref,destitemnum)==0);
        delete fpop;
	RefreshContents();
    }
    else delete fpop;
    msgs.ClearSelection();
    Paint();
}

void OMPageableListScreen::SaveItem(int saveastext)
{
    if (cnt < 1) return;
    char buf[82];
    PopupPrompt *p =
        new PopupPrompt(this,
		"Type the name of the file to save to and press Perform Save.",
		"SAVINGITEM", "Perform\n  Save", "  Shell\n  O/S",
		" Cancel\n  Save", 1);
    assert(p);
    buf[0] = '\0';
    int key;
    for (;;)
    {
        key = p->Run(buf, 1);
        if (key == KB_FKEY_CODE(1)) // shell
	    ShellOut();
	else break;
    }
    if (key == KB_FKEY_CODE(0) || key == '\n') // do save
    {
	FolderItem *fi = folder->Child(itemnum);
	if (fi->IsAtomic())
	{
	    if (saveastext)
	        ((AtomicItem*)fi)->Export(buf, 1167);
	    else
	        ((AtomicItem*)fi)->Export(buf);
	}
	else
	{
	    if (saveastext)
	        mstore->SaveAsText(itemnum, buf);
	    else
	        mstore->Save(itemnum, buf);
	}
    }
    delete p;
    ClearScreen();
    Paint();
}

//--------------------------------------------------------------------------
