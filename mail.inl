#ifndef _MAIL_INL
#define _MAIL_INL
//-------------------------------------------------------------------
// Inline routines

#include <assert.h>
#include "debug.h"
#include "omutil.h"

INLINE User::User(char *name_in, char *password_in)
{
    ConstructTrace("User");
    assert(name_in!=0 && password_in!=0);
    name = new char[strlen(name_in)+1];
    password = new char[strlen(password_in)+1];
    assert(name!=0 && password!=0);
    strcpy(name, name_in);
    strcpy(password, password_in);
}

INLINE char *User::Name() const
{
    return name;
}

INLINE void User::Name(char *name_in)
{
    assert(name_in!=0);
    delete name;
    name = new char[strlen(name_in)+1];
    assert(name);
    strcpy(name, name_in);
}

INLINE char *User::Password() const
{
    return password;
}

INLINE int User::CheckPassword(char *passwd) // probably should remove this
{
    assert(passwd!=0);
    return (strcmp(passwd, password)==0);
}

INLINE User::~User()
{
    delete [] name;
    delete [] password;
    DestructTrace("User");
}

//-------------------------------------------------------------------

INLINE FolderItem::FolderItem(FolderItem *parent_in, int typ_in,
			      int item_num_in, char *name_in, char *creator_in,
			      long create_date_in, long flags_in)
{
    ConstructTrace("FolderItem");
    Init(parent_in, typ_in, item_num_in, name_in, creator_in,
	 create_date_in, flags_in);
}

INLINE int FolderItem::IsComposite() const
{
    return !IsAtomic();	
}

INLINE char *FolderItem::Name() const
{
    return name;		
}

INLINE const int FolderItem::Ref() const
{
    return ref;		
}

INLINE const int FolderItem::ItemNum() const
{
    return item_num;	
}

INLINE char *FolderItem::Creator() const
{
    return creator;
}

INLINE long FolderItem::CreateDate()
{
    return create_date;
}

INLINE const int FolderItem::First() const
{
    return first_item;
}

INLINE const int FolderItem::Size() const
{
    return folder_size;
}

INLINE FolderItem *FolderItem::Parent() const
{
    return parent;	
}

INLINE int FolderItem::IsObsolete()
{
    return OMIsObsolete(flags);
}

INLINE int FolderItem::IsUnread()
{
    return OMIsUnread(flags);
}

INLINE void FolderItem::MarkAsRead()
{
    OMMarkAsRead(flags);
}

INLINE int FolderItem::IsError()
{
    return OMIsError(flags);
}

INLINE int FolderItem::HasDistListError()
{
    return OMHasDistListError(flags);
}

//--------------------------------------------------------------------

INLINE AtomicItem::AtomicItem(FolderItem *parent_in, int typ_in,
			      int item_num_in, char *name_in, char *creator_in,
			      long create_date_in, long flags_in)
     : FolderItem(parent_in, typ_in, item_num_in, name_in, creator_in,
		create_date_in, flags_in)
{
    for (int i = 0; i < MAX_MSG_LINES; i++)
	text[i] = 0;
    ConstructTrace("AtomicItem");
}

INLINE int AtomicItem::Export(char *fname)
{
    return OMExport(parent->Ref(), ItemNum(), fname, typ);
}

INLINE int AtomicItem::Export(char *fname, int etyp)
{
    return OMExport(parent->Ref(), ItemNum(), fname, etyp);
}

INLINE const char * const AtomicItem::Text(int ln_num) const
{
    return text[ln_num];
}

//--------------------------------------------------------------------

INLINE Folder::Folder(FolderItem *parent_in, int item_num_in, char *name_in,
	       char *creator_in, long create_date_in, long flags_in)
     : CompositeItem(parent_in, FolderType, item_num_in, name_in, creator_in, 
		create_date_in, flags_in)
{
    ConstructTrace("Folder");
}

//--------------------------------------------------------------------

INLINE BBArea::BBArea(FolderItem *parent_in, int item_num_in, char *name_in,
	       char *creator_in, long create_date_in, long flags_in)
     : CompositeItem(parent_in, FolderType, item_num_in, name_in, creator_in, 
		create_date_in, flags_in)
{
    ConstructTrace("BBArea");
}

//--------------------------------------------------------------------

INLINE Message::Message(FolderItem *parent_in, int item_num_in,
			char *subject_in, char *creator_in, long create_date_in,
			long flags_in, long msgflags_in, long receipt_date_in,
			long deferred_date_in)
    : CompositeItem(parent_in, MessageType, item_num_in, subject_in, creator_in,
		create_date_in, flags_in),
      msgflags(msgflags_in), receipt_date(receipt_date_in),
      deferred_date(deferred_date_in)
{
    ConstructTrace("Message");
}

