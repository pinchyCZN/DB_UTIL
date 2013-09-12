#define VC_EXTRALEAN

#include <windows.h>
#include <sql.h>
#include <sqlext.h>
#include <stdio.h>
#include <conio.h>
#include <stdlib.h>

int get_error_msg(SQLHANDLE handle,int handle_type,char *err,int len)
{
	SQLCHAR state[6]={0},msg[SQL_MAX_MESSAGE_LENGTH]={0};
	SQLINTEGER  error=0;
	SQLSMALLINT msglen;
	SQLGetDiagRec(handle_type,handle,1,state,&error,msg,sizeof(msg),&msglen);
	_snprintf(err,len,"%s %s",msg,state);
	return atoi(state);
}

int get_columns(DB_TREE *tree,char *table,HTREEITEM hitem)
{
	HSTMT hstmt;
	int count=0;
	if(SQLAllocStmt(tree->hdbc, &hstmt)!=SQL_SUCCESS)
		return count;
	if(SQLColumns(hstmt,NULL,SQL_NTS,NULL,SQL_NTS,table,SQL_NTS,NULL,SQL_NTS)==SQL_SUCCESS){
		if(SQLFetch(hstmt)!=SQL_NO_DATA_FOUND){
			char name[256]={0};
			int len=0;
			while(!SQLGetData(hstmt,4,SQL_C_CHAR,name,sizeof(name),&len)){
				insert_item(name,hitem,IDC_COL_ITEM);
				SQLFetch(hstmt);
				count++;
				name[0]=0;
				len=0;
			}
		}
	}
	SQLFreeStmt(hstmt,SQL_CLOSE);
	return count;
}
int get_tables(DB_TREE *tree)
{
	HSTMT hstmt;
	char table[256];
	long len;
	int count=0;
	
	if(SQLAllocStmt(tree->hdbc, &hstmt)!=SQL_SUCCESS)
		return count;
	if(SQLTables(hstmt,NULL,SQL_NTS,NULL,SQL_NTS,NULL,SQL_NTS,"'TABLE'",SQL_NTS)!=SQL_ERROR){
		if(SQLFetch(hstmt)!=SQL_NO_DATA_FOUND){
			HSTMT hpriv=0;
			SQLAllocStmt(tree->hdbc,&hpriv);
			while(!SQLGetData(hstmt,3,SQL_C_CHAR,table,sizeof(table),&len))
			{
				HTREEITEM hitem;
				table[sizeof(table)-1]=0;
				/*
				if(SQLTablePrivileges(hpriv,"",SQL_NTS,"",SQL_NTS,table,SQL_NTS)==SQL_SUCCESS)
				{
					char str[256]={0};
					SQLFetch(hpriv);
					SQLGetData(hpriv,6,SQL_C_CHAR,str,sizeof(str),&len);
					SQLCloseCursor(hpriv);
					printf("name=%s | %s\n",table,str);
				}
				*/
				hitem=insert_item(table,tree->hroot,IDC_TABLE_ITEM);
				//if(hitem!=0)
				//	get_columns(tree,table,hitem);

				SQLFetch(hstmt);
				count++;
				if(GetKeyState(VK_ESCAPE)&0x8000)
					break;
			}
			SQLFreeStmt(hpriv,SQL_CLOSE);
		}
	}
	SQLFreeStmt(hstmt,SQL_CLOSE);
	return count;
}

