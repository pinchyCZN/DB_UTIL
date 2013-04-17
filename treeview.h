HWND ghtreeview;

int test_items()
{
	TV_INSERTSTRUCT tvins;
	TV_ITEM tvi;
     
	tvi.mask=TVIF_TEXT;
	tvi.pszText="Item";
 
	tvins.hParent=TVI_ROOT;
	tvins.hInsertAfter=TVI_FIRST;
	tvins.item=tvi;
 
	TreeView_InsertItem(ghtreeview,&tvins);
    
}
LRESULT CALLBACK treeview_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	static int create_tree=FALSE;
	switch(msg){
	case WM_CREATE:
		PostMessage(hwnd,WM_USER,0,0);
		break;
	case WM_USER:
		if(create_tree==FALSE){
			RECT rect;
			GetClientRect(hwnd,&rect);
			ghtreeview=CreateWindow(WC_TREEVIEW, 
										 "dbtreeview",
										 WS_TABSTOP|WS_CHILD|WS_VISIBLE|TVS_HASBUTTONS,
										 0, 0,
										 rect.right,
										 rect.bottom,
										 hwnd,
										 IDC_TABLES,
										 (HINSTANCE)GetWindowLong(hwnd,GWL_HINSTANCE),
										 NULL);
			create_tree=TRUE;
			test_items();
		}
		break;
	case WM_SIZE:
		{
			RECT rect;
			//GetClientRect(hwnd,&rect);
			//MoveWindow(GetDlgItem(hwnd,IDC_TABLES),0,0,rect.right,rect.bottom,TRUE);
		}
		break;
	}
	return DefWindowProc(hwnd,msg,wparam,lparam);
}


int resize_treeview(HWND hwnd,HWND mdi,HWND treeframe,int tree_width)
{
	RECT rect;
	if(GetClientRect(hwnd,&rect)!=0){
		int gap=5;
		int xpos=tree_width;
		if(xpos<0)
			return TRUE;
		MoveWindow(mdi,xpos,0,rect.right-xpos,rect.bottom,TRUE);
		MoveWindow(treeframe,0,0,tree_width-gap,rect.bottom,TRUE);
		if(ghtreeview!=0){
			GetClientRect(treeframe,&rect);
			MoveWindow(ghtreeview,0,0,rect.right,rect.bottom,TRUE);
		}
	}
	return TRUE;
}