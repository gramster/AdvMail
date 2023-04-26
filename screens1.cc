#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <assert.h>

#include "debug.h"
#include "ansicurs.h"
#include "config.h"
#include "mail.h"
#include "agent.h"

#include "screens1.h"
#include "screens2.h"

//---------------------------------------------------------------

BBAreaScreen::BBAreaScreen(Screen *parent_in)
    : OMPageableListScreen(parent_in, "Bulletin Board Area",
		"Choose a bulletin board and a function.", 4, 0, 13, 7, 6,
		"There are no bulletin boards.")
{
    ConstructTrace("BBAreaScreen");
    AddLabel(3, 0, FormatAmount(cnt, "Bulletin Board"));
    AddLabel(5,  0, "Name");
    AddLabel(5, 60, "Contents");
    AddLabel(5, 70, "Date");
    fkeys = new FKeySet8by2(0, 0, "*  Open", 0, 0, 0, "  Help","  Done");
    assert(fkeys);
}

void BBAreaScreen::UpdateCount()
{
    SetLabel(0, 3, 0, FormatAmount(cnt, "Bulletin Board"));
}

char *BBAreaScreen::FormatItem(FolderItem *fi)
{
    static char buf[82];
    if (fi->IsFolder() || fi->IsBBoard())
    {
	// get the folder or bboard size 
	fi->Open(); fi->Close();
        sprintf(buf, "%-62.60s%-8d%-10s", 
	    fi->Name(), fi->Size(), FormatDate(fi->CreateDate()));
    }
    else sprintf(buf, "%-70.68s%-10s", 
	    fi->Name(), FormatDate(fi->CreateDate()));
    return buf;
}

Screen *BBAreaScreen::HandleKey(int ch, int &done)
{
    if (ch == '\n') return HandleFKey(F3_PRI, done);
    else return OMPageableListScreen::HandleKey(ch, done);
}

Screen *BBAreaScreen::HandleFKey(int fkeynum, int &done)
{
    done = 0;
    switch (fkeynum)
    {
    case F3_PRI: // open folder
	return (cnt>0) ? new BBoardScreen(this, itemnum) : 0;
    case F7_PRI: // help
	Help("BBAREA");
	break;
    case F8_PRI:
	done = 1;
	break;
    }
    return 0;
}

BBAreaScreen::~BBAreaScreen()
{
    DestructTrace("BBAreaScreen");
}

//----------------------------------------------------------------------

BBoardScreen::BBoardScreen(Screen *parent_in, int fnum)
    : OMPageableListScreen(parent_in, "Bulletin Board",
		"Choose a message and a function.", 8, 0, 13, 7, fnum,
		"This bulletin board is empty.")
{
    ConstructTrace("BBoardScreen");
    AddLabel(4, 0, FormatAmount(cnt, "Item"));
    if (folder)
    {
	char buf[82];
        sprintf(buf,"Bulletin Board: %s", folder->Name());
        AddLabel(3, 0, buf);
        sprintf(buf,"Created %s", FormatDate(folder->CreateDate()));
        AddLabel(4, 56, buf);
    }
    else AddLabel(3,0,"Bulletin Board: ERROR");
    AddLabel(5, 4, "Subject");
    AddLabel(5, 40, "Creator");
    AddLabel(5, 56, "Type");
    AddLabel(5, 64, "Size");
    AddLabel(5, 71, "Date");
#if 1
    emptyfkeys = new FKeySet8by2(0,0,0,0,0,0, 0);
    msgfkeys = new FKeySet8by2(" Other\n  Keys", "  Read:  File",
			   " Print:  Open\n Message"," Delete",
			   " Reply", "Forward:");
    foldfkeys = new FKeySet8by2(0, 0, "  Open\n  Area", 0, 0, 0);
    basicfkeys = new FKeySet8by2(" Other\n  Keys",
			"  Read\n  Item:",
	    		"  Print\n  Item:SaveItem\nas Text",
			" Delete\n  Item",
	    		"  Save\n  Item",
	    		"  File\n  Item:");
#else
    emptyfkeys = new FKeySet8by2(0,0,0,0," Create\n Area",0, 0);
    msgfkeys = new FKeySet8by2(" Other\n  Keys", "  Read:  File",
			   " Print:  Open\n Message"," Delete",
			   " Reply", "Forward: Create\n Area");
    foldfkeys = new FKeySet8by2(0, 0, "  Open\n  Area", " Delete",
			   " Create\n Area", 0);
    basicfkeys = new FKeySet8by2(" Other\n  Keys",
			"  Read\n  Item:",
	    		"  Print\n  Item:SaveItem\nas Text",
			" Delete\n  Item",
	    		"  Save\n  Item",
	    		"  File\n  Item: Create\n Area");
#endif
    assert(emptyfkeys && msgfkeys && foldfkeys && basicfkeys);
}

void BBoardScreen::UpdateCount()
{
    SetLabel(0, 4, 0, FormatAmount(cnt, "Item"));
    char buf[82];
}

void BBoardScreen::Paint()
{
    // before painting select the appropriate fkey set
    if (cnt==0)
	fkeys = emptyfkeys;
    else
    {
        FolderItem *fi = folder->Child(itemnum);
	if (fi->IsFolder() || fi->IsBBoard())
	    fkeys = foldfkeys;
        else if (fi->IsMessage())
	    fkeys = msgfkeys;
        else
	    fkeys = basicfkeys;
    }
    OMPageableListScreen::Paint();
}

char *BBoardScreen::FormatItem(FolderItem *fi)
{
    static char buf[82];
    char typ[20], *tp = typ, siz[6];
    strcpy(typ, fi->Type());
    while (*tp) { if (islower(*tp)) *tp = toupper(*tp); tp++; }
    if (fi->IsFolder() || fi->IsBBoard())
    {
	fi->Open(); fi->Close();
	sprintf(siz, "%d", fi->Size());
    }
    else siz[0] = '\0';
    sprintf(buf, "    %-31.30s%-21.20s%-8.7s%-6.5s%-10s", 
		fi->Name(), fi->Creator(), typ, siz,
		FormatDate(fi->CreateDate()));
    return buf;
}

Screen *BBoardScreen::HandleKey(int ch, int &done)
{
    if (ch == '\n' && cnt>0)
    {
	FolderItem *fi = folder->Child(itemnum);
	done = 0;
	if (fkeys == foldfkeys)
	    return (fi && fi->IsBBoard()) ?
				(new BBoardScreen(this, itemnum)) :
				(new FolderScreen(this, itemnum)) ;
	else if (fkeys == msgfkeys)
	    return new ReadMsgScreen(this, (Message *)fi,1,1);
	else return new ReadItemScreen(this, itemnum);
    }
    return OMPageableListScreen::HandleKey(ch, done);
}

Screen *BBoardScreen::HandleFKey(int fkeynum, int &done)
{
    FolderItem *fi = (cnt>0) ? folder->Child(itemnum) : 0;
    done = 0;
    if (fkeynum == F7_PRI || fkeynum==F7_SEC) // help
	Help("BBAREA");
    else if (fkeynum == F8_PRI || fkeynum == F8_SEC) // done
	done = 1;
    else if (fkeys == emptyfkeys) // no child
    {
	switch(fkeynum)
	{
	case F5_PRI: // create folder
	    CreateArea();
	    break;
	}
    }
    else if (fkeys == basicfkeys) // text/bin child
    {
	switch(fkeynum)
	{
        case F1_PRI:
        case F1_SEC: // other keys
	    ToggleFKeys();
	    break;
        case F2_PRI: // read
	    return HandleKey('\n', done);
        case F3_PRI: // print
	    Print();
	    break;
        case F4_PRI: // delete item
	case F4_SEC:
	    DeleteChild();
	    break;
        case F5_PRI: // save
        case F5_SEC:
	    SaveItem(0);
	    break;
        case F6_PRI: // file
	    FileItem();
	    break;
        case F3_SEC: // save item as text
	    SaveItem(1);
	    break;
	case F6_SEC: // create area
	    CreateArea();
	    break;
	}
    }
    else if (fkeys == msgfkeys) // child is a message
    {
	switch(fkeynum)
	{
        case F1_PRI:
        case F1_SEC: // other keys
	    ToggleFKeys();
	    break;
        case F2_PRI: // read message
	    return HandleKey('\n', done);
        case F3_PRI: // print message
	    Print();
	    break;
        case F4_PRI: // delete message
	case F4_SEC:
	    DeleteChild();
	    break;
        case F5_PRI: // reply to message
        case F5_SEC:
	    if (cnt>0) return new CreateReplyScreen(this,
					(Message *)fi, 1, GetQuotedPart(fi));
	    break;
        case F6_PRI: // forward message
	    if (cnt>0) return new CreateForwardScreen(this,
					(Message *)fi, GetQuotedPart(fi));
	    break;
        case F2_SEC: // file message
	    FileItem();
	    break;
        case F3_SEC: // open message
	    if (cnt > 0)
	        if (InWaste())
		    return new OpenWasteMsgScreen(this, itemnum);
	        else
		    return new OpenFiledMsgScreen(this, itemnum);
	    break;
	case F6_SEC: // create area
	    CreateArea();
	    break;
	}
    }
    else if (fkeys == foldfkeys) // child is a folder or bboard
    {
	switch(fkeynum)
	{
        case F3_PRI: // open child folder or bboard
	    return HandleKey('\n', done);
        case F4_PRI: // delete child folder or bboard
	    DeleteChild();
	    break;
	case F5_PRI: // create area
	    CreateArea();
	    break;
	}
    }
    return 0;
}

BBoardScreen::~BBoardScreen()
{
    DestructTrace("BBoardScreen");
    delete emptyfkeys;
    delete basicfkeys;
    delete msgfkeys;
    delete foldfkeys;
    fkeys = 0;
}

//-----------------------------------------------------------------------
// Distribution list area screen

DistListTrayScreen::DistListTrayScreen(Screen *parent_in)
    : OMPageableListScreen(parent_in, "List Area",
		"Choose a list and a function",
		3, 0, 13, 7, 5, "You have no Distribution Lists.")
{
    ConstructTrace("DistListTrayScreen");
    AddLabel(3, 0, FormatAmount(cnt, "List"));
    AddLabel(5,  0, "List Name");
    AddLabel(5, 67, "Created");
    fkeys = new FKeySet8by2("* Other\n  Keys", "*  Edit:*  Read",
			"* Print:", "* Delete:" ,
			" Create:", "Nickname\n  List:Re-order",
			"  Help", "  Done");
    assert(fkeys);
}

