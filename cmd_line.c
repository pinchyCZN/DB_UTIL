#include <windows.h>
#include <stdio.h>
#include "resource.h"

char * strstri(char *s1,char *s2);

#define SECTION_NAME "FILE_EXTENSIONS"
#define MAX_EXTENSIONS 20
#define MAX_EXTENSION_LENGTH 80
#define MAX_CONNECT_LENGTH 1024
#define TIMER_ID 1337

int get_path(char *fname,char *path,int plen)
{
	char drive[_MAX_DRIVE]={0},dir[_MAX_DIR]={0};
	char *trail="\\";
	int dirlen=0;
	_splitpath(fname,drive,dir,NULL,NULL);
	dirlen=strlen(dir);
	if(dirlen>0 && dir[dirlen-1]=='\\')
		trail="";
	_snprintf(path,plen,"%s%s%s",drive,dir,trail);
	return TRUE;
}
int remove_quotes(char *path,int max)
{
	int i,len,index=0,infile=FALSE;
	if(strchr(path,'\"')==0)
		return TRUE;
	len=strlen(path);
	if(len>=max-1)
		len=max-1;
	for(i=0;i<len;i++){
		if(infile){
			if(path[i]=='\"')
				break;
			path[index++]=path[i];
		}
		else if(path[i]=='\"')
			infile=TRUE;

	}
	path[index]=0;
	return TRUE;
}
int get_name(char *fname,char *name,int nsize)
{
	char file[_MAX_FNAME]={0};
	_splitpath(fname,NULL,NULL,file,NULL);
	_snprintf(name,nsize,"%s",file);
	if(nsize>0)
		name[nsize-1]=0;
	return TRUE;
}
int get_ext(char *fname,char *ext,int size)
{
	char extension[_MAX_FNAME]={0};
	_splitpath(fname,NULL,NULL,NULL,extension);
	_snprintf(ext,size,"%s",extension);
	if(size>0)
		ext[size-1]=0;
	return TRUE;
}
int find_association(char *ext,char *connect,int clen)
{
	int i,result=FALSE;
	char *section=SECTION_NAME;
	if(ext==0 || connect==0 || clen<=0)
		return result;
	if(ext[0]=='.')
		ext++;

	for(i=0;i<MAX_EXTENSIONS;i++){
		char key[20];
		char str[MAX_EXTENSION_LENGTH]={0};
		_snprintf(key,sizeof(key),"EXT%02i",i);
		get_ini_str(section,key,str,sizeof(str));
		if(str[0]!=0){
			if(stricmp(str,ext)==0){
				connect[0]=0;
				_snprintf(key,sizeof(key),"CONNECT%02i",i);
				get_ini_str(section,key,connect,clen);
				if(connect[0]!=0)
					result=TRUE;
				break;
			}
		}
	}
	return result;
}
/*
str=malloced string
*/
int str_replace(char **instr,char *find,char *replace)
{
	int len;
	char *s,*str;
	if(instr==0 || *instr==0 || find==0 || replace==0)
		return FALSE;
	str=*instr;
	s=strstri(str,find);
	if(s!=0){
		int a,b,delta=0;
		char *tmpstr=0;
		len=strlen(str);
		a=strlen(find);
		b=strlen(replace);
		if(b>a)
			delta=b-a;
		len=len+delta+1;
		tmpstr=malloc(len);
		if(tmpstr!=0){
			s[0]=0;
			tmpstr[0]=0;
			_snprintf(tmpstr,len,"%s%s%s",str,replace,s+a);
			tmpstr[len-1]=0;
			free(*instr);
			*instr=tmpstr;
			return TRUE;
		}
	}
	return FALSE;
}
int replace_params(char *connect,int con_size,char *full_name,char *path,char *name)
{
	int result=FALSE;
	char *tmp=0;
	if(connect==0 || con_size<=0)
		return result;
	tmp=malloc(con_size);
	if(tmp==0)
		return result;
	strncpy(tmp,connect,con_size);
	tmp[con_size-1]=0;
	if(full_name!=0);
		str_replace(&tmp,"%FPATH%",full_name);
	if(path!=0)
		str_replace(&tmp,"%PATH%",path);
	if(name!=0)
		str_replace(&tmp,"%NAME%",name);
	strncpy(connect,tmp,con_size);
	connect[con_size-1]=0;
	free(tmp);
	return TRUE;
}
int process_cmd_line(char *cmd)
{
	char fname[MAX_PATH+4]={0};
	char path[MAX_PATH]={0};
	char name[MAX_PATH]={0};
	char ext[_MAX_EXT]={0};
	char connect[MAX_CONNECT_LENGTH]={0};
	if(cmd==0)
		return FALSE;
	if(strlen(cmd)==0)
		return FALSE;
	_snprintf(fname,sizeof(fname),"%s",cmd);
	fname[sizeof(fname)-1]=0;
	remove_quotes(fname,sizeof(fname));
	get_path(fname,path,sizeof(path));
	get_name(fname,name,sizeof(name));
	get_ext(fname,ext,sizeof(ext));

	if(find_association(ext,connect,sizeof(connect))){
		if(connect[0]!=0){
			replace_params(connect,sizeof(connect),fname,path,name);
			task_open_db_and_table(connect);
		}
	}
	else if(stricmp(ext,".DBF")==0){
		if(name[0]!=0 && path[0]!=0){
			//char *cstr="DSN=Visual FoxPro Tables;UID=;PWD=;SourceDB=%s;SourceType=DBF;Exclusive=No;BackgroundFetch=Yes;Collate=Machine;Null=Yes;Deleted=Yes;TABLE=%s";
			//char *cstr="CollatingSequence=ASCII;DefaultDir=%s;Deleted=0;Driver={Microsoft dBASE Driver (*.dbf)};DriverId=533;Exclusive=0;FIL=dBase 5.0;MaxBufferSize=2048;MaxScanRows=8;PageTimeout=5;SafeTransactions=0;Statistics=0;Threads=3;UID=admin;UserCommitSync=Yes;TABLE=%s";
			char *cstr="Driver={Microsoft dBASE Driver (*.dbf)};DBQ=%s;TABLE=%s";
			_snprintf(connect,sizeof(connect),cstr,path,name);
			task_open_db_and_table(connect);
			return TRUE;
		}
	}
	else if(stricmp(ext,".MDB")==0){
		if(fname[0]!=0){
			char *cstr="Driver={Microsoft Access Driver (*.mdb)};Dbq=%s";
			_snprintf(connect,sizeof(connect),cstr,fname);
			task_open_db_and_table(connect);
			return TRUE;
		}
//		task_open_db_and_table("UID=dba;PWD=sql;DSN=Journal;TABLE=PATIENT");
	}

	return FALSE;
}
int populate_drivers(HWND hwnd)
{
	HKEY hkey=0;
	SendDlgItemMessage(hwnd,IDC_DRIVER_LIST,LB_RESETCONTENT,0,0);
	RegOpenKey(HKEY_LOCAL_MACHINE,"SOFTWARE\\ODBC\\ODBCINST.INI\\ODBC Drivers",&hkey);
	if(hkey!=0){
		int i,count=0;
		RegQueryInfoKey(
        hkey,   // key handle 
        NULL,   // buffer for class name 
        NULL,   // size of class string 
        NULL,   // reserved 
        NULL,	// number of subkeys 
        NULL,   // longest subkey size 
        NULL,   // longest class string 
        &count,   // number of values for this key 
        NULL,	// longest value name 
        NULL,	// longest value data 
        NULL,	// security descriptor 
        NULL);
		for(i=0;i<count;i++){
			char str[MAX_PATH]={0};
			int len=sizeof(str);
			RegEnumValue(hkey,i,str,&len,NULL,NULL,NULL,NULL);
			str[sizeof(str)-1]=0;
			if(str[0]!=0)
				SendDlgItemMessage(hwnd,IDC_DRIVER_LIST,LB_ADDSTRING,0,str);
		}
		RegCloseKey(hkey);
	}
	return TRUE;
}
int populate_assoc(HWND hwnd)
{
	int i,first_entry=-1,lowest_index=MAXLONG;
	char key[20];
	char ext[MAX_EXTENSION_LENGTH]={0};
	char *section=SECTION_NAME;
	SendDlgItemMessage(hwnd,IDC_EXT_COMBO,CB_RESETCONTENT,0,0);
	SetDlgItemText(hwnd,IDC_CONNECT_EDIT,"");
	for(i=0;i<20;i++){
		_snprintf(key,sizeof(key),"EXT%02i",i);
		ext[0]=0;
		get_ini_str(section,key,ext,sizeof(ext));
		if(ext[0]!=0){
			if(CB_ERR==SendDlgItemMessage(hwnd,IDC_EXT_COMBO,CB_FINDSTRINGEXACT,-1,ext)){
				int index;
				index=SendDlgItemMessage(hwnd,IDC_EXT_COMBO,CB_ADDSTRING,0,ext);
				if(index>=0 && index<=lowest_index){
					lowest_index=index;
					first_entry=i;
				}
			}
		}
	}
	if(first_entry>=0){
		char connect[MAX_CONNECT_LENGTH]={0};
		_snprintf(key,sizeof(key),"CONNECT%02i",first_entry);
		get_ini_str(section,key,connect,sizeof(connect));
		SetDlgItemText(hwnd,IDC_CONNECT_EDIT,connect);
		_snprintf(key,sizeof(key),"EXT%02i",first_entry);
		ext[0]=0;
		get_ini_str(section,key,ext,sizeof(ext));
		SendDlgItemMessage(hwnd,IDC_EXT_COMBO,CB_SETCURSEL,lowest_index,0);
		SetDlgItemText(hwnd,IDC_EXT_COMBO,ext);
	}
	return first_entry>=0;
}
int add_update_ext(HWND hwnd,char *ext,char *connect)
{
	int i,first_empty=-1;
	char key[20]={0};
	char str[MAX_EXTENSION_LENGTH]={0};
	char *section=SECTION_NAME;
	if(ext==0 || connect==0)
		return FALSE;
	if(ext[0]==0 || connect[0]==0)
		return FALSE;

	i=SendDlgItemMessage(hwnd,IDC_EXT_COMBO,CB_FINDSTRINGEXACT,-1,ext);
	if(i!=CB_ERR)
		SendDlgItemMessage(hwnd,IDC_EXT_COMBO,CB_SETCURSEL,i,0);
	else
		SendDlgItemMessage(hwnd,IDC_EXT_COMBO,CB_ADDSTRING,0,ext);

	for(i=0;i<MAX_EXTENSIONS;i++){
		_snprintf(key,sizeof(key),"EXT%02i",i);
		str[0]=0;
		get_ini_str(section,key,str,sizeof(str));
		if(str[0]==0 && first_empty==-1)
			first_empty=i;
		if(stricmp(str,ext)==0){
			write_ini_str(section,key,ext);
			_snprintf(key,sizeof(key),"CONNECT%02i",i);
			write_ini_str(section,key,connect);
			return TRUE;
		}
	}
	if(first_empty != -1)
		i=first_empty;
	else
		i=MAX_EXTENSIONS-1;
	_snprintf(key,sizeof(key),"EXT%02i",i);
	write_ini_str(section,key,ext);
	_snprintf(key,sizeof(key),"CONNECT%02i",i);
	write_ini_str(section,key,connect);
	return TRUE;
}
int delete_ext(HWND hwnd,char *ext)
{
	int i;
	char *section=SECTION_NAME;
	char *key[20]={0};
	char *str[MAX_EXTENSION_LENGTH]={0};
	if(ext==0 || ext[0]==0)
		return FALSE;
	for(i=0;i<MAX_EXTENSIONS;i++){
		_snprintf(key,sizeof(key),"EXT%02i",i);
		str[0]=0;
		get_ini_str(section,key,str,sizeof(str));
		if(stricmp(str,ext)==0){
			write_ini_str(section,key,"");
			_snprintf(key,sizeof(key),"CONNECT%02i",i);
			write_ini_str(section,key,"");
			return TRUE;
		}
	}
	return FALSE;
}
int ext_sel_changed(HWND hwnd)
{
	int i;
	char ext[MAX_EXTENSION_LENGTH]={0};
	char *section=SECTION_NAME;
	
	i=SendDlgItemMessage(hwnd,IDC_EXT_COMBO,CB_GETCURSEL,0,0);
	if(i==CB_ERR){
		SetDlgItemText(hwnd,IDC_CONNECT_EDIT,"");
		return FALSE;
	}

	SendDlgItemMessage(hwnd,IDC_EXT_COMBO,CB_GETLBTEXT,i,ext); 
	ext[sizeof(ext)-1]=0;

	if(ext[0]==0)
		return FALSE;
	for(i=0;i<MAX_EXTENSIONS;i++){
		char key[20]={0};
		char str[MAX_EXTENSION_LENGTH]={0};
		_snprintf(key,sizeof(key),"EXT%02i",i);
		str[0]=0;
		get_ini_str(section,key,str,sizeof(str));
		if(stricmp(str,ext)==0){
			char connect[MAX_CONNECT_LENGTH]={0};
			_snprintf(key,sizeof(key),"CONNECT%02i",i);
			get_ini_str(section,key,connect,sizeof(connect));
			connect[sizeof(connect)-1]=0;
			SetDlgItemText(hwnd,IDC_CONNECT_EDIT,connect);
			return TRUE;
		}
	}
	return TRUE;
}
static int print_error()
{
  int error = GetLastError();
  char buffer[128];
  if (FormatMessageA(FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_SYSTEM,
      NULL, error, 0, buffer, sizeof(buffer)/sizeof(char), NULL))
	printf("error=%s\n",buffer);
  return 0;
}
static int tooltip_message(HWND hwnd,HWND *tooltip,char *fmt,...)
{
	int timer_id=0;
	if(tooltip==0 || fmt==0 || fmt[0]==0)
		return timer_id;
	if(*tooltip!=0){
		destroy_tooltip(*tooltip);
		*tooltip=0;
	}
	KillTimer(hwnd,TIMER_ID);

	timer_id=SetTimer(hwnd,TIMER_ID,750,NULL);
	if(timer_id!=0){
		RECT rect;
		char msg[128]={0};
		va_list ap;
		va_start(ap,fmt);
		_vsnprintf(msg,sizeof(msg),fmt,ap);
		msg[sizeof(msg)-1]=0;
		GetWindowRect(hwnd,&rect);
		create_tooltip(hwnd,msg,rect.left,rect.top,tooltip);
	}
	return timer_id;
}