int extract_db_name(DB_TREE *tree)
{
	int i,len,index=0;
	char *s=0;
	s=strstr(tree->connect_str,"SourceDB=");
	if(s!=0){
		len=strlen(s);
		for(i=0;i<len;i++){
			if(s[i]==';')
				break;
			if(index>=sizeof(tree->name)-1)
				break;
			tree->name[index++]=s[i];
		}
	}
	s=strstr(tree->connect_str,"DSN=");
	if(s!=0){
		if(index>0 && index<sizeof(tree->name)-1)
			tree->name[index++]=';';
		len=strlen(s);
		for(i=0;i<len;i++){
			if(s[i]==';')
				break;
			if(index>=sizeof(tree->name)-1)
				break;
			tree->name[index++]=s[i];
		}
	}
	if(s!=0){
		tree->name[index]=0;
		if(index>=0)
			return TRUE;
	}
	return FALSE;
}
int open_db(DB_TREE *tree)
{
    SQLHENV     hEnv = NULL;
    SQLHDBC     hDbc = NULL;
	if(tree==0)
		return FALSE;
	if(tree->hdbc!=0 || tree->hdbenv!=0)
		return TRUE;
    if(SQLAllocHandle(SQL_HANDLE_ENV,SQL_NULL_HANDLE,&hEnv)==SQL_ERROR)
		return FALSE;
	if(SQLSetEnvAttr(hEnv,SQL_ATTR_ODBC_VERSION,(SQLPOINTER)SQL_OV_ODBC3,0)==SQL_SUCCESS){
		if(SQLAllocHandle(SQL_HANDLE_DBC, hEnv, &hDbc)==SQL_SUCCESS){
			char str[1024]={0};
			char obdc_str[1024]={0};
			int result=0;
			if(tree->connect_str[0]!=0)
				_snprintf(obdc_str,sizeof(obdc_str),"%s",tree->connect_str);
			else
				_snprintf(obdc_str,sizeof(obdc_str),"%s",tree->name);
			result=SQLDriverConnect(hDbc,
							 GetDesktopWindow(),
							 (SQLCHAR*)obdc_str, //"ODBC;",
							 SQL_NTS,
							 (SQLCHAR*)str,
							 sizeof(str),
							 NULL,
							 SQL_DRIVER_COMPLETE);
			if(result==SQL_SUCCESS || result==SQL_SUCCESS_WITH_INFO){
				if(result==SQL_SUCCESS_WITH_INFO){
					char msg[SQL_MAX_MESSAGE_LENGTH]={0};
					get_error_msg(hDbc,SQL_HANDLE_DBC,msg,sizeof(msg));
					printf("msg=%s\n",msg);
				}
				if(str[0]!=0){
					strncpy(tree->connect_str,str,sizeof(tree->connect_str));
					extract_db_name(tree);
					printf("connect str=%s\n",str);
					add_connect_str(str);
				}
				tree->hdbc=hDbc;
				tree->hdbenv=hEnv;
				return TRUE;
			}
		}
	}

    if (hDbc)
    {
        SQLDisconnect(hDbc);
        SQLFreeHandle(SQL_HANDLE_DBC, hDbc);
    }

    if (hEnv)
    {
        SQLFreeHandle(SQL_HANDLE_ENV, hEnv);
    }
	return FALSE;
}
int reassign_tables(DB_TREE *tree)
{
	int i;
	if(tree==0)
		return FALSE;
	for(i=0;i<sizeof(table_windows)/sizeof(TABLE_WINDOW);i++){
		if(table_windows[i].hwnd!=0 && stricmp(table_windows[i].name,tree->name)==0){
			table_windows[i].hroot=tree->hroot;
			table_windows[i].hdbc=tree->hdbc;
			table_windows[i].hdbenv=tree->hdbenv;
		}
	}
	return TRUE;

}
int release_tables(HWND hroot)
{
	int i;
	if(hroot==0)
		return FALSE;
	for(i=0;i<sizeof(table_windows)/sizeof(TABLE_WINDOW);i++){
		if(table_windows[i].hroot==hroot){
			table_windows[i].hdbc=0;
			table_windows[i].hdbenv=0;
		}
	}
	return TRUE;
}
int release_db(DB_TREE *tree)
{
	if(tree!=0){
		if(tree->hdbc!=0){
			SQLDisconnect(tree->hdbc);
			SQLFreeHandle(SQL_HANDLE_DBC,tree->hdbc);
		}
		if(tree->hdbenv!=0)
			SQLFreeHandle(SQL_HANDLE_ENV,tree->hdbenv);
		tree->hdbc=0;
		tree->hdbenv=0;
		return TRUE;
	}
	return FALSE;
}
int close_db(DB_TREE *tree)
{
	if(tree!=0){
		release_db(tree);
		release_tables(tree->hroot);
		return TRUE;
	}
	return FALSE;
}

