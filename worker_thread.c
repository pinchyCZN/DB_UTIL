#include <windows.h>
#include <stdio.h>

extern HWND ghmainframe,ghmdiclient,ghtreeview;
HANDLE event;
int task=0;
char taskinfo[1024*2]={0};
char localinfo[sizeof(taskinfo)]={0};
enum{
	TASK_OPEN_TABLE,
	TASK_OPEN_DB
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
	task=TASK_OPEN_TABLE;
	_snprintf(taskinfo,sizeof(taskinfo),"%s;%s",dbname,table);
	taskinfo[sizeof(taskinfo)-1]=0;
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
			case TASK_OPEN_DB:
				{
				void *win=0,*db=0;
				acquire_db_tree(localinfo,&db);
				if(mdi_open_db(db,localinfo))
					win=0;
					if(acquire_db_window(&win))
						create_db_window(ghmdiclient,win);
					else
						free_window(win);
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