void DistListTrayScreen::UpdateCount()
{
    SetLabel(0, 3, 0, FormatAmount(cnt, "List"));
}

char *DistListTrayScreen::FormatItem(FolderItem *fi)
{
    static char buf[82];
    assert(fi->IsDistList());
    sprintf(buf, "%-67.66s%-13s", 
		fi->Name(), FormatDate(fi->CreateDate()));
    return buf;
}

Screen *DistListTrayScreen::HandleKey(int ch, int &done)
{
    if (ch == '\n') return HandleFKey(F2_SEC, done);
    else return OMPageableListScreen::HandleKey(ch, done);
}

Screen *DistListTrayScreen::HandleFKey(int fkeynum, int &done)
{
    int listref = folder->Ref();
    done = 0;
    switch (fkeynum)
    {
    case F1_PRI:
    case F1_SEC: // other keys
	ToggleFKeys();
	break;
    case F7_PRI:
    case F7_SEC: // help
	Help("DISTLISTS");
	break;
    case F8_PRI:
    case F8_SEC: // done 
	done = 1;
	break;
    case F2_PRI: // edit
	return (cnt>0) ? new EditDistListScreen(this, itemnum) : 0;
    case F3_PRI: // print
	Print();
	break;
    case F4_PRI:  // delete
	DeleteChild();
	break;
    case F5_PRI:  // create
	return new EditDistListScreen(this);
    case F6_PRI: // nickname list
	return new NicknameScreen(this);
	break;
    case F2_SEC: // read
	return (cnt>0) ? new ReadItemScreen(this, itemnum) : 0;
    case F6_SEC: // reorder
	ReorderFolder();
	break;
    }
    return 0;
}

DistListTrayScreen::~DistListTrayScreen()
{
    DestructTrace("DistListTrayScreen");
}

//---------------------------------------------------------------

FileTrayScreen::FileTrayScreen(Screen *parent_in)
    : OMPageableListScreen(parent_in, "Filing Cabinet",
		"Choose a folder and a function.", 4, 0, 13, 7, 4,
		"You have no folders.")
{
    ConstructTrace("FileTrayScreen");
    AddLabel(3, 0, FormatAmount(cnt, "Folder"));
    AddLabel(5,  0, "Subject");
    AddLabel(5, 60, "Contents");
    AddLabel(5, 70, "Date");
    fkeys = new FKeySet8by2(0, "*Re-order", "*  Open\n Folder", "* Delete",
			" Create\n Folder", "*  File", "  Help", "  Done");
    assert(fkeys);
}

void FileTrayScreen::UpdateCount()
{
    SetLabel(0, 3, 0, FormatAmount(cnt, "Folder"));
}

char *FileTrayScreen::FormatItem(FolderItem *fi)
{
    static char buf[82];
    if (fi->IsFolder())
    {
	// get the folder size 
	fi->Open(); fi->Close();
        sprintf(buf, "%-62.60s%-8d%-10s", 
	    fi->Name(), fi->Size(), FormatDate(fi->CreateDate()));
    }
    else sprintf(buf, "%-70.68s%-10s", 
	    fi->Name(), FormatDate(fi->CreateDate()));
    return buf;
}

Screen *FileTrayScreen::HandleKey(int ch, int &done)
{
    if (ch == '\n') return HandleFKey(F3_PRI, done);
    else return OMPageableListScreen::HandleKey(ch, done);
}

Screen *FileTrayScreen::HandleFKey(int fkeynum, int &done)
{
    done = 0;
    switch (fkeynum)
    {
    case F2_PRI: // re-order
	ReorderFolder();
	break;
    case F3_PRI: // open folder
	return (cnt>0) ? new FolderScreen(this, itemnum) : 0;
    case F4_PRI: // delete
	DeleteChild();
	break;
    case F5_PRI: // create folder
	CreateFolder();
	break;
    case F6_PRI: // file
	FileItem();
	break;
    case F7_PRI: // help
	Help("FILECABINET");
	break;
    case F8_PRI:
	done = 1;
	break;
    }
    return 0;
}

FileTrayScreen::~FileTrayScreen()
{
    DestructTrace("FileTrayScreen");
}

//---------------------------------------------------------------

FolderScreen::FolderScreen(Screen *parent_in, int fnum)
    : OMPageableListScreen(parent_in, "Folder",
		"Choose a message and a function.", 8, 0, 13, 7, fnum,
		"This folder is empty.")
{
    ConstructTrace("FolderScreen");
    AddLabel(4, 0, FormatAmount(cnt, "Message"));
    if (folder)
    {
	char buf[82];
        sprintf(buf,"Folder: %s", folder->Name());
        AddLabel(3, 0, buf);
        sprintf(buf,"Created %s", FormatDate(folder->CreateDate()));
        AddLabel(4, 56, buf);
    }
    else AddLabel(3,0,"Folder: ERROR");
    AddLabel(5, 4, "Subject");
    AddLabel(5, 40, "Creator");
    AddLabel(5, 56, "Type");
    AddLabel(5, 64, "Size");
    AddLabel(5, 71, "Date");
    emptyfkeys = new FKeySet8by2(0,0,0," Rename\n Folder"," Create\n Folder",0, 0);
    msgfkeys = new FKeySet8by2(" Other\n  Keys", "  Read:  File",
			   " Print:  Open\n Message"," Delete: Rename\n Folder",
			   " Reply:Re-order", "Forward: Create\n Folder");
    foldfkeys = new FKeySet8by2(" Rename\n Folder", "Re-order", "  Open\n Folder",
			" Delete", " Create\n Folder", "  File");
    basicfkeys = new FKeySet8by2(" Other\n  Keys",
			"  Read\n  Item:",
	    		"  Print\n  Item:SaveItem\nas Text",
			" Delete\n  Item: Rename\n Folder",
	    		"  Save\n  Item:Re-order",
	    		"  File\n  Item: Create\n Folder");
    assert(emptyfkeys && msgfkeys && foldfkeys && basicfkeys);
}

void FolderScreen::UpdateCount()
{
    SetLabel(0, 4, 0, FormatAmount(cnt, "Message"));
    char buf[82];
}

void FolderScreen::RenameFolder()
{
    char buf[82];
    PopupPrompt *p =
        new PopupPrompt(this, "Change the subject and press Perform Change.",
			"RENAMEFOLDER", "Perform\n Change", 0,
			" Cancel\n Change", 1);
    assert(p);
    strcpy(buf, folder->Name());
    if (p->Run(buf) != KB_FKEY_CODE(7))
    {
	char buf2[82];
	if (mstore->RenameFolder(folder->Ref(), 0, buf) == 0)
	{
            sprintf(buf2,"Folder: %s", buf);
            SetLabel(1, 3, 0, buf2);
	    parent->RefreshContents();
	}
    }
    delete p;
    ClearScreen();
    Paint();
}

void FolderScreen::Paint()
{
    // before painting select the appropriate fkey set
    if (cnt==0)
	fkeys = emptyfkeys;
    else
    {
        FolderItem *fi = folder->Child(itemnum);
	if (fi->IsFolder())
	    fkeys = foldfkeys;
        else if (fi->IsMessage())
	    fkeys = msgfkeys;
        else
	    fkeys = basicfkeys;
    }
    OMPageableListScreen::Paint();
}

char *FolderScreen::FormatItem(FolderItem *fi)
{
    static char buf[82];
    char typ[20], *tp = typ, siz[6];
    strcpy(typ, fi->Type());
    while (*tp) { if (islower(*tp)) *tp = toupper(*tp); tp++; }
    if (fi->IsFolder())
    {
	fi->Open(); fi->Close();
	sprintf(siz, "%d", fi->Size());
    }
    else siz[0] = '\0';
    sprintf(buf, "    %-31.30s%-21.20s%-8.7s%-6.5s%-10s", 
		fi->Name(), fi->Creator(), typ, siz,
		FormatDate(fi->CreateDate()));
    return buf;
}

Screen *FolderScreen::HandleKey(int ch, int &done)
{
    if (ch == '\n' && cnt>0)
    {
	done = 0;
	if (fkeys == foldfkeys)
	    return new FolderScreen(this, itemnum);
	else if (fkeys == msgfkeys)
	{
	    FolderItem *fi = folder->Child(itemnum);
	    return new ReadMsgScreen(this, (Message *)fi,1,1);
	}
	else return new ReadItemScreen(this, itemnum);
    }
    return OMPageableListScreen::HandleKey(ch, done);
}

