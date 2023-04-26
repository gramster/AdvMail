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
#include "screens2.h"

FileTypeToggle::FileTypeToggle(int r_in, int c_in, int w_in,
		 		int nextchar_in, int prevchar_in)
    : ToggleField(r_in, c_in, w_in, 0, nextchar_in, prevchar_in)
{
    ConstructTrace("FileTypeToggle");
    cnt = mstore->filetypecnt;
    for (int i = 0; i < cnt; i++)
        if (mstore->filetypes[i].GetType() == 1167)
	{
	    value = i;
	    break;
	}
}

char *FileTypeToggle::GetText()
{
    return mstore->filetypes[value].GetName();
}

int FileTypeToggle::GetType()
{
    return mstore->filetypes[value].GetType();
}

FileTypeToggle::~FileTypeToggle()
{
    DestructTrace("FileTypeToggle");
}

//--------------------------------------------------------------------------

void SearchDirPopup::HandleKey(int ch, int &done, int &rtn)
{
    if (ch == '\n') HandleFKey(KB_FKEY_CODE(0), done, rtn);
    OMListBoxPopup::HandleKey(ch, done, rtn);
}

void SearchDirPopup::HandleFKey(int ch, int &done, int &rtn)
{
    switch(ch)
    {
    case KB_FKEY_CODE(0): // use alternate (i.e. done)
	rtn = mstore->SetNameDirectory(items[pos], 0, types[pos]);
	if (rtn != 0)
	{
	    // Get the password
	    char pbuf[82];
	    pbuf[0] = '\0';
	    PopupPrompt *p = new PopupPrompt(owner,
	        "Type the password for the directory (8 chars max.) and press Perform.",
	        "DIRPASSWORD", "Perform\n Change", 0, " Cancel\n Change", 0);
	    assert(p);
	    if (p->Run(pbuf) != KB_FKEY_CODE(7))
	        rtn = mstore->SetNameDirectory(items[pos], pbuf, types[pos]);
	    delete p;
	}
	if (rtn == 0) done = 1;
	else
	{
	    ClearScreen();
	    owner->Paint();
	    Paint();
	}
	break;
    default:
	OMListBoxPopup::HandleFKey(ch, done, rtn);
	break;
    }
}

SearchDirPopup::SearchDirPopup(Screen *owner_in)
    : OMListBoxPopup(owner_in, "Enter the Search Directory name and press Done.",
	"DIRSEARCH2")
{
    types = new int[maxitems];
    fkeys = new FKeySet8by2(" Perform\n Change", 0, 0, 0, 0, 0,
			"  Help", " Cancel\n Change");
    assert(fkeys && types);
    GetItems();
    ConstructTrace("SearchDirPopup");
}

void SearchDirPopup::GetItems()
{
    pagenum++;
    islast = -1;
    do
    {
	pagenum--;
	islast++;
        cnt = maxitems;
	for (int i = 0; i < maxitems; i++)
	{
	    delete [] items[i];
	    items[i] = 0;
	}
        mstore->GetDirectories(cnt, items, types, pagenum*maxitems);
    }
    while (cnt == 0 && pagenum > 0);
    if (islast > 0 || cnt < maxitems) islast = 1;
}

int SearchDirPopup::NextPage()
{
    return islast ? 0 : maxitems; // kludge
}

int SearchDirPopup::PrevPage()
{
    return maxitems; // kludge
}

SearchDirPopup::~SearchDirPopup()
{
    delete [] types;
    DestructTrace("SearchDirPopup");
}

//------------------------------------------------------------------

AddressField::AddressField(Screen *owner_in, int r_in, int c_in, int w_in,
	unsigned cancelkeys_in)
    : DataField(r_in, c_in, w_in, cancelkeys_in),
	owner(owner_in)
{
    ConstructTrace("AddressField");
}

int AddressField::Error(char *msg)
{
    char buf[1024];
    sprintf(buf, msg, value);
    buf[80] = 0;
    extern char *ServerError;
    delete [] ServerError;
    ServerError = 0;
    //ServerError = new char [strlen(buf)+1];
    //strcpy(ServerError, buf);
    fill(buf,80,' ');
    ReverseOn();
    PutString(0,0,buf);
    ReverseOff();
}

int AddressField::IsValid() // this is pretty ugly...
{
	extern char *ServerError;
	char **names = 0;
	int rtn = 1;
	long flag = 5l; // ALTSFILE | IN_DIR
	int cmd = 0;
	if (len > 0)
	{
	    // first do nickname substitution...
	    char *nn = mstore->LookupNickname(value);
	    if (nn)
	    {
		delete [] value;
		value = nn; 
		len = strlen(value);
	    }
	    char *ma = config->Get("maxaddresses");
	    int maxopts = isdigit(ma[0]) ? atoi(ma) : 64;
retry:
	    int cnt = mstore->CheckName(value, maxopts, names, flag);
	    if (cnt <= 0)
	    {
	        cnt = mstore->CheckName(value,maxopts,names, flag^4);
		if (cnt > 0) 
		    Error("Address %s is not in the directory and may need correction!");
	    }
	    if (cnt <= 0) // error!
	    {
		Error("Invalid address! Please correct it."); // fix this
	        rtn = 0;
	    }
	    else if (cnt == 1) // unique
	        SetValue(mstore->FormatName(names[0]));
	    else // not unique; make popup list
	    {
		Paint(2);
    		AddressPopup *apop = new AddressPopup(owner, cnt, names);
		if (apop)
		{
		    cmd = apop->Run();
		    if (cmd == 0) SetValue(apop->GetSelection());
		    else rtn = 0;
    		    delete apop;
	        }
		else rtn = 0;
		ClearScreen();
		owner->Paint();
	    }
	    for (int n = 0; n < cnt; n++)
	        delete [] names[n];
	    delete [] names;
	    if (flag == 5l && cmd == 1) // user requested fuzzy search
	    {
		flag = 21l;  // ALTSFILE | IN_DIR | GET_ALTS
		goto retry;
	    }
        }
    return rtn;
}

AddressField::~AddressField()
{
    DestructTrace("AddressField");
}

//----------------------------------------------------------------
// Popup for choosing addresses

AddressPopup::AddressPopup(Screen *owner_in, int cnt_in, char **names_in)
    : OMListBoxPopup(owner_in,
         "Choose the alternative name you would like to use and press Use Alternat.",
	  "DIRSEARCH2"), names(names_in), namecnt(cnt_in)
{
    ConstructTrace("AddressPopup");
    for (int n = 0; n < maxitems; n++)
	items[n] = 0;
    GetItems();
    fkeys = new FKeySet8by2("  Use\nAlternat", 0,
			0, " Fuzzy\n Search",
			"Extended\nAddress", 0,
			"  Help", " Cancel\nAlternat");
    assert(fkeys);
}

void AddressPopup::HandleKey(int ch, int &done, int &rtn)
{
    if (ch == '\n')
    {
        rtn = 0;
	done = 1;
    }
    else OMListBoxPopup::HandleKey(ch, done, rtn);
}

void AddressPopup::HandleFKey(int ch, int &done, int &rtn)
{
    switch(ch)
    {
    case KB_FKEY_CODE(0): // use alternate (i.e. done)
        rtn = 0;
	done = 1;
	break;
    case KB_FKEY_CODE(3): // fuzzy search
        rtn = 1;
	done = 1;
	break;
    case KB_FKEY_CODE(4): // extended address
	OMAddress *addr = new OMAddress;
	assert(addr);
        addr->AssignInternal(names[pos]);
	Screen *s = new ExtendedAddrScreen(owner, addr, 0);
	assert(s);
	(void)s->Run();
	delete s;
	delete addr;
	owner->Paint();
	Paint();
	break;
    default:
	OMListBoxPopup::HandleFKey(ch, done, rtn);
	break;
    }
}

void AddressPopup::GetItems()
{
    for (int n = 0; n < cnt; n++)
    {
	delete [] items[n];
	items[n] = 0;
    }
    int numleft = namecnt - pagenum*maxitems;
    if (numleft <= maxitems)
    {
	cnt = numleft;
	islast = 1;
    }
    else
    {
	cnt = maxitems;
	islast = 0;
    }
    for (n = 0; n < cnt; n++)
    {
	char buf[256];
	strcpy(buf, mstore->FormatName(names[pagenum*maxitems+n]));
	items[n] = new char[strlen(buf)+1];
	assert(items[n]);
	strcpy(items[n], buf);
    }
}

int AddressPopup::NextPage()
{
    int numleft = namecnt - (pagenum+1)*maxitems;
    return (numleft <= maxitems) ? (numleft<0 ? 0 : numleft) : maxitems;
}

int AddressPopup::PrevPage()
{
    return maxitems;
}

char *AddressPopup::GetSelection()
{
    return items[pos];
}

AddressPopup::~AddressPopup()
{
    DestructTrace("AddressPopup");
}

//-----------------------------------------------------------------

AddressPage::AddressPage(int addrcnt_in)
    : addrcnt(addrcnt_in), next(0), prev(0)
{
    addresses = new (OMAddress*[addrcnt]);
    assert(addresses);
    addrtyp = new int[addrcnt];
    assert(addrtyp);
    for (int a = 0; a < addrcnt; a++)
    {
	addrtyp[a] = 1;
	addresses[a] = 0;
    }
    ConstructTrace("AddressPage");
}

AddressPage::~AddressPage()
{
    for (int a = 0; a < addrcnt; a++)
	delete addresses[a];
    delete [] addresses;
    delete [] addrtyp;
    DestructTrace("AddressPage");
}

void AddressPage::SetAddress(int anum, OMAddress *addr)
{
    if (anum >= 0 && anum < addrcnt)
    {
        delete addresses[anum];
        addresses[anum] = addr;
    }
}

OMAddress *AddressPage::GetAddress(int anum)
{
    if (anum >= 0 && anum < addrcnt)
	return addresses[anum];
    return 0;
}

