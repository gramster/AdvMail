#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include <assert.h>

#include "debug.h"
#include "config.h"

#if DEBUG<2 // don't trace these normally - there's too many...
#undef ConstructTrace
#undef DestructTrace
#define ConstructTrace(m)
#define DestructTrace(m)
#endif

#include "mail.h"
    
#ifdef TEST
#define Trace	puts
#else
#define Trace(s)
#endif

#ifdef NO_INLINES
# include "mail.inl" // include inline routines
#endif

MessageStore *mstore = 0;
extern FILE *debugfp;


void strupr(char *buf)
{
    while (*buf)
    {
	if (islower(*buf)) *buf = toupper(*buf);
	buf++;
    }
}

//---------------------------------------------------------------

OMAddress::OMAddress(OMAddress *addr)
{
    ConstructTrace("OMAddress");
    for (int i = Surname; i <= Nickname; i++)
	flds[i] = 0;
    if (addr) Set(addr);
}

void OMAddress::Set(OMAddress *addr)
{
    for (int i = Surname; i <= Nickname; i++)
	SetField(i, addr->flds[i]);
}

OMAddress::~OMAddress()
{
    for (int i = Surname; i <= Nickname; i++)
	delete [] flds[i];
    DestructTrace("OMAddress");
}

int OMAddress::IsEmpty()
{
    for (int i = Surname; i <= Nickname; i++)
	if (flds[i]) return 0;
    return 1;
}

void OMAddress::SetField(int fnum, char *val)
{
    assert(fnum >= Surname && fnum <= Nickname);
    if (fnum==Initials || fnum==Generation)
	if (val && strchr(val, '.'))
	{
	    SetAbbrevField(fnum, val);
	    return;
	}
    delete [] flds[fnum];
    flds[fnum] = 0;
    if (val)
    {
	while (isspace(*val)) val++;
        if (val[0])
    	{
	    int l = strlen(val);
	    while (l > 0 && val[l-1] == ' ') val[--l] = '\0';
    	    flds[fnum] = new char[l+1];
    	    if (flds[fnum]) strcpy(flds[fnum], val);
        }
    }
}

void OMAddress::SetAbbrevField(int fnum, char *val) // strips out periods
{
    assert(fnum==Initials || fnum==Generation);
    char buf[1024];
    for (int i = 0, j = 0; val[i] != '\0'; i++)
    {
	if (val[i] != '.')
	    buf[j++] = val[i];
    }
    buf[j] = '\0';
    SetField(fnum, buf);
}

char *OMAddress::GetField(int fnum)
{
    assert(fnum >= Surname && fnum <= Nickname);
    return flds[fnum];
}

void OMAddress::AppendField(int fnum, char *buf, char *sep)
{
    if (flds[fnum] && flds[fnum][0])
    {
	strcat(buf, flds[fnum]);
	if (sep) strcat(buf, sep);
    }
}

char *OMAddress::GetExternal() // caller must delete returned string!
{
    char buf[1024];
    buf[0] = '\0';
    AppendField(Forename, buf, " ");
    AppendField(Initials, buf, " ");
    AppendField(Surname, buf, " ");
    AppendField(Generation, buf, ". ");
    strcat(buf, "/"); 
    AppendField(OrgUnit1, buf, ",");
    AppendField(OrgUnit2, buf, ",");
    AppendField(OrgUnit3, buf, ",");
    AppendField(OrgUnit4, buf, ",");
    int i = strlen (buf);
    while (i > 0 && buf[i-1]==',') i--;
    buf[i] = '\0';
    strcat(buf, "/"); AppendField(OrgName, buf);
    strcat(buf, "/"); AppendField(Country, buf);
    strcat(buf, "/"); AppendField(ADMD, buf);
    strcat(buf, "/"); AppendField(PRMD, buf);
    strcat(buf, "/"); AppendField(X121Addr, buf);
    strcat(buf, "/"); AppendField(UAUniqID, buf);
    strcat(buf, "/"); AppendField(TelematicID, buf);
    i = strlen (buf);
    while (i > 0 && (buf[i-1] == '/' || buf[i-1]==',')) i--;
    buf[i] = '\0';
    i = DDATyp1;
    for (int j = 0; j < 4; j++, i+=2)
    {
	if (flds[i+1] && flds[i+1][0])
	{
	    strcat(buf, " (");
	    if (flds[i] && memcmp(flds[i], "HPMEXT", 6) != 0)
	    {
		strcat(buf, flds[i]);
		strcat(buf, "|");
	    }
	    strcat(buf, flds[i+1]);
	    strcat(buf, ")");
	}
    }
    char *rtn = new char [strlen(buf)+1];
    assert(rtn);
    strcpy(rtn, buf);
    if (debugfp) fprintf(debugfp, "OMAddress::GetExternal() = %s\n", rtn);
    return rtn;
}

char *OMAddress::GetInternal() // caller must delete returned string!
{
    char buf[1024];
    if (flds[Surname]) strcpy(buf, flds[Surname]);
    else buf[0] = '\0';
    for (int f = Forename; f<= Spare; f++)
    {
	strcat(buf, "\035");
        if (flds[f]) strcat(buf, flds[f]);
    }
    strcat(buf, "\027");
    char *rtn = new char [strlen(buf)+1];
    assert(rtn);
    strcpy(rtn, buf);
    if (debugfp) fprintf(debugfp, "OMAddress::GetInternal() = %s\n", rtn);
    return rtn;
}