Screen *FolderScreen::HandleFKey(int fkeynum, int &done)
{
    FolderItem *fi = (cnt>0) ? folder->Child(itemnum) : 0;
    done = 0;
    if (fkeynum == F7_PRI || fkeynum==F7_SEC) // help
	Help("FILECABINET");
    else if (fkeynum == F8_PRI || fkeynum == F8_SEC) // done
	done = 1;
    else if (fkeys == emptyfkeys) // no child
    {
	switch(fkeynum)
	{
	case F4_PRI: // rename folder
	    RenameFolder();
	    break;
	case F5_PRI: // create folder
	    CreateFolder();
	    break;
	}
    }
    else if (fkeys == basicfkeys) // text/bin child
    {
	switch(fkeynum)
	{
        case F1_PRI:
        case F1_SEC: // other keys
	    ToggleFKeys();
	    break;
        case F2_PRI: // read
	    return HandleKey('\n', done);
        case F3_PRI: // print
	    Print();
	    break;
        case F4_PRI: // delete folder item
	    DeleteChild();
	    break;
        case F5_PRI: // save
	    SaveItem(0);
	    break;
        case F6_PRI: // file
	    FileItem();
	    break;
        case F3_SEC: // save item as text
	    SaveItem(1);
	    break;
	case F4_SEC: // rename folder
	    RenameFolder();
	    break;
        case F5_SEC: // reorder folder
	    ReorderFolder();
	    break;
	case F6_SEC: // create folder
	    CreateFolder();
	    break;
	}
    }
    else if (fkeys == msgfkeys) // child is a message
    {
	switch(fkeynum)
	{
        case F1_PRI:
        case F1_SEC: // other keys
	    ToggleFKeys();
	    break;
        case F2_PRI: // read
	    return HandleKey('\n', done);
        case F3_PRI: // print
	    Print();
	    break;
        case F4_PRI: // delete folder item
	    DeleteChild();
	    break;
        case F5_PRI: // reply
	    if (cnt>0) return new CreateReplyScreen(this,
					(Message *)fi, 0, GetQuotedPart(fi));
	    break;
        case F6_PRI: // forward
	    if (cnt>0) return new CreateForwardScreen(this,
					(Message *)fi, GetQuotedPart(fi));
	    break;
        case F2_SEC: // file
	    FileItem();
	    break;
        case F3_SEC: // open message
	    if (cnt > 0)
	        if (InWaste())
		    return new OpenWasteMsgScreen(this, itemnum);
	        else
		    return new OpenFiledMsgScreen(this, itemnum);
	    break;
	case F4_SEC: // rename folder
	    RenameFolder();
	    break;
        case F5_SEC: // reorder folder
	    ReorderFolder();
	    break;
	case F6_SEC: // create folder
	    CreateFolder();
	    break;
	}
    }
    else if (fkeys == foldfkeys) // child is a folder
    {
	switch(fkeynum)
	{
	case F1_PRI: // rename folder
	    RenameFolder();
	    break;
        case F2_PRI: // reorder folder
	    ReorderFolder();
	    break;
        case 2: // open child folder
	    if (cnt>0) return new FolderScreen(this, itemnum);
	    break;
        case F4_PRI: // delete child folder
	    DeleteChild();
	    break;
	case F5_PRI: // create folder
	    CreateFolder();
	    break;
        case F6_PRI: // file
	    FileItem();
	    break;
	}
    }
    return 0;
}

FolderScreen::~FolderScreen()
{
    DestructTrace("FolderScreen");
    delete emptyfkeys;
    delete basicfkeys;
    delete msgfkeys;
    delete foldfkeys;
    fkeys = 0;
}

//--------------------------------------------------------------

InTrayScreen::InTrayScreen(Screen *parent_in)
    : OMPageableListScreen(parent_in, "In Tray",
		"Choose a message and a function.", 7, 0, 13, 7, 1,
		"You have no Messages.")
{
    ConstructTrace("InTrayScreen");
    AddLabel(3, 0, FormatAmount(cnt, "Message"));
    AddLabel(3, 73, "N A P U");
    AddLabel(4, 71, "T E C R R");
    AddLabel(5, 71, "O W K I G");
    AddLabel(5,  4, "Subject");
    AddLabel(5, 36, "Sender");
    AddLabel(5, 61, "Received");
    fkeys = new FKeySet8by2(" Other\n  Keys", "*  Read:*  File",
			"* Print:*  Open\n Message", "* Delete:",
			"* Reply: Create", "*Forward:",
			"  Help", "  Done");
    assert(fkeys);
}

char *InTrayScreen::FormatItem(FolderItem *fi)
{
    static char buf[82];
    assert(fi->IsMessage());
    Message *m = (Message *)fi;
    char c = ' ';
    if (m->HasDistListError())
        c = 'D';
    else if (m->IsError())
        c = 'E';
    else if (strcmp(m->Type(), "Reply")==0)
        c = 'R';
    sprintf(buf, "%c   %-32.30s%-25.23s%-10s%c %c %c %c %c", c,
		m->Name(), m->Creator(), FormatDate(m->CreateDate()),
		(m->RecipCategory()==1) ? '*' : ' ',
		(m->IsUnread()) ? '*' : ' ',
		(m->RequestedAck()) ? '*' : ' ',
		(m->Sensitivity()==2) ? '*' : ' ',
		(m->Priority()==2) ? '*' : ' ');
    return buf;
}

Screen *InTrayScreen::HandleKey(int ch, int &done)
{
    if (ch == '\n') return HandleFKey(F2_PRI, done);
    else return OMPageableListScreen::HandleKey(ch, done);
}

Screen *InTrayScreen::HandleFKey(int fkeynum, int &done)
{
    done = 0;
    Message *m = (cnt>0) ? (Message *)folder->Child(itemnum) : 0;
    switch (fkeynum)
    {
    case F1_PRI:
    case F1_SEC: // other keys
	ToggleFKeys();
	break;
    case F2_PRI:
	if (m) return new ReadMsgScreen(this, m,1,1);
	break;
    case F3_PRI: // print
	Print();
	break;
    case F4_PRI: // delete
	DeleteChild();
	break;
    case F5_PRI: // reply
	if (m) return new CreateReplyScreen(this, m, 0, GetQuotedPart(m));
	break;
    case F6_PRI: // forward
	if (m) return new CreateForwardScreen(this, m, GetQuotedPart(m));
	break;
    case F7_PRI:
    case F7_SEC: // help
	Help("INTRAY");
	break;
    case F8_PRI:
    case F8_SEC: // done 
	done = 1;
	break;
    case F2_SEC: // file
	FileItem();
	break;
    case F3_SEC: // open message
	if (m) return new OpenInMsgScreen(this, itemnum);
	break;
    case F5_SEC: // create
	return new CreateNewScreen(this);
    }
    return 0;
}

void InTrayScreen::Paint()
{
    int gotnewmail = (mstore->NewMail()>0);
    if (gotnewmail)
    {
	RefreshContents();
        OMPageableListScreen::Paint();
	DrawTitle("New mail has arrived. Choose a message and a function.");
    }
    else OMPageableListScreen::Paint();
}

void InTrayScreen::UpdateCount()
{
    SetLabel(0, 3, 0, FormatAmount(cnt, "Message"));
}

InTrayScreen::~InTrayScreen()
{
    DestructTrace("InTrayScreen");
}

//------------------------------------------------------------

LoginScreen::LoginScreen()
    : OMFormScreen(0, "Login", "Copyright (c) 1997 Open Mind Solutions", 3, 3), 
      retry(3)
{
    ConstructTrace("LoginScreen");
    AddLabel(LINES-9, 1, "Server:");
    sname = AddField(new DataField(LINES-9, 9, 30));
    AddLabel(LINES-8, 1, "User:");
    uname = AddField(new DataField(LINES-8, 7, 38));
    AddLabel(LINES-7, 1, "Enter your password:");
    passwd = AddField(new DataField(LINES-7, 23, 22, 0, "\n\r", 0));
    fkeys = new FKeySet8by2("   F1\n ESC-1", "   F2\n ESC-2",
			"   F3\n ESC-3", "   F4\n ESC-4",
			"   F5\n ESC-5", "   F6\n ESC-6",
			"   F7\n ESC-7", "   F8\n ESC-8");
    assert(fkeys);
}

Screen *LoginScreen::HandleFKey(int fkeynum, int &done)
{
    assert(0); // should never be called as login screen has its own Run()
}

int LoginScreen::Run()
{
    int cnt = 3;
    Paint();
    char *server = config->Get("server");
    char *user = config->Get("user");
    char *password = config->Get("password");
    if (user[0]) 
	((DataField*)fields[uname])->SetValue(user);
    if (password[0]) 
	((DataField*)fields[passwd])->SetValue(password);
    if (server[0]) 
	((DataField*)fields[sname])->SetValue(server);
    fields[sname]->Paint();
    fields[uname]->Paint();
    fields[passwd]->Paint();
    while (cnt > 0)
    {
	if (cnt==3) fields[sname]->Run();
	fields[uname]->Run();
	fields[passwd]->Run();
	DrawTitle("Checking password, please wait.");
	cnt--;
	mstore = new MessageStore(((DataField*)fields[sname])->Value(),
				  ((DataField*)fields[uname])->Value(),
				  ((DataField*)fields[passwd])->Value());
	assert(mstore);
        if (mstore->Connect()==0)
        {
	    char *pi = config->Get("poll");
	    if (pi[0]) mstore->SetPollInterval(atoi(pi));
	    UserFolderScreen *ufs = new UserFolderScreen(this);
	    assert(ufs);
	    int rtn = ufs->Run();
	    delete ufs;
	    mstore->Disconnect();
	    delete mstore;
	    mstore = 0;
	    return rtn;
        }
	else
	{
	    delete mstore;
	    mstore = 0;
	    DrawTitle("Log on failed; please try again...");
	    ((DataField*)fields[passwd])->SetValue("");
	}
    }
    return -1;
}

LoginScreen::~LoginScreen()
{
    DestructTrace("LoginScreen");
    delete mstore;
}

//----------------------------------------------------------------

class IncludeFilePopup
{
    FKeySet8by2 fkeys;
    DataField fld;
    FileTypeToggle ftyp;
    Screen *owner;
public:
    IncludeFilePopup(Screen *owner_in);
    int Run(char *buf, int &typ);
    ~IncludeFilePopup();
};
 
IncludeFilePopup::IncludeFilePopup(Screen *owner_in)
	: fkeys("Perform\nInclude", 0, "Compose", "  Next\n  Type",
		"  Prev\n  Type", " Shell", "  Help", "  Done"),
    	  fld(2,0,80,0,"\n\t\r", 1, 0),
    	  ftyp(3,0,80, 0, 0),
	  owner(owner_in)
{
    ConstructTrace("IncludeFilePopup");
}

int IncludeFilePopup::Run(char *result, int &type)
{
    fld.SetValue(result);
    for (;;)
    {
        char buf[82];
        strcpy(buf, "Type a filename, set its type and press Perform Include.");
        fill(buf,80,' ');
        ReverseOn();
        PutString(1,0,buf);
        ReverseOff();
        fkeys.Paint();
        ftyp.Paint();
	int ch = fld.Run();
	int fkey = (ch=='\n') ? 0 : fkeys.GetCommand(ch);
	switch (fkey)
	{
	case 0:
	    strcpy(result, fld.Value());
	    type = ftyp.GetType();
	    return 0;
	case 2: // compose
	    strcpy(result, tmpnam(0));
	    Debug1("Using %s for temporary compose file", result);
	    if (ExternalCompose(result, ftyp.GetType()) != 0)
	    {
	        EditScreen *s = new EditScreen(owner, result);
		assert(s);
	        s->Run();
	        delete s; 
	    }
	    if (access(result,0)==0) // did user save something?
		return 0;
	    break;
	case 3: // next type
	    ftyp.NextValue();
	    break;
	case 4: // prev type
	    ftyp.PrevValue();
	    break;
	case 5: // shell out
	    owner->ShellOut();
	    owner->Paint();
	    break;
	case 6:
	    owner->Help("FILEATTACH");
	    break;
	case 7:
	    return -1;
	}
    }
}

