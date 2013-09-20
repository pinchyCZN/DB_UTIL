#include <windows.h>

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
int get_table(char *fname,char *table,int tsize)
{
	char file[_MAX_FNAME]={0};
	_splitpath(fname,NULL,NULL,file,NULL);
	_snprintf(table,tsize,"%s",file);
	if(tsize>0)
		table[tsize-1]=0;
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
int process_cmd_line(char *cmd)
{
	char fname[MAX_PATH+4]={0};
	char path[MAX_PATH]={0};
	char table[MAX_PATH]={0};
	char ext[_MAX_EXT]={0};
	if(cmd==0)
		return FALSE;
	if(strlen(cmd)==0)
		return FALSE;
	_snprintf(fname,sizeof(fname),"%s",cmd);
	fname[sizeof(fname)-1]=0;
	remove_quotes(fname,sizeof(fname));
	get_path(fname,path,sizeof(path));
	get_table(fname,table,sizeof(table));
	get_ext(fname,ext,sizeof(ext));

	if(stricmp(ext,".DBF")==0){
		if(table[0]!=0 && path[0]!=0){
			char connect[1024]={0};
			//char *cstr="DSN=Visual FoxPro Tables;UID=;PWD=;SourceDB=%s;SourceType=DBF;Exclusive=No;BackgroundFetch=Yes;Collate=Machine;Null=Yes;Deleted=Yes;TABLE=%s";
			//char *cstr="CollatingSequence=ASCII;DefaultDir=%s;Deleted=0;Driver={Microsoft dBASE Driver (*.dbf)};DriverId=533;Exclusive=0;FIL=dBase 5.0;MaxBufferSize=2048;MaxScanRows=8;PageTimeout=5;SafeTransactions=0;Statistics=0;Threads=3;UID=admin;UserCommitSync=Yes;TABLE=%s";
			char *cstr="Driver={Microsoft dBASE Driver (*.dbf)};DBQ=%s;TABLE=%s";
			_snprintf(connect,sizeof(connect),cstr,path,table);
			task_open_db_and_table(connect);
			return TRUE;
		}
	}
	else if(stricmp(ext,".MDB")==0){
		if(fname[0]!=0){
			char connect[1024]={0};
			char *cstr="Driver={Microsoft Access Driver (*.mdb)};Dbq=%s";
			_snprintf(connect,sizeof(connect),cstr,fname);
			task_open_db_and_table(connect);
			return TRUE;
		}
//		task_open_db_and_table("UID=dba;PWD=sql;DSN=Journal;TABLE=PATIENT");
	}
	return FALSE;
}