int get_str_width(HWND hwnd,char *str)
{
	if(hwnd!=0 && str!=0){
		SIZE size={0};
		HDC hdc;
		hdc=GetDC(hwnd);
		if(hdc!=0){
			GetTextExtentPoint32(hdc,str,strlen(str),&size);
			ReleaseDC(hwnd,hdc);
			return size.cx;
		}
	}
	return 0;
}
int lv_get_column_count(HWND hlistview)
{
	HWND header;
	int count=0;
	header=SendMessage(hlistview,LVM_GETHEADER,0,0);
	if(header!=0){
		count=SendMessage(header,HDM_GETITEMCOUNT,0,0);
		if(count<0)
			count=0;
	}
	return count;
}
int lv_get_col_text(HWND hlistview,int index,char *str,int size)
{
	LV_COLUMN col;
	if(hlistview!=0 && str!=0 && size>0){
		col.mask = LVCF_TEXT;
		col.pszText = str;
		col.cchTextMax = size;
		return ListView_GetColumn(hlistview,index,&col);
	}
	return FALSE;
}
int lv_add_column(HWND hlistview,char *str,int index)
{
	LV_COLUMN col;
	if(hlistview!=0 && str!=0){
		int width=get_str_width(hlistview,str);
		if(width<=0)
			width=40;
		col.mask = LVCF_WIDTH|LVCF_TEXT;
		col.cx = width;
		col.pszText = str;
		ListView_InsertColumn(hlistview,index,&col);
	}
}

int lv_insert_data(HWND hlistview,int row,int col,char *str)
{
	if(hlistview!=0 && str!=0){
		LV_ITEM item;
		memset(&item,0,sizeof(item));
		if(col==0){
			item.mask=LVIF_TEXT;
			item.iItem=row;
			item.pszText=str;
			ListView_InsertItem(hlistview,&item);
		}
		else{
			item.mask=LVIF_TEXT;
			item.iItem=row;
			item.pszText=str;
			item.iSubItem=col;
			ListView_SetItem(hlistview,&item);
		}
		return TRUE;
	}
	return FALSE;
}
static WNDPROC wporiglistview=0;
LRESULT APIENTRY sc_listview(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam)
{
	if(FALSE)
	if(msg<=0x1000)
	if(msg!=WM_NCMOUSEMOVE&&msg!=WM_MOUSEFIRST&&msg!=WM_NCHITTEST&&msg!=WM_SETCURSOR&&msg!=WM_ENTERIDLE&&msg!=WM_NOTIFY
		&&msg!=WM_USER)
	{
		static DWORD tick=0;
		if((GetTickCount()-tick)>500)
			printf("--\n");
		printf("l");
		print_msg(msg,lparam,wparam,hwnd);
		tick=GetTickCount();
	}
    return CallWindowProc(wporiglistview,hwnd,msg,wparam,lparam);
}
int subclass_listview(HWND hlistview)
{
	wporiglistview=SetWindowLong(hlistview,GWL_WNDPROC,(LONG)sc_listview);
	printf("subclass=%08X\n",wporiglistview);
}



int draw_item(DRAWITEMSTRUCT *di,TABLE_WINDOW *win)
{
	int i,count,xpos;
	int textcolor,bgcolor;
	RECT focus_rect={0},client_rect={0};

	count=lv_get_column_count(di->hwndItem);
	if(count>1000)
		count=1000;

	GetClientRect(di->hwndItem,&client_rect);

	bgcolor=GetSysColor(di->itemState&ODS_SELECTED ? COLOR_HIGHLIGHT:COLOR_WINDOW);
	textcolor=GetSysColor(di->itemState&ODS_SELECTED ? COLOR_HIGHLIGHTTEXT:COLOR_WINDOWTEXT);
	xpos=0;
	for(i=0;i<count;i++){
		char text[1024]={0};
		LV_ITEM lvi={0};
		RECT rect;
		SIZE size;
		int width,style;

		width=ListView_GetColumnWidth(di->hwndItem,i);

		rect=di->rcItem;
		rect.left+=xpos;
		rect.right=rect.left+width;
		if(rect.right<0)
			i=i;
		if(rect.left>client_rect.right)
			i=i;
		xpos+=width;


		if(rect.right>=0 && rect.left<=client_rect.right){
			lvi.mask=LVIF_TEXT;
			lvi.iItem=di->itemID;
			lvi.iSubItem=i;
			lvi.pszText=text;
			lvi.cchTextMax=sizeof(text);

			ListView_GetItemText(di->hwndItem,di->itemID,i,text,sizeof(text));
			text[sizeof(text)-1]=0;

			//rect.left=xpos;
			//rect.right=xpos+width;
		//	DrawText(di->hDC,text,-1,&rect,style);
		//	SetTextColor(di->hDC,(0xFFFFFF^GetSysColor(COLOR_BTNTEXT)));
			if((di->itemState&ODS_SELECTED) && win!=0 && win->selected_column==i){
				FillRect(di->hDC,&rect,COLOR_WINDOW+1);
				focus_rect=rect;
			}
			else{
				FillRect(di->hDC,&rect,di->itemState&ODS_SELECTED ? COLOR_HIGHLIGHT+1:COLOR_WINDOW+1);
			}
			FrameRect(di->hDC,&rect,GetStockObject(DKGRAY_BRUSH));
			if(text[0]!=0){

				SetTextColor(di->hDC,textcolor);
				SetBkColor(di->hDC,bgcolor);

				style=DT_LEFT|DT_NOPREFIX;
				style=DT_RIGHT|DT_NOPREFIX;

			//		SetTextColor(di->hDC,file_text_color);

				//GetTextExtentPoint32(di->hDC,text,strlen(text),&size);
				//rect.right=rect.left+size.cx;
				DrawText(di->hDC,text,-1,&rect,style);
			}
		}
	}
	if(di->itemState&ODS_SELECTED)
		DrawFocusRect(di->hDC,&focus_rect);

	//if(di->itemState&ODS_FOCUS)
		

	/*
	switch(di->itemAction){
	case ODA_DRAWENTIRE:
		if(di->itemState&ODS_FOCUS)
			DrawFocusRect(di->hDC,&di->rcItem); 
		break;
	case ODA_FOCUS:
		if(di->itemState&ODS_FOCUS)
			DrawFocusRect(di->hDC,&di->rcItem); 
		break;
	case ODA_SELECT:
		if(di->itemState&ODS_FOCUS)
			DrawFocusRect(di->hDC,&di->rcItem); 
		break;
	}
	*/
	//GetTextExtentPoint32(di->hDC,list_string,strlen(list_string),&size);
	//set_list_width(size.cx+16);
	return TRUE;
}