IncludeFilePopup::~IncludeFilePopup()
{
    DestructTrace("IncludeFilePopup");
}

//--------------------------------------------------------------------

OpenMsgScreen::OpenMsgScreen(Screen *parent_in, int inum,
			     int nlbls_in, int nflds_in)
    : OMPageableListScreen(parent_in, "Message", "Choose a function.",
		12+nlbls_in, nflds_in, 8, 12, inum, "Choose a function.")
{
    ConstructTrace("OpenMsgScreen");
    if (folder)
    {
	char buf[82];
        AddLabel(4, 0, "MESSAGE");
        sprintf(buf,"Dated %s at %s", FormatDate(folder->CreateDate()),
		FormatTime(folder->CreateDate()));
        AddLabel(4, 53, buf);
        AddLabel(5, 0, "Subject:");
        subjfld = AddLabel(5, 9, folder->Name());
        AddLabel(6, 0, "Sender:");
        AddLabel(6, 9, folder->Creator());
        AddLabel(7, 0, "Urgent:");
        AddLabel(7, 22, "Private:");
        AddLabel(7, 40, "Ack Level:");
        sprintf(buf,"Contents %d", folder->Size());
        cntfld = AddLabel(7, 63, buf);
        AddLabel(10, 0, "Type");
        AddLabel(10, 33, "Subject");
    }
}

void OpenMsgScreen::Paint()
{
    char buf[20];
    sprintf(buf,"Contents %-4d", folder->Size());
    SetLabel(cntfld, 7, 63, buf);
    OMPageableListScreen::Paint();
}

char *OpenMsgScreen::FormatItem(FolderItem *fi)
{
    static char buf[82];
    char typ[20], *tp = typ;
    strcpy(typ, fi->Type());
    while (*tp) { if (islower(*tp)) *tp = toupper(*tp); tp++; }
    sprintf(buf, "%-33.32s%-47.47s",  typ, fi->Name());
    return buf;
}

int OpenMsgScreen::IncludeFile()
{
    char buf[256];
    buf[0] = '\0';
    IncludeFilePopup ifp(this);
    int typ, rtn = ifp.Run(buf, typ);
    if (rtn == 0)
    {
	if (mstore->AttachFile(folder->Ref(), buf, buf, typ) == 0)
	{
	    Refresh(pagenum, itemnum);
	    parent->RefreshContents();
	}
    }
    ClearScreen();
    Paint();
    return rtn;
}

int OpenMsgScreen::Run()
{
    return OMPageableListScreen::Run();
}

int OpenMsgScreen::EditSubject(char *buf)
{
    PopupPrompt *p = new PopupPrompt(this,
		"Change the subject and press Perform Change.", "CHANGESUBJECT",
		"Perform\n Change", 0, " Cancel\n Change");
    if (p)
    {
	int ch = p->Run(buf);
    	delete p;
    	return (ch == KB_FKEY_CODE(0) || ch == '\n') ? 0 : -1;
    }
    else return -1;
}

void OpenMsgScreen::EditMessageSubject()
{
    char buf[COLS+2];
    strncpy(buf, folder->Name(), COLS);
    buf[COLS] = '\0';
    if (EditSubject(buf)==0)
    {
	if (mstore->ChangeSubject(folder->Ref(), 0, buf) == 0)
	{
    	    SetLabel(subjfld, 5, 9, buf);
	    parent->RefreshContents();
	}
    }
    ClearScreen();
    Paint();
}

void OpenMsgScreen::EditItemSubject()
{
    char buf[COLS+2];
    if (cnt < 1) return;
    strncpy(buf, ((CompositeItem*)folder)->Child(itemnum)->Name(), COLS);
    buf[COLS] = '\0';
    if (EditSubject(buf)==0)
    {
	if (mstore->ChangeSubject(folder->Ref(), itemnum, buf)==0)
	{
	    Refresh(pagenum, itemnum);
	    parent->RefreshContents();
	}
    }
    ClearScreen();
    Paint();
}

Screen *OpenMsgScreen::EditItem()
{
    if (cnt < 1) return 0;
    FolderItem *child = ((CompositeItem*)folder)->Child(itemnum);
    if (child->IsDistList())
        return new EditDistListScreen(this, itemnum);
    else
    {
	char *tn = tmpnam(0);
	Debug1("OpenMsgScreen exporting for compose to %s", tn);
	if ((((AtomicItem*)child)->Export(tn)) == 0)
	{
	    int mustreplace = 0;
	    if (ExternalCompose(tn, child->TypeID()) != 0)
	    {
    		if (strcmp(child->Type(), "Text") == 0)
		{
                    EditScreen *s = new EditScreen(this, tn);
	            if (s)
	            {
		        s->Run();
	    	        if (s->HasFileChanged())
			    mustreplace = 1;
	    	        delete s;
			Paint();
		    }
		}
    		else DrawTitle("Cannot edit this item."); // check this
	    }
	    else mustreplace = 1;
	    if (mustreplace)
	    {
		mstore->ReplaceText(itemnum, tn);
	    	Refresh(pagenum, itemnum);
	    	parent->RefreshContents();
		Paint();
	    }
	}
	unlink(tn);
    }
    return 0;
}

Screen *OpenMsgScreen::HandleKey(int ch, int &done)
{
    if (ch == '\n') return HandleFKey(F2_PRI, done);
    else return OMPageableListScreen::HandleKey(ch, done);
}

Screen *OpenMsgScreen::ReadItem()
{
    if (cnt < 1) return 0;
    if (folder->Child(itemnum)->IsMessage())
	return new ReadMsgScreen(this,(Message*)folder->Child(itemnum),0,0);
    else
	return new ReadItemScreen(this, itemnum);
}

OpenMsgScreen::~OpenMsgScreen()
{
    DestructTrace("OpenMsgScreen");
}

//-------------------------------------------------------------------

OpenROMsgScreen::OpenROMsgScreen(Screen *parent_in, int inum)
    : OpenMsgScreen(parent_in, inum, 3, 0)
{
    ConstructTrace("OpenROMsgScreen");
    if (folder)
    {
	Message *m = (Message *)folder;
	AddLabel(7, 8, m->Priority() ? "YES" : "NO");
	AddLabel(7, 30, m->Sensitivity() ? "YES" : "NO");
	switch(m->RequestedAck())
	{
	case 0:
	    AddLabel(7, 50, "NONE");
	    break;
	case 4:
	    AddLabel(7, 50, "DELIVERY");
	    break;
	case 7:
	    AddLabel(7, 50, "RECEIPT");
	    break;
	case 9:
	    AddLabel(7, 50, "REPLY");
	    break;
	}
    }
}

void OpenROMsgScreen::Paint()
{
    OpenMsgScreen::Paint();
}

OpenROMsgScreen::~OpenROMsgScreen()
{
    DestructTrace("OpenROMsgScreen");
}

//------------------------------------------------------------------

OpenInMsgScreen::OpenInMsgScreen(Screen *parent_in, int inum)
    : OpenROMsgScreen(parent_in, inum)
{
    ConstructTrace("OpenInMsgScreen");
    fkeys = new FKeySet8by2(0, "  Read\n  Item",
			" Print\n  Item", "  File\n  Item",
			"  Save\n  Item:  Open\nMessage", "SaveItem\nas Text");
    assert(fkeys);
}

void OpenInMsgScreen::Paint()
{
    int canfile = (folder && cnt>1) ?
		(folder->Child(itemnum)->IsDistList()==0) : 0;
    int ismsg = (folder && cnt>1) ?
		(folder->Child(itemnum)->IsMessage()==1) : 0;
    fkeys->Enable(0, 3, canfile);
    fkeys->Enable(1, 3, canfile);
    fkeys->SetMode(ismsg);
    OpenROMsgScreen::Paint();
}

Screen *OpenInMsgScreen::HandleFKey(int fkeynum, int &done)
{
    done = 0;
    switch(fkeynum)
    {
    case F2_PRI: // read item
    case F2_SEC:
	return ReadItem();
    case F3_PRI: // print
    case F3_SEC:
	Print();
	break;
    case F4_PRI: // file
    case F4_SEC:
	FileItem();
	break;
    case F5_PRI: // save
	SaveItem(0);
	break;
    case F5_SEC: // open message
	if (cnt>0) return new OpenInMsgScreen(this, itemnum);
	break;
    case F6_PRI: // save as text
    case F6_SEC:
	SaveItem(1);
	break;
    case F7_PRI: // help
    case F7_SEC:
	Help("OPENING");
	break;
    case F8_PRI: // done
    case F8_SEC:
	done = 1;
	break;
    }
    return 0;
}

OpenInMsgScreen::~OpenInMsgScreen()
{
    DestructTrace("OpenInMsgScreen");
}

//------------------------------------------------------------------------

OpenFiledMsgScreen::OpenFiledMsgScreen(Screen *parent_in, int inum)
    : OpenROMsgScreen(parent_in, inum)
{
    ConstructTrace("OpenFiledMsgScreen");
    dlfkeys = new FKeySet8by2(" Other\n  Keys",
			"  Read\n  Item:  Edit\n  Item",
	    		"  Print\n  Item:EditItem\nSubject",
			" Delete\n  Item:",
	    		"  Save\n  Item:SaveItem\nas Text",
	    		"Include\nFile/Doc:Edit Msg\nSubject");
    msgfkeys = new FKeySet8by2(" Other\n  Keys",
			"  Read\n  Item:  Open\nMessage",
	    		"  Print\n  Item:",
			" Delete\n  Item:  File\n  Item",
	    		":SaveItem\nas Text",
	    		"Include\nFile/Doc:Edit Msg\nSubject");
    basicfkeys = new FKeySet8by2(" Other\n  Keys",
			"  Read\n  Item:  Edit\n  Item",
	    		"  Print\n  Item:EditItem\nSubject",
			" Delete\n  Item:  File\n  Item",
	    		"  Save\n  Item:SaveItem\nas Text",
	    		"Include\nFile/Doc:Edit Msg\nSubject");
    fkeys = dlfkeys;
    assert(dlfkeys && msgfkeys && basicfkeys);
}