char *OMAddress::GetDirEntry() // caller must delete returned string!
{
    char buf[1024];
    strcpy(buf, "203\035");
    if (flds[Nickname]) strcat(buf, flds[Nickname]);
    for (int f = Surname; f <= Spare; f++)
    {
	char tagbuf[8];
	strcat(buf, "\036");
	sprintf(tagbuf, "%d", f+1);
	strcat(buf, tagbuf);
	strcat(buf, "\035");
        if (flds[f]) strcat(buf, flds[f]);
    }
    char *rtn = new char [strlen(buf)+1];
    assert(rtn);
    strcpy(rtn, buf);
    return rtn;
}

void OMAddress::AssignExternal(char *extaddr)
{
    char buf[1024], *extra, *leftover;
    strcpy(buf, extaddr);
    for (int i = 0; buf[i] != '\0'; i++) // sanitise the address
	if (!isprint(buf[i])) buf[i] = ' ';
    if ((extra = strchr(buf,'(')) != 0)
	*extra = '\0';
    if ((leftover = strchr(buf,'/')) != 0)
	*leftover++ = '\0';
    char *surname = 0;
    char *forename = 0;
    char *initials = 0;
    char *gen = 0;
    char surbuf[100];
    surbuf[0] = 0;
    for (char *tok = strtok(buf," "); tok != 0; tok = strtok(0, " "))
    {
	if (strchr(tok,'.') == 0)
	{
	    if (forename == 0 && surname != 0)
	    {
		forename = surname;
		surname = 0;
	    }
	    if (initials == 0 && gen != 0)
		initials = gen;
	    if (surname == 0)
	        strcpy(surbuf, tok);
	    else
	    {
	        strcat(surbuf, " ");
	        strcat(surbuf, tok);
	    }
	    surname = tok;
	    gen = 0;
	}
	else if (surname == 0) // haven't got to surname
	    initials = tok;
	else
	    gen = tok;
    }
    //SetField(Surname, surname);
    SetField(Surname, surbuf);
    SetField(Forename, forename);
    SetField(Initials, initials);
    SetField(Generation, gen);
    if (leftover)
    {
        strcpy(buf, leftover);
        if ((leftover = strchr(buf,'/')) != 0)
	    *leftover++ = '\0';
	tok = buf;
    }
    else tok = 0;
    char *next_tok;
    for (i = OrgUnit1; i <= OrgUnit4; i++)
    {
	if (tok != 0)
	{
	    if ((next_tok = strchr(tok,',')) != 0)
		*next_tok++ = '\0';
	    SetField(i, tok);
	    tok = next_tok;
	}
	else SetField(i, 0);
    }
    tok = leftover;
    for (i = OrgName; i <= TelematicID; i++)
    {
	if (tok != 0)
	{
	    if ((next_tok = strchr(tok,'/')) != 0)
		*next_tok++ = '\0';
	    SetField(i, tok);
	    tok = next_tok;
	}
	else SetField(i, 0);
    }
    if (extra) *extra = '(';
    tok = extra;
    for (i = 0; i < 4; i++)
    {
	char default_type[16];
	(void)sprintf(default_type, "HPMEXT%d", i);
	char *dda_type = 0;
	char *dda_value = 0;
	if (tok != 0)
	{
	    if ((next_tok = strchr(tok,')')) != 0)
		*next_tok++ = '\0';
	    if ((dda_value = strchr(tok,'|')) != 0)
	    {
		dda_type = tok+1;
		*dda_value++ = '\0';
	    }
	    else
	    {
		dda_type = default_type;
		dda_value = tok+1;
	    }
	    tok = (next_tok != 0 ? strchr(next_tok,'(') : 0);
	}
	SetField(DDATyp1+2*i, dda_type);
	SetField(DDA1+2*i, dda_value);
    }
}

void OMAddress::AssignInternal(char *intaddr)
{
    char tmp_buf[1024];
    char *tok;
    char *next_tok;
    (void)strcpy(tmp_buf,intaddr);
    tok = tmp_buf;
    for (int f = Surname; tok && f <= Spare; f++)
    {
        if ((next_tok = strchr(tok, '\035')) != 0)
	    *(next_tok++) = '\0';
	SetField(f, tok);
	tok = next_tok;
    }
    while (f <= Nickname)
	SetField(f++, 0);
}

void OMAddress::AssignDirEntry(char *diraddr)
{
    char tmp_buf[1024];
    char *tok;
    char *next_tok;
    (void)strcpy(tmp_buf,diraddr);
    tok = tmp_buf;
    for (int f = Surname; f <= Nickname; f++)
	SetField(f, 0);
    while (tok)
    {
        if ((next_tok = strchr(tok, '\036')) != 0)
	    *(next_tok++) = '\0';
	char *split = strchr(tok, '\035');
	assert(split);
	*split++ = '\0';
	int fld = atoi(tok);
	if (fld == 203) // nickname?
	    SetField(Nickname, split);
	else
	    SetField(fld-1, split);
	tok = next_tok;
    }
}

int OMAddress::operator==(OMAddress &rhs)
{
    if (flds == 0)
	if (rhs.flds == 0) return 1;
	else return 0;
    else if (rhs.flds == 0)
	return 0;
    for (int f = 0; f<= Nickname; f++)
    {
	if (flds[f] == rhs.flds[f]) continue;
	if (flds[f] == 0 || rhs.flds[f] == 0) return 0;
	if (strcmp(flds[f], rhs.flds[f]) != 0) return 0;
    }
    return 1;
}

int OMAddress::operator!=(OMAddress &rhs)
{
    return !(*this == rhs);
}

//---------------------------------------------------------------

