#include <windows.h>
#include <stdio.h>
#include <rpc.h>

#include "structs.h"


extern HWND ghmainframe,ghmdiclient,ghtreeview,ghdbview,ghstatusbar;
HANDLE event=0;
HANDLE event_idle=0;
HANDLE hworker=0;
int task=0;
int keep_closed=TRUE;
char taskinfo[1024*2]={0};
char localinfo[sizeof(taskinfo)]={0};
enum{
	TASK_OPEN_TABLE,
	TASK_REFRESH_TABLES,
	TASK_REFRESH_TABLES_ALL,
	TASK_LIST_TABLES,
	TASK_LIST_PROCS,
	TASK_OPEN_DB,
	TASK_OPEN_DB_AND_TABLE,
	TASK_CLOSE_DB,
	TASK_NEW_QUERY,
	TASK_EXECUTE_QUERY,
	TASK_UPDATE_ROW,
	TASK_DELETE_ROW,
	TASK_INSERT_ROW,
	TASK_GET_COL_INFO,
	TASK_GET_INDEX_INFO,
	TASK_GET_FOREIGN_KEYS,
	TASK_INTELLISENSE_ADD_ALL
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
int task_execute_query(void *win)
{
	_snprintf(taskinfo,sizeof(taskinfo),"0x%08X",win);
	task=TASK_EXECUTE_QUERY;
	SetEvent(event);
	return TRUE;
}
int task_refresh_tables(char *str,int all)
{
	_snprintf(taskinfo,sizeof(taskinfo),"%s",str);
	task=TASK_REFRESH_TABLES;
	if(all)
		task=TASK_REFRESH_TABLES_ALL;
	SetEvent(event);
	return TRUE;
}
int task_list_tables(char *str)
{
	_snprintf(taskinfo,sizeof(taskinfo),"%s",str);
	task=TASK_LIST_TABLES;
	SetEvent(event);
	return TRUE;
}
int task_list_procs(char *str)
{
	_snprintf(taskinfo,sizeof(taskinfo),"%s",str);
	task=TASK_LIST_PROCS;
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
int task_delete_row(void *win,int row)
{
	task=TASK_DELETE_ROW;
	_snprintf(taskinfo,sizeof(taskinfo),"WIN=0x%08X;ROW=%i",win,row);
	SetEvent(event);
	return TRUE;

}
int task_insert_row(void *win,void *hlistview)
{
	if(WaitForSingleObject(event,0)==WAIT_OBJECT_0)
		return FALSE;
	task=TASK_INSERT_ROW;
	_snprintf(taskinfo,sizeof(taskinfo),"WIN=%08X,HLISTVIEW=0x%08X",win,hlistview);
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
int	task_get_index_info(void *db,char *table)
{
	task=TASK_GET_INDEX_INFO;
	_snprintf(taskinfo,sizeof(taskinfo),"DB=0x%08X;TABLE=%s",db,table);
	SetEvent(event);
	return TRUE;
}
int	task_get_foreign_keys(void *db,char *table)
{
	task=TASK_GET_FOREIGN_KEYS;
	_snprintf(taskinfo,sizeof(taskinfo),"DB=0x%08X;TABLE=%s",db,table);
	SetEvent(event);
	return TRUE;
}
int task_intellisense_add_all(void *win)
{
	task=TASK_INTELLISENSE_ADD_ALL;
	_snprintf(taskinfo,sizeof(taskinfo),"0x%08X",win);
	SetEvent(event);
	return TRUE;
}
void __cdecl thread(void *args)
{
	int id;
	HANDLE *event_list=args;
	HANDLE event=0,event_idle=0;
	if(args==0)
		return;
	event=event_list[0];
	event_idle=event_list[1];
	if(event==0 || event_idle==0)
		return;
	printf("worker thread started\n");
	while(TRUE){
		stop_thread_menu(FALSE);
		SetEvent(event_idle);
		id=WaitForSingleObject(event,INFINITE);
		if(id==WAIT_OBJECT_0){
			stop_thread_menu(TRUE);
			strncpy(localinfo,taskinfo,sizeof(localinfo));
			printf("db=%s\n",localinfo);
			switch(task){
			case TASK_CLOSE_DB:
				{
					DB_TREE *db=0;
					if(find_db_tree(localinfo,&db)){
						intelli_del_db(db->name);
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
					DB_TREE *db=0;
					char *table;
					int result=FALSE;
					table=strstr(localinfo,";TABLE=");
					if(table!=0){
						table[0]=0;
						table++;
					}
					SetWindowText(ghstatusbar,"opening DB");
					if(!wait_for_treeview()){
						SetWindowText(ghstatusbar,"treeview error");
						break;
					}
					acquire_db_tree(localinfo,&db);
					if(!mdi_open_db(db)){
						char str[512];
						mdi_remove_db(db);
						_snprintf(str,sizeof(str),"Cant open %s",localinfo);
						str[sizeof(str)-1]=0;
						MessageBox(ghmainframe,str,"OPEN DB FAIL",MB_OK);
						SetWindowText(ghstatusbar,"error opening DB");
						set_focus_after_open(db);
					}
					else{
						intelli_add_db(db->name);
						set_focus_after_open(db);
						reassign_tables(db);
						if(task==TASK_OPEN_DB_AND_TABLE &&
							table!=0 && strncmp(table,"TABLE=",sizeof("TABLE=")-1)==0){
							table+=sizeof("TABLE=")-1;
							ResetEvent(event);
							task_open_table(db->name,table);
							continue;
						}
						else{
							load_tables_if_empty(db);
							if(keep_closed)
								close_db(db);
							result=TRUE;
						}
					}
					set_status_bar_text(ghstatusbar,0,"open DB %s %s",result?"done":"failed",keep_closed?"(closed DB)":"");
				}
				break;
			case TASK_GET_COL_INFO:
				{
					void *db=0;
					char table[80]={0};
					sscanf(localinfo,"DB=0x%08X;TABLE=%79[ -~]",&db,table);
					table[sizeof(table)-1]=0;
					if(db!=0){
						int result;
						set_status_bar_text(ghstatusbar,0,"getting col info for %s",table);
						result=get_col_info(db,table);
						if(keep_closed)
							close_db(db);
						if(result)
							set_status_bar_text(ghstatusbar,0,"col info done, %s",keep_closed?"(closed DB)":"");
						set_focus_after_open(db);
					}
				}
				break;
			case TASK_GET_INDEX_INFO:
				{
					void *db=0;
					char table[80]={0};
					sscanf(localinfo,"DB=0x%08X;TABLE=%79[ -~]",&db,table);
					table[sizeof(table)-1]=0;
					if(db!=0){
						int result;
						set_status_bar_text(ghstatusbar,0,"getting index info for %s",table);
						result=get_index_info(db,table);
						if(keep_closed)
							close_db(db);
						if(result)
							set_status_bar_text(ghstatusbar,0,"index info done, %s",keep_closed?"(closed DB)":"");
						set_focus_after_open(db);
					}
				}
				break;
			case TASK_GET_FOREIGN_KEYS:
				{
					void *db=0;
					char table[80]={0};
					sscanf(localinfo,"DB=0x%08X;TABLE=%79[ -~]",&db,table);
					table[sizeof(table)-1]=0;
					if(db!=0){
						int result;
						set_status_bar_text(ghstatusbar,0,"getting foreign keys for %s",table);
						result=get_foreign_keys(db,table);
						if(keep_closed)
							close_db(db);
						if(result)
							set_status_bar_text(ghstatusbar,0,"index info done, %s",keep_closed?"(closed DB)":"");
						set_focus_after_open(db);
					}
				}
				break;
			case TASK_UPDATE_ROW:
				{
					int result=FALSE;
					void *win=0;
					int row=0;
					sscanf(localinfo,"WIN=0x%08X;ROW=%i",&win,&row);
					if(win!=0){
						char *s=strstr(localinfo,"DATA=");
						if(s!=0){
							s+=sizeof("DATA=")-1;
							result=update_row(win,row,s);
							if(keep_closed){
								void *db=0;
								acquire_db_tree_from_win(win,&db);
								close_db(db);
							}
						}
					}
					set_status_bar_text(ghstatusbar,0,"update row %s %s",
						result?"done":"failed",
						keep_closed?"(closed DB)":"");
				}
				break;
			case TASK_DELETE_ROW:
				{
					int result=FALSE;
					void *win=0;
					int row=0;
					sscanf(localinfo,"WIN=0x%08X;ROW=%i",&win,&row);
					if(win!=0){
						result=delete_row(win,row);
						if(keep_closed){
							void *db=0;
							acquire_db_tree_from_win(win,&db);
							close_db(db);
						}
					}
					set_status_bar_text(ghstatusbar,0,"delete row %s",
						result?"done":"failed",
						(win!=0 && keep_closed)?"(closed DB)":"");
				}
				break;
			case TASK_INSERT_ROW:
				{
					int result=FALSE;
					void *win=0,*hlistview=0;
					sscanf(localinfo,"WIN=%08X,HLISTVIEW=0x%08X",&win,&hlistview);
					if(win!=0 && hlistview!=0){
						int msg=IDOK;
						result=insert_row(win,hlistview);
						if(!result)
							msg=IDCANCEL;
						PostMessage(GetParent(hlistview),WM_USER,msg,hlistview);
						if(keep_closed){
							void *db=0;
							acquire_db_tree_from_win(win,&db);
							close_db(db);
						}
					}
					set_status_bar_text(ghstatusbar,0,"inserting row %s %s",
						result?"done":"failed",
						(win!=0 && hlistview!=0 && keep_closed)?"(closed DB)":"");

				}
				break;
			case TASK_EXECUTE_QUERY:
				{
					void *win=0;
					int result=FALSE;
					win=strtoul(localinfo,0,0);
					if(win==0)
						mdi_get_current_win(&win);
					if(win!=0){
						char *s=0;
						int size=0;
						reopen_db(win);
						mdi_create_abort(win);
						mdi_get_edit_text(win,&s,&size);
						if(s!=0){
							sql_remove_comments(s,size);
							result=execute_sql(win,s,TRUE);
							free(s);
						}
						mdi_destroy_abort(win);
						if(keep_closed){
							void *db=0;
							acquire_db_tree_from_win(win,&db);
							close_db(db);
						}
						set_focus_after_result(win,result);
					}
					if(!result)
						set_status_bar_text(ghstatusbar,0,"execute sql failed %s",
							keep_closed?"(closed DB)":"");
				}
				break;
			case TASK_NEW_QUERY:
				{
					void *db=0;
					find_selected_tree(&db);
					if(db!=0){
						void *win=0;
						char *tname="new_query";
						if(acquire_table_window(&win,tname)){
							create_table_window(ghmdiclient,win);
							assign_db_to_table(db,win,tname);
						}
					}
				}
				break;
			case TASK_REFRESH_TABLES:
			case TASK_REFRESH_TABLES_ALL:
				{
					int result=FALSE;
					DB_TREE *db=0;
					SetWindowText(ghstatusbar,"refreshing tables");
					if(acquire_db_tree(localinfo,&db)){
						if(!mdi_open_db(db)){
							mdi_remove_db(db);
							SetWindowText(ghstatusbar,"error opening DB");
						}
						else{
							intelli_add_db(db->name);
							result=refresh_tables(db,task==TASK_REFRESH_TABLES_ALL);
							if(keep_closed)
								close_db(db);
						}
						if(result)
							set_status_bar_text(ghstatusbar,0,"refreshed tables %s",
								keep_closed?"(closed DB)":"");
					}
					else
						SetWindowText(ghstatusbar,"error refreshing cant acquire table");
				}
				break;
			case TASK_LIST_TABLES:
				{
					int result=FALSE;
					DB_TREE *db=0;
					SetWindowText(ghstatusbar,"listing tables");
					if(acquire_db_tree(localinfo,&db)){
						if(!mdi_open_db(db)){
							mdi_remove_db(db);
							SetWindowText(ghstatusbar,"error opening DB");
						}
						else{
							intelli_add_db(db->name);
							result=get_table_list(db);
							if(keep_closed)
								close_db(db);
						}
						if(result){
							set_status_bar_text(ghstatusbar,0,"done listing tables %s",
								keep_closed?"(closed DB)":"");
						}else{
							SetWindowText(ghstatusbar,"error listing tables");
						}
					}
					else
						SetWindowText(ghstatusbar,"error refreshing cant acquire tree");
				}
				break;
			case TASK_LIST_PROCS:
				{
					int result=FALSE;
					DB_TREE *db=0;
					SetWindowText(ghstatusbar,"listing stored procedures");
					if(acquire_db_tree(localinfo,&db)){
						if(!mdi_open_db(db)){
							mdi_remove_db(db);
							SetWindowText(ghstatusbar,"error opening DB");
						}
						else{
							intelli_add_db(db->name);
							result=get_proc_list(db);
							if(keep_closed)
								close_db(db);
						}
						if(result){
							set_status_bar_text(ghstatusbar,0,"done listing stored procs %s",
								keep_closed?"(closed DB)":"");
						}else{
							SetWindowText(ghstatusbar,"error listing stored procs");
						}
					}
					else
						SetWindowText(ghstatusbar,"error refreshing cant acquire tree");
				}
				break;
			case TASK_OPEN_TABLE:
				{
					int result=FALSE;
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
								assign_db_to_table(db,win,table);
								if(strchr(table,' ')!=0)
									_snprintf(sql,sizeof(sql),"SELECT * FROM [%s]",table);
								else
									_snprintf(sql,sizeof(sql),"SELECT * FROM %s",table);
								mdi_set_edit_text(win,sql);
								mdi_create_abort(win);
								result=execute_sql(win,sql,TRUE);
								mdi_destroy_abort(win);
								load_tables_if_empty(db);
								set_focus_after_result(win,result);
								if(keep_closed)
									close_db(db);

							}
							else
								free_window(win);

						}
					}
					set_status_bar_text(ghstatusbar,0,"open table:%s %s %s",
						table,result?"success":"failed",keep_closed?"(closed DB)":"");
				}
				break;
			case TASK_INTELLISENSE_ADD_ALL:
				{
					void *win=0;
					int result=FALSE;
					win=strtoul(localinfo,0,0);
					if(win==0)
						mdi_get_current_win(&win);
					if(win!=0){
						reopen_db(win);
						mdi_create_abort(win);
						result=intellisense_add_all(win);
						mdi_destroy_abort(win);
						if(keep_closed){
							void *db=0;
							acquire_db_tree_from_win(win,&db);
							close_db(db);
						}
						set_focus_after_result(win,FALSE);
					}
					if(!result)
						set_status_bar_text(ghstatusbar,0,"intellisense add all failed %s",
							keep_closed?"(closed DB)":"");
				}
				break;
			default:
				break;
			}
		}
		ResetEvent(event);
	}
	CloseHandle(event);
	hworker=0;
}
int get_guid_str(char *pre,char *str,int str_len)
{
	char guid[20]={0};
	_snprintf(guid,sizeof(guid),"%04X",rand());
	_snprintf(guid,sizeof(guid),"%s%04X",guid,rand());
	_snprintf(guid,sizeof(guid),"%s%04X",guid,rand());
	_snprintf(guid,sizeof(guid),"%s%04X",guid,rand());
	guid[sizeof(guid)]=0;
	_snprintf(str,str_len,"%s%s",pre,guid);
	str[str_len-1]=0;
	return TRUE;
}
int start_worker_thread()
{
	char str[80]={0};
	static HANDLE *events[2]={0,0};
	void *args=0;
	get_guid_str("worker thread event:",str,sizeof(str));
	if(event!=0)
		CloseHandle(event);
	event=CreateEvent(NULL,TRUE,FALSE,str);
	if(event==0)
		return FALSE;
	get_guid_str("worker thread idle:",str,sizeof(str));
	if(event_idle!=0)
		CloseHandle(event_idle);
	event_idle=CreateEvent(NULL,TRUE,FALSE,str);
	if(event_idle==0)
		return FALSE;
	events[0]=event;
	events[1]=event_idle;
	args=events;
	hworker=_beginthread(thread,0,args);
	if(hworker==-1){
		MessageBox(NULL,"Failed to create worker thread","Error",MB_OK|MB_SYSTEMMODAL);
		hworker=0;
	}
	return TRUE;
}
int terminate_worker_thread()
{
	int result=FALSE;
	if(hworker!=0){
		if(TerminateThread(hworker,0)!=0){
			erase_sql_handles();
			printf("terminated thread %08X\n",hworker);
			start_worker_thread();
			result=TRUE;
		}
	}
	return result;
}
int wait_worker_idle(int timeout,int showmsg)
{
	int result=FALSE;
	int sig=0;
	while(TRUE){
		sig=WaitForSingleObject(event_idle,timeout);
		if(sig==WAIT_OBJECT_0){
			ResetEvent(event_idle);
			result=TRUE;
			break;
		}
		else if(showmsg){
			if(IDYES==MessageBox(NULL,"wait some more?","waiting for thread",MB_YESNO|MB_SYSTEMMODAL))
				continue;
			else{
				result=TRUE;
				break;
			}
		}
		else
			break;
	};
	return result;
}
int automation_busy=0;
int automation_thread()
{
	struct ATASK{
		char *table;
		char *export;
	};
	struct ATASK list[]={
		{"SELECT STORENUM,TICK_DATETIME,REGNUM,TICKET,SHIFT_SEQ,ELSDATE,CASHIER,SOURCE_DEV,SOURCE_UNT,CUSTOMER,CUST_DEMO,CUST_DATE,TRANS_TYPE,CUST_TYPE,TOTAL_AMT,TOTAL_TAX,FUEL_AMT,FUEL_VOL,SCAN_COUNT,PLU_COUNT,DEPT_COUNT,NFND_COUNT,DIRC_COUNT,TOTAL_FS,TOTAL_WIC,TOTAL_DISC,ELAPSED,DELAY,MERCH_CUST,GAS_CUST,PRINTED,DRAWERTIME,KV_SENT,COPIED,DOB_TYPE,VOID_COUNT,POS_TYPE FROM H_TICKET ORDER BY TICKET,TICK_DATETIME","TICKET.txt"},
		{"SELECT STORENUM,tick_datetime,REGNUM,TICKET,ORDINAL,ELSDATE,TENDER_ID,SUB_ID,AMOUNT,TENDERED,FEE,ACCT_TYPE,ACCOUNT,TRANS_ID,REF,AUTH_CODE,EXP_DATE,INTERFACED,BATCH,SURCHARGE,FMT_ACCT,IV,SIG_STATUS FROM H_TENDER ORDER by TICKET,ORDINAL","tender.txt"},
		{"SELECT STORENUM,tick_datetime,REGNUM,TICKET,ORD_ASSOC,ELSDATE,TAX_TABLE,TAX,TAX_TOTAL,FS_TAXOFF,FS_TAX_TOT,WC_TAX_TOT FROM H_TAX ORDER by TICKET,tick_datetime","tax.txt"},
		{"SELECT STORENUM,CASHIER,start_datetime,ELSDATE,stop_datetime,SHIFT_SEQ,REGNUM,DRAWER,SOURCE,NRGT,TOTAL_OVRR,TOTAL_SLS,OVER_SHORT,RECEIPT,DROPPED,LOAN,shift,POS_TYPE FROM H_SHIFT ORDER by start_datetime","shift.txt"},
		{"SELECT STORENUM,tick_datetime,REGNUM,SHIFT_SEQ,ELSDATE,DESCRIPTION,AMOUNT,TYPE,TRAN_CODE,TEN_TYPE,ID,ACCOUNT,CAP_ACCT,ACCT_NUM,COPIED,POS_TYPE FROM H_POUT ORDER by tick_datetime","pout.txt"},
		{"SELECT STORENUM,TICK_DATETIME,REGNUM,TICKET,ORDINAL,ELSDATE,ITEM_TYPE,COMBO_NUM,ITEMNUM,DESCRIPTION,MODIFIER,PRICE,AMOUNT,QTY,TAX_TABLE,FOOD_STAMP,DISCOUNT,AGE,DEPARTMENT,PROMO_CODE,KV_SENT,PARENT_ORD FROM H_ITEM ORDER by TICKET,TICK_DATETIME","item.txt"},
		{"SELECT STORENUM,tick_datetime,REGNUM,TICKET,ORDINAL,ELSDATE,FUEL_TYPE,GRADE,FP,VOLUME,SALES,AMOUNT,PRICE,DISCOUNT,ticket_id,TRANSNUM,MOS,MOP,INIT_MOP FROM H_FUEL ORDER by TICKET,tick_datetime","fuel.txt"},
		{"SELECT STORENUM,TICK_DATETIME,REGNUM,TICKET,SHIFT_SEQ,ELSDATE,CASHIER,SOURCE_DEV,SOURCE_UNT,CUSTOMER,TRANS_TYPE,ITEMNUM,MODIFIER,[COUNT],AMOUNT,FP,FUEL_AMT,FUEL_VOL,DESCRIPTION,COPIED,LOCATION,POS_TYPE FROM H_EVENT ORDER by TICKET,TICK_DATETIME","event.txt"},
		{"SELECT STORENUM,SHIFT_SEQ,TICKET,REGNUM,TICK_DATETIME,ID,TYPE,DESCRIPTION,TOTALAMT,ORDINAL,DISCAMT,MAXGAL,PERCENTOFF FROM H_DISC ORDER by TICKET,SHIFT_SEQ","disc.txt"},
		{"SELECT tick_datetime,SHIFT_SEQ,ID,ELSDATE,ACCT_AREA,ACCT_TYPE,AMOUNT,DONE,STORENUM,REGNUM,LOCATION,POS_TYPE FROM H_ACCT ORDER by tick_datetime,SHIFT_SEQ","acct.txt"}

	};
	automation_busy=1;
	printf("auto thread started\n");
	{
		int i,max;
		HWND httip=0;
		max=get_max_table_windows();
		for(i=0;i<max;i++){
			HWND hwnd=0;
			get_win_hwnds(i,&hwnd,0,0);
			if(hwnd!=0){
				SendMessage(hwnd,WM_CLOSE,0,0);
			}
		}
		wait_worker_idle(100,FALSE);
		task_new_query();
		for(i=0;i<sizeof(list)/sizeof(struct ATASK);i++){
			void *win=0;
			RECT rect={0};
			int x,y;
			GetWindowRect(ghmainframe,&rect);
			x=(rect.left+rect.right)/2;
			y=(rect.bottom+rect.top)/2;
			create_tooltip(ghmainframe,"busy - press escape to exit",x,y,&httip);
			while(FALSE==wait_worker_idle(100,FALSE)){
				if(GetAsyncKeyState(VK_ESCAPE)&0x8000)
					break;
			};
			printf("running task\n");
			mdi_get_current_win(&win);
			mdi_set_edit_text(win,list[i].table);
			task_execute_query(win);
			if(httip){
				destroy_tooltip(httip);
				httip=0;
			}

			{
				HWND hwnd=0;
				get_win_hwnds(0,0,0,&hwnd);
				if(hwnd){
					char *path="C:\\temp\\PJR";
					printf("exporting\n");
					if(!is_path_directory(path)){
						MessageBox(NULL,"error","path no exist",MB_OK|MB_SYSTEMMODAL);
						break;
					}
					else{
						SetCurrentDirectory(path);
						export_listview(hwnd,list[i].export);
					}
				}
			}
		}
	}
	automation_busy=0;
	printf("auto thread finished\n");
	return 0;
}