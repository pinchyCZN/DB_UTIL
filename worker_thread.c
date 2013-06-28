#include <windows.h>
#include <stdio.h>

extern HWND ghmainframe,ghmdiclient,ghtreeview,ghdbview,ghstatusbar;
HANDLE event;
int task=0;
int keep_closed=TRUE;
char taskinfo[1024*2]={0};
char localinfo[sizeof(taskinfo)]={0};
enum{
	TASK_OPEN_TABLE,
	TASK_REFRESH_TABLES,
	TASK_OPEN_DB,
	TASK_OPEN_DB_AND_TABLE,
	TASK_CLOSE_DB,
	TASK_NEW_QUERY,
	TASK_EXECUTE_QUERY,
	TASK_UPDATE_ROW,
	TASK_GET_COL_INFO
};

int task_open_db(char *name)
{
	task=TASK_OPEN_DB;
	strncpy(taskinfo,name,sizeof(taskinfo));
	taskinfo[sizeof(taskinfo)-1]=0;
	SetEvent(event);
	return TRUE;
}
int task_open_db_and_table(char *name)
{
	task=TASK_OPEN_DB_AND_TABLE;
	strncpy(taskinfo,name,sizeof(taskinfo));
	taskinfo[sizeof(taskinfo)-1]=0;
	SetEvent(event);
	return TRUE;
}
int task_open_table(char *dbname,char *table)
{
	_snprintf(taskinfo,sizeof(taskinfo),"%s;%s",dbname,table);
	taskinfo[sizeof(taskinfo)-1]=0;
	task=TASK_OPEN_TABLE;
	SetEvent(event);
	return TRUE;
}
int task_close_db(char *dbname)
{
	if(dbname!=0){
		_snprintf(taskinfo,sizeof(taskinfo),"%s",dbname);
		taskinfo[sizeof(taskinfo)-1]=0;
		task=TASK_CLOSE_DB;
		SetEvent(event);
		return TRUE;
	}
	return FALSE;
}
int task_new_query()
{
	task=TASK_NEW_QUERY;
	SetEvent(event);
	return TRUE;

}
int task_execute_query(char *fname)
{
	taskinfo[0]=0;
	if(fname!=0 && fname[0]!=0)
		_snprintf(taskinfo,sizeof(taskinfo),"%s",fname);
	task=TASK_EXECUTE_QUERY;
	SetEvent(event);
	return TRUE;
}
int task_refresh_tables(char *str)
{
	_snprintf(taskinfo,sizeof(taskinfo),"%s",str);
	task=TASK_REFRESH_TABLES;
	SetEvent(event);
	return TRUE;
}
int task_update_record(void *win,int row,char *data)
{
	task=TASK_UPDATE_ROW;
	_snprintf(taskinfo,sizeof(taskinfo),"WIN=0x%08X;ROW=%i;DATA=%s",win,row,data);
	SetEvent(event);
	return TRUE;
}
int	task_get_col_info(void *db,char *table)
{
	task=TASK_GET_COL_INFO;
	_snprintf(taskinfo,sizeof(taskinfo),"DB=0x%08X;TABLE=%s",db,table);
	SetEvent(event);
	return TRUE;
}