void MessageSettings::SetToDefaults()
{
    (void)time(&tcks);
    // date should be set to zero to cause messages to be 
    // deilvered immediately
    // tcks = 0;
    pri = imp = sens = ack = 0;
    allow_pubdls = allow_altrecip = allow_convert = 
	allow_ndn = returnmsg = 1;
}

//--------------------------------------------------------------

int User::SetPassword(char *passwd)
{
    int rtn = OMChangePassword(password, passwd);
    if (rtn == 0)
    {
	delete [] password;
	password = new char[strlen(passwd)+1];
	assert(password);
	strcpy(password, passwd);
    }
    return rtn;
}

//--------------------------------------------------------------

void FolderItem::Init(FolderItem *parent_in, int typ_in, int item_num_in,
		      char *name_in, char *creator_in, 
			long create_date_in, long flags_in)
{
    assert(name_in);
    assert(creator_in);
    parent = parent_in;
    item_num = item_num_in;
    typ = typ_in;
    name = new char[strlen(name_in)+1];
    if (name) strcpy(name, name_in);
    creator = new char[strlen(creator_in)+1];
    if (creator) strcpy(creator, creator_in);
    create_date = create_date_in;
    flags = flags_in;
    ref = -1;
}

void FolderItem::Format(char * &buf)
{
    char tmp[100];
    sprintf(tmp, " %-4d %-20.20s %-20.20s %-12.12s",
	   item_num, name, creator, Type());
    buf = new char[strlen(tmp)+1];
    assert(buf);
    strcpy(buf, tmp);
}

FolderItem::~FolderItem()
{
    DestructTrace("FolderItem");
    delete name;
    delete creator;
}

// The next four predicates are somewhat artificial: to avoid
// pedanticism they all have default values that are certainly
// not true of all folderitems.

int FolderItem::IsAtomic() const
{
    return 0;
}

int FolderItem::IsMessage() const
{
    return 0;
}

int FolderItem::IsFolder() const
{
    return 0;
}

int FolderItem::IsBBoard() const
{
    return 0;
}

int FolderItem::IsDistList() const
{
    return 0;
}

int FolderItem::TypeID() const
{
    return typ;
}

//-----------------------------------------------------------

int CompositeItem::IsMessage() const
{
    return 0;
}

int CompositeItem::IsFolder() const
{
    return 0;
}

int CompositeItem::IsBBoard() const
{
    return 0;
}

void CompositeItem::AddItem(FolderItem *itm)
{
    assert(itm);
    for (int i = 0; i < MAX_MSG_LINES; i++)
    {
	if (items[i] == 0)
	{
	    items[i] = itm;
	    return;
	}
    }
    assert(0);
}

void CompositeItem::FreeItems()
{
    for (int i = 0; i < MAX_MSG_LINES; i++)
    {
	delete items[i];
	items[i] = 0;
    }
}

int CompositeItem::GetItems()
{
    int cnt = 0;
    FreeItems();
    for (;;)
    {
	char *name=0, *creator=0;
	int subitm, type;
	long flags, msgflags, receipt_date, create_date, deferred_date;
	FolderItem *itm = 0;
	int rtn = OMGetListElt(subitm, name, creator, type, flags, msgflags,
			   create_date, receipt_date, deferred_date);
	if (creator) // convert formats
	{
	    char *tmp = creator;
	    OMAddress addr;
	    addr.AssignInternal(creator);
	    creator = addr.GetExternal();
	    delete [] tmp;
	}
	if (rtn >= 0)
	    switch (type)
	    {
	    case BBoardType:
		itm = new BBArea(this,subitm,name,creator,create_date,flags);
		break;
	    case FolderType:
		itm = new Folder(this,subitm,name,creator,create_date,flags);
		break;
	    case MessageType:
		itm = new Message(this,subitm,name,creator,create_date,
			flags,msgflags, receipt_date, deferred_date);
		break;
	    case ReplyType:
		itm = new Reply(this,subitm,name,creator,create_date,
			flags,msgflags, receipt_date, deferred_date);
		break;
	    case NDNMessageType:
		itm = new NonDelivery(this,subitm,name,creator,create_date,
			flags,msgflags, receipt_date, deferred_date);
		break;
	    case DistListType:
		itm = new DistList(this,subitm,name,creator,create_date,flags);
		break;
	    case TextType:
		itm = new Text(this,subitm,name,creator,create_date,flags);
		break;
	    default:
		itm= new Other(this,type,subitm,name,creator,create_date,flags);
		break;
	    }
	delete [] name;
	delete [] creator;
#ifdef TEST
	//itm->Print();
#endif
	if (itm)
	{
	    AddItem(itm);
	    cnt++;
	}
	if (itm == 0 || rtn != 0)
	    break;
    }
    return cnt;
}

FolderItem *CompositeItem::Child(int item_num) const
{
    for (int i = 0; i < MAX_MSG_LINES; i++)
	if (items[i] && items[i]->ItemNum() == item_num)
	    return items[i];
    return 0;
}

CompositeItem::CompositeItem(FolderItem *parent_in, int typ_in, int item_num_in,
			     char *name_in, char *creator_in,
			    long create_date_in, long flags_in)
    : FolderItem(parent_in, typ_in, item_num_in, name_in, creator_in, create_date_in,
		flags_in)
{
    ConstructTrace("CompositeItem");
    for (int i = 0; i < MAX_MSG_LINES; i++)
	items[i] = 0;
}

