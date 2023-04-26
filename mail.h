// mail.h - header file for C++ class wrapper around OpenMail API
// Written by Graham Wheeler, June 1995.
// (c) 1995, Open Mind Solutions
// All Rights Reserved
//

#ifndef MAIL_H
#define MAIL_H

const MAX_MSG_LINES = 16;	// how many messages at a time we deal with

// Folder Item type codes for most important types

enum ItemType
{
	BinaryType = 0,		DistListType = 1166, 
	TextType = 1167,	NDNDetailType = 1527,
	MessageType = -100,	FolderType = -101,
	PackageType = -102,	BBoardType = -103,
	ReplyType = -300,	NDNMessageType = -400
};

// The OMAddress class allows conversion between internal OpenMail
// addresses and printable forms of such addresses, as well as given
// access to get/set individual address fields.

enum AddrFields
{
	Surname, Forename, Initials, Generation,
	OrgUnit1, OrgUnit2, OrgUnit3, OrgUnit4,
	OrgName, Country, ADMD, PRMD,
	X121Addr, UAUniqID, TelematicID, 
	DDATyp1, DDA1, DDATyp2, DDA2,
	DDATyp3, DDA3, DDATyp4, DDA4,
	Spare, Nickname
};

class OMAddress
{
    char *flds[25];
public:
    OMAddress(OMAddress *addr = 0);
    void Set(OMAddress *addr);
    void AppendField(int fnum, char *buf, char *sep = 0);
    void SetAbbrevField(int fnum, char *val);
    void SetField(int fnum, char *val);
    char *GetField(int fnum);
    char *GetExternal();
    char *GetInternal();
    char *GetDirEntry();
    void AssignExternal(char *extaddr);
    void AssignInternal(char *intaddr);
    void AssignDirEntry(char *diraddr);
    int IsEmpty();
    ~OMAddress();
    int operator==(OMAddress &rhs);
    int operator!=(OMAddress &rhs);
};

// This hides the mapping between message settings and OpenMail message flags.

class MessageSettings
{
    time_t tcks;
    int imp, pri, sens, ack;
    int allow_pubdls, allow_altrecip, allow_convert,
	allow_ndn, returnmsg;
public:
    void SetToDefaults();
    MessageSettings()
    {
	SetToDefaults();
    }
    void MessageSettings::SetImportance(int val_in)
    {
	imp = val_in;
    }
    int GetImportance()
    {
	return imp;
    }
    void SetPriority(int val_in)
    {
	pri = val_in;
    }
    int GetPriority()
    {
	return pri;
    }
    void SetSensitivity(int val_in)
    {
	sens = val_in;
    }
    int GetSensitivity()
    {
	return sens;
    }
    void SetAckLevel(int val_in)
    {
	ack = val_in;
    }
    int GetAckLevel()
    {
	return ack;
    }
    void SetDate(time_t tm_in)
    {
	tcks = tm_in;
    }
    time_t GetDate()
    {
	return tcks;
    }
    void AllowPublicDLs(int val_in)
    {
	allow_pubdls = val_in;
    }
    int DoAllowPublicDLs()
    {
	return allow_pubdls;
    }
    void AllowAltRecips(int val_in)
    {
	allow_altrecip = val_in;
    }
    int DoAllowAltRecips()
    {
	return allow_altrecip;
    }
    void AllowConversions(int val_in)
    {
	allow_convert = val_in;
    }
    int DoAllowConversions()
    {
	return allow_convert;
    }
    void AllowNDNs(int val_in)
    {
	allow_ndn = val_in;
    }
    int DoAllowNDNs()
    {
	return allow_ndn;
    }
    void ReturnMsg(int val_in)
    {
	returnmsg = val_in;
    }
    int MustReturnMsg()
    {
	return returnmsg;
    }
    ~MessageSettings()
    {
    }
};

