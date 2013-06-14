HWND ghtreeview;
static HMENU db_menu=0;
static HMENU table_menu=0;
enum {
	CMD_CLOSEDB=10000,
	CMD_DB_INFO,
	CMD_DB_REFRESH_TABLES,
	CMD_SELECTTOP,
	CMD_SELECTALL,
	CMD_TABLE_STRUCT
};
int insert_root(char *name,int lparam)
{
	TV_INSERTSTRUCT tvins;
	TV_ITEM tvi;
	tvi.mask=TVIF_TEXT|TVIF_PARAM;
	tvi.pszText=name;
	tvi.lParam=lparam;

	tvins.hParent=TVI_ROOT;
	tvins.hInsertAfter=TVI_SORT;
	tvins.item=tvi;
	return TreeView_InsertItem(ghtreeview,&tvins);
}
int insert_item(char *name,HTREEITEM hparent,int lparam)
{
	TV_INSERTSTRUCT tvins;
	TV_ITEM tvi;
	if(hparent==0)
		return 0;
	tvi.mask=TVIF_TEXT|TVIF_PARAM;
	tvi.pszText=name;
	tvi.lParam=lparam;
 
	tvins.hParent=hparent;
	tvins.hInsertAfter=TVI_SORT;
	tvins.item=tvi;
 	return TreeView_InsertItem(ghtreeview,&tvins);
}
int rename_tree_item(HTREEITEM hitem,char *str)
{
	TV_ITEM tvi;
	if(hitem!=0 && str!=0){
		memset(&tvi,0,sizeof(tvi));
		tvi.hItem=hitem;
		tvi.mask=TVIF_TEXT;
		tvi.pszText=str;
		return TreeView_SetItem(ghtreeview,&tvi);
	}
	return FALSE;
}
int tree_get_item_text(HTREEITEM hitem,char *str,int len)
{
	TV_ITEM tvi;
	if(hitem!=0 && str!=0 && len>0){
		memset(&tvi,0,sizeof(tvi));
		tvi.hItem=hitem;
		tvi.mask=TVIF_TEXT;
		tvi.pszText=str;
		tvi.cchTextMax=len;
		return TreeView_GetItem(ghtreeview,&tvi);
	}
	return FALSE;
}
int tree_get_root(char *name,HANDLE *hroot)
{
	HTREEITEM h;
	h=TreeView_GetRoot(ghtreeview);
	while(h!=0){
		char str[MAX_PATH]={0};
		if(tree_get_item_text(h,str,sizeof(str))){
			if(stricmp(str,name)==0){
				*hroot=h;
				return TRUE;
			}
		}
		h=TreeView_GetNextSibling(ghtreeview,h);
	}
	if(hroot!=0)
		*hroot=0;
	return FALSE;
}
int tree_find_focused_root(HANDLE *hroot)
{
	HTREEITEM h;
	h=TreeView_GetSelection(ghtreeview);
	while(h!=0){
		HTREEITEM hparent;
		hparent=TreeView_GetParent(ghtreeview,h);
		if(hparent==0)
			break;
		else
			h=hparent;
	}
	if(h!=0 && hroot!=0){
		*hroot=h;
		return TRUE;
	}
	else
		return FALSE;
}
int tree_get_info(HTREEITEM hitem,char *str,int str_size,int *lparam)
{
	int result=FALSE;
	if(hitem!=0){
		TV_ITEM tvi;
		memset(&tvi,0,sizeof(tvi));
		tvi.hItem=hitem;
		if(str!=0 && str_size>0){
			tvi.mask|=TVIF_TEXT;
			tvi.pszText=str;
			tvi.cchTextMax=str_size;
		}
		if(lparam!=0)
			tvi.mask|=TVIF_PARAM;
		if(TreeView_GetItem(ghtreeview,&tvi)){
			if(lparam!=0)
				*lparam=tvi.lParam;
			result=TRUE;
		}
	}
	return result;
}
int tree_get_db_table(HTREEITEM hitem,char *db,int db_size,char *table,int table_size,int *item_type)
{
	int result=FALSE;
	if(hitem!=0){
		char str[MAX_PATH]={0};
		int type=0;
		if(tree_get_info(hitem,str,sizeof(str),&type)){
			if(type==IDC_TABLE_ITEM){
				HTREEITEM hparent;
				if(table!=0 && table_size>0)
					strncpy(table,str,table_size);
				if(item_type!=0)
					*item_type=type;
				hparent=TreeView_GetParent(ghtreeview,hitem);
				result=TRUE;
				if(hparent!=0 && db!=0 && db_size>0){
					str[0]=0;
					if(tree_get_info(hparent,str,sizeof(str),&type))
						if(type==IDC_DB_ITEM)
							strncpy(db,str,db_size);
				}

			}
			else if(type==IDC_DB_ITEM && db!=0 && db_size>0){
				strncpy(db,str,db_size);
				if(item_type!=0)
					*item_type=type;
				result=TRUE;
			}
		}
	}
	return result;
}
int tree_delete_all_child(HTREEITEM hroot)
{
	HTREEITEM hitem;
	hitem=TreeView_GetChild(ghtreeview,hroot);
	while(hitem!=0){
		HTREEITEM hsib;
		hsib=TreeView_GetNextSibling(ghtreeview,hitem);
		TreeView_DeleteItem(ghtreeview,hitem);
		hitem=hsib;
	}
	return TRUE;
}
int tree_delete_all()
{
	return TreeView_DeleteAllItems(ghtreeview);
}
int expand_root(HTREEITEM hitem)
{
	return TreeView_Expand(ghtreeview,hitem,TVE_EXPAND);
}
int create_treeview_menus()
{
	if(db_menu!=0)DestroyMenu(db_menu);
	if(db_menu=CreatePopupMenu()){
		InsertMenu(db_menu,0xFFFFFFFF,MF_BYPOSITION|MF_STRING,CMD_DB_INFO,"DB info");
		InsertMenu(db_menu,0xFFFFFFFF,MF_BYPOSITION|MF_STRING,CMD_DB_REFRESH_TABLES,"Refresh tables");
		InsertMenu(db_menu,0xFFFFFFFF,MF_BYPOSITION|MF_SEPARATOR,0,0);
		InsertMenu(db_menu,0xFFFFFFFF,MF_BYPOSITION|MF_STRING,CMD_CLOSEDB,"close DB");
	}
	if(table_menu!=0)DestroyMenu(table_menu);
	if(table_menu=CreatePopupMenu()){
		InsertMenu(table_menu,0xFFFFFFFF,MF_BYPOSITION|MF_STRING,CMD_SELECTTOP,"SELECT * TOP 1000");
		InsertMenu(table_menu,0xFFFFFFFF,MF_BYPOSITION|MF_STRING,CMD_SELECTALL,"SELECT * ALL");
		InsertMenu(table_menu,0xFFFFFFFF,MF_BYPOSITION|MF_STRING,CMD_TABLE_STRUCT,"table struct");
		InsertMenu(table_menu,0xFFFFFFFF,MF_BYPOSITION|MF_SEPARATOR,0,0);
		InsertMenu(table_menu,0xFFFFFFFF,MF_BYPOSITION|MF_STRING,CMD_DB_REFRESH_TABLES,"Refresh tables");
		InsertMenu(table_menu,0xFFFFFFFF,MF_BYPOSITION|MF_STRING,CMD_CLOSEDB,"close DB");
	}
	return TRUE;
}
int test_items()
{
	HTREEITEM h;
	int i,j;
	for(j=0;j<10;j++){
		char str[80];
		sprintf(str,"root%i",j);
		insert_root(str,IDC_DB_ITEM);
		for(i=0;i<4;i++){
			sprintf(str,"root%i",j);
			tree_get_root(str,&h);
			sprintf(str,"item%i",i);
			insert_item(str,h,IDC_TABLE_ITEM);
		}
	}
	tree_get_root("root1",&h);
	TreeView_DeleteItem(ghtreeview,h);	
}
int open_selected_table(HWND htreeview)
{
	HTREEITEM hitem=0;
	hitem=TreeView_GetSelection(htreeview);
	if(hitem!=0){
		char db[80]={0},table[80]={0};
		int type=0;
		tree_get_db_table(hitem,db,sizeof(db),table,sizeof(table),&type);
		if(type==IDC_TABLE_ITEM){
			task_open_table(db,table);
			printf("---------------db=%s,item=%s\n",db,table);
			return TRUE;
		}
	}
	return FALSE;
}
static WNDPROC wporigtreeview=0;
LRESULT APIENTRY sc_treeview(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam)
{
	static DWORD tick=0;
	if(FALSE)
	//if(msg!=WM_MOUSEFIRST&&msg!=WM_NCHITTEST&&msg!=WM_SETCURSOR&&msg!=WM_ENTERIDLE&&msg!=WM_NOTIFY)
	if(msg!=WM_NCHITTEST&&msg!=WM_SETCURSOR&&msg!=WM_ENTERIDLE&&msg!=WM_MOUSEMOVE&&msg!=WM_NOTIFY)
	{
		if((GetTickCount()-tick)>500)
			printf("--\n");
		printf("t");
		print_msg(msg,lparam,wparam,hwnd);
		tick=GetTickCount();
	}
	switch(msg){
	case WM_RBUTTONDOWN:
		{
			TV_HITTESTINFO ht={0};
			ht.pt.x=LOWORD(lparam); //x
			ht.pt.y=HIWORD(lparam); 
			TreeView_HitTest(hwnd,&ht);
			if(ht.hItem!=0)
				TreeView_SelectItem(hwnd,ht.hItem);
				
		}
		break;
	case WM_LBUTTONDBLCLK:
		open_selected_table(hwnd);
		break;
	}
    return CallWindowProc(wporigtreeview,hwnd,msg,wparam,lparam);

}
LRESULT CALLBACK dbview_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	static int create_tree=FALSE;
	static DWORD tick=0;
	if(FALSE)
	//if(msg!=WM_MOUSEFIRST&&msg!=WM_NCHITTEST&&msg!=WM_SETCURSOR&&msg!=WM_ENTERIDLE&&msg!=WM_NOTIFY)
	if(msg!=WM_NCHITTEST&&msg!=WM_SETCURSOR&&msg!=WM_ENTERIDLE&&msg!=WM_MOUSEMOVE) //&&msg!=WM_NOTIFY)
	{
		if((GetTickCount()-tick)>500)
			printf("--\n");
		printf("db");
		print_msg(msg,lparam,wparam,hwnd);
		tick=GetTickCount();
	}
	switch(msg){
	case WM_CREATE:
		PostMessage(hwnd,WM_USER,0,0);
		break;
	case WM_CONTEXTMENU:
		{
			POINT p={0};
			TV_HITTESTINFO ht={0};
			int type=0;
			HMENU hmenu=0;
			p.x=LOWORD(lparam);
			p.y=HIWORD(lparam);
			ScreenToClient(ghtreeview,&p);
			ht.pt.x=p.x;
			ht.pt.y=p.y;
			TreeView_HitTest(ghtreeview,&ht);
			if(ht.hItem!=0)
				tree_get_info(ht.hItem,0,0,&type);
			if(type==IDC_DB_ITEM)
				hmenu=db_menu;
			else
				hmenu=table_menu;
			TrackPopupMenu(hmenu,TPM_LEFTALIGN,LOWORD(lparam),HIWORD(lparam),0,hwnd,NULL);
		}
		break;
	case WM_NOTIFY:
		{
		NMHDR *nm=lparam;
		if(nm!=0)
		switch(nm->idFrom){
		case IDC_TABLES:
			switch(nm->code){
			case TVN_KEYDOWN:
				{
				TV_KEYDOWN *tvn=lparam;
				switch(tvn->wVKey){
				case VK_RETURN:
					open_selected_table(ghtreeview);
					printf("%08x %08X %08X\n",nm->code,nm->hwndFrom,nm->idFrom);
					break;
				}
				}
			}
			break;
		}
		}
		break;
	case WM_COMMAND:
		switch(LOWORD(wparam)){
		case CMD_CLOSEDB:
			{
				char str[MAX_PATH]={0};
				tree_get_db_table(TreeView_GetSelection(ghtreeview),str,sizeof(str),0,0,0);
				if(str[0]!=0){
					task_close_db(str);
					printf("close db\n");
				}
			}
			break;
		case CMD_DB_REFRESH_TABLES:
			{
				char str[MAX_PATH]={0};
				tree_get_db_table(TreeView_GetSelection(ghtreeview),str,sizeof(str),0,0,0);
				if(str[0]!=0){
					task_refresh_tables(str);
					printf("reload tables\n");
				}
			}
			break;
		case CMD_DB_INFO:
		case CMD_SELECTTOP:
		case CMD_SELECTALL:
		case CMD_TABLE_STRUCT:
			break;
		}
		break;
	case WM_USER:
		if(create_tree==FALSE){
			RECT rect;
			GetClientRect(hwnd,&rect);
			ghtreeview=CreateWindow(WC_TREEVIEW, 
										 "dbtreeview",
										 WS_TABSTOP|WS_CHILD|WS_VISIBLE|TVS_LINESATROOT|TVS_HASLINES|TVS_HASBUTTONS|TVS_SHOWSELALWAYS,
										 0, 0,
										 rect.right,
										 rect.bottom,
										 hwnd,
										 IDC_TABLES,
										 (HINSTANCE)GetWindowLong(hwnd,GWL_HINSTANCE),
										 NULL);
			if(ghtreeview!=0)
				wporigtreeview=SetWindowLong(ghtreeview,GWL_WNDPROC,(LONG)sc_treeview);
			create_tree=TRUE;
		}
		break;
	case WM_SIZE:
		{
			RECT rect;
			GetClientRect(hwnd,&rect);
			MoveWindow(GetDlgItem(hwnd,IDC_TABLES),0,0,rect.right,rect.bottom,TRUE);
		}
		break;
	}
	return DefWindowProc(hwnd,msg,wparam,lparam);
}