void OpenFiledMsgScreen::Paint()
{
    if (folder && cnt>0)
    {
	FolderItem *fi = folder->Child(itemnum);
	if (fi->IsDistList()) fkeys = dlfkeys;
	else if (fi->IsMessage()) fkeys = msgfkeys;
	else fkeys = basicfkeys;
    }
    fkeys->Paint();
    OpenROMsgScreen::Paint();
}

Screen *OpenFiledMsgScreen::HandleFKey(int fkeynum, int &done)
{
    done = 0;
    switch(fkeynum)
    {
    case F1_PRI:
    case F1_SEC: // other keys
	ToggleFKeys();
	break;
    case F2_PRI: // read item
	return ReadItem();
    case F2_SEC: // edit item or open message
	if (cnt > 0)
	    if (fkeys == msgfkeys)
	        return new OpenFiledMsgScreen(this, itemnum);
	    else
	        return EditItem();
	break;
    case F3_PRI: // print
	Print();
	break;
    case F3_SEC: // edit item subject
	EditItemSubject();
	break;
    case F4_PRI: // delete
	DeleteChild();
	break;
    case F4_SEC: // file item
	FileItem();
	break;
    case F5_PRI: // save
	SaveItem(0);
	break;
    case F5_SEC: // save as text
	SaveItem(1);
	break;
    case F6_PRI: // include file/doc
	IncludeFile();
	break;
    case F6_SEC: // edit msg subject
	EditMessageSubject();
	break;
    case F7_PRI:
    case F7_SEC: // help
	Help("OPENING");
	break;
    case F8_PRI: 
    case F8_SEC: // done
	done = 1;
	break;
    }
    return 0;
}

OpenFiledMsgScreen::~OpenFiledMsgScreen()
{
    DestructTrace("OpenFiledMsgScreen");
    delete dlfkeys;
    delete msgfkeys;
    delete basicfkeys;
    fkeys = 0;
}

//---------------------------------------------------------------------
// Waste messages are handled the same as intray messages

OpenWasteMsgScreen::OpenWasteMsgScreen(Screen *parent_in, int inum)
    : OpenInMsgScreen(parent_in, inum)
{
    ConstructTrace("OpenWasteMsgScreen");
}

OpenWasteMsgScreen::~OpenWasteMsgScreen()
{
    DestructTrace("OpenWasteMsgScreen");
}

//---------------------------------------------------------------------

OpenRWMsgScreen::OpenRWMsgScreen(Screen *parent_in, int inum)
    : OpenMsgScreen(parent_in, inum, 0, 3)
{
    ConstructTrace("OpenRWMsgScreen");
    if (folder)
    {
	Message *m = (Message *)folder;
	prifld = AddField(new ToggleField(7,8,8,"NO@YES", ' ',0));
	sensfld = AddField(new ToggleField(7,30,7,"NO@YES", ' ',0));
	ackfld = AddField(new ToggleField(7,50,12,"NONE@DELIVERY@RECEIPT@REPLY", ' ',0));
	((ToggleField*)fields[prifld])->SetValue(m->Priority()?1:0);
	((ToggleField*)fields[sensfld])->SetValue(m->Sensitivity()?1:0);
	((ToggleField*)fields[ackfld])->SetValue(m->RequestedAck()/3);
    }
}

void OpenRWMsgScreen::Paint()
{
    OpenMsgScreen::Paint();
}

int OpenRWMsgScreen::Run()
{
    return OpenMsgScreen::Run();
}

void OpenRWMsgScreen::EditMessageSettings()
{
    MessageSettings mset;
    Message *m = (Message *)folder;
    m->GetSettings(mset);
    MsgSettingsScreen *mss = new MsgSettingsScreen(this, mset);
    assert(mss);
    mss->Run();
    delete mss;
    (void)m->SetSettings(mset);
    ((ToggleField*)fields[prifld])->SetValue(m->Priority()?1:0);
    ((ToggleField*)fields[sensfld])->SetValue(m->Sensitivity()?1:0);
    ((ToggleField*)fields[ackfld])->SetValue(m->RequestedAck()/3);
    ClearScreen();
    Paint();
}

void OpenRWMsgScreen::DeleteChild()
{
Debug1("In DeleteChild, itemnum %d\n", itemnum);
    int first, last;
    GetRange(first, last);
    if (cnt < 1)
	DrawTitle("Nothing to delete.");
    else if (first == 1l)
	DrawTitle("Deleting the distribution list makes no sense!");
    else
        OMPageableListScreen::DeleteChild();
}

OpenRWMsgScreen::~OpenRWMsgScreen()
{
    DestructTrace("OpenRWMsgScreen");
}

//---------------------------------------------------------------------

OpenOutMsgScreen::OpenOutMsgScreen(Screen *parent_in, int inum)
    : OpenRWMsgScreen(parent_in, inum)
{
    ConstructTrace("OpenOutMsgScreen");
    fkeys = new FKeySet8by2(" Other\n  Keys",
			"  Read\n  Item:  Edit\n  Item",
	    		"  Print\n  Item:EditItem\nSubject",
			" Delete\n  Item:  File\n  Item",
	    		"Message\nSettings:  Save\n  Item",
	    		"Include\nFile/Doc:SaveItem\nas Text",
			"  Help:Edit Msg\nSubject",
			"  Done");
    assert(fkeys);
}

void OpenOutMsgScreen::Paint()
{
    int canfile = (folder && cnt>0) ?
		(folder->Child(itemnum)->IsDistList()==0) : 0;
    fkeys->Enable(1, 3, canfile);
    OpenRWMsgScreen::Paint();
}

Screen *OpenOutMsgScreen::HandleFKey(int fkeynum, int &done)
{
    done = 0;
    switch(fkeynum)
    {
    case F1_PRI:
    case F1_SEC: // other keys
	ToggleFKeys();
	break;
    case F2_PRI: // read item
	return ReadItem();
    case F2_SEC: // edit item
	return EditItem();
    case F3_PRI: // print
	Print();
	break;
    case F3_SEC: // edit item subject
	EditItemSubject();
	break;
    case F4_PRI: // delete
	DeleteChild();
	break;
    case F4_SEC: // file item
	FileItem();
	break;
    case F5_PRI: // message settings
	EditMessageSettings();
	break;
    case F5_SEC: // save
	SaveItem(0);
	break;
    case F6_PRI: // include file/doc
	IncludeFile();
	break;
    case F6_SEC: // save as text
	SaveItem(1);
	break;
    case F7_PRI: // help
	Help("OPENING");
	break;
    case F7_SEC: // edit msg subject
	EditMessageSubject();
	break;
    case F8_PRI: 
    case F8_SEC: // done
	done = 1;
	break;
    }
    return 0;
}

OpenOutMsgScreen::~OpenOutMsgScreen()
{
    DestructTrace("OpenOutMsgScreen");
}

//----------------------------------------------------------------------

OpenNewMsgScreen::OpenNewMsgScreen(Screen *parent_in, int inum)
    : OpenRWMsgScreen(parent_in, inum), result(1)
{
    ConstructTrace("OpenNewMsgScreen");
    fkeys = new FKeySet8by2(" Other\n  Keys",
			"  Read\n  Item:  Edit\n  Item",
	    		" Delete\n  Item:EditItem\nSubject",
	    		"Include\nFile/Doc:",
			"  Hold\nMessage:Message\nSettings",
	    		"  Mail\nMessage:Edit Msg\nSubject",
			"  Help",
			"  Cancel");
    assert(fkeys);
}

void OpenNewMsgScreen::Paint()
{
    OpenRWMsgScreen::Paint();
}

int OpenNewMsgScreen::Run()
{
    int rtn;
    GetItems();
    Paint();
    while ((rtn=IncludeFile()) == 0);
    return OpenRWMsgScreen::Run();
}

int OpenNewMsgScreen::Result() // returns 0 (mailed) 1 (held) or -1 (deleted)
{
    return result;
}

Screen *OpenNewMsgScreen::HandleFKey(int fkeynum, int &done)
{
    done = 0;
    switch(fkeynum)
    {
    case F1_PRI:
    case F1_SEC: // other keys
	ToggleFKeys();
	break;
    case F2_PRI: // read item
	return ReadItem();
    case F2_SEC: // edit item
	return EditItem();
    case F3_PRI: // delete
	DeleteChild();
	break;
    case F3_SEC: // edit item subject
	EditItemSubject();
	break;
    case F4_PRI: // include file/doc
	IncludeFile();
	break;
    case F5_PRI: // hold message
	done = 1; // just return but don't delete message from out tray
	break;
    case F5_SEC: // message settings
	EditMessageSettings();
	break;
    case F6_PRI: // mail message
	FolderItem *parentf = folder->Parent();
	long itemnum = folder->ItemNum();
	folder->Close();
	{
            MessageSettings mset; // must be local to block
            ((Message*)folder)->GetSettings(mset);
	    if (mstore->MailMessage(mset, parentf->Ref(), itemnum) == 0)
	    {
		if (mset.GetAckLevel() || mset.GetDate() > time(0))
		    parent->RefreshContents(); // to update tracking area 
	        folder->Open(msglines);
	        result = 0;
	        done = 1;
	    }
	    else
	    {
	        folder->Open(msglines);
	        RefreshContents();
	        Paint();
	    }
	}
	break;
    case F6_SEC: // edit msg subject
	EditMessageSubject();
	break;
    case F7_PRI:
    case F7_SEC: // help
	Help("CREATING");
	break;
    case F8_PRI: 
    case F8_SEC: // done
	if (Confirm("You have not mailed this message.",
		    "Press Confirm if you wish to discard the message",
		    "Confirm\nDelete", " Cancel\n Delete"))
	{
	    done = 1;
	    result = -1;
	}
	else Paint();
	break;
    }
    return 0;
}

OpenNewMsgScreen::~OpenNewMsgScreen()
{
    DestructTrace("OpenNewMsgScreen");
}

//----------------------------------------------------------------