int thread(HANDLE event)
{
	int id;
	if(event==0)
		return 0;
	while(TRUE){
		printf("worker thread waiting for object\n");
		id=WaitForSingleObject(event,INFINITE);
		if(id==WAIT_OBJECT_0){
			strncpy(localinfo,taskinfo,sizeof(localinfo));
			printf("db=%s\n",localinfo);
			switch(task){
			case TASK_CLOSE_DB:
				{
					void *db=0;
					if(find_db_tree(localinfo,&db)){
						mdi_remove_db(db);
						set_status_bar_text(ghstatusbar,0,"closed %s",localinfo);
					}
					else
						set_status_bar_text(ghstatusbar,0,"cant find %s",localinfo);
				}
				break;
			case TASK_OPEN_DB_AND_TABLE:
			case TASK_OPEN_DB:
				{
					void *db=0;
					SetWindowText(ghstatusbar,"opening DB");
					if(!wait_for_treeview()){
						SetWindowText(ghstatusbar,"treeview error");
						break;
					}
					acquire_db_tree(localinfo,&db);
					if(!mdi_open_db(db,TRUE)){
						char str[512];
						mdi_remove_db(db);
						_snprintf(str,sizeof(str),"Cant open %s",localinfo);
						str[sizeof(str)-1]=0;
						MessageBox(ghmainframe,str,"OPEN DB FAIL",MB_OK);
						SetWindowText(ghstatusbar,"error opening DB");
						set_focus_after_open(db);
					}
					else{
						set_focus_after_open(db);
						reassign_tables(db);
						if(task==TASK_OPEN_DB_AND_TABLE){
							char *s=0;
							s=strstr(localinfo,";TABLE=");
							if(s!=0){
								s+=sizeof(";TABLE=")-1;
								select_all_table(db,s);
							}
						}
						else{
							if(keep_closed)
								close_db(db);
							SetWindowText(ghstatusbar,"ready");
						}
					}
				}
				break;
			case TASK_GET_COL_INFO:
				{
					void *db=0;
					char table[80]={0};
					sscanf(localinfo,"DB=0x%08X;TABLE=%79s",&db,table);
					table[sizeof(table)-1]=0;
					if(db!=0){
						set_status_bar_text(ghstatusbar,0,"getting col info for %s",table);
						get_col_info(db,table);
						if(keep_closed)
							close_db(db);
						set_status_bar_text(ghstatusbar,0,"done, %s",keep_closed?"closed DB":"");
					}
				}
				break;
			case TASK_UPDATE_ROW:
				{
					void *win=0;
					int row=0;
					sscanf(localinfo,"WIN=0x%08X;ROW=%i",&win,&row);
					if(win!=0){
						char *s=strstr(localinfo,"DATA=");
						if(s!=0){
							s+=sizeof("DATA=")-1;
							update_row(win,row,s);
						}
					}
				}
				break;
			case TASK_EXECUTE_QUERY:
				{
					void *win=0;
					int result=FALSE;
					mdi_get_current_win(&win);
					if(win!=0){
						char *s=0;
						int size=0x10000;
						reopen_db(win);
						mdi_create_abort(win);
						s=malloc(size);
						if(s!=0){
							mdi_get_edit_text(win,s,size);
							result=execute_sql(win,s,TRUE);
							free(s);
						}
						mdi_destroy_abort(win);
						if(keep_closed){
							void *db=0;
							acquire_db_tree_from_win(win,&db);
							close_db(db);
						}
						printf("set focus\n");
						set_focus_after_result(win,result);
					}
				}
				break;
			case TASK_NEW_QUERY:
				{
					void *db=0;
					find_selected_tree(&db);
					if(db!=0){
						void *win=0;
						if(acquire_table_window(&win,0)){
							create_table_window(ghmdiclient,win);
							assign_db_to_table(db,win);
						}
					}
				}
				break;
			case TASK_REFRESH_TABLES:
				{
					void *db=0;
					SetWindowText(ghstatusbar,"refreshing tables");
					acquire_db_tree(localinfo,&db);
					if(!mdi_open_db(db,TRUE)){
						mdi_remove_db(db);
						SetWindowText(ghstatusbar,"error refreshing tables");
					}
					SetWindowText(ghstatusbar,"done");
				}
				break;
			case TASK_OPEN_TABLE:
				{
					char dbname[MAX_PATH*2]={0},table[80]={0},*p;
					p=strrchr(localinfo,';');
					if(p!=0){
						void *db=0;
						p[0]=0;
						strncpy(dbname,localinfo,sizeof(dbname));
						strncpy(table,p+1,sizeof(table));
						if(find_db_tree(dbname,&db)){
							void *win=0;
							if(acquire_table_window(&win,table)){
								char sql[256]={0};
								int result;
								create_table_window(ghmdiclient,win);
								open_db(db);
								assign_db_to_table(db,win);
								if(strchr(table,' ')!=0)
									_snprintf(sql,sizeof(sql),"SELECT * FROM [%s]",table);
								else
									_snprintf(sql,sizeof(sql),"SELECT * FROM %s",table);
								mdi_set_edit_text(win,sql);
								mdi_create_abort(win);
								result=execute_sql(win,sql,TRUE);
								mdi_destroy_abort(win);
								set_focus_after_result(win,result);
								if(keep_closed)
									close_db(db);

							}
							else
								free_window(win);

						}
					}
					printf("dbname=%s  table=%s\n",dbname,table);
				}
				break;
			default:
				break;
			}
		}
		ResetEvent(event);
	}
	CloseHandle(event);
}

int start_worker_thread()
{
	event=CreateEvent(NULL,TRUE,FALSE,"worker thread");
	if(event==0)
		return FALSE;
	_beginthread(thread,0,event);
	return TRUE;
}