int CompositeItem::Open(int pgsz)
{
    assert(ref<0); // mustn't already be open
    pagesize = pgsz;
    if (OMPrepList(parent->Ref(), item_num, ref,folder_size)!=0)
	return -1;
    if (folder_size < 1l)
	first_item = 0;
    else first_item = 1;
    return 0;
}

int CompositeItem::FirstPage()
{
    if (ref < 0 || OMPrepListPage(ref, first_item = 1, pagesize)!=0)
	return -1;
    return GetItems();
}

int CompositeItem::NextPage()
{
    int ofi = first_item, rtn;
    first_item += pagesize;
    if (ref < 0 || first_item>folder_size ||
	OMPrepListPage(ref, first_item, pagesize)!=0)
    {
	first_item = ofi;
	return -1;
    }
    rtn = GetItems();
    if (rtn == 0)
	(void)PrevPage();
    return rtn;
}

int CompositeItem::PrevPage()
{
    int ofi = first_item, rtn;
    first_item -= pagesize;
    if (ref < 0 || first_item < 1 ||
	OMPrepListPage(ref, first_item, pagesize)!=0)
    {
	first_item = ofi;
	return -1;
    }
    rtn = GetItems();
    if (rtn == 0)
	(void)FirstPage();
    return rtn;
}

int CompositeItem::Close()
{
    if (ref >= 0)
    {
        OMCancel(ref);
	ref = -1;
        FreeItems();
    }
}

CompositeItem::~CompositeItem()
{
    Close();
    DestructTrace("CompositeItem");
}

//----------------------------------------------------------------------

int AtomicItem::IsAtomic() const
{
    return 1;	
}

int AtomicItem::IsDistList() const
{
    return 0;	
}

void AtomicItem::FreeItems()
{
    for (int i = 0; i < MAX_MSG_LINES; i++)
    {
	delete [] (text[i]);
	text[i] = 0;
    }
}

int AtomicItem::GetItems()
{
    int ln, rtn = 0;
    FreeItems();
    for (ln = 0; rtn == 0 && ln< pagesize; ln++)
    {
	char *txt=0;
	int line;
	rtn = OMReadLine(line, txt); // rtn = -1 (eof), 0 (more), 1 (eos)
	if (rtn<0 && txt[0]=='\0')
	{
	    delete [] txt; // hax to make like AdvMail/TI, which doesn't
				// show last line if empty
	    break;
	}
	assert(line == (ln+1));
	text[ln] = txt;
	if ((first_item+ln) > folder_size)
		folder_size = line; // probably we should
	             // move folder_size to composite_item only
    }
    return ln;
}

int AtomicItem::Open(int pgsz)
{
    assert(ref<0); // mustn't already be open
    pagesize = pgsz;
    if (OMPrepRead(parent->Ref(), item_num, 0, ref) != 0)
	return -1;
    first_item = 1;
    return 0;
}

int AtomicItem::FirstPage()
{
    if (ref < 0 || OMPrepReadPage(ref, pagesize, 0)!=0)
	return -1;
    first_item = 1;
    return GetItems();
}

int AtomicItem::NextPage()
{
    int ofi = first_item, rtn;
    first_item += pagesize;
    if (ref < 0 || OMPrepReadPage(ref, pagesize, 1)!=0)
    {
	first_item = ofi;
	return -1;
    }
    rtn = GetItems();
    if (rtn == 0)
	rtn = PrevPage();
    return rtn;
}

int AtomicItem::PrevPage()
{
    int ofi = first_item, rtn;
    first_item -= pagesize;
    if (ref < 0 || first_item < 1 || OMPrepReadPage(ref, pagesize, -1)!=0)
    {
	first_item = ofi;
	return -1;
    }
    rtn = GetItems();
    if (rtn == 0)
	rtn = FirstPage();
    return rtn;
}

int AtomicItem::Close()
{
    if (ref >= 0)
    {
        FreeItems();
        OMCancel(ref);
	ref = -1;
    }
}

AtomicItem::~AtomicItem()
{
    Close();
    DestructTrace("AtomicItem");
}

//----------------------------------------------------------------------

int Folder::IsFolder() const
{
    return 1;
}

char *Folder::Type() const
{
    return "Folder";
}

int Folder::Close()
{
    return CompositeItem::Close();
}

Folder::~Folder()
{
    Close();
    DestructTrace("Folder");
}

//----------------------------------------------------------------------

int BBArea::IsBBoard() const
{
    return 1;
}

char *BBArea::Type() const
{
    return "BBoard";
}

BBArea::~BBArea()
{
    Close();
    DestructTrace("BBArea");
}

//----------------------------------------------------------------------

UserFolder::UserFolder(int user_ref_in, char *user_in)
    : Folder(NULL, 1l, user_in, user_in, 0l, 0l)
{
    ref = user_ref_in;
    AddItem(new Folder(this, 1, "In Tray", creator, 0l, 0l));
    AddItem(new Folder(this, 2, "Out Tray", creator, 0l, 0l));
    AddItem(new Folder(this, 3, "Pending Tray", creator, 0l, 0l));
    AddItem(new Folder(this, 4, "File Area", creator, 0l, 0l));
    AddItem(new Folder(this, 5, "Distribution Lists", creator, 0l, 0l));
    AddItem(new Folder(this, 6, "Bulletin Boards", creator, 0l, 0l));
    folder_size = 6;
    ConstructTrace("UserFolder");
}

char *UserFolder::Type() const
{
    return "User Folder";
}

int UserFolder::Close()
{
    if (ref >= 0)
    {
        FreeItems();
	ref = -1;
    }
}