OutTrayScreen::OutTrayScreen(Screen *parent_in)
    : OMPageableListScreen(parent_in, "Out Tray",
		"Choose a message and a function.", 4, 0, 13, 7, 2,
		"You have no messages in your Out Tray.")
{
    ConstructTrace("OutTrayScreen");
    AddLabel(2, 0, FormatAmount(cnt, "Message"));
    AddLabel(5,  4, "Subject");
    AddLabel(5, 49, "Date");
    AddLabel(5, 59, "Status");
    fkeys = new FKeySet8by2("* Other\n  Keys", "*  Read:*  File",
			"* Print:*  Open\n Message",
			"* Delete:*  Edit\nDistList",
			" Create:", "*  Mail\nMessage:",
			"  Help", "  Done");
    assert(fkeys);
}

void OutTrayScreen::UpdateCount()
{
    SetLabel(0, 2, 0, FormatAmount(cnt, "Message"));
}

char *OutTrayScreen::FormatItem(FolderItem *fi)
{
    static char buf[82];
    assert(fi->IsMessage());
    char c = ' ', *tag = "HELD"; // is HELD the default??
    Message *m = (Message *)fi;
    // NB THERE ARE OTHER TYPES AS WELL !!
    if (m->HasDistListError())
    {
        tag = "ERROR";
        c = 'D';
    }
    else if (m->IsError())
    {
        tag = "ERROR";
        c = 'E';
    }
    else if (strcmp(m->Type(), "Reply")==0)
        c = 'R';
    sprintf(buf, "%c   %-45.44s%-10s%-21s", 
	    c, m->Name(), FormatDate(m->CreateDate()), tag);
    return buf;
}    

Screen *OutTrayScreen::HandleKey(int ch, int &done)
{
    if (ch == '\n') return HandleFKey(F2_PRI, done);
    else return OMPageableListScreen::HandleKey(ch, done);
}

Screen *OutTrayScreen::HandleFKey(int fkeynum, int &done)
{
    done = 0;
    switch (fkeynum)
    {
    case F1_PRI:
    case F1_SEC: // other keys
	ToggleFKeys();
	break;
    case F2_PRI: // Read
	if (cnt>0)
	    return new ReadMsgScreen(this,
				     (Message *)folder->Child(itemnum),0,0);
	break;
    case F3_PRI: // print
	Print();
	break;
    case F4_PRI: // delete
	DeleteChild();
	break;
    case F5_PRI: // create
	CreateNewScreen *cns = new CreateNewScreen(this);
	assert(cns);
	cns->Run();
	long newmsgnum = cns->MessageNum();
	delete cns;
	if (newmsgnum > 0)
	{
	    int oldpage = pagenum, olditem = itemnum;
	    Refresh((newmsgnum-1)%msglines, newmsgnum); // go to held msg
	    OpenNewMsgScreen *oms = new OpenNewMsgScreen(this, newmsgnum);
	    assert(oms);
	    oms->Run();
	    int result = oms->Result();
	    delete oms;
	    if (result != 1) // message cancelled or mailed?
	    {
    		mstore->Delete(folder->Ref(), newmsgnum);
	        Refresh(oldpage, olditem); // go to original message
		Paint();
		if (result == 0)
		    DrawTitle("Your message has been mailed (or deferred)"
				" as requested. Choose a function.");
	    }
	    else Paint();
	}
	else if (newmsgnum < 0)
	{
	    Paint();
	    DrawTitle("Your message has been mailed (or deferred)"
			" as requested. Choose a function.");
	}
	else Paint();
	break;
    case F6_PRI: // mail
	{
            MessageSettings mset;
            Message *m = (Message *)(folder->Child(itemnum));
            m->GetSettings(mset);
	    if (mstore->MailMessage(mset, folder->Ref(), itemnum) == 0)
	    {
	        DeleteChild();
	        RefreshContents();
	        Paint();
	        DrawTitle("Your message has been mailed (or deferred) as requested. Choose a function.");
	    }
	}
	break;
    case F7_PRI:
    case F7_SEC: // help
	Help("OUTTRAY");
	break;
    case F8_PRI:
    case F8_SEC: // done 
	done = 1;
	break;
    case F2_SEC: // file
	FileItem();
	break;
    case F3_SEC: // open
	if (cnt> 0) return new OpenOutMsgScreen(this, itemnum);
	break;
    case F4_SEC: // edit distlist
	if (cnt>0)
	    return new EditDistListScreen(this,
					 (Message *)folder->Child(itemnum));
	break;
    }
    return 0;
}

OutTrayScreen::~OutTrayScreen()
{
    DestructTrace("OutTrayScreen");
}

//-----------------------------------------------------------------------

PendingTrayScreen::PendingTrayScreen(Screen *parent_in)
    : OMPageableListScreen(parent_in, "Pending Tray",
		"Choose a message and a function.", 4, 0,
		13, 7, 3, "You have no messages in your Pending Tray.")
{
    ConstructTrace("PendingTrayScreen");
    AddLabel(3, 0, FormatAmount(cnt, "Message"));
    AddLabel(5,  4, "Subject");
    AddLabel(5, 50, "Deferred");
    AddLabel(5, 62, "Mailed");
    fkeys = new FKeySet8by2("*  File", "*  Read", "* Print", "* Delete",
			"* Track\nMessage", "*Forward", "  Help", "  Done");
    assert(fkeys);
}

void PendingTrayScreen::UpdateCount()
{
    SetLabel(0, 3, 0, FormatAmount(cnt, "Message"));
}

char *PendingTrayScreen::FormatItem(FolderItem *fi)
{
    static char buf[82];
    char *defdate = "", *sentdate = "";
    assert(fi->IsMessage());
    Message *m = (Message *)fi;
    if (m->DeferredDate() > time(0))
	defdate = FormatDate(m->DeferredDate());
    else if (m->DeferredDate() > 0l)
	sentdate = FormatDate(m->DeferredDate());
    else
	sentdate = FormatDate(m->CreateDate());
    sprintf(buf, "    %-47.45s%-11.10s%-18s", m->Name(), defdate, sentdate);
    return buf;
}

Screen *PendingTrayScreen::HandleKey(int ch, int &done)
{
    if (ch == '\n') return HandleFKey(F2_PRI, done);
    else return OMPageableListScreen::HandleKey(ch, done);
}

Screen *PendingTrayScreen::HandleFKey(int fkeynum, int &done)
{
    Message *m = (folder && cnt>0) ? (Message *)folder->Child(itemnum) : 0;
    done = 0;
    switch (fkeynum)
    {
    case F1_PRI: // file
	FileItem();
	break;
    case F2_PRI: // read
	if (m) return new ReadMsgScreen(this, m,0,1);
	break;
    case F3_PRI: // print
	Print();
	break;
    case F4_PRI: // delete
	DeleteChild();
	break;
    case F5_PRI: // track message
	if (cnt>0 && mstore->OpenFolder(itemnum,1) == 0)
	{
	    mstore->FirstPage();
	    Screen *s = new TrackMsgScreen(this);
	    if (s)
	    {
		s->Run();
		delete s;
	        mstore->CloseFolder();
		ClearScreen();
		Paint();
	    }
	    else mstore->CloseFolder();
	}
	break;
    case F6_PRI: // forward
	if (m) return new CreateForwardScreen(this, m, GetQuotedPart(m));
	break;
    case F7_PRI: // help
	Help("PENDINGTRAY");
	break;
    case F8_PRI: // done
	done = 1;
	break;
    }
    return 0;
}

PendingTrayScreen::~PendingTrayScreen()
{
    DestructTrace("PendingTrayScreen");
}

//-----------------------------------------------------------------------

ReadItemScreen::ReadItemScreen(Screen *parent_in, int itemnum)
    : OMCompositePageableScreen(parent_in, "Read Item",
	     "Press Done when you have finished reading.", MAX_MSG_LINES, 0,
		   MAX_MSG_LINES, itemnum)
{
    ConstructTrace("ReadItemScreen");
    fkeys = new FKeySet8by2(0,0," Print", 0, 0, 0);
    assert(fkeys);
}

void ReadItemScreen::GetItems()
{
    if (folder && folder->IsAtomic())
    {
	char *last = "End of Item.";
        AtomicItem *AI = (AtomicItem *)folder;
        for (int r = 0; r < msglines; r++)
	{
	    char *ln = (char*)AI->Text(r);
	    if (ln==0)
	    {
	        SetLabel(r, 4+r, 0, last);
		last = 0;
	    }
	    else SetLabel(r, 4+r, 0, ln);
	}
    }
}

Screen *ReadItemScreen::HandleFKey(int fkeynum, int &done)
{
    done = 0;
    switch (fkeynum)
    {
    case F3_PRI: // print
	if (folder) Print(folder->Parent()->Ref(), folder->ItemNum());
	break;
    case F7_PRI: // help
	Help("READING");
	break;
    case F8_PRI: // done
	done = 1;
	break;
    }
    return 0;
}

ReadItemScreen::~ReadItemScreen()
{
    DestructTrace("ReadItemScreen");
}

//-----------------------------------------------------------------------

ReadMsgScreen::ReadMsgScreen(Screen *parent_in, Message *msg_in, int can_reply,
		int can_forward)
    : OMPageableScreen(parent_in, "Read Message",
	     "Press Done when you have finished reading.", MAX_MSG_LINES, 0,
		     MAX_MSG_LINES), realmsg(msg_in)
{
    ConstructTrace("ReadMsgScreen");
    if (realmsg->IsUnread())
    {
	realmsg->MarkAsRead();
	parent->RefreshContents();
    }
    msg = new ViewedMessage(msg_in);
    if (msg)
    {
	msg->Open();
        is_lastpage = (msg->FirstPage() < msglines);
        GetItems();
    }
    else is_lastpage = 1;
    fkeys = new FKeySet8by2(0, " Create", " Print", " Delete",
			can_reply ? " Reply" : 0,
			can_forward ? "Forward" : 0,
			"  Help", "  Done");
    assert(fkeys);
}

ReadMsgScreen::~ReadMsgScreen()
{
    DestructTrace("ReadMsgScreen");
    if (msg) msg->Close();
    delete msg;
}

void ReadMsgScreen::GetItems()
{
    char *last = "End of Message.";
    for (int i = 0; i < msglines; i++)
    {
	if (msg==0 || msg->Text(i)==0)
	{
	    SetLabel(i, 4+i, 0, last);
	    last = 0;
	}
	else SetLabel(i, 4+i, 0, (char*)msg->Text(i));
    }
}