//----------------------------------------------------------------
// This class isn't really needed. It supports the change password
// functionality, but this could just as well be done from the
// message store class, it is so basic. Also, keeping a copy of the
// password around could be construed a security risk. Anyway, I 
// thought it may be useful if we want to associate other attributes
// with a user. The stored password can probably be removed, and the
// routine to change password modified to take the current and new
// passwords as parameters.

class User
{
    char	*name;
    char	*password;
 public:
    User(char *name_in, char *password_in);
    char *Name() const;
    void  Name(char *name_in);
    char *Password() const;
    int          SetPassword(char *passwd_in); // return 0 if OK
    int   CheckPassword(char *passwd_in); // return 0 if OK
    ~User();
};

//----------------------------------------------------------------
// Base class for all objects in the message store

class FolderItem
{
protected:
    FolderItem	*parent;	// folder containing this item
    int		item_num;	// parent's index of this item 
    int		ref;		// this item's liet reference
    int		folder_size;	// how many children we have
    int		pagesize;	// how many lines we show
    long	first_item;	// current first child in list
    char	*name;		// the name of the item
    char	*creator;	// who played god here
    long	create_date;	// time in secs since 1-1-1970
    long	flags;		// OpenMail flags
    int		typ;		// OpenMail type

    void Init(FolderItem *parent_in, int typ_in, int item_num_in, char *name_in,
	      char *creator_in, long create_date_in, long flags_in);
public:
    FolderItem(FolderItem *parent_in, int typ_in, int item_num_in,
		      char *name_in, char *creator_in, long create_date_in,
			long flags_in);
    virtual char *Type() const = 0;
    virtual int IsAtomic() const;
    virtual int IsMessage() const;
    virtual int IsFolder() const;
    virtual int IsBBoard() const;
    virtual int IsDistList() const;
    int IsComposite() const;
    char *Name() const;
    char *Creator() const;
    long CreateDate();
    const int First() const;
    const int Size() const;
    const int Ref() const;
    const int ItemNum() const;
    FolderItem *Parent() const;
    int IsObsolete();
    int IsUnread();
    void MarkAsRead();
    int IsError();
    int HasDistListError();
    int TypeID() const;

    virtual int Open(int pgsz = MAX_MSG_LINES) = 0;
    virtual int Close() = 0;
    virtual int FirstPage() = 0;
    virtual int NextPage() = 0;
    virtual int PrevPage() = 0;

    virtual void Format(char * &buf);
    virtual ~FolderItem();
};

//---------------------------------------------------------------------
// Base class for composite messagestore objects, such as folders and
// messages.

class CompositeItem : public FolderItem
{
protected:
    FolderItem	*(items[MAX_MSG_LINES]); // current (sub)list of children

    void	 AddItem(FolderItem *itm);
    int		 GetItems();
    void	 FreeItems();
    friend class MessageStore;
public:
    CompositeItem(FolderItem *parent_in, int typ_in, int item_num_in, char *name_in,
		  char *creator_in, long create_date_in, long flags_in);
    
    virtual char *Type() const = 0;
    virtual int IsMessage() const;
    virtual int IsFolder() const;
    virtual int IsBBoard() const;
    FolderItem *Child(int item_num) const;

    virtual int Open(int pgsz = MAX_MSG_LINES);
    virtual int Close();
    virtual int FirstPage();
    virtual int NextPage();
    virtual int PrevPage();

    virtual ~CompositeItem();
};

//---------------------------------------------------------------------
// Base class for basic messagestore items, such as distributions lists
// and file attachments

class AtomicItem : public FolderItem
{
    char * (text[MAX_MSG_LINES]); // current (sub)list of `children'
protected:
    virtual int GetItems();
    virtual void FreeItems();
public:
    AtomicItem(FolderItem *parent_in, int typ_in, int item_num_in, char *name_in,
	       char *creator_in, long create_date_in, long flags_in);
    