UserFolder::~UserFolder()
{
    Close();
    DestructTrace("UserFolder");
}

//----------------------------------------------------------------------

int DistList::OpenForAcks(int pgsz)
{
    assert(ref<0); // mustn't already be open
    pagesize = pgsz;
    if (OMPrepRead(parent->Ref(), item_num, 1, ref) != 0)
	return -1;
    first_item = 1;
    return 0;
}

int DistList::IsDistList() const
{
    return 1;
}

char *DistList::Type() const
{
    return "Distribution List";
}

DistList::~DistList()
{
    DestructTrace("DistList");
}

//----------------------------------------------------------------------

int Message::IsMessage() const
{
    return 1;	
}

char *Message::Type() const
{
    return "Message";
}

Message::~Message()
{
    DestructTrace("Message");
}

void Message::GetSettings(MessageSettings &mset)
{
    mset.SetImportance(Importance());
    mset.SetPriority(Priority());
    mset.SetSensitivity(Sensitivity());
    mset.SetAckLevel(RequestedAck()/3); // mega-kludge!
    mset.AllowPublicDLs(OMAllowPublicDLs(flags));
    mset.AllowAltRecips(OMAllowAltRecips(flags));
    mset.AllowConversions(OMAllowConversions(flags));
    mset.AllowNDNs(OMAllowNDNs(msgflags));
    mset.ReturnMsg(OMMustReturnMsg(flags));
    mset.SetDate(DeferredDate());
}

int Message::SetSettings(MessageSettings &mset)
{
    flags = OMFlags(mset.DoAllowPublicDLs(), mset.DoAllowAltRecips(),
    			mset.DoAllowConversions(), mset.MustReturnMsg());
    msgflags = OMMessageFlags(mset.GetPriority(), mset.GetImportance(),
    				mset.GetSensitivity(), mset.GetAckLevel(),
				mset.DoAllowNDNs());
    deferred_date = mset.GetDate();
    return OMModifyMessage(ref, item_num, flags, msgflags, deferred_date);
}

char *Reply::Type() const
{
    return "Reply";
}

Reply::~Reply()
{
    DestructTrace("Reply");
}

char *NonDelivery::Type() const
{
    return "NonDelivery";
}

NonDelivery::~NonDelivery()
{
    DestructTrace("NonDelivery");
}

//----------------------------------------------------------------------

char *ViewedMessage::Type() const
{
    return "Message";
}

ViewedMessage::~ViewedMessage()
{
    DestructTrace("ViewedMessage");
}

//----------------------------------------------------------------------

char *Text::Type() const
{
    return "Text";
}

Text::~Text()
{
    DestructTrace("Text");
}

//----------------------------------------------------------------------

char *Other::Type() const
{
    static char buf[12];
    switch (typ)
    {
    case 0: return "Binary";
    case -102: return "Package";
    default:
    	//sprintf(buf, "TYPE %d", typ);
    	sprintf(buf, "#%d", typ);
    	return buf;
    }
}

Other::~Other()
{
    DestructTrace("Other");
}

//----------------------------------------------------------------------

void Message::Format(char * &buf)
{
    char tmp[100];
    sprintf(tmp, "%c%-4d %-20.20s %-20.20s %-12.12s",
	   IsUnread()?'O' : ' ',
	   item_num, name, creator, Type());
    buf = new char[strlen(tmp)+1];
    assert(buf);
    strcpy(buf, tmp);
}

//----------------------------------------------------------------------

MessageStore::MessageStore(char *server_in, char *user_in, char *password)
    : pollticks(60), omuser(0), pushfolder(0), namefd(0), filetypecnt(0),
	filetypes(0), folder(0), ufolder(0), dlistfname(0),
	numnicknames(0), dlistfd(-1), nicknames(0)
{
    if (server_in)
    {
	server = new char [strlen(server_in)+1];
    	if (server) strcpy(server, server_in);
    }
    else server = 0;
    user = new User(user_in, password);
    ConstructTrace("MessageStore");
}

int MessageStore::Connect()
{
    OMAddress addr;
    addr.AssignExternal(UserName());
    char *int_name = addr.GetInternal();
    char uname[1024];
    if (OMConnect(server, int_name, user->Password(), list_ref, uname) != 0)
    {
	delete [] int_name;
	return -1;
    }
    delete [] int_name;
    addr.AssignInternal(uname);
    omuser = addr.GetExternal();
    folder = ufolder = new UserFolder(list_ref, UserName());
    assert(folder);
    if (OpenNicknames() == 0)
        (void)ReadNicknames();
    GetFileTypes();
    Trace("Connected");
    return 0;
}

int MessageStore::OpenFolder(long item_num, int pgsz)
{
    FolderItem *pf = folder;
    if (folder->IsAtomic())
	return -1;
    folder = ((CompositeItem*)folder)->Child(item_num);
    //printf("----------------------------\n%s %s\n\n",FolderType(),FolderName());
    if (folder==0 || folder->Open(pgsz) != 0) 
    {
	delete folder;
	folder = pf;
	return -1;
    }
    return 0;
}

int MessageStore::OpenDListForAcks(long item_num, int pgsz)
{
    if (folder->IsAtomic())
	return -1;
    FolderItem *pf = folder;
    folder = ((CompositeItem*)folder)->Child(item_num);
    //printf("----------------------------\n%s %s\n\n",FolderType(),FolderName());
    assert(folder->IsDistList());
    if (folder==0 || ((DistList*)folder)->OpenForAcks(pgsz) != 0) 
    {
	//delete folder; // should this be a delete??
	folder = pf;
	return -1;
    }
    return 0;
}

