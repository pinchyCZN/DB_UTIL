#include <windows.h>
#include <stdio.h>

extern HWND ghmainframe,ghmdiclient,ghtreeview,ghstatusbar;
HANDLE event;
int task=0;
int keep_closed=TRUE;
char taskinfo[1024*2]={0};
char localinfo[sizeof(taskinfo)]={0};
enum{
	TASK_OPEN_TABLE,
	TASK_REFRESH_TABLES,
	TASK_OPEN_DB,
	TASK_CLOSE_DB,
	TASK_NEW_QUERY,
	TASK_EXECUTE_QUERY,
	TASK_UPDATE_ROW
};

int task_open_db(char *name)
{
	task=TASK_OPEN_DB;
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
int task_execute_query()
{
	task=TASK_EXECUTE_QUERY;
	SetEvent(event);
	return TRUE;
}
int task_refresh_tables()
{
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
int thread(HANDLE event)
{
	int id;
	if(event==0)
		return 0;
	while(TRUE){
		printf("waiting for object\n");
		id=WaitForSingleObject(event,INFINITE);
		if(id==WAIT_OBJECT_0){
			strncpy(localinfo,taskinfo,sizeof(localinfo));
			printf("db=%s\n",localinfo);
			switch(task){
			case TASK_CLOSE_DB:
				{
				void *db=0;
				if(find_db_tree(localinfo,&db))
					mdi_remove_db(db);
				}
				break;
			case TASK_OPEN_DB:
				{
				void *db=0;
				SetWindowText(ghstatusbar,"opening DB");
				acquire_db_tree(localinfo,&db);
				if(!mdi_open_db(db,TRUE)){
					char str[80];
					mdi_remove_db(db);
					_snprintf(str,sizeof(str),"Cant open %s",localinfo);
					MessageBox(ghmainframe,str,"OPEN DB FAIL",MB_OK);
					SetWindowText(ghstatusbar,"error opening DB");
				}
				else
					reassign_tables(db);
					if(keep_closed)
						close_db(db);
					SetWindowText(ghstatusbar,"ready");
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
					destroy_intellisense(win);
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
								create_table_window(ghmdiclient,win);
								open_db(db);
								assign_db_to_table(db,win);
								_snprintf(sql,sizeof(sql),"SELECT * FROM %s",table);
								mdi_set_edit_text(win,sql);
								mdi_create_abort(win);
								execute_sql(win,sql,TRUE);
								mdi_destroy_abort(win);
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