int find_win_by_hlvedit(HWND hwnd,TABLE_WINDOW **win)
{
	int i;
	for(i=0;i<sizeof(table_windows)/sizeof(TABLE_WINDOW);i++){
		if(table_windows[i].hlvedit==hwnd){
			*win=&table_windows[i];
			return TRUE;
		}
	}
	return FALSE;
}
static WNDPROC lvorigedit=0;
LRESULT APIENTRY sc_lvedit(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam)
{
	if(FALSE)
	if(msg!=WM_NCMOUSEMOVE&&msg!=WM_MOUSEFIRST&&msg!=WM_NCHITTEST&&msg!=WM_SETCURSOR&&msg!=WM_ENTERIDLE&&msg!=WM_NOTIFY
		&&msg!=WM_ERASEBKGND)
		//if(msg!=WM_NCHITTEST&&msg!=WM_SETCURSOR&&msg!=WM_ENTERIDLE)
	{
		static DWORD tick=0;
		if((GetTickCount()-tick)>500)
			printf("--\n");
		printf("e");
		print_msg(msg,lparam,wparam,hwnd);
		tick=GetTickCount();
	}
	switch(msg){
	case WM_KILLFOCUS:
		//if(wparam!=0)
		{
			TABLE_WINDOW *win=0;
			if(find_win_by_hlvedit(hwnd,&win))
				PostMessage(win->hwnd,WM_USER,win,MAKELPARAM(IDC_LV_EDIT,IDCANCEL));
		}
		break;
	case WM_KEYFIRST:
		switch(wparam){
		case VK_RETURN:
			{
				TABLE_WINDOW *win=0;
				if(find_win_by_hlvedit(hwnd,&win)){
					int index;
					char text[1024]={0};
					index=ListView_GetSelectionMark(win->hlistview);
					GetWindowText(win->hlvedit,text,sizeof(text));
					task_update_record(win,index,text);
					PostMessage(win->hwnd,WM_USER,win,MAKELPARAM(IDC_LV_EDIT,IDOK));
				}
			}
			break;
		case VK_ESCAPE:
			{
				TABLE_WINDOW *win=0;
				if(find_win_by_hlvedit(hwnd,&win))
					PostMessage(win->hwnd,WM_USER,win,MAKELPARAM(IDC_LV_EDIT,IDCANCEL));
			}
		}
		break;
	}
    return CallWindowProc(lvorigedit,hwnd,msg,wparam,lparam);
}

int create_lv_edit_selected(TABLE_WINDOW *win)
{
	if(win!=0 && win->hlistview!=0){
		int index=0;
		index=ListView_GetSelectionMark(win->hlistview);
		if(index>=0){
			RECT rect={0};
			if(ListView_GetSubItemRect(win->hlistview,index,win->selected_column,LVIR_LABEL,&rect)!=0){
				char text[255]={0};
				create_lv_edit(win,&rect);
				ListView_GetItemText(win->hlistview,index,win->selected_column,text,sizeof(text));
				if(text[0]!=0 && win->hlvedit!=0){
					SetWindowText(win->hlvedit,text);
					SetFocus(win->hlvedit);
					return TRUE;
				}
			}
		}
	}
	return FALSE;
}

int create_lv_edit(TABLE_WINDOW *win,RECT *rect)
{
	if(win!=0 && rect!=0){
		if(win->hlvedit!=0)
			destroy_lv_edit(win);
		win->hlvedit = CreateWindow("RichEdit50W",
										 "",
										 WS_TABSTOP|WS_CHILD|WS_CLIPSIBLINGS|WS_VISIBLE|
										 ES_RIGHT|ES_AUTOHSCROLL,
										 rect->left-1,rect->top-1,
										 rect->right - rect->left+2,
										 rect->bottom - rect->top+2,
										 win->hlistview,
										 IDC_LV_EDIT,
										 ghinstance,
										 NULL);
		if(win->hlvedit!=0)
			lvorigedit=SetWindowLong(win->hlvedit,GWL_WNDPROC,(LONG)sc_lvedit);
	}
	return FALSE;
}

int destroy_lv_edit(TABLE_WINDOW *win)
{
	if(win!=0 && win->hlvedit!=0){
		DestroyWindow(win->hlvedit);
		win->hlvedit=0;
	}
	return TRUE;
}