int ReadMsgScreen::PrevPage()
{
    return msg ? msg->PrevPage() : 0;
}

int ReadMsgScreen::NextPage()
{
    return msg ? msg->NextPage() : 0;
}

Screen *ReadMsgScreen::HandleKey(int ch, int &done)
{
    if (msg && ch=='\n')
    {
	done = 0;
	PageDown();
	return 0;
    }
    return PageableScreen::HandleKey(ch, done);
}

Screen *ReadMsgScreen::HandleFKey(int fkeynum, int &done)
{
    done = 0;
    switch (fkeynum)
    {
    case F2_PRI: // create
	return new CreateNewScreen(this);
    case F3_PRI: // print
	if (msg)
	    Print(realmsg->Parent()->Ref(), realmsg->ItemNum());
	break;
    case F4_PRI: // delete
	if (msg)
	    DeleteMessage();
	done = 1;
	break;
    case F5_PRI: // reply
	if (msg)
	    return new CreateReplyScreen(this,
				realmsg, 0, GetQuotedPart(realmsg));
	break;
    case F6_PRI: // forward
	if (msg)
	    return new CreateForwardScreen(this,
				realmsg, GetQuotedPart(realmsg));
	break;
    case F7_PRI: // help
	Help("READING");
	break;
    case F8_PRI: // done
	done = 1;
	break;
    }
    return 0;
}

void ReadMsgScreen::DeleteMessage()
{
    DrawTitle("Deleting.");
    mstore->Delete(realmsg->Parent()->Ref(), realmsg->ItemNum());
    RefreshContents();
}

//---------------------------------------------------------------

FilePathsScreen::FilePathsScreen(Screen *parent_in)
    : OMFormScreen(parent_in, "File Paths",
		"Set the files paths and press Done.", 4, 3)
{
    ConstructTrace("FilePathsScreen");
    DataField *df;
    AddLabel(3, 4, "File Types File:");
    df = new DataField(3, 30, 40);
    assert(df);
    df->SetValue(config->Get("filetypes"));
    typfld = AddField(df);
    AddLabel(5, 30, "(Use empty string to get file types from server).");
    AddLabel(8, 4, "Linked Applications File:");
    df = new DataField(8, 30, 40);
    assert(df);
    df->SetValue(config->Get("links"));
    lnkfld = AddField(df);
    AddLabel(11, 4, "Hypertext Help File:");
    df = new DataField(11, 30, 40);
    assert(df);
    df->SetValue(config->Get("helpfile"));
    hlpfld = AddField(df);
    fkeys = new FKeySet8by2(" Cancel\nChanges", 0, 0, 0, 0, 0, "  Help", "  Done");
    assert(fkeys);
}

void FilePathsScreen::SaveConfigElt(int fld, char *fldname)
{
    char *newval = ((DataField*)fields[fld])->Value();
    if (newval == 0) newval = "";
    char *oldval = config->Get(fldname);
    if (oldval[0] == 0) oldval = "";
    if (strcmp(oldval, newval) != 0)
        config->Set(fldname, newval);
}

Screen *FilePathsScreen::HandleFKey(int fkeynum, int &done)
{
    done = 0;
    switch (fkeynum)
    {
    case F7_PRI:
	Help("CONFIGURING");
	break;
    case F8_PRI:
        SaveConfigElt(hlpfld, "helpfile");
        SaveConfigElt(lnkfld, "links");
        SaveConfigElt(typfld, "filetypes");
	// fall thru
    case F1_PRI:
	done = 1;
	break;
    }
    return 0;
}

FilePathsScreen::~FilePathsScreen()
{
    DestructTrace("FilePathsScreen");
}

//---------------------------------------------------------------

void ReconfigScreen::GetConfigElt(int fld, int &val)
{
    char *newval = ((DataField*)fields[fld])->Value();
    if (newval  && newval[0])
	val = atoi(newval);
}

void ReconfigScreen::SaveConfigElt(int fld, char *fldname)
{
    char *newval = ((DataField*)fields[fld])->Value();
    if (newval == 0) newval = "";
    char *oldval = config->Get(fldname);
    if (oldval[0] == 0) oldval = "";
    if (strcmp(oldval, newval) != 0)
        config->Set(fldname, newval);
}

void ReconfigScreen::SaveConfig()
{
    // Client side config (.omsrc)
    SaveConfigElt(usrfld, "user");
    SaveConfigElt(srvfld, "server");
    SaveConfigElt(prtfld, "print");
    SaveConfigElt(polfld, "poll");
    // Server side config
    int wbclear, tabsize, readdl, printdl, xtra, savemail;
    mstore->GetUserConfig(wbclear, tabsize, readdl, printdl, xtra, savemail);
    GetConfigElt(cwfld, wbclear);
    GetConfigElt(tabfld, tabsize);
    GetConfigElt(rdlfld, readdl);
    GetConfigElt(pdlfld, printdl);
    xtra = ((ToggleField*)fields[xtrfld])->Value();
    savemail = ((ToggleField*)fields[ssfld])->Value();
    mstore->SetUserConfig(wbclear, tabsize, readdl, printdl, xtra, savemail);
}

ReconfigScreen::ReconfigScreen(Screen *parent_in)
    : OMFormScreen(parent_in, "Configuration",
		"Choose a function to configure or press Done.", 16, 10)
{
    DataField *df;
    char *v, buf[10];
    ConstructTrace("ReconfigScreen");

    int wbclear, tabsize, readdl, printdl, xtra, savemail;
    mstore->GetUserConfig(wbclear, tabsize, readdl, printdl, xtra, savemail);
    
    Debug6("Returned config(%d,%d,%d,%d,%d,%d)",
		wbclear, tabsize, readdl, printdl, xtra, savemail);

    AddLabel(3, 4, "User Name:");
    df = new DataField(3, 15, 45);
    assert(df);
    df->SetValue(config->Get("user"));
    usrfld = AddField(df);

    AddLabel(4, 4, "Server:");
    df = new DataField(4, 15, 45);
    assert(df);
    df->SetValue(config->Get("server"));
    srvfld = AddField(df);

    AddLabel(6, 4, "Check for new mail every");
    df = new NumberField(6, 30, 4, 60, 86400);
    assert(df);
    v = config->Get("poll");
    if (v[0] == '\0') v = "60";
    df->SetValue(v);
    polfld = AddField(df);
    AddLabel(6, 35, "seconds");

    AddLabel(7, 4, "Remove from waste basket after");
    df = new NumberField(7, 36, 3, 1, 21);
    assert(df);
    sprintf(buf, "%d", wbclear);
    df->SetValue(buf);
    cwfld = AddField(df);
    AddLabel(7, 40, "days");

    AddLabel(9, 4, "Save copy of outgoing messages:");
    ToggleField *tf = new ToggleField(9, 36, 3, "NO@YES", ' ');
    assert(tf);
    tf->SetValue(savemail);
    ssfld = AddField(tf);

    AddLabel(10, 4, "Show extra message details:");
    tf = new ToggleField(10, 36, 3, "NO@YES", ' ');
    assert(tf);
    tf->SetValue(xtra);
    xtrfld = AddField(tf);

    AddLabel(12, 4, "Print command:");
    df = new DataField(12, 20, 40);
    assert(df);
    v = config->Get("print");
    if (v[0] == '\0') v = "lp !";
    df->SetValue(v);
    prtfld = AddField(df);

    AddLabel(13, 4, "Distribution list lines when printing:");
    df = new NumberField(13, 44, 16, 0, 9999);
    assert(df);
    sprintf(buf, "%d", printdl);
    df->SetValue(buf);
    pdlfld = AddField(df);

    AddLabel(14, 4, "Distribution list lines when viewing:");
    df = new NumberField(14, 44, 16, 0, 16);
    assert(df);
    sprintf(buf, "%d", readdl);
    df->SetValue(buf);
    rdlfld = AddField(df);

    AddLabel(15, 4, "Tab stop size:");
    df = new NumberField(15, 20, 3, 1, 79);
    assert(df);
    sprintf(buf, "%d", tabsize);
    df->SetValue(buf);
    tabfld = AddField(df);

    AddLabel(17, 4, "Auto-Forward:");
    AddLabel(19, 4, "Auto-Reply:");
    
    int afa, ara, ic, dfm, koc, fprm, fpsm, fccm, rtam, rtum, rtrra;
    (void)mstore->GetAutoActions(afa, ara, ic, dfm, koc, fprm, fpsm, fccm,
				 	rtam, rtum, rtrra);
    afalbl = AddLabel(17, 18, "");
    aralbl = AddLabel(19, 18, "");
    SetAutoLabels();

    fkeys = new FKeySet8by2(" Cancel\nChanges", "Password",
			"  Auto\nForward", "  Auto\n Reply",
			0, "  File\n  Paths",
			"  Help", "  Done");
    assert(fkeys);
}

void ReconfigScreen::SetAutoLabels()
{
    int afa, ara, ic, dfm, koc, fprm, fpsm, fccm, rtam, rtum, rtrra;
    (void)mstore->GetAutoActions(afa, ara, ic, dfm, koc, fprm, fpsm, fccm,
				 	rtam, rtum, rtrra);
    SetLabel(afalbl, 17, 18, afa ? "YES" : "NO ");
    SetLabel(aralbl, 19, 18, ara ? "YES" : "NO ");
}