void MessageStore::CloseFolder()
{
    assert(folder);
    assert(folder != ufolder);
    folder->Close();
    folder = folder->Parent();
}

// Unfortunately, as the message store has a method named Folder
// I can't do a "new Folder" from within it without the compiler
// barfing. Hence this kludge:

static Folder *NewSubfolder(FolderItem *parent, int fnum)
{
    Folder *rtn = new Folder(parent, fnum, "", "", 0l, 0l);
    assert(rtn);
    return rtn;
}

int MessageStore::OpenSubfolder(int fnum, int pgsz)
{
    assert(pushfolder == 0);
    pushfolder = folder;
    folder = NewSubfolder(ufolder, fnum);
    if (folder==0 || folder->Open(pgsz) != 0) 
    {
	delete folder;
	folder = pushfolder;
	pushfolder = 0;
	return -1;
    }
    return 0;
}

void MessageStore::CloseSubfolder()
{
    if (pushfolder)
    {
        assert(folder);
	while (folder->Parent() != ufolder)
	{
	    CloseFolder();
	}
        folder->Close();
        folder = pushfolder;
        pushfolder = 0;
    }
}

int MessageStore::IsSubfolder(int fnum)
{
    return (folder->Parent()== ufolder && folder->ItemNum()==fnum);
}

int MessageStore::NewMail()
{
    static time_t lasttime = 0;
    if ((time(0) - lasttime) > pollticks)
    {
	lasttime = time(0);
        return OMNewMail();
    }
    return 0;
}

int MessageStore::CreateMessage(int listref, long itemnum, char *subject,
		MessageSettings &mset, int type)
{
    long flags = OMNewMessageFlags(mset.GetPriority(),
			mset.GetImportance(),
			mset.GetSensitivity(),
			mset.DoAllowPublicDLs(),
			mset.DoAllowAltRecips(),
			mset.DoAllowConversions(),
			mset.DoAllowNDNs(),
			mset.MustReturnMsg());
    if (type == 0)
        return OMCreateMessage(subject,flags,mset.GetAckLevel(),mset.GetDate());
    else if (type == 1)
        return OMCreateReply(listref, itemnum, subject,flags,
			mset.GetAckLevel(),mset.GetDate());
    else
        return OMCreateForward(listref, itemnum, subject,flags,
			mset.GetAckLevel(),mset.GetDate());
}

char *MessageStore::FormatName(char *int_name)
{
    static char rtn[82];
    OMAddress addr;
    addr.AssignInternal(int_name);
    char *cvt = addr.GetExternal();
    strncpy(rtn, cvt, 80);
    delete [] cvt;
    rtn[80] = '\0';
    return rtn;
}

int MessageStore::CheckName(char *name_in, int maxmatches, char ** &names_out,
				long flags)
{
    OMAddress addr;
    addr.AssignExternal(name_in);
    name_in = addr.GetInternal();
    int rtn = OMCheckName(name_in, maxmatches, names_out, flags, namefd);
    delete [] name_in;
    return rtn;
}

int MessageStore::AddRecipient(OMAddress *addr, int typ)
{
    char *name = addr->GetInternal();
    int rtn = OMAddRecipient(name, typ);
    delete [] name;
    return rtn;
}

void MessageStore::Disconnect()
{
    assert(folder);
    while (folder != ufolder) // back out
        CloseFolder();
    delete ufolder;
    ufolder = 0;
    delete [] omuser;
    omuser = 0;
    CloseNicknames();
    (void)OMDisconnect();
    Trace("Disconnected");
}

MessageStore::~MessageStore()
{
    delete user;
    delete [] server;
    delete [] filetypes;
    DestructTrace("MessageStore");
}

//--------------- Dist List creation

int MessageStore::CreateDList(int from, int to, int cc, int bcc)
{
    assert(dlistfname == 0);
    char *tn = tmpnam(0);
    dlistfname = new char[strlen(tn)+1];
    if (dlistfname == 0) return -1;
    strcpy(dlistfname, tn);
    dlistfd = OMCreateDListTF(dlistfname, from, to, cc, bcc);
    if (dlistfd < 0)
    {
	unlink(dlistfname);
	delete [] dlistfname;
	dlistfname = 0;
	return -1;
    }
    return 0;
}

int MessageStore::AddAddress(OMAddress *addr, int typ)
{
    assert(dlistfname);
    char *ia = addr->GetInternal();
    int rtn = OMAddAddress(dlistfd, typ, ia);
    delete [] ia;
    return rtn;
}

int MessageStore::SaveDList(char *name, int folderref)
{
    assert(dlistfname);
    int rtn = OMSaveDList(dlistfd, dlistfname, name, folderref);
    unlink(dlistfname);
    delete [] dlistfname;
    dlistfname = 0;
    return rtn;
}

int MessageStore::ReplaceDList(char *name, int ref, long itemnum)
{
    assert(dlistfname);
    int rtn = OMReplaceDList(dlistfd, dlistfname, name, ref, itemnum);
    unlink(dlistfname);
    delete [] dlistfname;
    dlistfname = 0;
    return rtn;
}

int MessageStore::OpenDListTF(FolderItem *fi)
{
    assert(dlistfname == 0);
    assert(fi->IsDistList());
    char *tn = tmpnam(0);
    dlistfname = new char[strlen(tn)+1];
    if (dlistfname == 0) return -1;
    strcpy(dlistfname, tn);
    ((DistList*)fi)->Export(dlistfname);
    if ((dlistfd = OMOpenDListTF(dlistfname)) < 0)
    {
	unlink(dlistfname);
    	delete [] dlistfname;
	dlistfname = 0;
	return -1;
    }
    return 0;
}

