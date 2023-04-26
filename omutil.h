#ifndef _OMUTIL_H
#define _OMUTIL_H

#define PCTYPE	1
#define CHAR_SET "ROMAN8"	// or ISO8859_1

//---------------------------------------------------------------

extern void (*FatalErrorHandler)(char *);

int OMConnect(char *server, char *user, char *password, int &list_ref,
	char *omuser = NULL);
int OMDisconnect();

int OMPrepList(const int list_ref, const long item_ref, int &newref, int &cnt);
int OMPrepListPage(const int list_ref, const int first, const int pgsz);
int OMGetListElt(int &ref, char * &subject, char * &creator, int &type,
		 long &flags, long &msgflags,
		 long &created, long &received, long &deferred);

int OMPrepRead(const int list_ref, const long item_ref,int getAcks,int &newref);
int OMPrepReadPage(const int list_ref, const int pgsz, const int code);
int OMReadLine(int &linenum, char * &text);

int OMCancel(const int list_ref);

char *OMFolderName(const long folder);

int OMSendFile(char *local, char *remote, int isbinary);
int OMReceiveFile(char *local, char *remote, int isbinary);

int OMNewMail();

int OMSave(const int list_ref, const long item_ref, char *fname, int astext=0);
int OMPrint(const int list_ref, const long item_ref, char *cmd = "lp");
int OMDelete(const int list_ref, const long item_ref);
int OMChangePassword(char *old_password, char *new_password);
int OMMoveItem(int srcref, long srcitem, int destref, long destitem);
int OMCopyItem(int srcref, long srcitem, int destref, long destitem);
int OMAttachItem(int srcref, int destref, long destnum = 0l);
int OMCreateComposite(int parentref, char *name, int typ);
int OMRenameFolder(int listref, long itemnum, char *name);
int OMReorderFolder(int listref, long item, long after_item);
int OMExport(int listref, long item, char *fname, int type = 1167);
int OMReplaceText(int ref, long itemnum, char *fname);
int OMCheckName(char *name_in, int maxmatches, char ** &names_out, long flags,
	int namefd = 0);
void OMFormatName(char *int_name, char *fmt_name);
int OMAddRecipient(char *name, int typ);
int OMCreateMessage(char *subject, long msgflags, int acklevel,time_t earliest);
int OMCreateReply(int listref, long itemnum, char *subject,
		long msgflags, int acklevel,time_t earliest);
int OMCreateForward(int listref, long itemnum, char *subject,
		long msgflags, int acklevel,time_t earliest);
int OMAttachFile(int msgref, char *fname, char *descrip = 0, int ftype =1167);
int OMMailMessage(int listref, long itemnum, int musttrack);
int OMChangeSubject(int listref, long itemnum, char *subject);
int OMModifyMessage(int listref, long itemnum, long flags, long msgflags,
	long deferred_date);
int OMDeleteFile(char *fname);
int OMGetDirectories(int &cnt, char **items, int *types, int first = 0);

/* construct flags */

long OMNewMessageFlags(int pri, int imp, int sens, int allowpublicdls,
			int allowaltrecips, int allowconversion, int allowndns,
			int returnmsg);
long OMMessageFlags(int pri, int imp, int sens, int acklvl, int allowndns);
long OMFlags(int allowpublicdls, int allowaltrecips, int allowconversions,
		int returnmsg);

int OMCreateDListTF(char *fname, int from, int to, int cc, int bcc);
int OMAddAddress(int xtfh, int typ, char *addr);
int OMSaveDList(int xtfh, char *fname, char *descrip, int destfolderref);
int OMReplaceDList(int xtfh, char *fname, char *descrip, int ref, long itemnum);

int OMOpenDListTF(char *fname);
int OMReadDListRecord(int xtf, char *namebuf, int &typ, int &acklvl);
int OMCloseDListTF(int xtf);

// test item flags

int OMIsObsolete(long flags);
int OMIsUnread(long flags);
int OMIsError(long flags);
int OMHasDistListError(long flags);
int OMAllowPublicDLs(long flags);
int OMAllowAltRecips(long flags);
int OMAllowConversions(long flags);
int OMMustReturnMsg(long flags);

void OMMarkAsRead(long &flags);

// test message flags

int OMPriority(long msgflags);
int OMSensitivity(long msgflags);
int OMRequestedAck(long msgflags);
int OMSentAck(long msgflags);
int OMImportance(long msgflags);
int OMRecipCategory(long msgflags);
int OMAllowNDNs(long msgflags);

// Directory access

int OMOpenDirectory(char *name, char *passwd = 0, int ispublic = 0);
int OMCreateDirectory(char *name, char *passwd = 0, int ispublic = 0);
int OMDeleteDirectory(char *name, int ispublic = 0);
char *OMSearchDirectory(int &cnt, int handle, char *attribs = 0,
	char *filter = 0, long flags = 0);
int OMAddDirEntry(int handle, char *entry);
int OMModifyDirEntry(int handle, char *modifier, char *attribs = 0,
	char *filter = 0, long flags = 0);
int OMDeleteDirEntry(int handle, char *attribs = 0, char *filter = 0,
	long flags = 0);
int OMCloseDirectory(int handle);

// configuration related

int OMGetFile(char *fname, int fileid, int fromfno = 0, int getsyscopy = 0);
int OMGetFileTypes(char *fname);
int OMSetUserConfig(int wbclear, int tabsize, int readdl, int printdl,
	int xtra, int savemail);
int OMGetUserConfig(int &wbclear, int &tabsize, int &readdl, int &printdl,
	int &xtra, int &savemail);
int OMGetAutoActions(int &afa, int &ara, int &ic, int &dfm, int &koc,
	int &fprm, int &fpsm, int &fccm, int &rtam, int &rtum, int &rtrra);
int OMSetAutoActions(int afa, int ara, int ic, int dfm, int koc,
	int fprm, int fpsm, int fccm, char* dlist, char* comments,
	int rtam, int rtum, int rtrra, char* reply);
int OMGetAutoForwardDList(char *fname);
int OMGetAutoForwardComment(char *fname);
int OMGetAutoReply(char *fname);

#endif