Screen *ReconfigScreen::HandleFKey(int fkeynum, int &done)
{
    done = 0;
    Screen *s;
    switch (fkeynum)
    {
    case F2_PRI: // change password
	PopupPrompt *p = new PopupPrompt(this,
		"Type your current password and press Perform.",
		"PASSWORD", "Perform\n Change", 0, " Cancel\n Change", 0);
	assert(p);
	char buf[82];
	for (;;)
	{
	    buf[0] = '\0';
	    if (p->Run(buf) == KB_FKEY_CODE(7)) break;
	    if (strcmp(buf, mstore->Password())==0)
	    {
	        delete p;
	        p = new PopupPrompt(this, 
		    "Type your new password (8 chars max.) and press Perform.",
		    "PASSWORD", "Perform\n Change", 0, " Cancel\n Change", 0);
		assert(p);
		buf[0] = '\0';
	        if (p->Run(buf) != KB_FKEY_CODE(7))
			mstore->ChangePassword(buf);
		break;
	    }
	    else
	    {
		strcpy(buf, "Incorrect password.");
		fill(buf,80,' ');
		BoldOn();
		ReverseOn();
		PutString(0,0,buf);
		ReverseOff();
		BoldOff();
	    }
	}
	delete p;
	ClearScreen();
	Paint();
	break;
    case F3_PRI:
	s = new AutoForwardScreen(this);
	if (s) s->Run();
	delete s;
	SetAutoLabels();
	break;
    case F4_PRI:
	s = new AutoReplyScreen(this);
	if (s) s->Run();
	delete s;
	SetAutoLabels();
	break;
    case F6_PRI:
	return new FilePathsScreen(this);
	break;
    case F7_PRI:
	Help("CONFIGURING");
	break;
    case F8_PRI:
	SaveConfig();
	// fall thru
    case F1_PRI:
	done = 1;
	break;
    }
    return 0;
}

ReconfigScreen::~ReconfigScreen()
{
    DestructTrace("ReconfigScreen");
}

//---------------------------------------------------------------------

TrackMsgScreen::TrackMsgScreen(Screen *parent_in)
    : OMPageableScreen(parent_in, "Track Message",
	     "Check your acknowledgements then press done.",
		18, 0, 11)
{
    Message *msg = (Message*)mstore->Folder();
    ConstructTrace("TrackMsgScreen");
    fkeys = new FKeySet8by2(0,0,0,0,0,0);
    assert(fkeys);
    SetLabel(msglines, 4, 0, "Name:");
    SetLabel(msglines+1, 4, 6, msg->Child(1l)->Name());
    SetLabel(msglines+2, 6, 0, "Acknowledgement Requested:");
    SetLabel(msglines+3, 6, 28, 0);
    switch (msg->RequestedAck())
    {
    case 4: // delivery
    	SetLabel(msglines+3, 6, 28, "Delivered ACK");
	break;
    case 7: // read
    	SetLabel(msglines+3, 6, 28, "Read ACK");
	break;
    case 9: // reply
    	SetLabel(msglines+3, 6, 28, "Reply ACK");
	break;
    }
    SetLabel(msglines+4, 6, 61, "Status");
}

void TrackMsgScreen::ClearItems(int from)
{
    for (int r = from; r < msglines; r++)
	SetLabel(r, 8+r, 0, "TO:");
}

void TrackMsgScreen::GetItems()
{
    Message *msg = (Message*)mstore->Folder();
    ClearItems(0);
restart:
    if (mstore->OpenDListTF(msg->Child(1l)) != 0)
	return;
    // skip leading pages (quite inefficiently), and also make sure
    // that the page number is valid (if not, fix it and restart).
    for (int pg = 0; pg < pagenum; pg++)
    {
        for (int i = 0; i < msglines; i++)
	{
	    int typ, acklvl;
    	    OMAddress *newaddr = mstore->ReadDListRecord(typ, acklvl);
	    if (newaddr == 0)
	    {
		if (i == 0 && pg > 0) // end on prev page
		    pagenum = pg-1;
		else // end on this page
		    pagenum = pg;
    		mstore->CloseDListTF(1);
		goto restart;
	    }
	    delete newaddr;
	}
    }
    is_lastpage = 0;
    for (int r = 0; r < msglines; r++)
    {
	char buf[1024], *styp, *acktyp;
	int typ, acklvl;
    	OMAddress *newaddr = mstore->ReadDListRecord(typ, acklvl);
	if (newaddr == 0)
	{
	    is_lastpage = 1; // above assumption was false
	    break;
	}
	switch(typ)
	{
	case 0: styp = "FROM:";	break;
	case 1: styp = "TO:";	break;
	case 2: styp = "CC:";	break;
	case 4: styp = "BCC:";	break;
	default: styp = "????:";break;
	}
	switch(acklvl)
	{
	case 0:
	case 1:
	case 2: acktyp = "";			break;
	case 3: acktyp = "Non-Delivery ACK";	break;
	case 4: acktyp = "Delivered ACK";	break;
	case 5: acktyp = "Auto-Forward ACK";	break;
	case 6: acktyp = "Deleted ACK";		break;
	case 7: acktyp = "Read ACK";		break;
	case 8: acktyp = "Auto-Reply ACK";	break;
	case 9: acktyp = "Reply ACK";		break;
	default: acktyp = "????";		break;
	}
	char *xa = newaddr->GetExternal();
	sprintf(buf,"%-6s%-55s%-20s", styp, xa, acktyp);
	delete [] xa;
	delete newaddr;
	SetLabel(r, 8+r, 0, buf);
    }
    mstore->CloseDListTF(1);
}

int TrackMsgScreen::PrevPage() // for now
{
    if (pagenum > 0)
    {
	pagenum--;
    	GetItems();
    }
}

int TrackMsgScreen::NextPage()
{
    if (!is_lastpage)
    {
	pagenum++;
        GetItems();
    }
}

Screen *TrackMsgScreen::HandleFKey(int fkeynum, int &done)
{
    done = 0;
    switch (fkeynum)
    {
    case F7_PRI: // help
	Help("TRACKING");
	break;
    case F8_PRI: // done
	done = 1;
	break;
    }
    return 0;
}

TrackMsgScreen::~TrackMsgScreen()
{
    DestructTrace("TrackMsgScreen");
}

//---------------------------------------------------------------------

UserFolderScreen::UserFolderScreen(Screen *parent_in)
    : OMScreen(parent_in, "Main", "Select an area or choose a function.", 5),
      menu(6), mustrefresh(0)
{
    ConstructTrace("UserFolderScreen");
    AddLabel(4, 0, "User:");
    AddLabel(4, 6, mstore->OMUserName());
    AddLabel(5, 65, "Date:");
    AddLabel(8, 16, "Area");
    AddLabel(8, 52, "Contents");
    // 'cos GetItems clobbers the new mail setting we have a kludge...
    if (mstore->NewMail()>0) mustrefresh = -1;
    GetItems();
    fkeys = new FKeySet8by2(" Select\n  Area", 0, 0, 0, 0,
			" Config", " Help", " Exit\nADV'MAIL");
    assert(fkeys);
}

void UserFolderScreen::GetItems()
{
    char buf[80];
    if (mstore->OpenFolder(1) == 0)
    {
        sprintf(buf, "%-38s%6d", "In Tray", mstore->Folder()->Size());
        menu.AddEntry(0, 9, 16, buf);
        mstore->CloseFolder();
    }
    if (mstore->OpenFolder(2) == 0)
    {
        sprintf(buf, "%-38s%6d", "Out Tray", mstore->Folder()->Size());
        menu.AddEntry(1, 10, 16, buf);
        mstore->CloseFolder();
    }
    if (mstore->OpenFolder(4) == 0)
    {
        sprintf(buf, "%-38s%6d", "Filing Cabinet", mstore->Folder()->Size());
        menu.AddEntry(2, 12, 16, buf);
        mstore->CloseFolder();
    }
    if (mstore->OpenFolder(5) == 0)
    {
        sprintf(buf, "%-38s%6d", "Distribution Lists", mstore->Folder()->Size());
        menu.AddEntry(3, 13, 16, buf);
        mstore->CloseFolder();
    }
    if (mstore->OpenFolder(3) == 0)
    {
        sprintf(buf, "%-38s%6d", "Pending Tray", mstore->Folder()->Size());
        menu.AddEntry(4, 15, 16, buf);
        mstore->CloseFolder();
    }
    if (mstore->OpenFolder(6) == 0) // check this number!!
    {
        sprintf(buf, "%-38s%6d", "Bulletin Boards", mstore->Folder()->Size());
        menu.AddEntry(5, 17, 16, buf);
        mstore->CloseFolder();
    }
}

void UserFolderScreen::Paint()
{
    OMScreen::Paint();
    time_t tm;
    (void)time(&tm);
    struct tm *tmp = localtime(&tm);
    char buf[20];
    sprintf(buf,"%02d/%02d/%02d",tmp->tm_mday, tmp->tm_mon+1, tmp->tm_year);
    PutString(5,71,buf);
    // update in tray contents count
    if (mustrefresh || mstore->NewMail() > 0)
    {
	if (mustrefresh>=0) GetItems(); // not first time so refresh
	if (mustrefresh <= 0) // first time or new mail
	    DrawTitle("New mail has arrived. Choose a function or press Help for more information.");
        mustrefresh = 0;
    }
    menu.Paint();
    SetCursor(0,0);
    RefreshScreen();
}

Screen *UserFolderScreen::NextScreen()
{
    switch(menu.Current())
    {
    case 0:
	mustrefresh = 1; // refresh on return
    	return new InTrayScreen(this);
    case 1:
    	return new OutTrayScreen(this);
    case 2:
    	return new FileTrayScreen(this);
    case 3:
    	return new DistListTrayScreen(this);
    case 4:
    	return new PendingTrayScreen(this);
    case 5:
	return new BBAreaScreen(this);
    }
    return 0;
}

Screen *UserFolderScreen::HandleKey(int ch, int &done)
{
    done = 0;
    if (ch == KB_UP)
	menu.Up();
    else if (ch == KB_DOWN)
	menu.Down();
    else if (ch == '\n')
	return NextScreen();
    else return OMScreen::HandleKey(ch, done);	
}

Screen *UserFolderScreen::HandleFKey(int fkeynum, int &done)
{
    done = 0;
    switch (fkeynum)
    {
    case F1_PRI:
	return NextScreen();
	break;
    case F6_PRI:
	return new ReconfigScreen(this); // config
	break;
    case F7_PRI:	// help
	Help("MAINSCREEN");
	break;
    case F8_PRI: // exit ADVMAIL
	done = Quit();
	break;
    }
    return 0;
}

void UserFolderScreen::RefreshContents()
{
    mustrefresh = 1;
}

int UserFolderScreen::Quit()
{
    return Confirm(0, "Press Confirm Exit to leave Advancemail/Remote.",
			" Confirm\n  Exit", " Cancel\n  Exit");
}

UserFolderScreen::~UserFolderScreen()
{
    DestructTrace("UserFolderScreen");
}