int AddressPage::GetAddrType(int anum)
{
    if (anum >= 0 && anum < addrcnt)
	return addrtyp[anum];
    return -1;
}

void AddressPage::SetAddrType(int anum, int typ)
{
    if (anum >= 0 && anum < addrcnt)
    {
	addrtyp[anum] = typ;
    }
}

AddressPage *AddressPage::NewPage()
{
    AddressPage *ap = this;
    if (ap->next)
	while (ap->next) ap = ap->next;
    ap->next = new AddressPage(addrcnt);
    if (ap->next) ap->next->prev = ap;
    return ap->next;
}

int AddressPage::IsFull()
{
    for (int a = 0; a < addrcnt; a++)
	if (addresses[a]==0 || addresses[a]->IsEmpty())
	    return 0;
    return 1;
}

int AddressPage::IsEmpty()
{
    for (int a = 0; a < addrcnt; a++)
	if (addresses[a]!=0 && !addresses[a]->IsEmpty())
	    return 0;
    return 1;
}

int AddressPage::PageSize()
{
    return addrcnt;
}

AddressPage *AddressPage::NextPage()
{
    // If the page is full and there is no next page, make one;
    // else if there is a next page, return it, else return current page
    return (IsFull() && next==0) ? NewPage() : (next ? next : this);
}

AddressPage *AddressPage::PrevPage()
{
    return prev ? prev : this;
}

//--------------------------------------------------------------------

EnterDistListScreen::EnterDistListScreen(Screen *parent_in, char *title_in, 
		char *prompt_in, int nlabels_in, int nfields_in, int offset,
		int pgsz, unsigned novalidate_in)
	: OMFormScreen(parent_in, title_in, prompt_in, pgsz+nlabels_in+1,	
			pgsz+nfields_in, ' ', 0),
	  pagesize(pgsz), firstpage(pgsz), changed(0),
	  novalidate(novalidate_in)
{
    ConstructTrace("EnterDistListScreen");
    for (int r = 0; r < pagesize; r++)
    {
	AddLabel(offset+r, 0, "TO:  ");
	AddField(new AddressField(this, offset+r, 6, 68, novalidate));
    }
    apage = &firstpage;
    AddLabel(offset+pagesize, 44, "Press PgDn for more names.");
}

void EnterDistListScreen::UpdateTypeLabel(int lnum)
{
    int r, c;
    if (lnum < 0 || lnum>=pagesize) return;
    labels[lnum].Position(r,c);
    switch(apage->GetAddrType(lnum))
    {
    default:
        SetLabel(lnum,r,c,"TO:  ");
        break;
    case 0:
        SetLabel(lnum,r,c,"FROM:");
        break;
    case 2:
        SetLabel(lnum,r,c,"CC:  ");
        break;
    case 3:
        SetLabel(lnum,r,c,"BCC: ");
        break;
    }
    labels[lnum].Paint();
}

void EnterDistListScreen::ExtendedAddress()
{
    if (fnow>=0 && fnow < pagesize)
    {
        OMAddress *addr = apage->GetAddress(fnow);
	if (addr == 0)
	    apage->SetAddress(fnow, addr = new OMAddress());
	Screen *s = new ExtendedAddrScreen(this, addr);
	assert(s);
	s->Run();
	delete s;
        char *newaddr =addr->GetExternal();
        ((AddressField*)fields[fnow])->SetValue(newaddr);
	delete [] newaddr;
    }
}

void EnterDistListScreen::GetItems()
{
    for (int i = 0; i < pagesize; i++)
    {
	char *txt = 0;
	if (apage && apage->GetAddress(i))
	    txt = apage->GetAddress(i)->GetExternal();
	((DataField*)fields[i])->SetValue(txt?txt:0);
	delete [] txt;
	UpdateTypeLabel(i);
    }
}

void EnterDistListScreen::SyncAddresses()
{
    for (int f = 0; f < pagesize; f++)
    {
        char *addr = ((AddressField*)fields[f])->Value();
	OMAddress *oma = apage->GetAddress(f);
	if (oma == 0)
	    if (addr && addr[0])
	        apage->SetAddress(f, oma = new OMAddress());
	    else
		continue;
	char *oldval = oma->GetExternal();
	if (strcmp(oldval, addr) != 0)
	{
	    changed = 1;
	    oma->AssignExternal(addr);
	}
	delete [] oldval;
    }
}

void EnterDistListScreen::ToggleTypes()
{
    if (fnow >= pagesize) return;
    int typ = apage->GetAddrType(fnow);
    if (typ < 0) typ = 1; // default is TO
    typ = (typ+1)%4;
    apage->SetAddrType(fnow, typ);
    UpdateTypeLabel(fnow);
    for (int f = fnow+1; f < pagesize; f++)
    {
        OMAddress *addr = apage->GetAddress(f);
        if (addr==0 || addr->IsEmpty())
	{
    	    apage->SetAddrType(f, typ);
	    UpdateTypeLabel(f);
	}
	else break;
    }
}

Screen *EnterDistListScreen::HandleKey(int ch, int &done)
{
    if (ch != KB_PGUP && ch != KB_PGDN)
        return OMFormScreen::HandleKey(ch, done);
    SyncAddresses();
    if (ch == KB_PGDN)
	apage = apage->NextPage();
    else 
	apage = apage->PrevPage();
    fnow = 0;
    ClearScreen();
    GetItems();
    int r, c;
    labels[pagesize].Position(r, c);
    if (apage == &firstpage)
	SetLabel(pagesize, r, c, "Press PgDn for more names.     ");
    else
	SetLabel(pagesize, r, c, "Press PgUp/PgDn for more names.");
    Paint();
    return 0;
}

void EnterDistListScreen::AssignNickname()
{
    extern char *ServerError; // kludge...to get error msg on line 1
    OMAddress *addr = apage->GetAddress(fnow);
    if (addr == 0)
    {
	delete [] ServerError;
	char *msg = "The cursor must be on a name to assign a nickname.";
	ServerError = new char[strlen(msg)+1];
	assert(ServerError);
	strcpy(ServerError, msg);
    }
    else
    {
        char buf[82];
        PopupPrompt *p = new PopupPrompt(this,"Type the nickname that you would"
					" like to use and press Save Nickname.",
		                "NICKNAMECREATE", "  Save\nNickname", 0,
				" Cancel\nNickname", 1);
	assert(p);
        buf[0] = '\0';
        int key = p->Run(buf, 1);
	if (key == KB_FKEY_CODE(0) || key == '\n') // do save
	{
            OMAddress *na = new OMAddress(addr);
	    assert(na);
            na->SetField(Nickname, buf);
            int idx;
            char *nn = mstore->LookupNickname(buf, &idx);
	    delete [] nn;
            (void)mstore->SetNickname(idx, na);
	}
	delete p;
	Paint();
    }
}

void EnterDistListScreen::ResetChangeFlag()
{
    changed = 0;
}

int EnterDistListScreen::HasDListChanged()
{
    return changed;
}

