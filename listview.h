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



int draw_item(DRAWITEMSTRUCT *di)
{
	int i,count,xpos;
	int textcolor,bgcolor;
	HWND header;

	header=SendMessage(di->hwndItem,LVM_GETHEADER,0,0);
	if(header==0)
		return;
	count=SendMessage(header,HDM_GETITEMCOUNT,0,0);
	if(count>1000)
		count=1000;

	bgcolor=GetSysColor(di->itemState&ODS_SELECTED ? COLOR_HIGHLIGHT:COLOR_WINDOW);
	textcolor=GetSysColor(di->itemState&ODS_SELECTED ? COLOR_HIGHLIGHTTEXT:COLOR_WINDOWTEXT);
	xpos=0;
	for(i=0;i<count;i++){
		char text[1024]={0};
		LV_ITEM lvi={0};
		RECT rect;
		SIZE size;
		int width,style;

		lvi.mask=LVIF_TEXT;
		lvi.iItem=di->itemID;
		lvi.iSubItem=i;
		lvi.pszText=text;
		lvi.cchTextMax=sizeof(text);

		ListView_GetItemText(di->hwndItem,di->itemID,i,text,sizeof(text));
		text[sizeof(text)-1]=0;
		width=ListView_GetColumnWidth(di->hwndItem,i);

		rect=di->rcItem;
		rect.left=xpos;
		rect.right=xpos+width;
	//		DrawText(di->hDC,text,-1,&rect,style);
	//	SetTextColor(di->hDC,(0xFFFFFF^GetSysColor(COLOR_BTNTEXT)));
		FillRect(di->hDC,&rect,di->itemState&ODS_SELECTED ? COLOR_HIGHLIGHT+1:COLOR_WINDOW+1);
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
		xpos+=width;
	}
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
	//GetTextExtentPoint32(di->hDC,list_string,strlen(list_string),&size);
	//set_list_width(size.cx+16);
	return TRUE;
}
int list_drawitem(HWND hwnd,int id,DRAWITEMSTRUCT *di)
{
	switch(di->itemAction){
	default:
	case ODA_DRAWENTIRE:
		draw_item(di);
		break;
	}
	return TRUE;
}