LRESULT CALLBACK file_assoc_proc(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam)
{
	static HWND grippy=0,tooltip=0;
	static int help_busy=FALSE;
	static char *help_str="example cmd line: C:\\temp\\mydatabase.dbf\r\n"
				"%FPATH%=C:\\temp\\mydatabase.dbf\r\n"
				"%PATH%=C:\\temp\\\r\n"
				"%NAME%=mydatabase\r\n\r\n"
				"example connect: Driver={Microsoft dBASE Driver (*.dbf)};DBQ=%FPATH%;TABLE=%NAME%";

	switch(msg){
	case WM_INITDIALOG:
		SendDlgItemMessage(hwnd,IDC_EXT_COMBO,CB_LIMITTEXT,MAX_EXTENSION_LENGTH-1,0);
		SendDlgItemMessage(hwnd,IDC_CONNECT_EDIT,EM_LIMITTEXT,MAX_CONNECT_LENGTH-1,0);
		SetDlgItemText(hwnd,IDC_STATIC_HELP,help_str);
		populate_assoc(hwnd);
		populate_drivers(hwnd);
		grippy=create_grippy(hwnd);
		tooltip=0;
		help_busy=FALSE;
		break;
	case WM_SIZE:
		grippy_move(hwnd,grippy);
		resize_file_assoc(hwnd);
		break;
	case WM_TIMER:
		KillTimer(hwnd,TIMER_ID);
		destroy_tooltip(tooltip);
		tooltip=0;
		break;
	case WM_HELP:
		resize_file_assoc(hwnd);
		break;
		if(!help_busy){
			help_busy=TRUE;
			MessageBox(hwnd,help_str,"HELP",MB_OK);
			help_busy=FALSE;
		}
		break;
	case WM_COMMAND:
		switch(LOWORD(wparam)){
		case IDC_EXT_COMBO:
			switch(HIWORD(wparam)){
			case CBN_SELCHANGE:
				ext_sel_changed(hwnd);
				break;
			}
			break;
		case IDC_DRIVER_LIST:
			if(HIWORD(wparam)==LBN_DBLCLK){
				int sel;
				sel=SendDlgItemMessage(hwnd,IDC_DRIVER_LIST,LB_GETCURSEL,0,0);
				if(sel!=LB_ERR){
					char str[MAX_PATH]={0};
					SendDlgItemMessage(hwnd,IDC_DRIVER_LIST,LB_GETTEXT,sel,str);
					if(str[0]!=0){
						char tmp[MAX_PATH];
						_snprintf(tmp,sizeof(tmp),"%s%s%s","Driver={",str,"};");
						tmp[sizeof(tmp)-1]=0;
						SendDlgItemMessage(hwnd,IDC_CONNECT_EDIT,EM_REPLACESEL,TRUE,tmp);
					}
				}
			}
			break;
		case IDCANCEL:
			if(tooltip!=0)
				destroy_tooltip(tooltip);
			tooltip=0;
			KillTimer(hwnd,TIMER_ID);
			EndDialog(hwnd,0);
			break;
		case IDC_DELETE:
			{
				char ext[MAX_EXTENSION_LENGTH]={0};
				GetDlgItemText(hwnd,IDC_EXT_COMBO,ext,sizeof(ext));
				ext[sizeof(ext)-1]=0;
				if(delete_ext(hwnd,ext))
					tooltip_message(hwnd,&tooltip,"deleted extension");
				else
					tooltip_message(hwnd,&tooltip,"failed to delete");
				populate_assoc(hwnd);
			}
			break;
		case IDOK:
			{
				HWND focus=GetFocus();
				if(focus==GetDlgItem(hwnd,IDOK) || focus==GetDlgItem(hwnd,IDC_CONNECT_EDIT)){
					char ext[MAX_EXTENSION_LENGTH]={0};
					char connect[MAX_CONNECT_LENGTH]={0};
					GetDlgItem(hwnd,IDOK);
					GetDlgItemText(hwnd,IDC_EXT_COMBO,ext,sizeof(ext));
					ext[sizeof(ext)-1]=0;
					GetDlgItemText(hwnd,IDC_CONNECT_EDIT,connect,sizeof(connect));
					connect[sizeof(connect)-1]=0;
					if(add_update_ext(hwnd,ext,connect))
						tooltip_message(hwnd,&tooltip,"added extension");
					else
						tooltip_message(hwnd,&tooltip,"failed to add extension");
				}else if(focus==GetDlgItem(hwnd,IDC_DRIVER_LIST)){
					SendMessage(hwnd,WM_COMMAND,MAKEWPARAM(IDC_DRIVER_LIST,LBN_DBLCLK),0);
				}
				else{
					ext_sel_changed(hwnd);
					tooltip_message(hwnd,&tooltip,"refreshed");
					break;
				}
			}
			break;
		}
		break;
	}
	return 0;
}

int process_drop(HWND hwnd,HANDLE hdrop)
{
	int i,count,is_file=FALSE;
	char str[MAX_PATH];

	count=DragQueryFile(hdrop,-1,NULL,0);
	for(i=0;i<count;i++){
		str[0]=0;
		DragQueryFile(hdrop,i,str,sizeof(str));
		if(str[0]==0)
			continue;
		if(!is_path_directory(str)){
			is_file=TRUE;
			break;
		}
	}
	DragFinish(hdrop);
	if(is_file)
		process_cmd_line(str);
	return 0;
}