int MessageStore::OpenDListTF(char *fname)
{
    assert(dlistfname == 0);
    dlistfname = new char[strlen(fname)+1];
    if (dlistfname == 0) return -1;
    strcpy(dlistfname, fname);
    if ((dlistfd = OMOpenDListTF(fname)) < 0)
    {
	unlink(fname);
	delete [] dlistfname;
	dlistfname = 0;
	return -1;
    }
    return 0;
}

OMAddress *MessageStore::ReadDListRecord(int &typ, int &acklvl)
{
    assert(dlistfname);
    char buf[1024];
    int rtn = OMReadDListRecord(dlistfd, buf, typ, acklvl);
    if (rtn == 0)
    {
	OMAddress *addr = new OMAddress;
	if (addr) addr->AssignInternal(buf);
	return addr;
    }
    return 0;
}

char *MessageStore::CloseDListTF(int remove)
{
    char *rtn = dlistfname;
    assert(dlistfname);
    if (OMCloseDListTF(dlistfd) != 0) rtn = 0;
    if (remove) unlink(dlistfname);
    delete [] dlistfname;
    dlistfname = 0;
    return rtn;
}

//------------------------------------------------------------
// Nickname directory handling

int MessageStore::OpenNicknames()
{
    nnfd = OMOpenDirectory("nicknames");
    if (nnfd < 0)
    {
        if (OMCreateDirectory("nicknames") == 0)
    	    nnfd = OMOpenDirectory("nicknames");
    }
    numnicknames = 0;
    return (nnfd < 0) ? -1 : 0;
}

int MessageStore::ReadNicknames()
{
    assert(nicknames == 0);
    nicknames = new OMAddress*[MAXNICKNAMES];
    assert(nicknames);
    for (int n = 0; n < MAXNICKNAMES; n++)
	nicknames[n] = 0;
    char *rtn = OMSearchDirectory(numnicknames, nnfd,
			"203\035\036@X400-ATTR@", 0, 2);
    if (numnicknames <= 0) return 0;
    if (numnicknames > MAXNICKNAMES) numnicknames = MAXNICKNAMES;
    if (numnicknames == 1)
    {
	nicknames[0] = new OMAddress;
	assert(nicknames[0]);
	nicknames[0]->AssignDirEntry(rtn);
    }
    else // numnicknames>1
    {
	// read the file whose name is in rtn.
	FILE *fp = fopen(rtn, "r");
	if (fp)
	{
	    int n = 0;
	    while (!feof(fp))
	    {
		char buf[1096];
		if (fgets(buf, 1096, fp) == 0) break;
		buf[strlen(buf)-1] = '\0';
		nicknames[n] = new OMAddress;
		assert(nicknames[n]);
		nicknames[n++]->AssignDirEntry(buf);
	    }
	    fclose(fp);
	    numnicknames = n;
	}
	else numnicknames = 0;
	unlink(rtn);
    }
    return numnicknames;
}

int MessageStore::HasNickname(char *name)
{
    int cnt;
    char buf[32];
    strcpy(buf, "203\035");
    strcat(buf, name);
    (void)OMSearchDirectory(cnt, nnfd, "203\035\036@X400-ATTR@", buf);
    return cnt;
}

int MessageStore::AddNickname(OMAddress *nickname)
{
    char *entry = nickname->GetDirEntry();
    int rtn = OMAddDirEntry(nnfd, entry);
    delete [] entry;
    return rtn;
}

int MessageStore::ChangeNickname(char *oldname, OMAddress *newaddr)
{
    for (int n = 0; n < numnicknames; n++)
    {
	if (nicknames[n] && strcmp(nicknames[n]->GetField(Nickname),oldname)==0)
	{
    	    char *filter = nicknames[n]->GetDirEntry();
    	    char *modifier = newaddr->GetDirEntry();
    	    int rtn = OMModifyDirEntry(nnfd, modifier, 0, filter);
    	    delete [] modifier;
    	    delete [] filter;
    	    return rtn;
	}
    }
    return -1;
}

int MessageStore::DeleteNickname(char *name)
{
    char buf[64];
    strcpy(buf, "203\035");
    strcat(buf, name);
    return OMDeleteDirEntry(nnfd, "203\035\036@X400-ATTR@", buf);
}

int MessageStore::CloseNicknames()
{
    for (int n = 0; n < MAXNICKNAMES; n++)
	delete nicknames[n];
    delete [] nicknames;
    return OMCloseDirectory(nnfd);
}

// Public access to these

OMAddress *MessageStore::GetNickname(int num)
{
    if (nicknames == 0 || num < 0 || num>= numnicknames)
	return 0;
    return nicknames[num];
}

int MessageStore::SetNickname(int num, OMAddress *newval)
{
    int rtn = 0;
    if (nicknames == 0 || num < 0 || num >= MAXNICKNAMES)
	rtn = -1;
    else if (nicknames[num] == newval)
	rtn = 0;
    else if (nicknames[num]==0 || newval==0 || *nicknames[num] != *newval)
    {
	if (nicknames[num]==0 || nicknames[num]->IsEmpty()) // new addition
	{
	    if (newval && !newval->IsEmpty())
		rtn = AddNickname(newval);
	}
	else
	{
	    char *oldname = nicknames[num]->GetField(Nickname);
	    if (newval == 0 || newval->IsEmpty()) // this is a deletion
	    	rtn = DeleteNickname(oldname);
	    else // this is a modification
	    	rtn = ChangeNickname(oldname, newval);
	}
        delete nicknames[num];
        nicknames[num] = newval;
        if (num >= numnicknames) numnicknames = num+1;
	newval = 0; // prevent delete below
    }
    delete newval;
    return rtn;
}