    const char * const Text(int ln_num) const;
    int Export(char *fname);
    int Export(char *fname, int etyp);
    virtual char *Type() const = 0;
    virtual int IsAtomic() const;
    virtual int IsDistList() const;

    virtual int Open(int pgsz = MAX_MSG_LINES);
    virtual int Close();
    virtual int FirstPage();
    virtual int NextPage();
    virtual int PrevPage();

    virtual ~AtomicItem();
};

//---------------------------------------------------------------------
// Base class for message store folders

class Folder: public CompositeItem
{
public:
    Folder(FolderItem *parent_in, int item_num_in, char *name_in,
	   char *creator_in, long create_date_in, long flags_in);
    virtual int Close();
    virtual char *Type() const;
    virtual int IsFolder() const;
    virtual ~Folder();
};

//---------------------------------------------------------------------
// Base class for the user's top-level folder

class UserFolder: public Folder 
{
public:
    UserFolder(int user_ref_in, char *user_in);
    virtual int Close();
    virtual char *Type() const;
    virtual ~UserFolder();
};

//---------------------------------------------------------------------
// Base class for bulletin board areas

class BBArea: public CompositeItem
{
public:
    BBArea(FolderItem *parent_in, int item_num_in, char *name_in,
	   char *creator_in, long create_date_in, long flags_in);
    virtual char *Type() const;
    virtual int IsBBoard() const;
    virtual ~BBArea();
};

//---------------------------------------------------------------------
// Base class for messages

class Message: public CompositeItem
{
    long	msgflags;	// openmail message flags
    long	receipt_date;	// time in secs since 1-1-1970
    long	deferred_date;	// time in secs since 1-1-1970
public:
    Message(FolderItem *parent_in, int item_num_in, char *subject_in,
	    char *creator_in, long create_date_in, long flags_in,
		long msgflags_in, long receipt_date_in, long deferred_date_in);
    virtual void Format(char * &buf);
    virtual char *Type() const;
    virtual int IsMessage() const;
    int Priority();	// 0 - none, 1 - high, 2 - urgent
    int Sensitivity();	// 0 - none, 1 - personal, 2 - private, 3 - co.
    int RequestedAck();	// 0 - none, 4 - deliver, 7 - read, 9 - reply
    int SentAck();	// 0 - none, 4 - deliver, 7 - read, 9 - reply
    int Importance();	// 0 - normal, 1 - low, 2 - high
    int RecipCategory();	// 0 - from, 1 - to, 2 - cc, 3 - bcc
    long ReceiveDate();
    long DeferredDate();
    void GetSettings(MessageSettings &mset);
    int SetSettings(MessageSettings &mset);
    virtual ~Message();

    friend class ViewedMessage;
};

//-------------------------------------------------------------------
// Some special types of messages

class Reply : public Message
{
public:
    Reply(FolderItem *parent_in, int item_num_in, char *subject_in,
	    char *creator_in, long create_date_in, long flags_in,
		long msgflags_in, long receipt_date_in,
		long deferred_date_in);
    virtual char *Type() const;
    virtual ~Reply();
};

class NonDelivery : public Message
{
public:
    NonDelivery(FolderItem *parent_in, int item_num_in, char *subject_in,
	    char *creator_in, long create_date_in, long flags_in,
		long msgflags_in, long receipt_date_in,
		long deferred_date_in);
    virtual char *Type() const;
    virtual ~NonDelivery();
};

//---------------------------------------------------------------------
// When a message is viewed, the server turns it into a textual form,
// which is examined in the same was as basic items. This class represents
// a message being viewed.

class ViewedMessage : public AtomicItem
{
public:
    ViewedMessage(Message *msg);
    virtual char *Type() const;
    virtual ~ViewedMessage();
};

//---------------------------------------------------------------------
// Base class for distribution lists

