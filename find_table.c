#include <windows.h>
#include <commctrl.h>
#include "resource.h"

extern HINSTANCE ghinstance;
extern HWND ghdbview;


static int fill_listbox(HWND hwnd,HWND htreeview)
{
	int result=0;
	char str[80]={0};
	char *rootname,*entry;
	int rootname_size=2048;
	int entry_size=4096;
	GetWindowText(GetDlgItem(hwnd,IDC_EDIT1),str,sizeof(str));
	if(str[0]==0)
		return result;
	rootname=malloc(rootname_size);
	entry=malloc(entry_size);
	if(rootname!=0 && entry!=0){
		HTREEITEM hroot;
		HWND hlist;
		int len;
		len=strlen(str);
		hlist=GetDlgItem(hwnd,IDC_LIST1);
		SendMessage(hlist,LB_RESETCONTENT,0,0);
		hroot=TreeView_GetRoot(htreeview);
		while(hroot!=0){
			HTREEITEM hchild;
			rootname[0]=0;
			tree_get_item_text(hroot,rootname,rootname_size);
			hchild=TreeView_GetChild(htreeview,hroot);
			while(hchild!=0){
				char item[80]={0};
				tree_get_item_text(hchild,item,sizeof(item));
				if(strnicmp(item,str,len)==0){
					entry[0]=0;
					_snprintf(entry,entry_size,"%s\t  %s",item,rootname);
					SendMessage(hlist,LB_ADDSTRING,0,entry);
				}
				hchild=TreeView_GetNextSibling(htreeview,hchild);
			}
			hroot=TreeView_GetNextSibling(htreeview,hroot);
		}
	}
	if(rootname!=0)
		free(rootname);
	if(entry!=0)
		free(entry);
	return result;
}
int open_selection(HWND hwnd,HWND htreeview)
{
	int result=FALSE;
	HWND hlist;
	int sel=-1;
	char *str,*table,*db;
	int str_size=4096,table_size=256,db_size=4096;

	str=malloc(str_size);
	table=malloc(table_size);
	db=malloc(db_size);
	if(str==0 || table==0 || db==0)
		goto exit;

	hlist=GetDlgItem(hwnd,IDC_LIST1);
	sel=SendMessage(hlist,LB_GETCURSEL,0,0);
	if(sel<0){
		int count=SendMessage(hlist,LB_GETCOUNT,0,0);
		if(count==1)
			sel=0;
	}
	if(sel>=0){
		str[0]=0;
		SendMessage(hlist,LB_GETTEXT,sel,str);
		if(str[0]!=0){
			table[0]=0;
			db[0]=0;
			sscanf(str,"%255[ -~]  %4095[ -~]",table,db);
			if(table[0]!=0 && db[0]!=0){
				HTREEITEM hroot;
				hroot=TreeView_GetRoot(htreeview);
				while(hroot!=0){
					char *rootname=str;
					int rootname_size=str_size;
					rootname[0]=0;
					tree_get_item_text(hroot,rootname,rootname_size);
					if(rootname[0]!=0){
						if(stricmp(rootname,db)==0){
							HTREEITEM hchild;
							hchild=TreeView_GetChild(htreeview,hroot);
							while(hchild!=0){
								char *item=str;
								int item_size=str_size;
								item[0]=0;
								tree_get_item_text(hchild,item,item_size);
								if(item[0]!=0){
									if(stricmp(table,item)==0){
										TreeView_SelectItem(htreeview,hchild);
										PostMessage(ghdbview,WM_USER,IDC_TABLE_ITEM,0);
										result=TRUE;
										goto exit;
									}
								}
								hchild=TreeView_GetNextSibling(htreeview,hchild);	
							}
						}
					}
					hroot=TreeView_GetNextSibling(htreeview,hroot);
				}
			}
		}
	}
exit:
	if(str!=0)
		free(str);
	if(table!=0)
		free(table);
	if(db!=0)
		free(db);
	return result;
}
LRESULT CALLBACK find_table_proc(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam)
{
	static HWND htreeview;
	if(FALSE)
	{
		static DWORD tick=0;
		if((GetTickCount()-tick)>500)
			printf("--\n");
		printf("i");
		print_msg(msg,lparam,wparam,hwnd);
		tick=GetTickCount();
	}
	switch(msg){
	case WM_INITDIALOG:
		if(lparam==0)
			EndDialog(hwnd,0);
		htreeview=lparam;
		{
			RECT rect={0};
			GetWindowRect(htreeview,&rect);
			SetWindowPos(hwnd,NULL,rect.right,rect.top,NULL,NULL,SWP_NOSIZE|SWP_NOZORDER);

		}
		{
			HFONT hfont;
			hfont=SendMessage(htreeview,WM_GETFONT,0,0);
			if(hfont!=0){
				SendDlgItemMessage(hwnd,IDC_EDIT1,WM_SETFONT,hfont,MAKELPARAM(TRUE,0));
				SendDlgItemMessage(hwnd,IDC_LIST1,WM_SETFONT,hfont,MAKELPARAM(TRUE,0));
			}

		}
		SetFocus(GetDlgItem(hwnd,IDC_EDIT1));
		break;
	case WM_COMMAND:
		switch(LOWORD(wparam)){
		case IDC_LIST1:
			switch(HIWORD(wparam)){
			case LBN_DBLCLK:
				if(open_selection(hwnd,htreeview))
					EndDialog(hwnd,0);
			}
			break;
		case IDC_EDIT1:
			if(HIWORD(wparam)==EN_CHANGE)
				fill_listbox(hwnd,htreeview);
			break;
		case IDOK:
			{
				HWND hfocus=GetFocus();
				if(hfocus==GetDlgItem(hwnd,IDC_EDIT1)){
					int count;
					fill_listbox(hwnd,htreeview);
					count=SendDlgItemMessage(hwnd,IDC_LIST1,LB_GETCOUNT,0,0);
					if(count==1)
						open_selection(hwnd,htreeview);
				}
				else if(hfocus==GetDlgItem(hwnd,IDC_LIST1))
					open_selection(hwnd,htreeview);
			}
			break;
		case IDCANCEL:
			EndDialog(hwnd,0);
			break;
		}
		break;
	}
	return 0;
}

int do_tree_find(HWND htreeview)
{
	HTREEITEM hitem;
	if(htreeview==0)
		return FALSE;
	hitem=TreeView_GetRoot(htreeview);
	if(hitem==0)
		return FALSE;
	return DialogBoxParam(ghinstance,MAKEINTRESOURCE(IDD_FIND_TABLE),htreeview,find_table_proc,htreeview);
}