int EnterDistListScreen::Run()
{
    int done = 0, ch;
    changed = 0;
    GetItems();
    while (!done)
    {
        Paint();
	if (canedit)
	{
	    if (fnow < pagesize)
	    {
	    	char buf[COLS+1];
	    	strcpy(buf, ((AddressField*)fields[fnow])->Value());
            	ch = fields[fnow]->Run();
		if (strcmp(buf, ((AddressField*)fields[fnow])->Value()) != 0)
		    changed = 1;
	    }
            else ch = fields[fnow]->Run();
	}
	else ch = GetKey();
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

void EnterDistListScreen::ChangeDirectory()
{
    SearchDirPopup *p = new SearchDirPopup(this);
    assert(p);
    int rtn= p->Run();
    delete p;
    ClearScreen();
    Paint();
    if (rtn != 0) DrawTitle("Invalid password.");
}

EnterDistListScreen::~EnterDistListScreen()
{
    AddressPage *ap = firstpage.next;
    while (ap)
    {
	AddressPage *tmp = ap;
	ap = ap->next;
	delete tmp;
    }
    DestructTrace("EnterDistListScreen");
}

void EnterDistListScreen::EmptyDList()
{
    AddressPage *ap = firstpage.next;
    while (ap)
    {
	AddressPage *oldap = ap;
	ap = ap->next;
	delete oldap;
    }
    apage = &firstpage;
    apage->next = 0;
    fnow = 0;
    for (int i = 0; i < pagesize; i++)
    {
	apage->SetAddress(i, 0);
	apage->SetAddrType(i, 1);
    }
    GetItems();
    Paint();
}

//----------------------------------------------------------------

CreateScreen::CreateScreen(Screen *parent_in, char *title_in, 
		char *prompt_in, int nlabels_in, int nfields_in,
		int lref_in, long inum_in, int offset, char *quoted_in)
	: EnterDistListScreen(parent_in, title_in, prompt_in,
		2+nlabels_in, 1+nfields_in, offset, 11,
		1|(1<<8)|(1<<3)|(1<<7)|(1<<15)), // F1, F4 (mode 0) and F8
	  lref(lref_in), inum(inum_in), fname(0), msgnum(0l), quoted(quoted_in)
{
    ConstructTrace("CreateScreen");
    AddLabel(offset+13, 0, "Document/File type:");
    typfld = AddField(new FileTypeToggle(offset+13, 20, 30, ' ', '-'));
    AddLabel(offset+13, 51, "Space for next, '-' for prev.");
    fkeys=new FKeySet8by2(" Other\n  Keys", "  Hold\nMessage: TO/CC/\nBCC/FROM",
	" Merge\n  List:Compose", "Extended\nAddress:Include\nFile/Doc",
	" Change\nDirectry:Message\nSettings", " Assign\nNickname:  Mail\nMessage",
	"  Help", " Cancel");
    assert(fkeys);
    apage = &firstpage;
    ToggleFKeys(); // kludge; I did them the wrong way around 8-)
}

Screen *CreateScreen::HandleKey(int ch, int &done)
{
    return EnterDistListScreen::HandleKey(ch, done);
}

long CreateScreen::MessageNum()
{
    return msgnum;
}

Screen *CreateScreen::HandleFKey(int fkeynum, int &done)
{
    extern char *ServerError; // kludge...to get error msg on line 1
    SyncAddresses();
    done = 0;
    switch (fkeynum)
    {
    case F1_PRI:
    case F1_SEC: // other keys
	ToggleFKeys();
	break;
    case F2_SEC: // TO/CC/BCC/FROM
	if (fnow>=0 && fnow < pagesize)
	    ToggleTypes();
	break;
    case F3_PRI: // merge list
	MergeList();
	break;
    case F3_SEC: // compose
	if (fname == 0)
	{
	    char *tn = tmpnam(0);
	    if (quoted)
	    {
		char cmd[128];
		FILE *fp = fopen(tn, "w");
		if (fp)
		{
		    fprintf(fp, "\n------------------------------ Reply Separator ------------------------------\n");
		    fclose(fp);
		}
		sprintf(cmd, "cat %s >> %s 2>/dev/null", quoted, tn);
		system(cmd);
		unlink(quoted);
		quoted = 0;
	    }
	    Debug1("Create Screen composing with file %s", tn);
	    fname = new char[strlen(tn)+1];
	    if (fname) strcpy(fname, tn);
	}
	if (fname)
	{
	    int ftyp = ((FileTypeToggle*)fields[typfld])->GetType();
	    if (ExternalCompose(fname, ftyp) != 0)
	        return new EditScreen(this, fname);
	}
	break;
    case F4_PRI: // extended address
	ExtendedAddress();
	break;
    case F5_PRI: // change directory
	ChangeDirectory();
	break;
    case F5_SEC: // message settings
	return new MsgSettingsScreen(this, mset);
    case F6_PRI: // assign nickname
	AssignNickname();
	break;
    case F2_PRI: // hold message
    case F6_SEC: // mail message
	if (fname == 0 && GetType()!=2) // no contents yet!
	{
	    delete [] ServerError;
	    char *msg = "You have not yet Composed anything for the message.";
	    ServerError = new char[strlen(msg)+1];
	    assert(ServerError);
	    strcpy(ServerError, msg);
	    break;
	}
	// fall thru...
    case F4_SEC: // include file/doc
	AddressPage *ap = &firstpage;
	for (;;)
	{
	    for (int r = 0; r < ap->PageSize(); r++)
	    {
		OMAddress *addr = ap->GetAddress(r);
		int typ = ap->GetAddrType(r);
		if (addr && !addr->IsEmpty())
		    mstore->AddRecipient(addr, typ);
	    }
	    if (ap == ap->NextPage()) break; // last page
	    ap = ap->NextPage();
	}
	int msgref = mstore->CreateMessage(lref, inum, GetSubject(), mset,
					GetType());
	if (msgref)
	{
	    int ftyp = ((FileTypeToggle*)fields[typfld])->GetType();
	    if (fname == 0 || mstore->AttachFile(msgref, fname, 0, ftyp) == 0)
	    {
		if (fkeynum == F6_SEC) // mail?
		{
		    done = (mstore->MailMessage(mset, msgref) == 0);
		    if (done && (mset.GetAckLevel()||mset.GetDate()>time(0)))
		        parent->RefreshContents(); // to update tracking area 
		    if (done) msgnum = -1l;
		}
		else // hold
		{
		    msgnum = mstore->HoldMessage(msgref);
		    done = (msgnum > 0l);
		    if (fkeynum == F2_PRI)
			msgnum = 0l; // only keep msgnum for transient messages
		}
	    }
	}
	break;
    case F7_PRI:
    case F7_SEC: // help
	Help("CREATING");
	break;
    case F8_PRI:
    case F8_SEC: // cancel
	if (fname || changed)
	    done = Confirm("You have not mailed this message",
			   "Press Confirm if you wish to discard the message.",
			   "Confirm\n Exit", " Cancel\n Exit");
	else
	    done = 1;
	break;
    }
    return 0;
}

CreateScreen::~CreateScreen()
{
    DestructTrace("CreateScreen");
    if (fname) unlink(fname);
    if (quoted) unlink(quoted);
    delete [] fname;
    delete [] quoted;
}

CreateNewScreen::CreateNewScreen(Screen *parent_in)
	: CreateScreen(parent_in, "Create",
		"Fill in form, and press Compose. Then press Mail Message.",
		   1, 1, 0, 0, 6)
{
    AddLabel(4, 0, "Subject:");
    fnow = subjfld = AddField(new DataField(4, 10, 60));
    ConstructTrace("CreateNewScreen");
}

char *CreateNewScreen::GetSubject()
{
    char *rtn = ((DataField*)fields[subjfld])->Value();
    if (rtn[0]) changed = 1;
    return rtn;
}

int CreateNewScreen::GetType()
{
    return 0;
}

CreateNewScreen::~CreateNewScreen()
{
    DestructTrace("CreateNewScreen");
}

//--------------------------------------------------------------------

class DListToggleField : public ToggleField
{
protected:
    int inBBarea;
    CreateReplyScreen *owner;
    virtual int Run();
public:
    DListToggleField(CreateReplyScreen *owner_in, int inBBarea_in = 0)
	: ToggleField(5, 8, 40,
		(inBBarea_in ? "Sender@All@Bulletin Boards":"Sender@All"),
		' ', 0), owner(owner_in), inBBarea(inBBarea_in)
    {
        ConstructTrace("DListToggleField");
    }
    ~DListToggleField()
    {
        DestructTrace("DListToggleField");
    }
};

int DListToggleField::Run()
{
    for (;;)
    {
	Paint(1);
	int ch = GetKey();
	if (ch == ' ' || ch == nextchar || ch == prevchar)
	{
	    if (owner->changed) // get confirmation
    		if (owner->Confirm("Your changes to the list will be lost "
			    "if you change this value.",
			"Press Confirm to change list type and lose changes.",
    			" Confirm", " Cancel") == 0)
		    continue;
	    if (ch == prevchar)
	        value = (value+cnt-1)%cnt;
	    else
	    	value = (value+1)%cnt;
	    owner->EmptyDList();
	    // adjust list
	    Message *msg = owner->msg;
	    if (value == 2)
	    {
		if (msg->Ref()>=0) // already open?
		    owner->MergeDList(msg->Child(1l), 1, "+BB");
		else if (msg->Open(1) == 0)
		{
		    if (msg->FirstPage() == 1)
			owner->MergeDList(msg->Child(1l), 1, "+BB");
		    msg->Close();
		}
	    }
	    else
	    {
    	    	OMAddress *newaddr = new OMAddress;
	    	assert(newaddr);
	    	newaddr->AssignExternal(msg->Creator());
	    	owner->firstpage.SetAddress(0, newaddr);
		if (value != 0) // reply to all
		{
		    if (msg->Ref()>=0) // already open?
			owner->MergeDList(msg->Child(1l), 1);
		    else if (msg->Open(1) == 0)
		    {
			if (msg->FirstPage() == 1)
			    owner->MergeDList(msg->Child(1l), 1);
			msg->Close();
		    }
		}
		else owner->GetItems(); // MergeDList does this too...
	    }
	    owner->changed = 0;
	    ClearScreen();
    	    owner->Paint();
	}
	else if (ch > 127)
	    return ch;
    }
}

CreateReplyScreen::CreateReplyScreen(Screen *parent_in, Message *msg_in,
		int inBBarea, char *quoted)
	: CreateScreen(parent_in, "Reply",
		"Press Compose Reply, then press Mail Message.", 3, 1,
		msg_in->Parent()->Ref(), msg_in->ItemNum(), 7, quoted),
		msg(msg_in)
{
    ConstructTrace("CreateReplyScreen");
    AddLabel(3, 0, "In reply to:");
    subjlbl = AddLabel(3, 14, msg->Name());
    fnow = AddField(new DListToggleField(this, inBBarea));
    AddLabel(5, 50, "To change, press space bar.");
    OMAddress *newaddr = new OMAddress;
    assert(newaddr);
    newaddr->AssignExternal(msg->Creator());
    firstpage.SetAddress(0, newaddr);
    GetItems();
    // must initialise mset from message??
}

char *CreateReplyScreen::GetSubject()
{
    return labels[subjlbl].Text();
}

int CreateReplyScreen::GetType()
{
    return 1;
}

CreateReplyScreen::~CreateReplyScreen()
{
    DestructTrace("CreateReplyScreen");
}

CreateForwardScreen::CreateForwardScreen(Screen *parent_in, Message *msg,
						char *quoted)
	: CreateScreen(parent_in, "Forward",
		"Fill in form. Then press Mail Message.", 2, 0, 
		msg->Parent()->Ref(), msg->ItemNum(), 6, quoted)
{
    AddLabel(4, 0, "Subject:");
    subjlbl = AddLabel(4, 10, msg->Name());
    ConstructTrace("CreateForwardScreen");
    // must initialise mset from message??
}

char *CreateForwardScreen::GetSubject()
{
    return labels[subjlbl].Text();
}

int CreateForwardScreen::GetType()
{
    return 2;
}

CreateForwardScreen::~CreateForwardScreen()
{
    DestructTrace("CreateForwardScreen");
}

//--------------------------------------------------------------------

void EditDistListScreen::Initialise()
{
    AddLabel(4,0,"Name:");
    namefld = AddField(new DataField(4,6,50));
    fkeys = new FKeySet8by2(" Cancel\n Changes", " TO/CC/\nBCC/FROM", 
			" Change\nDirectry", " Assign\nNickname",
			"Extended\nAddress", " Merge\n  List",
			"  Help", "  Save\nChanges");
    assert(fkeys);
    if (msg) // editing dlist in out tray msg?
    {
	msg->Open(1);
	msg->FirstPage();
	FolderItem *fi = msg->Child(1l);
	MergeDList(fi);
        ((DataField*)fields[namefld])->SetValue(fi->Name());
	msg->Close();
    }
    else if (itemnum!=0) // editing existing dlist?
    {
	FolderItem *fi = ((Folder*)mstore->Folder())->Child(itemnum);
	MergeDList(fi);
        ((DataField*)fields[namefld])->SetValue(fi->Name());
    }
    // else new dlist
    fnow = namefld;
}

EditDistListScreen::EditDistListScreen(Screen *parent_in, long itemnum_in)
    : EnterDistListScreen(parent_in, "Distribution List", 
	"Type the names for your distribution list and press Save Changes.",
	1, 1, 7, 13, (1 | (1<<4))), itemnum(itemnum_in), msg(0)
{
    Initialise();
    ConstructTrace("EditDistListScreen");
}

EditDistListScreen::EditDistListScreen(Screen *parent_in, Message *msg_in)
    : EnterDistListScreen(parent_in, "Distribution List", 
	"Type the names for your distribution list and press Save Changes.",
	1, 1, 7, 13, (1 | (1<<4))), msg(msg_in), itemnum(1l)
{
    Initialise();
    ConstructTrace("EditDistListScreen");
}

Screen *EditDistListScreen::HandleFKey(int fkeynum, int &done)
{
    SyncAddresses();
    done = 0;
    switch (fkeynum)
    {
    case F1_PRI:
    case F1_SEC: // cancel changes
	done = 1;
	break;
    case F2_PRI: // TO/CC/BCC/FROM
	if (fnow>=0 && fnow < pagesize)
	    ToggleTypes();
	break;
    case F3_PRI: // change directory
	ChangeDirectory();
	break;
    case F4_PRI: // assign nickname
	AssignNickname();
	break;
    case F5_PRI: // extended address
	ExtendedAddress();
	break;
    case F6_PRI: // merge list
	MergeList();
	break;
    case F7_PRI: // help
	Help("DISTLISTCREATE");
	break;
    case F8_PRI: // save changes or new dlist
	AddressPage *ap = &firstpage;
	int cnts[4];
	for (;;)
	{
	    for (int r = 0; r < ap->PageSize(); r++)
	    {
		OMAddress *addr = ap->GetAddress(r);
		int typ = ap->GetAddrType(r);
		if (addr && !addr->IsEmpty())
		    cnts[typ]++;
	    }
	    if (ap == ap->NextPage()) break; // last page
	    ap = ap->NextPage();
	}
	mstore->CreateDList(cnts[0], cnts[1], cnts[2], cnts[3]);
	ap = &firstpage;
	for (;;)
	{
	    for (int r = 0; r < ap->PageSize(); r++)
	    {
		OMAddress *addr = ap->GetAddress(r);
		int typ = ap->GetAddrType(r);
		if (addr && !addr->IsEmpty())
		    mstore->AddAddress(addr, typ);
	    }
	    if (ap == ap->NextPage()) break; // last page
	    ap = ap->NextPage();
	}
	int ok = 0;
	if (msg) // replace dlist in message?
	{
	    msg->Open(1);
	    ok = (mstore->ReplaceDList(((DataField*)fields[namefld])->Value(),
				msg->Ref(), 1l)==0);
	    msg->Close();
	}
	else if (itemnum != 0) // modify existing dlist
	{
	    ok = (mstore->ReplaceDList(((DataField*)fields[namefld])->Value(),
				mstore->Folder()->Ref(), itemnum)==0);
	}
	else // new dlist
	{
	    ok = (mstore->SaveDList(((DataField*)fields[namefld])->Value(),
				mstore->Folder()->Ref())==0);
	}
	if (ok)
	{
	    RefreshContents();
	    done = 1;
	}
	break;
    }
    return 0;
}

EditDistListScreen::~EditDistListScreen()
{
    DestructTrace("EditDistListScreen");
}

//------------------------------------------------------------------------

AutoForwardScreen::AutoForwardScreen(Screen *parent_in)
    : EnterDistListScreen(parent_in, "Auto Forward", 
	"Edit the list and press Save Changes.",
	8, 7, 8, 13, (1 | (1<<4))), dlist(0), comments(0)
{
    ConstructTrace("AutoForwardScreen");
    AddLabel(3,2,"Auto-Forward Active:");
    afafld = AddField(new ToggleField(3,24,9, "NO@YES", ' ', 0));
    AddLabel(4,2,"Include Comments:");
    icfld = AddField(new ToggleField(4,24,9, "NO@YES", ' ', 0));
    AddLabel(5,2,"Delete Forwarded Mail:");
    dfmfld = AddField(new ToggleField(5,24,9, "NO@YES", ' ', 0));
    AddLabel(6,2,"Keep Original Creator:");
    kocfld = AddField(new ToggleField(6,24,9, "NO@YES", ' ', 0));
    AddLabel(3,38,"To change selection, press space bar.");
    AddLabel(4,38,"Forward Private Mail:");
    fprmfld = AddField(new ToggleField(4,69,9, "NO@YES", ' ', 0));
    AddLabel(5,38,"Forward Personal Mail:");
    fpsmfld = AddField(new ToggleField(5,69,9, "NO@YES", ' ', 0));
    AddLabel(6,38,"Forward Co. Confidential Mail:");
    fccmfld = AddField(new ToggleField(6,69,9, "NO@YES", ' ', 0));
    fkeys = new FKeySet8by2(" Cancel\n Changes", " Other\n  Keys", 
			"  Edit\nComments:", " TO/CC/\nBCC/FROM:",
			"Extended\nAddress: Change\nDirectry",
			" Merge\n  List: Assign\nNickname",
			"  Help", "  Save\nChanges");
    assert(fkeys);
    fnow = afafld;
    (void)mstore->GetAutoActions(afa, ara, ic, dfm, koc, fprm, fpsm, fccm,
				 	rtam, rtum, rtrra);
    ((ToggleField*)fields[afafld])->SetValue(afa);
    ((ToggleField*)fields[icfld])->SetValue(ic);
    ((ToggleField*)fields[dfmfld])->SetValue(dfm);
    ((ToggleField*)fields[kocfld])->SetValue(koc);
    ((ToggleField*)fields[fprmfld])->SetValue(fprm);
    ((ToggleField*)fields[fpsmfld])->SetValue(fpsm);
    ((ToggleField*)fields[fccmfld])->SetValue(fccm);
    dlist = tmpnam(0);
    mstore->GetAutoForwardDList(dlist);
    MergeDList(dlist);
    unlink(dlist);
    dlist = 0;
}

int AutoForwardScreen::SetAction()
{
    afa = ((ToggleField*)fields[afafld])->Value();
    ic = ((ToggleField*)fields[icfld])->Value();
    dfm = ((ToggleField*)fields[dfmfld])->Value();
    koc = ((ToggleField*)fields[kocfld])->Value();
    fprm = ((ToggleField*)fields[fprmfld])->Value();
    fpsm = ((ToggleField*)fields[fpsmfld])->Value();
    fccm = ((ToggleField*)fields[fccmfld])->Value();
    return mstore->SetAutoActions(afa, ara, ic, dfm, koc, fprm, fpsm, fccm,
				  dlist, comments, rtam, rtum, rtrra, 0);
}

Screen *AutoForwardScreen::HandleFKey(int fkeynum, int &done)
{
    SyncAddresses();
    done = 0;
    switch (fkeynum)
    {
    case F1_PRI:
    case F1_SEC: // cancel changes
	done = 1;
	break;
    case F2_PRI: // other keys
    case F2_SEC: 
	ToggleFKeys();
	break;
    case F3_PRI: // edit comments
	if (comments == 0)
	{
	    char *tn = tmpnam(0);
	    Debug1("AutoForward editing comments in %s", tn);
	    comments = new char[strlen(tn)+1];
	    if (comments) strcpy(comments, tn);
	}
	if (comments)
	{
	    mstore->GetAutoForwardComment(comments);
	    return new EditScreen(this, comments);
	}
	break;
    case F4_PRI: // TO/CC/BCC/FROM
	if (fnow>=0 && fnow < pagesize)
	    ToggleTypes();
	break;
    case F5_PRI: // extended address
	ExtendedAddress();
	break;
    case F5_SEC: // change directory
	ChangeDirectory();
	break;
    case F6_PRI: // merge list
	MergeList();
	break;
    case F6_SEC: // assign nickname
	AssignNickname();
	break;
    case F7_PRI: // help
	Help("AUTOFORWARD");
	break;
    case F8_PRI: // save changes 
	AddressPage *ap = &firstpage;
	int cnts[4], cnt = 0;
	for (;;)
	{
	    for (int r = 0; r < ap->PageSize(); r++)
	    {
		OMAddress *addr = ap->GetAddress(r);
		int typ = ap->GetAddrType(r);
		if (addr && !addr->IsEmpty())
		{
		    cnts[typ]++;
		    cnt++;
		}
	    }
	    if (ap == ap->NextPage()) break; // last page
	    ap = ap->NextPage();
	}
	if (cnt == 0)
	{
    	    if (((ToggleField*)fields[afafld])->Value() == 1)
	    {
	        DrawTitle("Please enter some names in the distribution list!");
		RefreshScreen();
		sleep(2);
		return 0;
	    }
	}
	mstore->CreateDList(cnts[0], cnts[1], cnts[2], cnts[3]);
	ap = &firstpage;
	for (;;)
	{
	    for (int r = 0; r < ap->PageSize(); r++)
	    {
		OMAddress *addr = ap->GetAddress(r);
		int typ = ap->GetAddrType(r);
		if (addr && !addr->IsEmpty())
		    mstore->AddAddress(addr, typ);
	    }
	    if (ap == ap->NextPage()) break; // last page
	    ap = ap->NextPage();
	}
	dlist = mstore->CloseDListTF(0);
	SetAction();
	done = 1;
	break;
    }
    return 0;
}

AutoForwardScreen::~AutoForwardScreen()
{
    if (dlist) unlink(dlist);
    if (comments) unlink(comments);
    delete [] comments;
    delete [] dlist;
    DestructTrace("AutoForwardScreen");
}

//-----------------------------------------------------------------

class ReplyTypeToggle : public ToggleField
{
    Screen *owner;
    virtual char *GetText();
public:
    ReplyTypeToggle(int r_in,int c_in,int w_in,char *opts_in,Screen *owner_in)
        : ToggleField(r_in, c_in, w_in, opts_in, ' ', 0), owner(owner_in)
    {
        ConstructTrace("ReplyTypeToggle");
    }
    ~ReplyTypeToggle()
    {
        DestructTrace("ReplyTypeToggle");
    }
};

char *ReplyTypeToggle::GetText()
{
    ((AutoReplyScreen*)owner)->Sanitise();
    return ToggleField::GetText();
}

void AutoReplyScreen::Sanitise()
{
    int rtamtmp = ((ToggleField*)fields[rtamfld])->Value();
    int rtumtmp = ((ToggleField*)fields[rtumfld])->Value();
    int rtrratmp = ((ToggleField*)fields[rtrrafld])->Value();
    // We have to check things quite carefullt below, to avoid infinite
    // recursion in Paint calls (as Sanitise is called from the field
    // Paint method)... Hence we don't collapse otherwise common cases
    if (fnow == rtamfld)
    {
        if (((ToggleField*)fields[rtamfld])->Value())
	{
            if (((ToggleField*)fields[rtumfld])->Value() == 0)
	    {
                ((ToggleField*)fields[rtumfld])->SetValue(1);
                ((ToggleField*)fields[rtumfld])->Paint();
	    }
            if (((ToggleField*)fields[rtrrafld])->Value() == 0)
	    {
                ((ToggleField*)fields[rtrrafld])->SetValue(1);
                ((ToggleField*)fields[rtrrafld])->Paint();
	    }
	}
    }
    else if (fnow == rtumfld)
    {
        if (((ToggleField*)fields[rtumfld])->Value()==0 &&
           ((ToggleField*)fields[rtamfld])->Value()==1)
	{
             ((ToggleField*)fields[rtamfld])->SetValue(0);
             ((ToggleField*)fields[rtamfld])->Paint();
	}
    }
    else if (fnow == rtrrafld)
    {
        if (((ToggleField*)fields[rtrrafld])->Value()==0 &&
           ((ToggleField*)fields[rtamfld])->Value()==1)
	{
             ((ToggleField*)fields[rtamfld])->SetValue(0);
             ((ToggleField*)fields[rtamfld])->Paint();
	}
    }
}

AutoReplyScreen::AutoReplyScreen(Screen *parent_in)
    : OMFormScreen(parent_in, "Auto Reply", 
	"Change your Auto-Reply configuration and press Save Changes.",
	5, 4), reply(0)
{
    ConstructTrace("AutoReplyScreen");
    AddLabel(6,7,"Auto-Reply Active:");
    arafld = AddField(new ToggleField(6,29,9, "NO@YES", ' ', 0));
    AddLabel(6,41,"To change, press space bar.");
    AddLabel(9,7,"Reply to All Messages:");
    rtamfld = AddField(new ReplyTypeToggle(9,34,9, "NO@YES", this));
    AddLabel(11,7,"Reply to URGENT Messages:");
    rtumfld = AddField(new ReplyTypeToggle(11,34,9, "NO@YES", this));
    AddLabel(12, 7, "Reply to Read/Reply Acks:");
    rtrrafld = AddField(new ReplyTypeToggle(12,34,9, "NO@YES", this));
    fkeys = new FKeySet8by2(" Cancel\n Changes", 0,
			"Compose\n Reply", 0, 0, 0,
			"  Help", "Perform\nChanges");
    assert(fkeys);
    (void)mstore->GetAutoActions(afa, ara, ic, dfm, koc, fprm, fpsm, fccm,
				 	 rtam, rtum, rtrra);
    ((ToggleField*)fields[arafld])->SetValue(ara);
    ((ToggleField*)fields[rtamfld])->SetValue(rtam);
    ((ToggleField*)fields[rtumfld])->SetValue(rtum);
    ((ToggleField*)fields[rtrrafld])->SetValue(rtrra);
}

int AutoReplyScreen::SetAction()
{
     ara = ((ToggleField*)fields[arafld])->Value();
     rtam = ((ToggleField*)fields[rtamfld])->Value();
     rtum = ((ToggleField*)fields[rtumfld])->Value();
     rtrra = ((ToggleField*)fields[rtrrafld])->Value();
     return mstore->SetAutoActions(afa, ara, ic, dfm, koc, fprm, fpsm, fccm,
				  0, 0, rtam, rtum, rtrra, reply);
}

Screen *AutoReplyScreen::HandleFKey(int fkeynum, int &done)
{
    done = 0;
    switch (fkeynum)
    {
    case F1_PRI:
	done = 1;
	break;
    case F3_PRI: // compose reply
	if (reply == 0)
	{
	    char *tn = tmpnam(0);
	    Debug1("AutoReply editing comments in %s", tn);
	    reply = new char[strlen(tn)+1];
	    if (reply) strcpy(reply, tn);
	}
	if (reply)
	{
            (void)mstore->GetAutoReply(reply);
	    return new EditScreen(this, reply);
	}
	break;
    case F7_PRI:
	Help("AUTOREPLY");
	break;
    case F8_PRI:
	SetAction();
	done = 1;
	break;
    }
    return 0;
}

AutoReplyScreen::~AutoReplyScreen()
{
    if (reply) unlink(reply);
    delete [] reply;
    DestructTrace("AutoReplyScreen");
}

//----------------------------------------------------------------------

void MsgSettingsScreen::GetSettings()
{
    time_t tcks = mset.GetDate();
    ((DataField*)fields[datefld])->SetValue(FormatDate(tcks));
    ((DataField*)fields[timefld])->SetValue(FormatTime(tcks));
    ((ToggleField*)fields[impfld])->SetValue(mset.GetImportance());
    ((ToggleField*)fields[prifld])->SetValue(mset.GetPriority());
    ((ToggleField*)fields[sensfld])->SetValue(mset.GetSensitivity());
    ((ToggleField*)fields[ackfld])->SetValue(mset.GetAckLevel());
    ((ToggleField*)fields[apdlfld])->SetValue(mset.DoAllowPublicDLs());
    ((ToggleField*)fields[aarfld])->SetValue(mset.DoAllowAltRecips());
    ((ToggleField*)fields[acfld])->SetValue(mset.DoAllowConversions());
    ((ToggleField*)fields[andnfld])->SetValue(mset.DoAllowNDNs());
    ((ToggleField*)fields[rmndfld])->SetValue(mset.MustReturnMsg());
}

MsgSettingsScreen::MsgSettingsScreen(Screen *parent_in,MessageSettings &mset_in)
    : OMFormScreen(parent_in, "Message Settings", 
	"Change the message settings and press Save Changes.", 11, 11,
	KB_FKEY_CODE(3), KB_FKEY_CODE(2)), mset(mset_in)
{
    ConstructTrace("MsgSettingsScreen");
    AddLabel(3,4,"Importance of message:");
    impfld = AddField(new ToggleField(3,50,28,"NORMAL@LOW@HIGH"));
    AddLabel(5,4,"Priority (grade of delivery):");
    prifld = AddField(new ToggleField(5,50,28,"NORMAL@NON-URGENT@URGENT"));
    AddLabel(7,4,"Sensitivity of message:");
    sensfld = AddField(new ToggleField(7,50,28,
	"NOT SENSITIVE@PERSONAL@PRIVATE@COMPANY CONFIDENTIAL"));
    AddLabel(9,4,"Ack setting for all recipients:");
    ackfld = AddField(new ToggleField(9,50,28,"NONE@DELIVERY ACK@READ ACK@REPLY"));
    AddLabel(11,4,"Earliest delivery date:");
    datefld = AddField(new DataField(11,50,8,1));
    AddLabel(11,61,"Time:");
    timefld = AddField(new DataField(11,68,8,1));
    AddLabel(13,4,"Allow Public Distribution Lists:");
    apdlfld = AddField(new ToggleField(13,50,8,"NO@YES"));
    AddLabel(15,4,"Allow Alternative Recipients:");
    aarfld = AddField(new ToggleField(15,50,8,"NO@YES"));
    AddLabel(17,4,"Allow Conversions:");
    acfld = AddField(new ToggleField(17,50,8,"NO@YES"));
    AddLabel(19,4,"Allow Non-Delivery Notifications:");
    andnfld = AddField(new ToggleField(19,50,8,"NO@YES"));
    AddLabel(21,4,"Return Message with Non-Deliveries:");
    rmndfld = AddField(new ToggleField(21,50,8,"NO@YES"));
    GetSettings();
    fkeys = new FKeySet8by2(" Cancel\nChanges", 0, "Previous\n Choice",
		"  Next\n Choice", 0, "Default\n Values", "  Help",
		"  Save\n Changes");
    assert(fkeys);
}

Screen *MsgSettingsScreen::HandleFKey(int fkeynum, int &done)
{
    done = 0;
    switch (fkeynum)
    {
    case F1_PRI: // cancel
	done = 1;
	break;
    case F6_PRI: // default values
	mset.SetToDefaults();
	GetSettings();
	break;
    case F7_PRI: // help
	Help("MESSAGESETTINGS");
	break;
    case F8_PRI: // save
	SaveSettings();
	done = 1;
	break;
    }
    return 0;
}

void MsgSettingsScreen::SaveSettings()
{
    mset.SetImportance(((ToggleField*)fields[impfld])->Value());
    mset.SetPriority(((ToggleField*)fields[prifld])->Value());
    mset.SetSensitivity(((ToggleField*)fields[sensfld])->Value());
    mset.SetAckLevel(((ToggleField*)fields[ackfld])->Value());
    mset.AllowPublicDLs(((ToggleField*)fields[apdlfld])->Value());
    mset.AllowAltRecips(((ToggleField*)fields[aarfld])->Value());
    mset.AllowConversions(((ToggleField*)fields[acfld])->Value());
    mset.AllowNDNs(((ToggleField*)fields[andnfld])->Value());
    mset.ReturnMsg(((ToggleField*)fields[rmndfld])->Value());
    time_t tmnow = time(0);
    struct tm tm = *localtime(&tmnow);
    char *s = ((DataField*)fields[datefld])->Value();
    // This will have to be changed if FormatDate changes!!
    sscanf(s, "%2d/%2d/%2d", &tm.tm_mday, &tm.tm_mon, &tm.tm_year);
    s = ((DataField*)fields[timefld])->Value();
    sscanf(s, "%2d:%2d", &tm.tm_hour, &tm.tm_min);
    tm.tm_mon--; // why months run from 0 I'll never know...
    tm.tm_isdst = 0;
    time_t mktmp = mktime(&tm);
    struct tm gmt = *gmtime(&mktmp);
    mset.SetDate(mktmp);
};

MsgSettingsScreen::~MsgSettingsScreen()
{
    DestructTrace("MsgSettingsScreen");
}

//-----------------------------------------------------------------

ExtendedAddrScreen::ExtendedAddrScreen(Screen *parent_in, OMAddress *addr_in,
	int canedit_in)
    : OMFormScreen(parent_in, "Extended Address", 
	"Enter the full OpenMail address and press Done.",
	8, 8, 0, 0, canedit_in),
      addr(addr_in)
{
    ConstructTrace("ExtendedAddressScreen");
    AddLabel(6, 5, "Surname");
    surfld = AddField(new DataField(6, 17, 40, 1));
    AddLabel(8, 5, "Forename");
    forfld = AddField(new DataField(8, 17, 16, 1));
    AddLabel(8, 36, "Initials");
    inifld = AddField(new DataField(8, 47, 5, 1));
    AddLabel(8, 55, "Generation");
    genfld = AddField(new DataField(8, 68, 3, 1));
    AddLabel(11, 5, "Address Component 1");
    ac1fld = AddField(new DataField(11, 27, 32, 1));
    AddLabel(13, 5, "Address Component 2");
    ac2fld = AddField(new DataField(13, 27, 32, 1));
    AddLabel(15, 5, "Address Component 3");
    ac3fld = AddField(new DataField(15, 27, 32, 1));
    AddLabel(17, 5, "Address Component 4");
    ac4fld = AddField(new DataField(17, 27, 32, 1));
    fkeys = new FKeySet8by2(" Cancel", " X.400\nAddress", "Foreign\nAddress", 0,
			0, " Start\n  Over", "  Help", "  Done");
    assert(fkeys);
    LoadFields(addr);
    newaddr = new OMAddress(addr);
    assert(newaddr);
}

void ExtendedAddrScreen::LoadFields(OMAddress *a)
{
    ((DataField*)fields[surfld])->SetValue(a->GetField(Surname));
    ((DataField*)fields[forfld])->SetValue(a->GetField(Forename));
    ((DataField*)fields[inifld])->SetValue(a->GetField(Initials));
    ((DataField*)fields[genfld])->SetValue(a->GetField(Generation));
    ((DataField*)fields[ac1fld])->SetValue(a->GetField(OrgUnit1));
    ((DataField*)fields[ac2fld])->SetValue(a->GetField(OrgUnit2));
    ((DataField*)fields[ac3fld])->SetValue(a->GetField(OrgUnit3));
    ((DataField*)fields[ac4fld])->SetValue(a->GetField(OrgUnit4));
    Paint();
}

void ExtendedAddrScreen::SaveFields(OMAddress *a)
{
    a->SetField(Surname, ((DataField*)fields[surfld])->Value());
    a->SetField(Forename, ((DataField*)fields[forfld])->Value());
    a->SetField(Initials, ((DataField*)fields[inifld])->Value());
    a->SetField(Generation, ((DataField*)fields[genfld])->Value());
    a->SetField(OrgUnit1, ((DataField*)fields[ac1fld])->Value());
    a->SetField(OrgUnit2, ((DataField*)fields[ac2fld])->Value());
    a->SetField(OrgUnit3, ((DataField*)fields[ac3fld])->Value());
    a->SetField(OrgUnit4, ((DataField*)fields[ac4fld])->Value());
}

Screen *ExtendedAddrScreen::HandleFKey(int fkeynum, int &done)
{
    done = 0;
    switch(fkeynum)
    {
    case F1_PRI:
	done = 1;
	break;
    case F2_PRI:
	{
	    SaveFields(newaddr);
	    Screen *s = new X400AddrScreen(this, newaddr, canedit);
	    assert(s);
	    s->Run();
	    delete s;
	    LoadFields(newaddr);
	    Paint();
	}
	break;
    case F3_PRI:
	{
	    SaveFields(newaddr);
	    Screen *s = new ForeignAddrScreen(this, newaddr, canedit);
	    assert(s);
	    s->Run();
	    delete s;
	    LoadFields(newaddr);
	    Paint();
	}
	break;
    case F6_PRI: // start over
	LoadFields(addr);
	newaddr->Set(addr);
	break;
    case F7_PRI:
	Help("OPENMAILADDRESSES");
	break;
    case F8_PRI:
	SaveFields(newaddr);
	addr->Set(newaddr);
	done = 1;
	break;
    }
    return 0;
}

ExtendedAddrScreen::~ExtendedAddrScreen()
{
    delete newaddr;
    DestructTrace("ExtendedAddressScreen");
}

//-----------------------------------------------------------------

X400AddrScreen::X400AddrScreen(Screen *parent_in, OMAddress *addr_in,
	int canedit_in)
    : OMFormScreen(parent_in, "X.400 Address", 
	"Enter the X.400 address components and press Done.",
	12, 12, 0, 0, canedit_in),
      addr(addr_in)
{
    ConstructTrace("X400AddrScreen");
    AddLabel(3, 5, "Surname");
    surfld = AddField(new DataField(3, 17, 40, 1));
    AddLabel(5, 5, "Forename");
    forfld = AddField(new DataField(5, 17, 16, 1));
    AddLabel(5, 36, "Initials");
    inifld = AddField(new DataField(5, 47, 5, 1));
    AddLabel(5, 55, "Generation");
    genfld = AddField(new DataField(5, 68, 3, 1));
    AddLabel(7, 5, "Organizational Unit 1");
    ac1fld = AddField(new DataField(7, 31, 32, 1));
    AddLabel(9, 5, "Organizational Unit 2");
    ac2fld = AddField(new DataField(9, 31, 32, 1));
    AddLabel(11, 5, "Organizational Unit 3");
    ac3fld = AddField(new DataField(11, 31, 32, 1));
    AddLabel(13, 5, "Organizational Unit 4");
    ac4fld = AddField(new DataField(13, 31, 32, 1));
    AddLabel(15, 0, "Organization Name");
    orgfld = AddField(new DataField(16, 0, 64, 1));
    AddLabel(18, 0, "Country");
    countryfld = AddField(new DataField(18, 9, 3, 1));
    AddLabel(18, 23, "Admin Domain Name");
    admdfld = AddField(new DataField(18, 45, 16, 1));
    AddLabel(19, 23, "Private Domain Name");
    prmdfld = AddField(new DataField(19, 45, 16, 1));
    fkeys = new FKeySet8by2(" Cancel", "Further\nDetails", 0, 0,
			0, " Start\n  Over", "  Help", "  Done");
    assert(fkeys);
    LoadFields();
}

void X400AddrScreen::LoadFields()
{
    ((DataField*)fields[surfld])->SetValue(addr->GetField(Surname));
    ((DataField*)fields[forfld])->SetValue(addr->GetField(Forename));
    ((DataField*)fields[inifld])->SetValue(addr->GetField(Initials));
    ((DataField*)fields[genfld])->SetValue(addr->GetField(Generation));
    ((DataField*)fields[ac1fld])->SetValue(addr->GetField(OrgUnit1));
    ((DataField*)fields[ac2fld])->SetValue(addr->GetField(OrgUnit2));
    ((DataField*)fields[ac3fld])->SetValue(addr->GetField(OrgUnit3));
    ((DataField*)fields[ac4fld])->SetValue(addr->GetField(OrgUnit4));
    ((DataField*)fields[orgfld])->SetValue(addr->GetField(OrgName));
    ((DataField*)fields[countryfld])->SetValue(addr->GetField(Country));
    ((DataField*)fields[admdfld])->SetValue(addr->GetField(ADMD));
    ((DataField*)fields[prmdfld])->SetValue(addr->GetField(PRMD));
    Paint();
}

void X400AddrScreen::SaveFields()
{
    addr->SetField(Surname, ((DataField*)fields[surfld])->Value());
    addr->SetField(Forename, ((DataField*)fields[forfld])->Value());
    addr->SetField(Initials, ((DataField*)fields[inifld])->Value());
    addr->SetField(Generation, ((DataField*)fields[genfld])->Value());
    addr->SetField(OrgUnit1, ((DataField*)fields[ac1fld])->Value());
    addr->SetField(OrgUnit2, ((DataField*)fields[ac2fld])->Value());
    addr->SetField(OrgUnit3, ((DataField*)fields[ac3fld])->Value());
    addr->SetField(OrgUnit4, ((DataField*)fields[ac4fld])->Value());
    addr->SetField(OrgName, ((DataField*)fields[orgfld])->Value());
    addr->SetField(Country, ((DataField*)fields[countryfld])->Value());
    addr->SetField(ADMD, ((DataField*)fields[admdfld])->Value());
    addr->SetField(PRMD, ((DataField*)fields[prmdfld])->Value());
}

Screen *X400AddrScreen::HandleFKey(int fkeynum, int &done)
{
    done = 0;
    switch(fkeynum)
    {
    case F1_PRI:
	done = 1;
	break;
    case F2_PRI:
	return new X400AddrScreen2(this, addr, canedit);
    case F6_PRI: // start over 
	LoadFields();
	break;
    case F7_PRI:
	Help("X400");
	break;
    case F8_PRI:
	SaveFields();
	done = 1;
	break;
    }
    return 0;
}

X400AddrScreen::~X400AddrScreen()
{
    DestructTrace("X400AddrScreen");
}

//-----------------------------------------------------------------

X400AddrScreen2::X400AddrScreen2(Screen *parent_in, OMAddress *addr_in,
	int canedit_in)
    : OMFormScreen(parent_in, "X.400 Address", 
	"Enter the X.400 address components and press Done.",
	11, 15, 0, 0, canedit_in),
      addr(addr_in)
{
    ConstructTrace("X400AddrScreen2");
    AddLabel(3, 8, "X.121 Address");
    x121fld = AddField(new DataField(3, 32, 15, 1));
    AddLabel(4, 8, "UA Unique Address");
    uauafld = AddField(new DataField(4, 32, 32, 1));
    AddLabel(5, 8, "Telematic Terminal Id");
    telefld = AddField(new DataField(5, 32, 24, 1));
    AddLabel(7, 0, "Domain Defined Attributes");
    AddLabel(8, 0, "No.");
    AddLabel(8, 5, "Type");
    AddLabel(8, 16, "Value");
    AddLabel(9, 0, "1");
    ddat1fld = AddField(new DataField(9, 5, 8, 1));
    dda1fld = AddField(new DataField(9, 16, 64, 1));
    AddField(new DataField(10, 16, 64, 1));
    AddLabel(12, 0, "2");
    ddat2fld = AddField(new DataField(12, 5, 8, 1));
    dda2fld = AddField(new DataField(12, 16, 64, 1));
    AddField(new DataField(13, 16, 64, 1));
    AddLabel(15, 0, "3");
    ddat3fld = AddField(new DataField(15, 5, 8, 1));
    dda3fld = AddField(new DataField(15, 16, 64, 1));
    AddField(new DataField(16, 16, 64, 1));
    AddLabel(18, 0, "4");
    ddat4fld = AddField(new DataField(18, 5, 8, 1));
    dda4fld = AddField(new DataField(18, 16, 64, 1));
    AddField(new DataField(19, 16, 64, 1));
    fkeys = new FKeySet8by2(" Cancel", "Previous\n Details", 0, 0,
			0, " Start\n  Over", "  Help", "  Done");
    assert(fkeys);
    LoadFields();
}

void X400AddrScreen2::LoadFields()
{
    ((DataField*)fields[x121fld])->SetValue(addr->GetField(X121Addr));
    ((DataField*)fields[uauafld])->SetValue(addr->GetField(UAUniqID));
    ((DataField*)fields[telefld])->SetValue(addr->GetField(TelematicID));
    ((DataField*)fields[dda1fld])->SetValue(addr->GetField(DDA1));
    ((DataField*)fields[dda2fld])->SetValue(addr->GetField(DDA2));
    ((DataField*)fields[dda3fld])->SetValue(addr->GetField(DDA3));
    ((DataField*)fields[dda4fld])->SetValue(addr->GetField(DDA4));
    LoadMultiField(ddat1fld, 2, addr->GetField(DDATyp1));
    LoadMultiField(ddat2fld, 2, addr->GetField(DDATyp2));
    LoadMultiField(ddat3fld, 2, addr->GetField(DDATyp3));
    LoadMultiField(ddat4fld, 2, addr->GetField(DDATyp4));
    Paint();
}

void X400AddrScreen2::SaveFields()
{
    addr->SetField(X121Addr, ((DataField*)fields[x121fld])->Value());
    addr->SetField(UAUniqID, ((DataField*)fields[uauafld])->Value());
    addr->SetField(TelematicID, ((DataField*)fields[telefld])->Value());
    addr->SetField(DDA1, ((DataField*)fields[dda1fld])->Value());
    addr->SetField(DDA2, ((DataField*)fields[dda2fld])->Value());
    addr->SetField(DDA3, ((DataField*)fields[dda3fld])->Value());
    addr->SetField(DDA4, ((DataField*)fields[dda4fld])->Value());
    char *v;
    addr->SetField(DDATyp1, v = GetMultiField(ddat1fld, 2)); delete [] v;
    addr->SetField(DDATyp2, v = GetMultiField(ddat2fld, 2)); delete [] v;
    addr->SetField(DDATyp3, v = GetMultiField(ddat3fld, 2)); delete [] v;
    addr->SetField(DDATyp4, v = GetMultiField(ddat4fld, 2)); delete [] v;
}

Screen *X400AddrScreen2::HandleFKey(int fkeynum, int &done)
{
    done = 0;
    switch(fkeynum)
    {
    case F1_PRI: // need some extra stuff to distinguish F1/F2 actions
	done = 1;
	break;
    case F2_PRI:
	done = 1;
	break;
    case F6_PRI: // start over
	LoadFields();
	break;
    case F7_PRI:
	Help("EXTENDEDX400");
	break;
    case F8_PRI:
	SaveFields();
	done = 1;
	break;
    }
    return 0;
}

X400AddrScreen2::~X400AddrScreen2()
{
    DestructTrace("X400AddrScreen2");
}

//-----------------------------------------------------------------

ForeignAddrScreen::ForeignAddrScreen(Screen *parent_in, OMAddress *addr_in,
	int canedit_in)
    : OMFormScreen(parent_in, "Extended Address", 
	"Enter the full OpenMail address and press Done.",
	9, 16, 0, 0, canedit_in),
      addr(addr_in)
{
    ConstructTrace("ForeignAddrScreen");
    AddLabel(3, 5, "Surname");
    surfld = AddField(new DataField(3, 17, 40, 1));
    AddLabel(5, 5, "Forename");
    forfld = AddField(new DataField(5, 17, 16, 1));
    AddLabel(5, 36, "Initials");
    inifld = AddField(new DataField(5, 47, 5, 1));
    AddLabel(5, 55, "Generation");
    genfld = AddField(new DataField(5, 68, 3, 1));
    AddLabel(7, 5, "Address Component 1");
    ac1fld = AddField(new DataField(7, 27, 32, 1));
    AddLabel(8, 5, "Address Component 2");
    ac2fld = AddField(new DataField(8, 27, 32, 1));
    AddLabel(9, 5, "Address Component 3");
    ac3fld = AddField(new DataField(9, 27, 32, 1));
    AddLabel(10, 5, "Address Component 4");
    ac4fld = AddField(new DataField(10, 27, 32, 1));
    AddLabel(12, 0, "Foreign Address:");
    for (int i = 0; i < 8; i++)
        AddField(new DataField(13+i, 4, 64, 1));
    fkeys = new FKeySet8by2(" Cancel", " X.400\nAddress", 0, 0,
			0, " Start\n  Over", "  Help", "  Done");
    assert(fkeys);
    LoadFields();
}

void ForeignAddrScreen::LoadFields()
{
    ((DataField*)fields[surfld])->SetValue(addr->GetField(Surname));
    ((DataField*)fields[forfld])->SetValue(addr->GetField(Forename));
    ((DataField*)fields[inifld])->SetValue(addr->GetField(Initials));
    ((DataField*)fields[genfld])->SetValue(addr->GetField(Generation));
    ((DataField*)fields[ac1fld])->SetValue(addr->GetField(OrgUnit1));
    ((DataField*)fields[ac2fld])->SetValue(addr->GetField(OrgUnit2));
    ((DataField*)fields[ac3fld])->SetValue(addr->GetField(OrgUnit3));
    ((DataField*)fields[ac4fld])->SetValue(addr->GetField(OrgUnit4));
    LoadMultiField(ac4fld+1, 8, addr->GetField(DDA1));
    Paint();
}

void ForeignAddrScreen::SaveFields()
{
    addr->SetField(Surname, ((DataField*)fields[surfld])->Value());
    addr->SetField(Forename, ((DataField*)fields[forfld])->Value());
    addr->SetField(Initials, ((DataField*)fields[inifld])->Value());
    addr->SetField(Generation, ((DataField*)fields[genfld])->Value());
    addr->SetField(OrgUnit1, ((DataField*)fields[ac1fld])->Value());
    addr->SetField(OrgUnit2, ((DataField*)fields[ac2fld])->Value());
    addr->SetField(OrgUnit3, ((DataField*)fields[ac3fld])->Value());
    addr->SetField(OrgUnit4, ((DataField*)fields[ac4fld])->Value());
    char *v;
    addr->SetField(DDA1, v=GetMultiField(ac4fld+1, 8));
    delete [] v;
}

Screen *ForeignAddrScreen::HandleFKey(int fkeynum, int &done)
{
    done = 0;
    switch(fkeynum)
    {
    case F1_PRI:
	done = 1;
	break;
    case F2_PRI:
	return new X400AddrScreen(this, addr, canedit);
    case F6_PRI: // start over
	LoadFields();
	break;
    case F7_PRI:
	Help("FOREIGNADDRESS");
	break;
    case F8_PRI:
	SaveFields();
	done = 1;
	break;
    default:
	Beep();
    }
    return 0;
}

ForeignAddrScreen::~ForeignAddrScreen()
{
    DestructTrace("ForeignAddrScreen");
}

//---------------------------------------------------------------------
// This popup's Run() returns 1 (merge) or -1 (cancel).
// The listref and itemref of the dlist can be fetched by calling
// GetSelection(). The user must then do the merge
// before deleting the popup (as this closes the destination
// folder).

DListPopup::DListPopup(Screen *owner_in)
    : OMListBoxPopup(owner_in,
         "Select a distribution list to merge and press Perform Merge.",
	 "DISTLISTUSING")
{
    ConstructTrace("DListPopup");
    fkeys = new FKeySet8by2("Perform\n Merge", 0, 0, 0,
			0, 0, "  Help", " Cancel\n Merge");
    assert(fkeys);
    if (mstore->OpenSubfolder(5, maxitems) == 0)
        cnt = mstore->Folder()->FirstPage();
    else cnt = 0;
    GetItems();
}

void DListPopup::GetSelection(int &lref_out, long &inum_out)
{
    lref_out = listref_out;
    inum_out = itemnum_out;
}

int DListPopup::NextPage()
{
    return cnt = mstore->Folder()->NextPage();
}

int DListPopup::PrevPage()
{
    return cnt = mstore->Folder()->PrevPage();
}

void DListPopup::GetItems()
{
    Folder *f = (Folder*)mstore->Folder();
    for (int r = 0; r < cnt; r++)
    {
	FolderItem *fi = f->Child(pagenum*maxitems+r+1);
	delete [] items[r];
	items[r] = new char[strlen(fi->Name())+1];
	if (items[r]) strcpy(items[r], fi->Name());
	else break;
    }
    cnt = r;
    islast = ((pagenum*maxitems+cnt)>=f->Size());
}

void DListPopup::HandleKey(int ch, int &done, int &rtn)
{
    if (ch == '\n')
	HandleFKey(KB_FKEY_CODE(0), done, rtn);
    else
	OMListBoxPopup::HandleKey(ch, done, rtn);
}

void DListPopup::HandleFKey(int ch, int &done, int &rtn)
{
    switch (ch)
    {
    case KB_FKEY_CODE(0): // done
	listref_out = mstore->Folder()->Ref();
	itemnum_out = pos+1;
	done = 1;
	rtn = 1;
	break;
    default:
	OMListBoxPopup::HandleFKey(ch, done, rtn);
    }
}

DListPopup::~DListPopup()
{
    mstore->CloseSubfolder();
    owner->RefreshContents();
    ClearScreen();
    owner->Paint();
    DestructTrace("DListPopup");
}

int EnterDistListScreen::HasAddress(OMAddress *addr)
{
    AddressPage *ap = &firstpage;
    while (ap)
    {
        for (int i = 0; i < pagesize; i++)
        {
	    OMAddress *addr2 = ap->GetAddress(i);
	    if (*addr2 == *addr) return 1;
	}
	ap = ap->next;
    }
    return 0;
}

void EnterDistListScreen::MergeOpenDListTF(int invert, char *pat)
{
    int typ = -1, cnt = -1, acklvl, ac = 0;
    AddressPage *ap = &firstpage;
    OMAddress *newaddr;
    while ((newaddr = mstore->ReadDListRecord(typ, acklvl)) != 0)
    {
	if (HasAddress(newaddr)) // avoid duplicates
	{
	    delete newaddr;
	    continue;
	}
	if (invert)
	{
	    if (typ == 0) typ = 1; // convert FROM: to TO:
	    else if (typ == 1) typ = 2; // convert TO: to CC:
	}
	if (pat)
	{
	    char *txt = newaddr->GetExternal();
	    int gotmatch = (strstr(txt, pat) != 0);
	    delete [] txt;
	    if (!gotmatch)
	    {
		delete newaddr;
		continue;
	    }
	    // THIS IS A KLUDGE - IF FIRST MATCH MAKE IT A TO: ADDRESS... 
	    if (ac++ == 0) typ = 1;
	}
	for (;;) // find next free address slot
	{
	    if (++cnt >= ap->PageSize())
	    {
		ap = ap->NextPage();
		cnt = 0;
	    }
	    OMAddress *ad = ap->GetAddress(cnt);
	    if (ad == 0 || ad->IsEmpty())
		break;
	}
	ap->SetAddress(cnt, newaddr);
	ap->SetAddrType(cnt, typ);
    }
    mstore->CloseDListTF(1);
    GetItems();
}

void EnterDistListScreen::MergeDList(char *fname, int invert, char *pat)
{
    if (mstore->OpenDListTF(fname) == 0) MergeOpenDListTF(invert, pat);
}

void EnterDistListScreen::MergeDList(FolderItem *fi, int invert, char *pat)
{
    if (mstore->OpenDListTF(fi) == 0) MergeOpenDListTF(invert, pat);
}

void EnterDistListScreen::MergeList()
{
    DListPopup *dlpop = new DListPopup(this);
    int cmd = dlpop ? dlpop->Run() : -1;
    if (cmd == 1)
    {
        int lref;
        long inum;
        dlpop->GetSelection(lref, inum);
	MergeDList(((Folder*)mstore->Folder())->Child(inum));
	GetItems();
    }
    delete dlpop;
}

//-----------------------------------------------------------------

void NicknameScreen::GetItems()
{
    for (int i = 0; i < pagesize; i++)
    {
	char *txt = 0;
	OMAddress *addr;
	if (apage && (addr = apage->GetAddress(i)) != 0)
	{
	    char *txt = addr->GetExternal();
	    ((DataField*)fields[2*i])->SetValue(addr->GetField(Nickname));
	    ((AddressField*)fields[2*i+1])->SetValue(txt);
	    delete [] txt;
	}
	else
	{
	    ((DataField*)fields[2*i])->SetValue(0);
	    ((AddressField*)fields[2*i+1])->SetValue(0);
	}
    }
}

Screen *NicknameScreen::HandleKey(int ch, int &done)
{
    if (ch != KB_PGUP && ch != KB_PGDN)
        return OMFormScreen::HandleKey(ch, done);
    SyncAddresses();
    if (ch == KB_PGDN)
	apage = apage->NextPage();
    else 
	apage = apage->PrevPage();
    fnow = 0;
    ClearScreen();
    GetItems();
    int r, c;
    labels[0].Position(r, c);
    if (apage == &firstpage)
	SetLabel(0, r, c, "Press PgDn for more names.     ");
    else
	SetLabel(0, r, c, "Press PgUp/PgDn for more names.");
    Paint();
    return 0;
}

Screen *NicknameScreen::HandleFKey(int fkey, int &done)
{
    SyncAddresses();
    done = 0;
    switch(fkey)
    {
    case F1_PRI: // cancel changes
	done = 1;
	break;
    case F5_PRI: // extended address
	if ((fnow%2) == 0) fnow++;
        OMAddress *addr = apage->GetAddress(fnow/2);
	if (addr == 0)
	    apage->SetAddress(fnow/2, addr = new OMAddress());
	Screen *s = new ExtendedAddrScreen(this, addr);
	assert(addr && s);
	s->Run();
	delete s;
        char *newaddr =addr->GetExternal();
        ((AddressField*)fields[fnow])->SetValue(newaddr);
	delete [] newaddr;
	break;
    case F7_PRI: // help
	Help("NICKNAMES");
	break;
    case F8_PRI: // done
	AddressPage *ap = &firstpage;
	int nn = 0;
	for (;;)
	{
	    for (int r = 0; r < pagesize; r++)
	    {
		OMAddress *addr = ap->GetAddress(r);
		if (addr) addr = new OMAddress(addr);
		mstore->SetNickname(nn++, addr);
	    }
	    if (ap == ap->NextPage()) break; // last page
	    ap = ap->NextPage();
	}
	mstore->ClearNicknames(nn);
	done = 1;
	break;
    default:
	Beep();
    }
    return 0;
}

void NicknameScreen::SyncAddresses()
{
    for (int f = 0; f < pagesize; f++)
    {
        char *name = ((DataField*)fields[2*f])->Value();
        char *addr = ((AddressField*)fields[2*f+1])->Value();
	OMAddress *oma = apage->GetAddress(f);
	if (oma == 0)
	    if ((name && name[0]) || (addr && addr[0]))
	        apage->SetAddress(f, oma = new OMAddress());
	    else continue;
	assert(oma);
Debug3("Syncing %d, %s, %s", f, addr?addr:"NULL", name?name:"NULL");
	oma->AssignExternal(addr?addr:"");
	oma->SetField(Nickname, name?name:"");
    }
}

NicknameScreen::NicknameScreen(Screen *parent_in)
	: OMFormScreen(parent_in, "Nicknames List",
			"Edit the list and press Save Changes.", 3, 26),
	  pagesize(13), firstpage(13)
{
    ConstructTrace("NicknamesScreen");
    for (int r = 0; r < pagesize; r++)
    {
	AddField(new DataField(7+r, 2, 20));
	AddField(new AddressField(this, 7+r, 24, 52, (1<<7)|(1<<15)));
    }
    apage = &firstpage;
    AddLabel(7+pagesize, 44, "Press PgDn for more names.");
    AddLabel(4, 2, "Nickname");
    AddLabel(4, 24, "Full Name/Address");
    AddressPage *ap = apage;
    int nn = 0;
    OMAddress *addr;
    while ((addr = mstore->GetNickname(nn)) != 0)
    {
	OMAddress *newaddr = new OMAddress(addr);
	assert(newaddr);
	ap->SetAddress((nn % pagesize), newaddr);
	nn++;
	if ((nn%pagesize) == 0)
	    ap = ap->NextPage();
    }
    GetItems();
    fkeys = new FKeySet8by2(" Cancel\nChanges", 0, 0, 0,
			"Extended\nAddress", 0, "  Help", "  Save\n Changes");
}

NicknameScreen::~NicknameScreen()
{
    AddressPage *ap = firstpage.next;
    while (ap)
    {
	AddressPage *tmp = ap;
	ap = ap->next;
	delete tmp;
    }
    DestructTrace("NicknamesScreen");
}

//------------------------------------------------------------------

int ExternalCompose(char *fname, int typ)
{
    for (int t = 0; t < mstore->filetypecnt; t++)
	if (mstore->filetypes[t].GetType() == typ)
	{
	    char *xapp = mstore->filetypes[t].GetEditor();
	    if (xapp && xapp[0])
	    {
		char cmd[256];
		strcpy(cmd, xapp);
		strcat(cmd, " ");
		strcat(cmd, fname);
		StopCurses();
		system(cmd);
		StartCurses();
		return 0;
	    }
	    break;
	}
    return -1;
}