class DistList: public AtomicItem
{
public:
    DistList(FolderItem *parent_in, int item_num_in,
	     char *name_in, char *creator_in, long create_date_in,
		long flags_in);
    virtual char *Type() const;
    virtual int IsDistList() const;
    int OpenForAcks(int pgsz);
    virtual ~DistList();
};

//---------------------------------------------------------------------
// Base class for textual basic items

class Text : public AtomicItem
{
public:
    Text(FolderItem *parent_in, int item_num_in,
		char *name_in, char *creator_in, long create_date_in,
		long flags_in);
    virtual char *Type() const;
    virtual ~Text();
};

//---------------------------------------------------------------------
// Base class for basic items other than text and distribution lists

class Other : public AtomicItem
{
public:
    Other(FolderItem *parent_in, int typ_in, int item_num_in,
		       char *name_in, char *creator_in, long create_date_in,
			long flags_in);
    virtual char *Type() const;
    virtual ~Other();
};

//---------------------------------------------------------------------
// Class used to hold associations between OpenMail file type IDs, 
// type names, and (optionally) associated editor applications

class FileTypeRecord
{
    char *name;		// Descriptive name
    long  typid;	// Openmail type 
    char *linkapp;	// linked (editor) application
public:
    void SetName(char *name_in);
    void SetEditor(char *app_in);
    void SetType(long typ_in)
    {
	typid = typ_in;
    }
    char *GetName() 
    {
	return name;
    }
    char *GetEditor() 
    {
	return linkapp;
    }
    int GetType()
    {
 	return typid;
    }
    FileTypeRecord();
    FileTypeRecord(char *name_in, long typ_in, char *app_in = 0);
    ~FileTypeRecord();
};

//---------------------------------------------------------------------
// The main message store class.

class MessageStore
{
    const MAXNICKNAMES = 128;
    int		 list_ref;	// user's list reference
    FolderItem	*folder;	// current folder
    UserFolder	*ufolder;	// top-level folder
    FolderItem	*pushfolder;	// last open folder before file tray open
    User	*user;		// user name
    char	*omuser;	// full openmail user name
    char	*server;	// name of OpenMail server
    int		 pollticks;	// seconds between new mail polling

    // this is the handle of the name search directory
    int		 namefd;
    // next two are used when building distlists
    int		 dlistfd;
    char	 *dlistfname;
    // these are used for nickname lists
    int		 nnfd;		// file descriptor of nickname file
    OMAddress	 **nicknames;	// current set of nicknames
    int		 numnicknames;  // current number of nicknames

    int		 OpenNicknames();
    int		 ReadNicknames();
    int		 HasNickname(char *name);
    int		 AddNickname(OMAddress *nickname);
    int		 ChangeNickname(char *oldname, OMAddress *newaddr);
    int		 DeleteNickname(char *name);
    int		 CloseNicknames();

    // Used for temporarily switching to a different folder, for example
    // when filing an item.

    void	 PushFolder(FolderItem *f);
    void	 PopFolder(FolderItem * &f);
public:
    FileTypeRecord *filetypes;
    int		    filetypecnt;
    MessageStore(char *server, char *user, char *password);
   ~MessageStore();