int fetch_columns(SQLHSTMT hstmt,TABLE_WINDOW *win)
{
	SQLSMALLINT i,cols=0; 
	if(hstmt!=0 && win!=0){
		SQLNumResultCols(hstmt,&cols);
		win->columns=0;
		for(i=0;i<cols;i++){
			char str[255]={0};
			SQLColAttribute(hstmt,i+1,SQL_DESC_NAME,str,sizeof(str),NULL,NULL);
			if(str[0]!=0){
				SQLINTEGER sqltype=0,sqllength=0;
				int *mem=0,width;
				win->columns++;
				width=lv_add_column(win->hlistview,str,i);
				SQLColAttribute(hstmt,i+1,SQL_DESC_TYPE,NULL,0,NULL,&sqltype);

				{
					char err[256]={0};
					char str[256]={0};
				SQLColAttribute(hstmt,i+1,2,str,sizeof(str),NULL,&sqltype);
				get_error_msg(hstmt,SQL_HANDLE_STMT,err,sizeof(err));
				}
				SQLColAttribute(hstmt,i+1,SQL_DESC_LENGTH,NULL,0,NULL,&sqllength); //SQL_DESC_DISPLAY_SIZE 
				mem=realloc(win->col_attr,(sizeof(COL_ATTR))*win->columns);
				if(mem!=0){
					win->col_attr=mem;
					win->col_attr[win->columns-1].type=sqltype;
					win->col_attr[win->columns-1].length=sqllength;
					win->col_attr[win->columns-1].col_width=width;
				}
			}
		}
		return cols;
	}
	return cols;
}
int trim_trail_space(char *s,int slen)
{
	int i,len;
	len=strlen(s);
	if(len>slen)
		len=slen;
	for(i=len-1;i>=0;i--){
		if(s[i]<=' ')
			s[i]=0;
		else
			break;
	}
	return len;
}
int fetch_rows(SQLHSTMT hstmt,TABLE_WINDOW *win,int cols)
{
	SQLINTEGER rows=0;
	FILE *f=0;
	char *fname=0;
	if(fname!=0 && fname[0]!=0)
		f=fopen(fname,"wb");
	if(hstmt!=0 && win!=0){
		RECT rect={0};
		DWORD tick=GetTickCount();
		int max_width;
		int buf_size=0x20000;
		char *buf=malloc(buf_size);
		GetClientRect(win->hlistview,&rect);
		max_width=rect.right*.95;
		while(TRUE){
			int result=0;
			int i;
			if(win->abort || win->hwnd==0)
				break;
			result=SQLFetch(hstmt);
			if(!(result==SQL_SUCCESS || result==SQL_SUCCESS_WITH_INFO))
				break;
			for(i=0;i<cols;i++){
				char tmp[255]={0};
				char *str=tmp;
				int sizeof_str=sizeof(tmp);
				int len=0;
				if(win->abort || win->hwnd==0)
					break;
				if(buf!=0){ //use buffer if its avail
					str=buf;
					sizeof_str=buf_size;
					str[0]=0;
				}
				result=SQLGetData(hstmt,i+1,SQL_C_CHAR,str,sizeof_str,&len);
				if(f!=0)
					fprintf(f,"%s%s",str,i==cols-1?"\n----------------------------------------------------\n":",@@@@\n");
				if(result==SQL_SUCCESS || result==SQL_SUCCESS_WITH_INFO){
					extern int trim_trailing;
					char *s=str;
					int width;
					if(len==SQL_NULL_DATA)
						s="(NULL)";
					else if(trim_trailing)
						trim_trail_space(s,sizeof_str);

					if(i==0)
						lv_insert_data(win->hlistview,rows,i,s);
					else
						lv_update_data(win->hlistview,rows,i,s);
					if(len>255){
						int j;
						for(j=0;j<255;j++){
							if(str[j]=='\r' || str[j]=='\n'){
								str[j]=0;
								break;
							}
						}
					}
					width=get_str_width(win->hlistview,s)+4;
					if(max_width!=0 && width>max_width)
						width=max_width;
					if(win->col_attr!=0 && (width>win->col_attr[i].col_width))
						win->col_attr[i].col_width=width;
//Sleep(250);
				}
				else{
					char err[256]={0};
					int code;
					code=get_error_msg(hstmt,SQL_HANDLE_STMT,err,sizeof(err));
					if(code==22012)
						lv_insert_data(win->hlistview,rows,i,"0");
					else
						lv_insert_data(win->hlistview,rows,i,"error retr value");
				}
			}
			rows++;
			if((GetTickCount()-tick) > 750){
				set_status_bar_text(ghstatusbar,0,"fetching results %i",rows);
				tick=GetTickCount();
			}
		}
		if(buf!=0)
			free(buf);
		{
			int i;
			if(win->col_attr!=0)
			for(i=0;i<win->columns;i++){
				ListView_SetColumnWidth(win->hlistview,i,win->col_attr[i].col_width);
			}
		}
	}
	if(f!=0)
		fclose(f);
	return rows;
}
int is_dbf(TABLE_WINDOW *win)
{
	int result=FALSE;
	if(win!=0){
		if(win->name!=0){
			if(strstri(win->name,"DSN=visual foxpro")!=0)
				result=TRUE;
		}
	}
	return result;
}
int is_sql_reserved(const char *sql)
{
	extern const char *sql_reserved_words[];
	int i,result=FALSE;
	for(i=0;i<1000;i++){
		if(sql_reserved_words[i]==0)
			break;
		if(stricmp(sql_reserved_words[i],sql)==0)
			return TRUE;
	}
	return result;
}
int get_column_type(TABLE_WINDOW *win,int col)
{
	int result=0;
	if(win!=0 && win->col_attr!=0){
		result=win->col_attr[col].type;
	}
	return result;
}
int sanitize_value(char *str,char *out,int size,int type)
{
	int result=FALSE;
	if(str!=0 && out!=0 && size>0){
		char tmp[255]={0};
		strncpy(tmp,str,sizeof(tmp));
		tmp[sizeof(tmp)-1]=0;
		if(stricmp(str,"NULL")==0 || stricmp(str,"(NULL)")==0){
			_snprintf(out,size,"%s","NULL");
		}
		else{
			switch(type){
			default:
			case SQL_UNKNOWN_TYPE:
			case SQL_NUMERIC:
			case SQL_DECIMAL:
			case SQL_INTEGER:
			case SQL_SMALLINT:
			case SQL_FLOAT:
			case SQL_REAL:
			case SQL_DOUBLE:
				_snprintf(out,size,"%s",tmp);
				break;
			case -1:
			case SQL_VARCHAR:
			case SQL_CHAR:
				{
					char *s=0;
					int ssize=size;
					if(ssize>0x4000)
						ssize=0x4000;
					s=malloc(ssize);
					if(s!=0 && ssize>0){
						if(strchr(str,'\'')!=0){
							int i,index,len;
							len=strlen(str);
							index=0;
							for(i=0;i<len;i++){
								char a=str[i];
								if(i==0) //surround with quote
									s[index++]='\'';

								if(a=='\''){
									s[index++]=a;
									s[index++]=a;
								}
								else
									s[index++]=a;
								if(i==len-1) //surround with quote
									s[index++]='\'';
								if(index>=ssize){
									index=ssize-1;
									break;
								}
							}
							s[index]=0;
							strncpy(out,s,size);
						}
						else{
							strncpy(s,str,ssize);
							_snprintf(out,size,"'%s'",s);
						}
						out[size-1]=0;
					}
					if(s!=0)
						free(s);
				}
				break;
			case SQL_DATETIME:
			case SQL_TYPE_DATE:
			case SQL_TYPE_TIMESTAMP:
				if(strchr(tmp,'-')){
					if(strchr(tmp,':'))
						_snprintf(out,size,"{ts'%s'}",tmp);
					else{
						if(strcmp(tmp,"1899-12-30")==0)
							_snprintf(out,size,"{dt'%s'}",tmp);
						else
							_snprintf(out,size,"{d'%s'}",tmp);
					}
				}
				else
					_snprintf(out,size,"'%s'",tmp);

				break;
			case SQL_TYPE_TIME:
				_snprintf(out,size,"{t'%s'}",tmp);
				break;
			}
		}
		result=TRUE;
	}
	return result;
}
int delete_row(TABLE_WINDOW *win,int row)
{
	int result=FALSE;
	if(win!=0 && win->hlistview!=0 && win->table[0]!=0){
		char *sql=0,*tmp=0;
		int sql_size=0x10000;
		int tmp_size=0x10000;
		int col_count;
		sql=malloc(sql_size);
		tmp=malloc(tmp_size);
		col_count=lv_get_column_count(win->hlistview);
		if(sql!=0 && tmp!=0){
			int i;
			int DBF_TYPE=FALSE;
			char *lbrack="",*rbrack="";
			DBF_TYPE=is_dbf(win);

			if(is_sql_reserved(win->table) || strchr(win->table,' ')){
				if(DBF_TYPE){
					lbrack="`",rbrack="`";
				}else{
					lbrack="[",rbrack="]";
				}
			}
			sql[0]=0;
			_snprintf(sql,sql_size,"DELETE FROM %s%s%s WHERE \n",lbrack,win->table,rbrack);
			for(i=0;i<col_count;i++){
				char *v=0,*eq="=";
				char col_name[80]={0};
				lv_get_col_text(win->hlistview,i,col_name,sizeof(col_name));
				if(is_sql_reserved(col_name) || strchr(col_name,' ')){
					if(DBF_TYPE){
						lbrack="`",rbrack="`";
					}
					else{
						lbrack="[",rbrack="]";
					}
				}
				else{
					lbrack="",rbrack="";
				}
				tmp[0]=0;
				ListView_GetItemText(win->hlistview,row,i,tmp,tmp_size);
				if(stricmp(tmp,"(NULL)")==0){
					v="NULL";
					eq=" is ";
				}
				else if(tmp[0]==0)
					v="''";
				else
					v=tmp;
				sanitize_value(tmp,tmp,tmp_size,get_column_type(win,i));
				_snprintf(sql,sql_size,"%s%s%s%s%s%s%s",sql,lbrack,col_name,rbrack,eq,v,i>=col_count-1?"":" AND\r\n");
			}
			if(col_count>0){
				if(reopen_db(win)){
					mdi_create_abort(win);
					set_status_bar_text(ghstatusbar,0,"deleting row %i",row+1);
					result=execute_sql(win,sql,FALSE);
					if(result){
						ListView_DeleteItem(win->hlistview,row);
						set_status_bar_text(ghstatusbar,0,"row %i deleted",row+1);
					}
					else{
						copy_str_clipboard(sql);
						set_status_bar_text(ghstatusbar,0,"FAILED to delete row %i",row+1);
					}
					mdi_destroy_abort(win);
					PostMessage(win->hwnd,WM_USER,win->hlistview,IDC_MDI_CLIENT);
				}
			}
		}
		if(sql!=0)
			free(sql);
		if(tmp!=0)
			free(tmp);

	}
	return result;
}