INLINE int Message::Priority()
{
    return OMPriority(msgflags);
}

INLINE int Message::Sensitivity()
{
    return OMSensitivity(msgflags);
}

INLINE int Message::RequestedAck()
{
    return OMRequestedAck(msgflags);
}

INLINE int Message::SentAck()
{
    return OMSentAck(msgflags);
}

INLINE int Message::Importance()
{
    return OMImportance(msgflags);
}

INLINE int Message::RecipCategory()
{
    return OMRecipCategory(msgflags);
}

INLINE long Message::ReceiveDate()
{
    return receipt_date;
}

INLINE long Message::DeferredDate()
{
    return deferred_date;
}

INLINE Reply::Reply(FolderItem *parent_in, int item_num_in, char *subject_in,
		 char *creator_in, long create_date_in, long flags_in,
		long msgflags_in, long receipt_date_in, 
		long deferred_date_in)
	: Message(parent_in, item_num_in, subject_in,
		 creator_in, create_date_in, flags_in,
		msgflags_in, receipt_date_in, deferred_date_in)
{
    typ = ReplyType;
    ConstructTrace("Reply");
}

INLINE NonDelivery::NonDelivery(FolderItem *parent_in, int item_num_in,
		char *subject_in, char *creator_in, long create_date_in,
		long flags_in, long msgflags_in, long receipt_date_in, 
		long deferred_date_in)
	: Message(parent_in, item_num_in, subject_in,
		 creator_in, create_date_in, flags_in,
		msgflags_in, receipt_date_in, deferred_date_in)
{
    typ = NDNMessageType;
    ConstructTrace("NonDelivery");
}

//--------------------------------------------------------------------

INLINE ViewedMessage::ViewedMessage(Message *msg)
	: AtomicItem(msg->parent, MessageType, msg->item_num,
			msg->name, msg->creator,
			msg->create_date, msg->flags)
{
    ConstructTrace("ViewedMessage");
    folder_size = msg->folder_size;
    first_item = msg->first_item;
    ref = msg->ref;
}

//--------------------------------------------------------------------

 INLINE DistList::DistList(FolderItem *parent_in, int item_num_in,
		    char *name_in, char *creator_in, long create_date_in,
		long flags_in)
    : AtomicItem(parent_in, DistListType, item_num_in, name_in, creator_in,
		create_date_in, flags_in)
{
    ConstructTrace("DistList");
}

//--------------------------------------------------------------------

INLINE Text::Text(FolderItem *parent_in, int item_num_in,
	    char *name_in, char *creator_in, long create_date_in, long flags_in)
    : AtomicItem(parent_in, TextType, item_num_in, name_in, creator_in,
		create_date_in, flags_in)
{
    ConstructTrace("Text");
}

//--------------------------------------------------------------------

INLINE Other::Other(FolderItem *parent_in, int typ_in, int item_num_in,
		    char *name_in, char *creator_in, long create_date_in,
		    long flags_in)
     : AtomicItem(parent_in, typ_in, item_num_in, name_in, creator_in,
		create_date_in, flags_in)
{
    ConstructTrace("Other");
}

//-----------------------------------------------------------------

INLINE int MessageStore::NextPage()
{
    assert(folder);
    return folder->NextPage();
}

INLINE int MessageStore::PrevPage()
{
    assert(folder);
    return folder->PrevPage();
}

INLINE int MessageStore::FirstPage()
{
    assert(folder);
    return folder->FirstPage();
}

// NB THESE ARE FOR SAVING MESSAGES. USE EXPORT TO SAVE BASIC ITEMS!!

INLINE int MessageStore::Save(long item_num, char *fname)
{
    OMSave(folder->Ref(), item_num, fname);
}

INLINE int MessageStore::Save(int list_ref, long item_num, char *fname)
{
    OMSave(list_ref, item_num, fname);
}

INLINE int MessageStore::SaveAsText(long item_num, char *fname)
{
    OMSave(folder->Ref(), item_num, fname, 1);
}

INLINE int MessageStore::SaveAsText(int list_ref, long item_num, char *fname)
{
    OMSave(list_ref, item_num, fname, 1);
}

INLINE int MessageStore::ReplaceText(long item_num, char *fname)
{
    return OMReplaceText(folder->Ref(), item_num, fname);
}

INLINE int MessageStore::Print(long item_num, char *cmd)
{
    OMPrint(folder->Ref(), item_num, cmd);
}

INLINE int MessageStore::Print(int listref, long item_num, char *cmd)
{
    OMPrint(listref, item_num, cmd);
}

INLINE int MessageStore::Delete(int listref, long item_num)
{
    OMDelete(listref, item_num);
}

INLINE FolderItem *MessageStore::Folder()
{
    assert(folder);
    return folder;
}

