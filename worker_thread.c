#include <windows.h>
#include <stdio.h>

extern HWND ghmainframe,ghmdiclient,ghtreeview;
HANDLE event;
int task=0;
char taskinfo[1024*2]={0};
char localinfo[sizeof(taskinfo)]={0};
enum{
	TASK_OPEN_TABLE,
	TASK_OPEN_DB,
	TASK_CLOSE_DB,
	TASK_NEW_QUERY,
	TASK_EXECUTE_QUERY
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
					mdi_close_db(db);
				}
				break;
			case TASK_OPEN_DB:
				{
				void *db=0;
				acquire_db_tree(localinfo,&db);
				mdi_open_db(db,localinfo);
				}
				break;
			case TASK_EXECUTE_QUERY:
				{
				void *win=0;
				mdi_get_current_win(&win);
				if(win!=0){
					char *s=0;
					int size=0x10000;
					mdi_create_abort(win);
					mdi_clear_listview(win);
					s=malloc(size);
					if(s!=0){
						mdi_get_edit_text(win,s,size);
						execute_sql(win,s);
						free(s);
					}
					mdi_destroy_abort(win);
				}
				}
				break;
			case TASK_NEW_QUERY:
				{
				void *db=0;
				find_selected_tree(&db);
				if(db!=0){
					void *win=0;
					if(acquire_table_window(&win)){
						create_table_window(ghmdiclient,win);
						assign_db_to_table(db,win);
					}
				}
				}
				break;
			case TASK_OPEN_TABLE:
				{
					char dbname[80]={0},table[80]={0},*p;
					p=strchr(localinfo,';');
					if(p!=0){
						void *db=0;
						p[0]=0;
						strncpy(dbname,localinfo,sizeof(dbname));
						strncpy(table,p+1,sizeof(table));
						if(find_db_tree(dbname,&db)){
							void *win=0;
							if(acquire_table_window(&win)){
								char sql[256]={0};
								create_table_window(ghmdiclient,win);
								assign_db_to_table(db,win);
								_snprintf(sql,sizeof(sql),"SELECT * FROM %s;",table);
								mdi_set_edit_text(win,sql);
								mdi_create_abort(win);
								execute_sql(win,sql);
								mdi_destroy_abort(win);
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