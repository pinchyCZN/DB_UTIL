HWND ghtreeview;

int insert_root(char *name)
{
	TV_INSERTSTRUCT tvins;
	TV_ITEM tvi;
	tvi.mask=TVIF_TEXT;
	tvi.pszText=name;

	tvins.hParent=TVI_ROOT;
	tvins.hInsertAfter=TVI_SORT;
	tvins.item=tvi;
	return TreeView_InsertItem(ghtreeview,&tvins);
}
int insert_item(char *name,HTREEITEM hparent)
{
	TV_INSERTSTRUCT tvins;
	TV_ITEM tvi;
	if(hparent==0)
		return 0;
	tvi.mask=TVIF_TEXT;
	tvi.pszText=name;
 
	tvins.hParent=hparent;
	tvins.hInsertAfter=TVI_SORT;
	tvins.item=tvi;
 	return TreeView_InsertItem(ghtreeview,&tvins);
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
		TV_ITEM tvi;
		char str[MAX_PATH]={0};
		memset(&tvi,0,sizeof(tvi));
		tvi.hItem=h;
		tvi.mask=TVIF_TEXT;
		tvi.pszText=str;
		tvi.cchTextMax=sizeof(str);
		TreeView_GetItem(ghtreeview,&tvi);
		if(tvi.pszText!=0){
			if(stricmp(tvi.pszText,name)==0){
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
int test_items()
{
	HTREEITEM h;
	int i,j;
	for(j=0;j<10;j++){
		char str[80];
		sprintf(str,"root%i",j);
		insert_root(str);
		for(i=0;i<4;i++){
			sprintf(str,"root%i",j);
			tree_get_root(str,&h);
			sprintf(str,"item%i",i);
			insert_item(str,h);
		}
	}
	tree_get_root("root1",&h);
	TreeView_DeleteItem(ghtreeview,h);	
}
static WNDPROC wporigtreeview=0;
LRESULT APIENTRY sc_treeview(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam)
{
	static DWORD tick=0;
	//if(FALSE)
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
	case WM_LBUTTONDBLCLK:
		{
		HTREEITEM hitem=0;
		hitem=TreeView_GetSelection(hwnd);
			if(hitem!=0){
				HTREEITEM hroot=TreeView_GetParent(hwnd,hitem);
				if(hroot!=0){
					char root[80]={0},table[80]={0};
					tree_get_item_text(hroot,root,sizeof(root));
					tree_get_item_text(hitem,table,sizeof(table));
					task_open_table(root,table);
					printf("---------------root=%s,item=%s\n",root,table);
				}
			}
		}
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
	if(msg!=WM_NCHITTEST&&msg!=WM_SETCURSOR&&msg!=WM_ENTERIDLE&&msg!=WM_MOUSEMOVE&&msg!=WM_NOTIFY)
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
	case WM_MOUSEACTIVATE:

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