INLINE int MessageStore::ChangePassword(char *passwd)
{
    return user->SetPassword(passwd);
}

INLINE char *MessageStore::Password()
{
    return user->Password();
}

INLINE char *MessageStore::UserName()
{
    return user->Name();
}

INLINE char *MessageStore::OMUserName()
{
    return omuser;
}

INLINE void  MessageStore::SetPollInterval(int pollticks_in)
{
    pollticks = pollticks_in;
}

INLINE int MessageStore::MoveItem(int srcref, long srcitem, int destref,
				  long destitem)
{
    return OMMoveItem(srcref, srcitem, destref, destitem);
}

INLINE int MessageStore::CopyItem(int srcref, long srcitem, int destref,
					long destitem)
{
    return OMCopyItem(srcref, srcitem, destref, destitem);
}

INLINE int MessageStore::AttachItem(int srcref, int destref)
{
    return OMAttachItem(srcref, destref);
}

INLINE int MessageStore::HoldMessage(int msgref) // attach to out tray
{
    int rtn = OMAttachItem(msgref, ufolder->Ref(), 2l);
    (void)OMCancel(msgref);
    return rtn;
}

INLINE int MessageStore::CreateFolder(int parentref, char *name)
{
    return OMCreateComposite(parentref, name, FolderType);
}

INLINE int MessageStore::CreateArea(int parentref, char *name)
{
    return OMCreateComposite(parentref, name, BBoardType);
}

INLINE int MessageStore::RenameFolder(int listref, long itemnum, char *name)
{
    return OMRenameFolder(listref, itemnum, name);
}

INLINE int MessageStore::ReorderFolder(int listref, long item, long after_item)
{
    return OMReorderFolder(listref, item, after_item);
}

INLINE int MessageStore::AttachFile(int msgref, char *fname, char *descrp,
					int ftype)
{
    return OMAttachFile(msgref, fname, descrp, ftype);
}

INLINE int MessageStore::MailMessage(MessageSettings &mset, int listref,
					long itemnum)
{
    int musttrack = (mset.GetDate() > time(0) || mset.GetAckLevel());
    return OMMailMessage(listref, itemnum, musttrack);
}

INLINE int MessageStore::ChangeSubject(int listref, long itemnum, char *subject)
{
    return OMChangeSubject(listref, itemnum, subject);
}

INLINE int MessageStore::IsLoginUser(OMAddress *addr)
{
    char *a = addr->GetExternal();
    int rtn = (strcmp(a, omuser)==0);
    delete [] a;
    return rtn;
}

INLINE int MessageStore::SetUserConfig(int wbclear, int tabsize, int readdl,
	int printdl, int xtra, int savemail)
{
    return OMSetUserConfig(wbclear, tabsize, readdl, printdl, xtra, savemail);
}

INLINE int MessageStore::GetUserConfig(int &wbclear, int &tabsize, int &readdl,
	int &printdl, int &xtra, int &savemail)
{
    return OMGetUserConfig(wbclear, tabsize, readdl, printdl, xtra, savemail);
}

INLINE int MessageStore::SetNameDirectory(char *name, char *passwd,
						int ispublic)
{
    if (namefd > 0) OMCloseDirectory(namefd);
    namefd = OMOpenDirectory(name, passwd, ispublic);
    return (namefd > 0) ? 0 : -1;
}

INLINE int MessageStore::GetDirectories(int &cnt, char **items, int *types,
			int first)
{
    return OMGetDirectories(cnt, items, types, first);
}

INLINE int MessageStore::GetAutoForwardDList(char *fname)
{
    return OMGetAutoForwardDList(fname);
}

INLINE int MessageStore::GetAutoForwardComment(char *fname)
{
    return OMGetAutoForwardComment(fname);
}

INLINE int MessageStore::GetAutoReply(char *fname)
{
    return OMGetAutoReply(fname);
}

INLINE int MessageStore::GetAutoActions(int &afa, int &ara, int &ic, int &dfm,
				 int &koc, int &fprm, int &fpsm, int &fccm,
				 int &rtam, int &rtum, int &rtrra)
{
    return OMGetAutoActions(afa, ara, ic, dfm, koc, fprm, fpsm, fccm,
				rtam, rtum, rtrra);
}

INLINE int MessageStore::SetAutoActions(int afa, int ara, int ic, int dfm,
				int koc, int fprm, int fpsm, int fccm,
				char* dlist, char* comments, int rtam,
				int rtum, int rtrra,
				char* reply)
{
    return OMSetAutoActions(afa, ara, ic, dfm, koc, fprm, fpsm, fccm,
				dlist, comments, rtam, rtum, rtrra, reply);
}

//--------------------------------------------------------------------
#endif