    int	 Connect();
    void Disconnect();
    int	 OpenFolder(long f_item_num, int pgsz = MAX_MSG_LINES);
    int	 OpenDListForAcks(long f_item_num, int pgsz = MAX_MSG_LINES);
    void CloseFolder();
    int	 OpenSubfolder(int fnum, int pgsz = MAX_MSG_LINES);
    void CloseSubfolder();
    int	 IsSubfolder(int fnum);
    int	 NewMail();
    int	 NextPage();
    int	 PrevPage();
    int	 FirstPage();
    int	 Save(long item_num, char *fname);
    int	 Save(int list_ref, long item_num, char *fname);
    int	 SaveAsText(long item_num, char *fname);
    int	 SaveAsText(int list_ref, long item_num, char *fname);
    int	 ReplaceText(long item_num, char *fname);
    int	 Print(int listref, long item_num, char *cmd = "lp");
    int	 Print(long item_num, char *cmd = "lp");
    int	 Delete(int listref, long item_num);
    int	 ChangePassword(char *password);
    FolderItem *Folder();
    char *UserName();
    char *Password();
    char *OMUserName();
    int   IsLoginUser(OMAddress *addr);
    void  SetPollInterval(int pollticks_in);
    int   MoveItem(int srcref, long srcitem, int destref, long destitem);
    int   CopyItem(int srcref, long srcitem, int destref, long destitem);
    int	  AttachItem(int srcref, int destref);
    int	  HoldMessage(int msgref);
    int	  ChangeSubject(int listref, long itemnum, char *subject);
    int   CreateArea(int parentref, char *name);
    int   CreateFolder(int parentref, char *name);
    int   RenameFolder(int listref, long itemnum, char *name);
    int   ReorderFolder(int listref, long item, long after_item);
    char *FormatName(char *int_name);
    int   	 CheckName(char *name_in, int maxmatches, char ** &names_out,
			long flags = 5l); // kludgish: ALTSFILE|IN_DIR

    // Configuration handling 

    int	 SetUserConfig(int wbclear, int tabsize, int readdl,
				int printdl, int xtra, int savemail);
    int	 GetUserConfig(int &wbclear, int &tabsize, int &readdl,
				int &printdl, int &xtra, int &savemail);

    // Auto-action handling

    int	 GetAutoActions(int &afa, int &ara, int &ic, int &dfm,
				 int &koc, int &fprm, int &fpsm, int &fccm,
				 int &rtam, int &rtum, int &rtrra);
    int	 SetAutoActions(int afa, int ara, int ic, int dfm, int koc,
				 int fprm, int fpsm, int fccm, char* dlist,
				 char* comments, int rtam, int rtum, int rtrra,
				 char* reply);
    int	 GetAutoForwardDList(char *fname);
    int	 GetAutoForwardComment(char *fname);
    int	 GetAutoReply(char *fname);

    // Message creation
    int   	 AddRecipient(OMAddress *addr, int typ);
    int   	 CreateMessage(int listref, long itemnum, char *subject,
				MessageSettings &mset, int type);
    int   AttachFile(int msgref, char *fname, char *descrp = 0,
			int ftype=1167);
    int   MailMessage(MessageSettings &mset, int listref,
			long itemnum = 0l);

    // Distribution list creation
    int		 CreateDList(int from, int to, int cc, int bcc);
    int   	 AddAddress(OMAddress *addr, int typ);
    int		 SaveDList(char *name, int folderref);
    int		 ReplaceDList(char *name, int ref, long itemnum);

    // Distribution list reading
    int		 OpenDListTF(FolderItem *fi);
    int		 OpenDListTF(char *fname);
    OMAddress 	*ReadDListRecord(int &typ, int &acklvl);
    char	*CloseDListTF(int remove);

    // Nickname reading/creating/modifying
    OMAddress   *GetNickname(int num);
    int		 SetNickname(int num, OMAddress *newval);
    int		 NumNicknames();
    char	*LookupNickname(char *name, int *idx = 0);
    void	 ClearNicknames(int from);

    // Changing name directory
    int	 SetNameDirectory(char *name, char *passwd, int ispublic);
    int	 GetDirectories(int &cnt, char **items, int *types, int first = 0);

    // File Type Handling
    int 	 SaveFileTypes();
    void	 GetFileTypes();

    FolderItem	 *FirstChild(CompositeItem *parent);
    FolderItem	 *NextChild(CompositeItem *parent);
    FolderItem	 *FindChild(CompositeItem *parent, char *name);
};

#ifndef INLINE
#  ifdef NO_INLINES
#    define INLINE
#  else
#    define INLINE inline
#  endif
#endif

#ifndef NO_INLINES
# include "mail.inl" // include inline routines
#endif

#endif

