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
	if(msg!=WM_NCMOUSEMOVE&&msg!=WM_MOUSEFIRST&&msg!=WM_NCHITTEST&&msg!=WM_SETCURSOR&&msg!=WM_ENTERIDLE&&msg!=WM_NOTIFY)
	{
		static DWORD tick=0;
		if((GetTickCount()-tick)>500)
			printf("--\n");
		printf("l");
		print_msg(msg,lparam,wparam,hwnd);
		tick=GetTickCount();
	}
	switch(msg){
	case WM_DRAWITEM:
		list_drawitem(hwnd,wparam,lparam);
		return TRUE;
		break;
	}
    return CallWindowProc(wporiglistview,hwnd,msg,wparam,lparam);
}
int subclass_listview(HWND hlistview)
{
	wporiglistview=SetWindowLong(hlistview,GWL_WNDPROC,(LONG)sc_listview);
	printf("subclass=%08X\n",wporiglistview);
}



int draw_item(DRAWITEMSTRUCT *di,char *list_string)
{
	int style;
	RECT rect;
	SIZE size;
    HDC hdcMem;
    HBITMAP hbmpOld;
	int bgcolor=COLOR_BACKGROUND;
	int is_file=FALSE,is_line=FALSE,is_offset=FALSE;


	hdcMem=CreateCompatibleDC(di->hDC);

	if(di->itemState&ODS_SELECTED)
		bgcolor=COLOR_HIGHLIGHT;
	else
		bgcolor=COLOR_WINDOW;

	FillRect(di->hDC,&di->rcItem,bgcolor+1);
	BitBlt(di->hDC,di->rcItem.left,di->rcItem.top,di->rcItem.left+16,di->rcItem.top+16,hdcMem,0,0,SRCINVERT);
	rect=di->rcItem;
	rect.left+=16;
//		DrawText(di->hDC,text,-1,&rect,style);
//	SetTextColor(di->hDC,(0xFFFFFF^GetSysColor(COLOR_BTNTEXT)));
	SetTextColor(di->hDC,GetSysColor(di->itemState&ODS_SELECTED ? COLOR_HIGHLIGHTTEXT:COLOR_WINDOWTEXT));
	SetBkColor(di->hDC,GetSysColor(di->itemState&ODS_SELECTED ? COLOR_HIGHLIGHT:COLOR_WINDOW));
	style=DT_LEFT|DT_NOPREFIX;

//		SetTextColor(di->hDC,file_text_color);

	GetTextExtentPoint32(di->hDC,list_string,strlen(list_string),&size);
	rect.right=rect.left+size.cx;
	DrawText(di->hDC,list_string,-1,&rect,style);
	if(di->itemState&ODS_FOCUS)
		DrawFocusRect(di->hDC,&di->rcItem); 

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
	GetTextExtentPoint32(di->hDC,list_string,strlen(list_string),&size);
	//set_list_width(size.cx+16);
    DeleteDC(hdcMem);
	return TRUE;
}
int list_drawitem(HWND hwnd,int id,DRAWITEMSTRUCT *di)
{
	char text[1024];
	if(FALSE)
	{
		{
			static DWORD tick=0;
			if((GetTickCount()-tick)>500)
				printf("--\n");
			tick=GetTickCount();
		}
		{
			char s1[100]={0},s2[100]={0};
			int c=0;
			if(di->itemAction&ODA_DRAWENTIRE)
				c++;
			if(di->itemAction&ODA_SELECT)
				c++;
			if(di->itemAction&ODS_FOCUS)
				c++;
			if(c>1)
				c=c;
			if(di->itemAction&ODA_DRAWENTIRE)
				strcat(s1,"ODA_DRAWENTIRE|");
			if(di->itemAction&ODA_FOCUS)
				strcat(s1,"ODA_FOCUS|");
			if(di->itemAction&ODA_SELECT)
				strcat(s1,"ODA_SELECT|");
			if(di->itemState&ODS_DEFAULT)
				strcat(s2,"ODS_DEFAULT|");
			if(di->itemState&ODS_DISABLED)
				strcat(s2,"ODS_DISABLED|");
			if(di->itemState&ODS_FOCUS)
				strcat(s2,"ODS_FOCUS|");
			if(di->itemState&ODS_GRAYED)
				strcat(s2,"ODS_GRAYED|");
			if(di->itemState&ODS_SELECTED)
				strcat(s2,"ODS_SELECTED|");
			printf("%2i %s\t%s\n",di->itemID,s1,s2);
		}
	}
	switch(di->itemAction){
	case ODA_DRAWENTIRE:
		text[0]=0;
		SendDlgItemMessage(hwnd,id,LB_GETTEXT,di->itemID,text);
		draw_item(di,text);
	case ODA_FOCUS:
		text[0]=0;
		SendDlgItemMessage(hwnd,id,LB_GETTEXT,di->itemID,text);
		draw_item(di,text);
		break;
	case ODA_SELECT:
		text[0]=0;
		SendDlgItemMessage(hwnd,id,LB_GETTEXT,di->itemID,text);
		draw_item(di,text);
		break;

	}
	return TRUE;
}