int MessageStore::NumNicknames()
{
    return numnicknames;
}

// look up a nickname and return value, plus (optionally) the index.
// if there is no nickname the index of a free slot is returned.

char *MessageStore::LookupNickname(char *name, int *num)
{
    char nm[40], nn[40];
    int freeslot = numnicknames;
    strcpy(nm, name);
    strupr(nm);
    for (int n = 0; n < numnicknames; n++)
    {
	if (nicknames[n]==0)
	    freeslot = n;
	else
	{
	    strcpy(nn, nicknames[n]->GetField(Nickname));
	    strupr(nn);
	    if (strcmp(nn,nm)==0)
	    {
	        if (num) *num = n;
	        return nicknames[n]->GetExternal();
	    }
	}
    }
    if (num) *num = freeslot;
    return 0;
}

void MessageStore::ClearNicknames(int from)
{
    while (from < numnicknames)
	(void)SetNickname(from++, 0); // THIS WON'T WORK I DON'T THINK!!
}

FolderItem *MessageStore::FirstChild(CompositeItem *parent)
{
    parent->Open(1);
    if (parent->FirstPage() == 1)
	return parent->items[0];
    parent->Close();
    return 0;
}

FolderItem *MessageStore::NextChild(CompositeItem *parent)
{
    if (parent->NextPage() == 1)
	return parent->items[0];
    parent->Close();
    return 0;
}

FolderItem *MessageStore::FindChild(CompositeItem *parent, char *name)
{
    FolderItem *fi = FirstChild(parent);
    while (fi)
    {
	if (strcmp(fi->Name(), name) == 0)
	{
	    parent->Close();
	    return fi;
	}
	fi = NextChild(parent);
    }
    return 0;
}

void FileTypeRecord::SetName(char *name_in)
{
    delete [] name;
    if (name_in)
    {
        name = new char[strlen(name_in)+1];
        if (name) strcpy(name, name_in);
    }
    else name = 0;
}

void FileTypeRecord::SetEditor(char *app_in)
{
    delete [] linkapp;
    if (app_in)
    {
    	linkapp = new char[strlen(app_in)+1];
    	if (linkapp) strcpy(linkapp, app_in);
    }
    else linkapp = 0;
}

FileTypeRecord::FileTypeRecord()
    : name(0), linkapp(0), typid(0)
{
}

FileTypeRecord::FileTypeRecord(char *name_in, long typ_in, char *app_in)
    : name(0), linkapp(0), typid(typ_in)
{
    SetName(name_in);
    SetEditor(app_in);
}

FileTypeRecord::~FileTypeRecord()
{
    delete [] name;
    delete [] linkapp;
}

#define MAXFILETYPES	800

int MessageStore::SaveFileTypes()
{
    return 0;
}

void MessageStore::GetFileTypes()
{
    int mustunlink = 0;
    delete [] filetypes;
    filetypes = new FileTypeRecord[MAXFILETYPES];
    assert(filetypes);
    filetypecnt = 0;
    char *tn = config->Get("filetypes"); // are they local?
    if (tn == 0 || tn[0] == '\0')
    {
        tn = tmpnam(0); 	// no; fetch from server into temp file
        OMGetFileTypes(tn);
	mustunlink = 1;
    }
    FILE *fp = fopen(tn, "r");
    if (fp)
    {
	while (!feof(fp))
        {
	    char buf[80], *p;
	    if (fgets(buf, 79, fp) == 0) break;
	    p = buf;
	    while (isspace(*p)) p++;
	    if (!isdigit(*p)) continue;
	    char *d = p;
	    while (isdigit(*d)) d++;
	    while (isspace(*d)) d++;
	    d[strlen(d)-1] = '\0'; // strip newline
	    if (d[0] == '\0') continue;
	    filetypes[filetypecnt].SetName(d);
	    filetypes[filetypecnt++].SetType(atoi(p));
	}
        fclose(fp);
    }
    Debug1("Read %d file type entries", filetypecnt);
    if (filetypecnt>0)
    {
        Debug2("Entry 0 Name %s Type %d", filetypes[0].GetName(),
		filetypes[0].GetType());
        Debug3("Entry %d Name %s Type %d", filetypecnt-1, 
		filetypes[filetypecnt-1].GetName(),
		filetypes[filetypecnt-1].GetType());
    }
    if (mustunlink) unlink(tn);
    tn = config->Get("links");
    if (tn && tn[0] && filetypecnt>0)
    {
        FILE *fp = fopen(tn, "r");
        if (fp)
        {
	    while (!feof(fp))
            {
	        char buf[80], *p;
	        if (fgets(buf, 79, fp) == 0) break;
	        p = buf;
	        while (isspace(*p)) p++;
	        if (!isdigit(*p)) continue;
	        char *d = p;
	        while (isdigit(*d)) d++;
	        while (isspace(*d)) d++;
	        d[strlen(d)-1] = '\0'; // strip newline
	        int typ = atoi(p);
	        for (int t = 0; t < filetypecnt; t++)
		    if (filetypes[t].GetType() == typ)
		    {
		        filetypes[t].SetEditor(d);
		        break;
		    }
	    }
	    fclose(fp);
	}
    }
}

//------------------------------------------------------------------------