int insert_row(TABLE_WINDOW *win,HWND hlistview)
{
	int result=FALSE;
	int FIELD_COL=0,VAL_COL=1,TYPE_COL=2;
	char *sql=0;
	char *tmp=0;
	int sql_size=0x10000,tmp_size=0x10000;
	if(win==0 || hlistview==0)
		return result;
	sql=malloc(sql_size);
	tmp=malloc(tmp_size);
	if(sql!=0 && tmp!=0){
		int i,count,inserted;
		char *lbrack="",*rbrack="";
		int DBF_TYPE=FALSE;
		tmp[0]=0;
		GetWindowText(GetParent(hlistview),tmp,tmp_size);
		if(strstri(tmp,"add row ")!=0){
			_snprintf(tmp,tmp_size,tmp+sizeof("add row ")-1);
		}
		_snprintf(sql,sql_size,"INSERT INTO %s (",tmp);
		if(tmp[0]=='`')
			DBF_TYPE=TRUE;

		count=ListView_GetItemCount(hlistview);
		inserted=0;
		for(i=0;i<count;i++){
			lbrack="",rbrack="";

			ListView_GetItemText(hlistview,i,VAL_COL,tmp,tmp_size);
			if(tmp[0]==0)
				continue;

			tmp[0]=0;
			ListView_GetItemText(hlistview,i,FIELD_COL,tmp,tmp_size);
			if(is_sql_reserved(tmp) || strchr(tmp,' ')){
				if(DBF_TYPE){
					lbrack="`",rbrack="`";
				}else{
					lbrack="[",rbrack="]";
				}
			}
			_snprintf(sql,sql_size,"%s%s%s%s%s",sql,(inserted>0)?",":"",lbrack,tmp,rbrack);
			inserted++;
		}
		
		_snprintf(sql,sql_size,"%s) VALUES (",sql);
		
		inserted=0;
		for(i=0;i<count;i++){
			char *d=0;
			char str[80]={0};
			char *lquote="",*rquote="";
			tmp[0]=0;
			ListView_GetItemText(hlistview,i,VAL_COL,tmp,tmp_size);
			if(tmp[0]==0)
				continue;
			d=tmp;
			if(stricmp(tmp,"(NULL)")==0){
				d="NULL";
			}
			else{
				ListView_GetItemText(hlistview,i,TYPE_COL,str,sizeof(str));
				if(strstri(str,"date")!=0 || strstri(str,"time")!=0 || strstri(str,"char")!=0){
					lquote=rquote="'";
				}
			}
			_snprintf(sql,sql_size,"%s%s%s%s%s",sql,(inserted>0)?",":"",lquote,d,rquote);
			inserted++;
		}
		if(count>0){
			_snprintf(sql,sql_size,"%s)",sql);
			sql[sql_size-1]=0;
			copy_str_clipboard(sql);
			if(reopen_db(win)){
				result=execute_sql(win,sql,FALSE);
			}
		}
	}
	if(sql!=0)
		free(sql);
	if(tmp!=0)
		free(tmp);
	return result;
}
int update_row(TABLE_WINDOW *win,int row,char *data,int only_copy)
{
	int result=FALSE;
	if(win!=0 && win->hlistview!=0 && win->table[0]!=0 && data!=0){
		char *sql=0;
		char col_name[80]={0};
		char *tmp=0;
		int i,count,sql_size=0x10000,tmp_size=0x10000;
		count=lv_get_column_count(win->hlistview);
		lv_get_col_text(win->hlistview,win->selected_column,col_name,sizeof(col_name));
		sql=malloc(sql_size);
		tmp=malloc(tmp_size);
		if(count>0 && sql!=0 && tmp!=0 && col_name[0]!=0){
			int DBF_TYPE=FALSE;
			char *lbrack="",*rbrack="";
			DBF_TYPE=is_dbf(win);
			if(is_sql_reserved(col_name) || strchr(col_name,' ')){
				if(DBF_TYPE){
					lbrack="`",rbrack="`";
				}else{
					lbrack="[",rbrack="]";
				}
			}
			sql[0]=0;
			tmp[0]=0;
			sanitize_value(data,tmp,tmp_size,get_column_type(win,win->selected_column));
			_snprintf(sql,sql_size,"UPDATE [%s] SET %s%s%s=%s WHERE ",win->table,lbrack,col_name,rbrack,tmp[0]==0?"''":tmp);
			for(i=0;i<count;i++){
				char *v=0,*eq="=";
				col_name[0]=0;
				lv_get_col_text(win->hlistview,i,col_name,sizeof(col_name));
				if(is_sql_reserved(col_name) || strchr(col_name,' ')){
					if(DBF_TYPE){
						lbrack="`",rbrack="`";
					}
					else{
						lbrack="[",rbrack="]";
					}
				}
				else{
					lbrack="",rbrack="";
				}
				tmp[0]=0;
				ListView_GetItemText(win->hlistview,row,i,tmp,tmp_size);
				if(stricmp(tmp,"(NULL)")==0){
					v="NULL";
					eq=" is ";
				}
				else if(tmp[0]==0)
					v="''";
				else
					v=tmp;
				sanitize_value(tmp,tmp,tmp_size,get_column_type(win,i));
				_snprintf(sql,sql_size,"%s%s%s%s%s%s%s",sql,lbrack,col_name,rbrack,eq,v,i>=count-1?"":" AND\r\n");
			}
			printf("%s\n",sql);
			if(only_copy){
				result=copy_str_clipboard(sql);
				//SetWindowText(win->hedit,sql);
			}
			else{
				if(reopen_db(win)){
					mdi_create_abort(win);
					result=execute_sql(win,sql,FALSE);
					if(result)
						lv_update_data(win->hlistview,row,win->selected_column,data);
					else
						copy_str_clipboard(sql);
					mdi_destroy_abort(win);
					PostMessage(win->hwnd,WM_USER,win->hlistview,IDC_MDI_CLIENT);
				}
			}
#ifdef _DEBUG
			copy_str_clipboard(sql);
#endif

		}
		if(sql!=0)
			free(sql);
		if(tmp!=0)
			free(tmp);
	}
	return result;
}
int execute_sql(TABLE_WINDOW *win,char *sql,int display_results)
{
	int result=FALSE;
	if(win!=0 && win->hdbc!=0 && win->hdbenv!=0){
		int retcode;
		SQLHSTMT hstmt=0;
		SQLAllocHandle(SQL_HANDLE_STMT,win->hdbc,&hstmt);
		if(hstmt!=0){
			SetWindowText(ghstatusbar,"executing query");
			retcode=SQLExecDirect(hstmt,sql,SQL_NTS);
			switch(retcode){
			case SQL_SUCCESS_WITH_INFO:
			case SQL_SUCCESS:
				{
				SQLINTEGER rows=0,cols=0,total=0;
				SQLRowCount(hstmt,&rows);
				if(display_results){
					int mark;
					RECT rect={0};
					ListView_GetItemRect(win->hlistview,0,&rect,LVIR_BOUNDS);
					mark=ListView_GetSelectionMark(win->hlistview);
					SetWindowText(ghstatusbar,"clearing listview");
					mdi_clear_listview(win);
					cols=fetch_columns(hstmt,win);
					SetWindowText(ghstatusbar,"fetching results");
					total=fetch_rows(hstmt,win,cols);
					if(mark>=0){
						RECT newrect={0};
						int dx,dy;
						ListView_SetItemState(win->hlistview,mark,LVIS_FOCUSED|LVIS_SELECTED,LVIS_FOCUSED|LVIS_SELECTED);
						ListView_GetItemRect(win->hlistview,0,&newrect,LVIR_BOUNDS);
						dx=-rect.left+newrect.left;
						dy=-rect.top+newrect.top;
						ListView_Scroll(win->hlistview,dx,dy);
					}
					win->rows=total;
				}
				result=TRUE;
				if(total==rows)
					set_status_bar_text(ghstatusbar,0,"returned %i rows %i cols",win->rows,win->columns);
				else
					set_status_bar_text(ghstatusbar,0,"returned %i of %i rows, %i cols",total,rows,win->columns);
				printf("executed sql sucess\n");
				}
				break;
			case SQL_ERROR:
				{
					char msg[SQL_MAX_MESSAGE_LENGTH]={0};
					get_error_msg(hstmt,SQL_HANDLE_STMT,msg,sizeof(msg));
					printf("msg=%s\n",msg);
					SetWindowText(ghstatusbar,"error occured");
					MessageBox(win->hwnd,msg,"SQL Error",MB_OK);
				}
				break;
			case SQL_NO_DATA:
				SetWindowText(ghstatusbar,"no data returned");
				break;
			default:
				set_status_bar_text(ghstatusbar,0,"unhandled return code %i",retcode);
				break;
			}
			SQLFreeStmt(hstmt,SQL_CLOSE);
		}
	}
	return result;
}
int reopen_db(TABLE_WINDOW *win)
{
	if(win!=0){
		if(win->hdbc==0 && win->hdbenv==0){
			DB_TREE *db=0;
			acquire_db_tree(win->name,&db);
			if(!mdi_open_db(db)){
				char str[80];
				mdi_remove_db(db);
				_snprintf(str,sizeof(str),"Cant open %s",win->name);
				MessageBox(ghmainframe,str,"OPEN DB FAIL",MB_OK);
				return FALSE;
			}
			else{
				reassign_tables(db);
				return TRUE;
			}

		}
		else
			return TRUE;
	}
	return FALSE;
}
int get_col_info(DB_TREE *tree,char *table)
{
	if(tree==0)
		return FALSE;
	if(open_db(tree)){
		HSTMT hstmt;
		if(SQLAllocStmt(tree->hdbc, &hstmt)==SQL_SUCCESS){
			if(SQLColumns(hstmt,NULL,SQL_NTS,NULL,SQL_NTS,table,SQL_NTS,NULL,SQL_NTS)==SQL_SUCCESS){
				char *buf;
				int buf_size=0x10000;
				buf=malloc(buf_size);
				if(buf!=0){
					struct INFO{
						char *col_name;
						int col_number;
						int col_type;
					};
					struct INFO info[]={ 
						{"field name",4,SQL_C_CHAR},
						{"type",6,SQL_C_CHAR},
						{"type #",5,SQL_C_SHORT},
						{"size",7,SQL_C_LONG},
						{"index",0,0},
						{"decimal digits",9,SQL_C_SHORT},
						{"num prec radix",10,SQL_C_SHORT},
						{"nullable",11,SQL_C_SHORT},
						{"remarks",12,SQL_C_CHAR},
						{"default val",13,SQL_C_CHAR},
						{"sql data type",14,SQL_C_SHORT},
						{"sql datetime sub",15,SQL_C_SHORT},
						{"char octect_len",16,SQL_C_LONG},
						{"ordinal position",17,SQL_C_LONG},
						{"is nullable",18,SQL_C_CHAR},
						{"catalog",1,SQL_C_CHAR},
						{"schema",2,SQL_C_CHAR},
					};
					int i,col=0;
					memset(buf,0,buf_size);
					for(i=0;i<sizeof(info)/sizeof(struct INFO);i++){
						_snprintf(buf,buf_size,"%s%s%s",buf,i>0?"\t":"",info[i].col_name);
					}
					_snprintf(buf,buf_size,"%s\n",buf);
					while(SQLFetch(hstmt)==SQL_SUCCESS){
						int len;
						char str[256];
						short short_data;
						long long_data;
						for(i=0;i<sizeof(info)/sizeof(struct INFO);i++){
							const char *delim;
							if(i==0)
								delim="";
							else
								delim="\t";
							switch(info[i].col_type){
							case SQL_C_CHAR:
								str[0]=0;
								len=0;
								SQLGetData(hstmt,info[i].col_number,info[i].col_type,str,sizeof(str),&len);
								str[sizeof(str)-1]=0;
								_snprintf(buf,buf_size,"%s%s%s",buf,delim,str);
								break;
							case SQL_C_SHORT:
								short_data=0;
								len=0;
								SQLGetData(hstmt,info[i].col_number,info[i].col_type,&short_data,sizeof(short_data),&len);
								_snprintf(buf,buf_size,"%s%s%i",buf,delim,short_data);
								break;
							case SQL_C_LONG:
								long_data=0;
								len=0;
								SQLGetData(hstmt,info[i].col_number,info[i].col_type,&long_data,sizeof(long_data),&len);
								_snprintf(buf,buf_size,"%s%s%i",buf,delim,long_data);
								break;
							default:
								_snprintf(buf,buf_size,"%s%s%i",buf,delim,col);
								break;
							}
						}
						_snprintf(buf,buf_size,"%s\n",buf);
						col++;
					}
					buf[buf_size-1]=0;
					DialogBoxParam(ghinstance,MAKEINTRESOURCE(IDD_COL_INFO),tree->htree,col_info_proc,buf);
					free(buf);
				}
			}
		}
		SQLFreeStmt(hstmt,SQL_CLOSE);	
	}
	return 0;
}
int assign_db_to_table(DB_TREE *db,TABLE_WINDOW *win,char *table)
{
	if(db!=0 && win!=0){
		win->hdbc=db->hdbc;
		win->hdbenv=db->hdbenv;
		win->hroot=db->hroot;
		strncpy(win->name,db->name,sizeof(win->name));
		set_window_text(win->hwnd,"%s - %s",table,db->name);
		return TRUE;
	}
	return FALSE;
}
