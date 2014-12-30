#include "search.h"

static HMENU lv_menu=0;
static HMENU lv_col_menu=0;
enum {
	CMD_COL_INFO=10000,
	CMD_COL_WIDTH_HEADER,
	CMD_COL_WIDTH_DATA,
	CMD_SQL_SELECT_ALL,
	CMD_SQL_SELECT_COL,
	CMD_SQL_WHERE,
	CMD_SQL_ORDERBY,
	CMD_SQL_GROUPBY,
	CMD_SQL_UPDATE,
	CMD_SQL_DELETE,
	CMD_EXPORT_DATA,
};
int get_first_line_len(char *str)
{
	int i,len;
	len=0x10000;
	for(i=0;i<len;i++){
		if(str[i]=='\n' || str[i]=='\r' || str[i]==0)
			break;
	}
	return i;
}
int get_string_width_wc(HWND hwnd,char *str,int wide_char)
{
	if(hwnd!=0 && str!=0){
		SIZE size={0};
		HDC hdc;
		hdc=GetDC(hwnd);
		if(hdc!=0){
			HFONT hfont;
			int len=get_first_line_len(str);
			hfont=SendMessage(hwnd,WM_GETFONT,0,0);
			if(hfont!=0){
				HGDIOBJ hold=0;
				hold=SelectObject(hdc,hfont);
				if(wide_char)
					GetTextExtentPoint32W(hdc,str,wcslen(str),&size);
				else
					GetTextExtentPoint32(hdc,str,len,&size);
				if(hold!=0)
					SelectObject(hdc,hold);
			}
			else{
				if(wide_char)
					GetTextExtentPoint32W(hdc,str,wcslen(str),&size);
				else
					GetTextExtentPoint32(hdc,str,len,&size);
			}
			ReleaseDC(hwnd,hdc);
			return size.cx;
		}
	}
	return 0;

}
int get_str_width(HWND hwnd,char *str)
{
	return get_string_width_wc(hwnd,str,FALSE);
}
int lv_scroll_column(HWND hlistview,int index)
{
	RECT rect={0};
	int result=FALSE;
	if(lv_get_col_rect(hlistview,index,&rect)){
		RECT rectlv={0};
		SCROLLINFO si;
		si.cbSize=sizeof(si);
		si.fMask=SIF_POS;
		if(	GetClientRect(hlistview,&rectlv) &&
			GetScrollInfo(hlistview,SB_HORZ,&si)){
			int diff=0;
			if(rect.right-si.nPos>rectlv.right)
				diff=rect.right-rectlv.right-si.nPos;
			else if(rect.left-si.nPos<rectlv.left)
				diff=-si.nPos+rect.left;
			if(diff!=0)
				result=ListView_Scroll(hlistview,diff,0);
		}
	}
	return result;
}
int lv_get_col_rect(HWND hlistview,int col,RECT *rect)
{
	int result=FALSE;
	if(hlistview!=0 && rect!=0){
		HWND header=SendMessage(hlistview,LVM_GETHEADER,0,0);
		if(header!=0)
			if(SendMessage(header,HDM_GETITEMRECT,col,rect))
				result=TRUE;
	}
	return result;
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
		HWND header;
		int width=0;
		header=SendMessage(hlistview,LVM_GETHEADER,0,0);
		width=get_str_width(header,str);
		width+=14;
		if(width<40)
			width=40;
		col.mask = LVCF_WIDTH|LVCF_TEXT;
		col.cx = width;
		col.pszText = str;
		if(ListView_InsertColumn(hlistview,index,&col)>=0)
			return width;
	}
	return 0;
}
int lv_update_data(HWND hlistview,int row,int col,char *str)
{
	if(hlistview!=0 && str!=0){
		LV_ITEM item;
		memset(&item,0,sizeof(item));
		item.mask=LVIF_TEXT;
		item.iItem=row;
		item.pszText=str;
		item.iSubItem=col;
		return ListView_SetItem(hlistview,&item);
	}
	return FALSE;
}
int lv_insert_data(HWND hlistview,int row,int col,char *str)
{
	if(hlistview!=0 && str!=0){
		LV_ITEM item;
		memset(&item,0,sizeof(item));
		if(col==0){
			item.mask=LVIF_TEXT|LVIF_PARAM;
			item.iItem=row;
			item.pszText=str;
			item.lParam=row;
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
int export_listview(HWND hwnd,const char *fname)
{
	int result=FALSE;
	FILE *f;
	if(hwnd==0 || fname==0 || fname[0]==0)
		return result;
	f=fopen(fname,"wb");
	if(f!=0){
		int i,j,item_count,col_count;
		char *buf;
		int buf_size=0x10000;
		item_count=ListView_GetItemCount(hwnd);
		col_count=lv_get_column_count(hwnd);
		set_status_bar_text(ghstatusbar,0,"exporting data to %s",fname);
		for(i=0;i<col_count;i++){
			char str[255]={0};
			lv_get_col_text(hwnd,i,str,sizeof(str));
			fprintf(f,"%s%s",str,i==col_count-1?"\n":",");
		}
		buf=malloc(buf_size);
		if(buf!=0){
			for(i=0;i<item_count;i++){
				for(j=0;j<col_count;j++){
					buf[0]=0;
					ListView_GetItemText(hwnd,i,j,buf,buf_size);
					fprintf(f,"%s%s",buf,j==col_count-1?"\n":",");
				}
			}
			result=TRUE;
			free(buf);
		}
		else
			fprintf(f,"cant allocate buffer of size %08X\n",buf_size);
		fclose(f);
		set_status_bar_text(ghstatusbar,0,"finished export to %s",fname);
	}else{
		set_status_bar_text(ghstatusbar,0,"cant open %s",fname);
	}
	return result;
}
LRESULT CALLBACK filename_proc(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam)
{
	static char *fname=0;
	switch(msg){
	case WM_INITDIALOG:
		{
			char **params=lparam;
			if(params==0 || params[0]==0)
				EndDialog(hwnd,FALSE);
			else{
				char str[MAX_PATH]={0};
				char *s=params[0];
				fname=params[0];
				if(fname[0]==0){
					GetCurrentDirectory(sizeof(str),str);
					s=str;
				}
				SendDlgItemMessage(hwnd,IDC_EDIT1,WM_SETTEXT,0,s);
				if(params[1]!=0){
					_snprintf(str,sizeof(str),"Export filename for table [%s]",params[1]);
					str[sizeof(str)-1]=0;
					SetWindowText(hwnd,str);
				}
			}
			SendDlgItemMessage(hwnd,IDC_EDIT1,EM_SETLIMITTEXT,MAX_PATH,0);
			SetFocus(GetDlgItem(hwnd,IDC_EDIT1));
			SendDlgItemMessage(hwnd,IDC_EDIT1,EM_SETSEL,MAX_PATH,-1);
			{
				RECT rect={0};
				int x,y;
				GetWindowRect(ghmdiclient,&rect);
				x=(rect.left+rect.right)/2;
				y=(rect.top+rect.bottom)/2;
				GetWindowRect(hwnd,&rect);
				x-=(rect.right-rect.left)/2;
				y-=(rect.bottom-rect.top)/2;
				SetWindowPos(hwnd,NULL,x,y,0,0,SWP_NOSIZE|SWP_NOZORDER);
			}
		}
		break;
	case WM_SIZE:
		break;
	case WM_COMMAND:
		switch(LOWORD(wparam)){
		case IDOK:
			{
				char str[MAX_PATH]={0};
				SendDlgItemMessage(hwnd,IDC_EDIT1,WM_GETTEXT,sizeof(str),str);
				_snprintf(fname,MAX_PATH,"%s",str);
				EndDialog(hwnd,TRUE);
			}
			break;
		case IDCANCEL:
			EndDialog(hwnd,FALSE);
			break;
		}
		break;
	}
	return 0;
}
int insert_col_sql(TABLE_WINDOW *win,char *col)
{
	int result=FALSE;
	if(win!=0 && win->hedit!=0 && col!=0){
		HWND hedit=win->hedit;
		char *str;
		int str_size=0x1000;
		str=malloc(str_size);
		if(str){
			str[0]=0;
			GetWindowText(hedit,str,str_size);
			if(str[0]!=0){
				char *s=strstri(str," FROM ");
				if(s!=0){
					char *n=malloc(str_size);
					if(n!=0){
						char *lbrack="",*rbrack="";
						s[0]=0;
						get_col_brackets(win,col,&lbrack,&rbrack);
						_snprintf(n,str_size,"%s,%s%s%s %s",str,lbrack,col,rbrack,s+1);
						n[str_size-1]=0;
						SetWindowText(hedit,n);
						free(n);
					}
				}
			}
			free(str);
		}
	}
	return result;
}

static WNDPROC wporiglistview=0;
LRESULT APIENTRY sc_listview(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam)
{
	if(FALSE)
	if(msg<=0x1000)
	if(msg!=WM_NCMOUSEMOVE&&msg!=WM_MOUSEFIRST&&msg!=WM_NCHITTEST&&msg!=WM_SETCURSOR&&msg!=WM_ENTERIDLE&&msg!=WM_NOTIFY
		&&msg!=WM_USER&&msg!=WM_GETFONT)
	{
		static DWORD tick=0;
		if((GetTickCount()-tick)>500)
			printf("--\n");
		printf("l");
		print_msg(msg,lparam,wparam,hwnd);
		tick=GetTickCount();
	}
	switch(msg){
	case WM_COMMAND:
		switch(wparam){
		case CMD_COL_INFO:
			DialogBoxParam(ghinstance,MAKEINTRESOURCE(IDD_COL_INFO),hwnd,col_info_proc,hwnd);
			break;
		case CMD_SQL_SELECT_ALL:
		case CMD_SQL_SELECT_COL:
		case CMD_SQL_WHERE:
		case CMD_SQL_ORDERBY:
		case CMD_SQL_GROUPBY:
		case CMD_SQL_UPDATE:
		case CMD_SQL_DELETE:
			{
				TABLE_WINDOW *win=0;
				if(find_win_by_hlistview(hwnd,&win)){
					char *sql;
					int sql_size=0x10000;
					int row=ListView_GetSelectionMark(win->hlistview);
					sql=malloc(sql_size);
					if(sql!=0){
						sql[0]=0;
						switch(wparam){
						case CMD_SQL_SELECT_ALL:
						case CMD_SQL_SELECT_COL:
						case CMD_SQL_WHERE:
						case CMD_SQL_ORDERBY:
						case CMD_SQL_GROUPBY:
							if(win->table!=0){
								char *lbrack="",*rbrack="";
								char col_name[80]={0};
								char *sql_count="";
								if(wparam==CMD_SQL_GROUPBY){
									lv_get_col_text(win->hlistview,win->selected_column,col_name,sizeof(col_name));
									get_col_brackets(win,col_name,&lbrack,&rbrack);
									sql_count=",COUNT(*)";
								}
								else if(wparam==CMD_SQL_SELECT_COL){
									lv_get_col_text(win->hlistview,win->selected_column,col_name,sizeof(col_name));
									get_col_brackets(win,col_name,&lbrack,&rbrack);
									if(GetKeyState(VK_CONTROL)&0x8000){
										insert_col_sql(win,col_name);
										break;
									}
									else if(GetKeyState(VK_SHIFT)&0x8000){
										int i;
										_snprintf(sql,sql_size,"SELECT ");
										for(i=0;i<win->columns;i++){
											char *comma="";
											col_name[0]=0;
											lv_get_col_text(win->hlistview,i,col_name,sizeof(col_name));
											get_col_brackets(win,col_name,&lbrack,&rbrack);
											if(i<(win->columns-1))
												comma=",";
											_snprintf(sql,sql_size,"%s%s%s%s%s",sql,lbrack,col_name,rbrack,comma);
										}
										get_col_brackets(win,win->table,&lbrack,&rbrack);
										_snprintf(sql,sql_size,"%s FROM %s%s%s",sql,lbrack,win->table,rbrack);
										break;
									}
								}
								else{
									col_name[0]='*';col_name[1]=0;
								}
								if(strchr(win->table,' ')!=0)
									_snprintf(sql,sql_size,"SELECT %s%s%s%s FROM [%s]",lbrack,col_name,rbrack,sql_count,win->table);
								else
									_snprintf(sql,sql_size,"SELECT %s%s%s%s FROM %s",lbrack,col_name,rbrack,sql_count,win->table);
							}
							if(wparam==CMD_SQL_SELECT_ALL)
								break;
							if(wparam==CMD_SQL_WHERE){
								char *lbrack="",*rbrack="";
								char *tmp;
								char col_name[80]={0};
								int tmp_size=0x1000;
								lv_get_col_text(win->hlistview,win->selected_column,col_name,sizeof(col_name));
								get_col_brackets(win,col_name,&lbrack,&rbrack);
								tmp=malloc(tmp_size);
								if(tmp!=0){
									char *v=0,*eq="=";
									tmp[0]=0;
									ListView_GetItemText(win->hlistview,row,win->selected_column,tmp,tmp_size);
									if(stricmp(tmp,"(NULL)")==0){
										v="NULL";
										eq=" is ";
									}
									else if(tmp[0]==0)
										v="''";
									else
										v=tmp;
									sanitize_value(tmp,tmp,tmp_size,get_column_type(win,win->selected_column));
									_snprintf(sql,sql_size,"%s WHERE %s%s%s%s%s",sql,lbrack,col_name,rbrack,eq,v);
									free(tmp);
								}
							}
							else if(wparam==CMD_SQL_ORDERBY || wparam==CMD_SQL_GROUPBY){
								char *lbrack="",*rbrack="";
								char col_name[80]={0};
								char *op="ORDER BY";
								if(wparam==CMD_SQL_GROUPBY)
									op="GROUP BY";
								lv_get_col_text(win->hlistview,win->selected_column,col_name,sizeof(col_name));
								get_col_brackets(win,col_name,&lbrack,&rbrack);
								_snprintf(sql,sql_size,"%s %s %s%s%s",sql,op,lbrack,col_name,rbrack);
								if(GetKeyState(VK_CONTROL)&0x8000){
									SendMessage(win->hedit,EM_SETSEL,-1,-1);
									_snprintf(sql,sql_size,",%s%s%s",lbrack,col_name,rbrack);
									SendMessage(win->hedit,EM_REPLACESEL,TRUE,sql);
									sql[0]=0;
								}
							}
							break;
						case CMD_SQL_UPDATE:
							{
								char tmp[80]={0};
								int tmp_size=sizeof(tmp);
								ListView_GetItemText(win->hlistview,row,win->selected_column,tmp,tmp_size);
								tmp[sizeof(tmp)-1]=0;
								create_statement(win,row,tmp,sql,sql_size,FALSE);
							}
							break;
						case CMD_SQL_DELETE:
							{
								char tmp[1]={0};
								create_statement(win,row,tmp,sql,sql_size,TRUE);
							}
							break;
						}
						if(sql[0]!=0)
							SetWindowText(win->hedit,sql);
						free(sql);
					}
				}
			}
			break;
		case CMD_EXPORT_DATA:
			{
				static char fname[MAX_PATH]={0};
				static char *params[2]={fname,0};
				TABLE_WINDOW *win=0;
				if(find_win_by_hlistview(hwnd,&win))
					params[1]=win->table;
				
				if(DialogBoxParam(ghinstance,MAKEINTRESOURCE(IDD_FILENAME),hwnd,filename_proc,params)==TRUE){
					export_listview(hwnd,fname);
				}
			}
			break;
		case CMD_COL_WIDTH_DATA:
			{
				TABLE_WINDOW *win=0;
				if(find_win_by_hlistview(hwnd,&win)){
					int i,width,top,count,done;
					top=ListView_GetTopIndex(win->hlistview);
					count=ListView_GetItemCount(win->hlistview);
					width=0;
					done=0;
					for(i=top;i<count;i++){
						char str[100]={0};
						ListView_GetItemText(win->hlistview,i,win->selected_column,str,sizeof(str));
						str[sizeof(str)-1]=0;
						if(str[0]!=0){
							int w=get_str_width(win->hlistview,str);
							if(w>width)
								width=w;
						}
						done++;
						if(done>500)
							break;
					}
					if(width<=0)
						width=10;
					else if(width>1000)
						width=1000;
					ListView_SetColumnWidth(win->hlistview,win->selected_column,width);
				}
			}
			break;
		case CMD_COL_WIDTH_HEADER:
			{
				TABLE_WINDOW *win=0;
				if(find_win_by_hlistview(hwnd,&win)){
					char str[80]={0};
					lv_get_col_text(win->hlistview,win->selected_column,str,sizeof(str));
					if(str[0]!=0){
						int width;
						width=get_str_width(win->hlistview,str);
						if(width>0)
							ListView_SetColumnWidth(win->hlistview,win->selected_column,width+14);

					}
				}
			}
			break;
		}
		break;
	case WM_MOUSEWHEEL:
		{
			int x=0,y=0,ctrl=0,shift=0;
			if(GetKeyState(VK_CONTROL)&0x8000)
				ctrl=1;
			if(GetKeyState(VK_SHIFT)&0x8000)
				shift=1;
			if(ctrl!=0 && shift!=0)
				y=3;
			else if(ctrl!=0)
				x=1;
			else if(shift!=0)
				y=1;
			if(x!=0 || y!=0){
				TABLE_WINDOW *win=0;
				if(find_win_by_hlistview(hwnd,&win)){
					RECT rect={0};
					int zdelta;
					GetClientRect(win->hlistview,&rect);
					if(x!=0){
						x=rect.right/10;
						if(x<=10)
							x=10;
					}
					if(y!=0){
						y=y*rect.bottom/3;
						if(y<=20)
							y=20;
					}
					zdelta=HIWORD(wparam);
					if(zdelta&0x8000)
						x=-x;
					else
						y=-y;
					ListView_Scroll(win->hlistview,-x,y);
					return 0;
				}
				break;
			}
		}
	case WM_VSCROLL:
		{
			TABLE_WINDOW *win=0;
			if(find_win_by_hlistview(hwnd,&win) && win->hlvedit!=0){
				InvalidateRect(win->hlistview,NULL,FALSE);
				InvalidateRect(win->hlvedit,NULL,FALSE);
			}
		}
		break;
	case WM_HSCROLL:
		{
			TABLE_WINDOW *win=0;
			if(find_win_by_hlistview(hwnd,&win) && win->hlvedit!=0){
				switch(LOWORD(wparam)){
				default:
					{
						RECT rect={0},crect={0};
						int w;
						GetWindowRect(win->hlvedit,&rect);
						lv_get_col_rect(hwnd,win->selected_column,&crect);
						MapWindowPoints(NULL,win->hlistview,&rect,2);
						w=rect.right-rect.left;
						rect.left=crect.left-GetScrollPos(hwnd,SB_HORZ);
						rect.right=rect.left+w;
						if(SetWindowPos(win->hlvedit,HWND_TOP,rect.left,rect.top,0,0,SWP_NOSIZE)){
							InvalidateRect(win->hlistview,NULL,FALSE);
							InvalidateRect(win->hlvedit,NULL,FALSE);
						}
					}
					break;
				}
			}
		}
		break;
	case WM_KEYFIRST:
		switch(wparam){
/*
		case VK_ESCAPE:
			{
				TABLE_WINDOW *win=0;
				if(find_win_by_hlistview(hwnd,&win)){
					SendMessage(win->hwnd,WM_USER,0,IDC_MDI_LISTVIEW);
					SetFocus(win->hedit);
				}
			}
			break;
		case 'C':
			if(GetKeyState(VK_CONTROL)&0x8000){
				TABLE_WINDOW *win=0;
				if(find_win_by_hlistview(hwnd,&win)){
					int sel=ListView_GetSelectionMark(win->hlistview);
					if(sel>=0){
						char str[255]={0};
						ListView_GetItemText(win->hlistview,sel,win->selected_column,str,sizeof(str))
						copy_str_clipboard(str);
					}
				}
			}
			break;
		case 'F':
			if(GetKeyState(VK_CONTROL)&0x8000){
				TABLE_WINDOW *win=0;
				if(find_win_by_hlistview(hwnd,&win)){
					DialogBoxParam(ghinstance,MAKEINTRESOURCE(IDD_SEARCH),hwnd,search_proc,win);
				}
			}
			break;
*/
		}
		break;
	case WM_CONTEXTMENU:
		{
			RECT rect={0};
			HANDLE header;
			int x=0,y=0;
			HMENU hmenu=0;
			header=SendMessage(hwnd,LVM_GETHEADER,0,0);
			if(header!=0)
				GetWindowRect(header,&rect);
			x=LOWORD(lparam);
			y=HIWORD(lparam);
			if(lparam==-1){
				POINT p={0};
				GetCursorPos(&p);
				x=p.x;y=p.y;
			}
			if(y<=rect.bottom){
				LV_HITTESTINFO ht={0};
				ht.pt.x=x;
				ht.pt.y=rect.bottom+2;
				ScreenToClient(hwnd,&ht.pt);
				if(ListView_SubItemHitTest(hwnd,&ht)>=0){
					TABLE_WINDOW *win=0;
					if(find_win_by_hlistview(hwnd,&win)){
						win->selected_column=ht.iSubItem;
						InvalidateRect(hwnd,NULL,FALSE);
					}
					printf("hit test %08X %i %i\n",ht.flags,ht.iItem,ht.iSubItem);
				}
				hmenu=lv_col_menu;
			}
			else{
				LV_HITTESTINFO ht={0};
				ht.pt.x=x;
				ht.pt.y=y;
				ScreenToClient(hwnd,&ht.pt);
				ListView_SubItemHitTest(hwnd,&ht);
				printf("hit test %08X %i %i\n",ht.flags,ht.iItem,ht.iSubItem);
				hmenu=lv_menu;
				//LVHT_ONITEMLABEL
			}
			if(hmenu!=0)
				TrackPopupMenu(hmenu,TPM_LEFTALIGN,x,y,0,hwnd,NULL);
		}
		break;
	case WM_DESTROY:
		break; //return 0; //CallWindowProc(wporiglistview,hwnd,msg,wparam,lparam);
	}
    return CallWindowProc(wporiglistview,hwnd,msg,wparam,lparam);
}
int subclass_listview(HWND hlistview)
{
	wporiglistview=SetWindowLong(hlistview,GWL_WNDPROC,(LONG)sc_listview);
	printf("subclass=%08X\n",wporiglistview);
	return wporiglistview;
}

int reduce_color(int color)
{
	unsigned char r,g,b;
	//BGR
	r=color&0xFF;
	g=(color>>8)&0xFF;
	b=(color>>16)&0xFF;
	r/=3;
	g/=3;
	b/=3;
	color=(b<<16)|(g<<8)|r;
	return color;
}

int draw_item(DRAWITEMSTRUCT *di,TABLE_WINDOW *win)
{
	int i,count,xpos;
	int textcolor,bgcolor;
	RECT bound_rect={0},client_rect={0};
	static HBRUSH hbrush=0;

	if(hbrush==0){
		LOGBRUSH brush={0};
		DWORD color=GetSysColor(COLOR_HIGHLIGHT);
		color=reduce_color(color);
		brush.lbColor=color;
		brush.lbStyle=BS_SOLID;
		hbrush=CreateBrushIndirect(&brush);
	}
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

			if(win!=0 && win->selected_column==i){
				bound_rect=rect;
				bound_rect.bottom++;
				bound_rect.right++;
			}
			if( (di->itemState&(ODS_FOCUS|ODS_SELECTED))==(ODS_FOCUS|ODS_SELECTED) && win!=0 && win->selected_column==i){
				FillRect(di->hDC,&rect,hbrush);
			}
			else{
				FillRect(di->hDC,&rect,di->itemState&ODS_SELECTED ? COLOR_HIGHLIGHT+1:COLOR_WINDOW+1);
			}
			{
				RECT tmprect=rect;
				tmprect.bottom++;
				tmprect.right++;
				FrameRect(di->hDC,&tmprect,GetStockObject(DKGRAY_BRUSH));
			}

			if(text[0]!=0){
				extern int left_justify;
				SetTextColor(di->hDC,textcolor);
				SetBkColor(di->hDC,bgcolor);

				if(left_justify)
					style=DT_LEFT|DT_NOPREFIX;
				else
					style=DT_RIGHT|DT_NOPREFIX;

				DrawText(di->hDC,text,-1,&rect,style);
			}
		}
	}
	if(di->itemState&ODS_FOCUS){
		SetTextColor(di->hDC,0x0000FF);
		bound_rect.right--;
		bound_rect.bottom--;
		DrawFocusRect(di->hDC,&bound_rect);
	}
	/*
	if(hbrush!=0){
		DeleteObject(hbrush);
		hbrush=0;
	}
	*/
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
	if(msg!=WM_NCMOUSEMOVE&&msg!=WM_MOUSEFIRST&&msg!=WM_NCHITTEST&&msg!=WM_SETCURSOR&&msg!=WM_ENTERIDLE //&&msg!=WM_NOTIFY
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
	case WM_HELP:
		return TRUE;
		break;
	case WM_KILLFOCUS:
		//if(wparam!=0)
		{
			TABLE_WINDOW *win=0;
			if(find_win_by_hlvedit(hwnd,&win)){
				PostMessage(win->hwnd,WM_USER,win,MAKELPARAM(IDC_LV_EDIT,IDCANCEL));
				PostMessage(ghmainframe,WM_USER,IDC_LV_EDIT,win->hlistview);
			}
		}
		break;
	case WM_SYSKEYDOWN:
		{
			int dx=0,dy=0;
			switch(wparam){
			case VK_HOME:
				SetWindowPos(hwnd,NULL,0,0,0,0,SWP_NOSIZE|SWP_NOZORDER);
				break;
			case VK_UP:
				dy=-1;
				break;
			case VK_DOWN:
				dy=1;
				break;
			case VK_LEFT:
				dx=-1;
				break;
			case VK_RIGHT:
				dx=1;
				break;
			}
			if(dx!=0 || dy!=0){
				RECT rect={0};
				int height;
				ListView_GetItemRect(GetParent(hwnd),0,&rect,LVIR_BOUNDS);
				height=rect.bottom-rect.top;
				if(height==0)
					height=8;
				dx=dx*height;dy=dy*height;
				rect.left=0;rect.top=0;
				GetWindowRect(hwnd,&rect);
				MapWindowPoints(NULL,GetParent(hwnd),&rect,2);
				SetWindowPos(hwnd,NULL,rect.left+dx,rect.top+dy,0,0,SWP_NOSIZE|SWP_NOZORDER);
			}
		}
		break;
	case WM_KEYFIRST:
		switch(wparam){
		case 'A':
			if(GetKeyState(VK_CONTROL)&0x8000)
				SendMessage(hwnd,EM_SETSEL,0,-1);
			break;
		case VK_F1:
			{
				RECT rect={0};
				int w,h,x,y;
				GetWindowRect(hwnd,&rect);
				w=rect.right-rect.left;
				h=rect.bottom-rect.top;
				MapWindowPoints(NULL,GetParent(hwnd),&rect,2);
				x=rect.left;
				y=rect.top;
				GetClientRect(GetParent(hwnd),&rect);
				if(GetKeyState(VK_CONTROL)&0x8000){
					w=w*.6;
					h=h*.79;
				}
				else{
					w=w*1.5;
					h=h*1.25;
				}
				if(w>rect.right)
					w=rect.right;
				else if(w<20)
					w=20;
				if(h>rect.bottom)
					h=rect.bottom;
				else if(h<20)
					h=20;
				if(x+w>rect.right)
					x-=(x+w)-rect.right;
				if(y+h>rect.bottom)
					y-=(y+h)-rect.bottom;
				SetWindowPos(hwnd,NULL,x,y,w,h,SWP_NOOWNERZORDER);
			}
			break;
		case VK_RETURN:
			{
				TABLE_WINDOW *win=0;
				if(find_win_by_hlvedit(hwnd,&win)){
					int sizeof_text=0x4000;
					char *text=0;
					text=malloc(sizeof_text);
					if(text!=0){
						int index;
						index=ListView_GetSelectionMark(win->hlistview);
						GetWindowText(win->hlvedit,text,sizeof_text);
						task_update_record(win,index,text);
						PostMessage(win->hwnd,WM_USER,win,MAKELPARAM(IDC_LV_EDIT,IDOK));
						free(text);
					}
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
	int result=FALSE;
	if(win!=0 && win->hlistview!=0){
		int index=0;
		index=ListView_GetSelectionMark(win->hlistview);
		if(index>=0){
			RECT rect={0};
			if(ListView_GetSubItemRect(win->hlistview,index,win->selected_column,LVIR_LABEL,&rect)!=0){
				char *str=0;
				int str_size=0x10000;
				create_lv_edit(win,&rect);
				str=malloc(str_size);
				if(win->hlvedit!=0 && str!=0){
					int len;
					str[0]=0;
					ListView_GetItemText(win->hlistview,index,win->selected_column,str,str_size);
					SetWindowText(win->hlvedit,str);
					len=strlen(str);
					if(len>0){
						int i,lf=0,c=0,max=0;
						set_status_bar_text(ghstatusbar,0,"len=%i",len);
						for(i=0;i<len;i++){
							c++;
							if(c>max)
								max=c;
							if(str[i]=='\n'){
								lf++;
								c=0;
							}
						}
						if(lf>0){
							int w,h,x,y;
							w=rect.right-rect.left;
							h=rect.bottom-rect.top;
							x=rect.left-4; //account for border
							y=rect.top-4;
							if(w<max*8) //try to fit the widest string
								w=max*8;
							lf++; //add last line
							if(lf>100)
								lf=100;
							w+=8; //account for border
							h+=8+(lf*h); //account for border and lines
							//now move the window up/left if its out of the client area
							GetClientRect(GetParent(win->hlvedit),&rect);
							if(x+w>rect.right)
								x-=(x+w)-rect.right;
							if(y+h>rect.bottom)
								y-=(y+h)-rect.bottom;
							if(x<0)
								x=0;
							if(y<0)
								y=0;
							SetWindowPos(win->hlvedit,NULL,x,y,
								w,h,SWP_NOZORDER);

						}
					}
					SetFocus(win->hlvedit);
					result=TRUE;
				}
				if(str!=0)
					free(str);
			}
		}
	}
	return result;
}
int create_lv_menus()
{
	if(lv_col_menu!=0)DestroyMenu(lv_col_menu);
	if(lv_col_menu=CreatePopupMenu()){
		InsertMenu(lv_col_menu,0xFFFFFFFF,MF_BYPOSITION|MF_STRING,CMD_COL_WIDTH_HEADER,"col width from header");
		InsertMenu(lv_col_menu,0xFFFFFFFF,MF_BYPOSITION|MF_STRING,CMD_COL_WIDTH_DATA,"col width from data");
		InsertMenu(lv_col_menu,0xFFFFFFFF,MF_BYPOSITION|MF_STRING,CMD_COL_INFO,"col info");
	}
	if(lv_menu!=0)DestroyMenu(lv_menu);
	if(lv_menu=CreatePopupMenu()){
		InsertMenu(lv_menu,0xFFFFFFFF,MF_BYPOSITION|MF_STRING,CMD_COL_INFO,"col info");
		InsertMenu(lv_menu,0xFFFFFFFF,MF_BYPOSITION|MF_SEPARATOR,0,0);
		InsertMenu(lv_menu,0xFFFFFFFF,MF_BYPOSITION|MF_STRING,CMD_SQL_SELECT_ALL,"SQL select *");
		InsertMenu(lv_menu,0xFFFFFFFF,MF_BYPOSITION|MF_STRING,CMD_SQL_WHERE,"SQL where =");
		InsertMenu(lv_menu,0xFFFFFFFF,MF_BYPOSITION|MF_STRING,CMD_SQL_ORDERBY,"SQL order by");
		InsertMenu(lv_menu,0xFFFFFFFF,MF_BYPOSITION|MF_STRING,CMD_SQL_GROUPBY,"SQL group by [col]");
		InsertMenu(lv_menu,0xFFFFFFFF,MF_BYPOSITION|MF_STRING,CMD_SQL_SELECT_COL,"SQL select [col]");
		InsertMenu(lv_menu,0xFFFFFFFF,MF_BYPOSITION|MF_SEPARATOR,0,0);
		InsertMenu(lv_menu,0xFFFFFFFF,MF_BYPOSITION|MF_STRING,CMD_SQL_UPDATE,"SQL update where =");
		InsertMenu(lv_menu,0xFFFFFFFF,MF_BYPOSITION|MF_STRING,CMD_SQL_DELETE,"SQL delete from [] where =");
		InsertMenu(lv_menu,0xFFFFFFFF,MF_BYPOSITION|MF_SEPARATOR,0,0);
		InsertMenu(lv_menu,0xFFFFFFFF,MF_BYPOSITION|MF_STRING,CMD_EXPORT_DATA,"export data");
	}
	return TRUE;
}
int create_lv_edit(TABLE_WINDOW *win,RECT *rect)
{
	int result=FALSE;
	if(win!=0 && rect!=0){
		int x,y,w,h;
		if(win->hlvedit!=0)
			destroy_lv_edit(win);
		x=rect->left-4;
		y=rect->top-4;
		w=rect->right - rect->left+8;
		h=rect->bottom - rect->top+8;
		win->hlvedit = CreateWindow("EDIT", //"RichEdit50W",
										 "",
										 WS_THICKFRAME|WS_TABSTOP|WS_CHILD|WS_CLIPCHILDREN|WS_VISIBLE|
										 ES_LEFT|ES_AUTOHSCROLL|ES_MULTILINE|ES_AUTOVSCROLL,
										 x,y,
										 w,h,
										 win->hlistview,
										 IDC_LV_EDIT,
										 ghinstance,
										 NULL);
		if(win->hlvedit!=0){
			HFONT hfont=SendMessage(win->hlistview,WM_GETFONT,0,0);
			if(hfont!=0)
				SendMessage(win->hlvedit,WM_SETFONT,hfont,0);
			lvorigedit=SetWindowLong(win->hlvedit,GWL_WNDPROC,(LONG)sc_lvedit);
			SetWindowText(ghstatusbar,"");
			result=TRUE;
		}
	}
	return result;
}

int destroy_lv_edit(TABLE_WINDOW *win)
{
	if(win!=0 && win->hlvedit!=0){
		SendMessage(win->hlvedit,WM_CLOSE,0,0);
		win->hlvedit=0;
	}
	return TRUE;
}