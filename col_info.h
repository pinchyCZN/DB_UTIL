
int find_win_by_hlistview(HWND hwnd,TABLE_WINDOW **win)
{
	int i;
	static TABLE_WINDOW *old_win=0;
	if(old_win!=0 && old_win->hlistview==hwnd){
		*win=old_win;
		return TRUE;
	}
	for(i=0;i<sizeof(table_windows)/sizeof(TABLE_WINDOW);i++){
		if(table_windows[i].hlistview==hwnd){
			*win=&table_windows[i];
			old_win=*win;
			return TRUE;
		}
	}
	old_win=0;
	return FALSE;
}
static const char *SQL_TYPES[]={
	"SQL_UNKNOWN_TYPE",0,
	"SQL_CHAR",1,
	"SQL_NUMERIC",2,
	"SQL_DECIMAL",3,
	"SQL_INTEGER",4,
	"SQL_SMALLINT",5,
	"SQL_FLOAT",6,
	"SQL_REAL",7,
	"SQL_DOUBLE",8,
	"SQL_DATETIME",9,
	"SQL_VARCHAR",12,
	"SQL_TYPE_DATE",91,
	"SQL_TYPE_TIME",92,
	"SQL_TYPE_TIMESTAMP",93,
	"SQL_LONGVARCHAR",-1,
	"SQL_BINARY",-2,
	"SQL_VARBINARY",-3,
	"SQL_LONGVARBINARY",-4,
	"SQL_BIGINT",-5,
	"SQL_TINYINT",-6,
	"SQL_BIT",-7,
	"SQL_BLOB",-10,
	"SQL_CLOB",-11,
	"SQL_OTHER",100
};
int find_sql_type_str(int type,const char **str)
{
	int i;
	for(i=0;i<sizeof(SQL_TYPES)/sizeof(char *);i+=2){
		if(SQL_TYPES[i+1]==type){
			*str=SQL_TYPES[i];
			return TRUE;
		}
	}
	return FALSE;
}
static int find_num_cols(char *str)
{
	int count=0;
	if(str!=0){
		int i,len;
		len=strlen(str);
		for(i=0;i<len;i++){
			if(str[i]=='\t')
				count++;
			else if(str[i]=='\n'){
				count++;
				break;
			}
			else if(str[i]==0)
				break;
		}
	}
	return count;
}
int populate_col_info(HWND hwnd,HWND hlistview,LPARAM lparam)
{
	int i;

	if(IsWindow(lparam)){
		TABLE_WINDOW *win=0;
		char *cols[]={"field name","type","type #","size","index"};
		for(i=0;i<sizeof(cols)/sizeof(char *);i++)
			lv_add_column(hlistview,cols[i],i);
		if(find_win_by_hlistview(lparam,&win)){
			for(i=0;i<win->columns;i++){
				char str[20]={0};
				char name[80]={0};
				char *sql_name="";
				lv_get_col_text(win->hlistview,i,name,sizeof(name));
				lv_insert_data(hlistview,i,0,name);
				find_sql_type_str(win->col_attr[i].type,&sql_name);
				lv_insert_data(hlistview,i,1,sql_name);
				_snprintf(str,sizeof(str),"%i",win->col_attr[i].type);
				lv_insert_data(hlistview,i,2,str);
				_snprintf(str,sizeof(str),"%i",win->col_attr[i].length);
				lv_insert_data(hlistview,i,3,str);
				_snprintf(str,sizeof(str),"%i",i);
				lv_insert_data(hlistview,i,4,str);
			}
			if(win->table[0]!=0)
				SetWindowText(hwnd,win->table);
		}
		for(i=0;i<sizeof(cols)/sizeof(char *);i++){
			int method=LVSCW_AUTOSIZE;
			if(i>=2)
				method=LVSCW_AUTOSIZE_USEHEADER;
			ListView_SetColumnWidth(hlistview,i,method);
		}
	}
	else if(lparam!=0){
		char *str=lparam;
		int len=0;
		int index=0;
		int count=0;
		int found=FALSE;
		int col_row=TRUE;
		int *widths;
		int num_cols;
		{
			char tmp[80]={0};
			char *title="Info";
			for(i=0;i<sizeof(tmp)-1;i++){
				if(str[i]==0 || str[i]=='\n')
					break;
				tmp[i]=str[i];
			}
			tmp[i]=0;
			if(strlen(tmp)==0)
				SetWindowText(hwnd,title);
			else
				SetWindowText(hwnd,tmp);
			title=strchr(str,'\n');
			if(title!=0)
				str=title+1;
		}
		len=strlen(str);
		num_cols=find_num_cols(str);
		widths=malloc(num_cols*sizeof(int));
		if(widths!=0)
			memset(widths,0,num_cols*sizeof(int));
		for(i=0;i<len;i++){
			if(str[i]=='\t'){
				index++;
				found=FALSE;
			}
			else if(str[i]=='\n'){
				if(!col_row)
					count++;
				index=0;
				col_row=FALSE;
				found=FALSE;
			}
			else if(str[i]==0)
				break;
			else if(found==FALSE){
				char tmp[256]={0};
				int w;
				sscanf(str+i,"%255[ -~]",tmp);
				tmp[sizeof(tmp)-1]=0;
				if(col_row){
					if(tmp[0]==0){
						_snprintf(tmp,sizeof(tmp),"empty");
					}
					w=lv_add_column(hlistview,tmp,index);
					if(widths!=0){
						if(index<num_cols)
							widths[index]=w;
					}
				}
				else{
					lv_insert_data(hlistview,count,index,tmp);
					w=get_str_width(hlistview,tmp)+12;
					if(widths!=0){
						if(index<num_cols){
							if(w>widths[index])
								widths[index]=w;
						}
					}
				}
				found=TRUE;
			}
		}
		if(widths!=0){
			for(i=0;i<num_cols;i++){
				ListView_SetColumnWidth(hlistview,i,widths[i]);
			}
			free(widths);
		}
	}
	return TRUE;
}
struct find_helper{
	int dir;
	int col;
	HWND hlistview;
};
int CALLBACK compare_func(LPARAM lparam1, LPARAM lparam2,struct find_helper *fh)
{
	LV_FINDINFO find1,find2;
	char str1[80]={0},str2[80]={0};
	int index1,index2;
	find1.flags=LVFI_PARAM;
	find1.lParam=lparam1;
	find2.flags=LVFI_PARAM;
	find2.lParam=lparam2;
	index1=ListView_FindItem(fh->hlistview,-1,&find1);
	index2=ListView_FindItem(fh->hlistview,-1,&find2);
	if(index1>=0 && index2>=0){
		int result;
		ListView_GetItemText(fh->hlistview,index1,fh->col,str1,sizeof(str1));
		ListView_GetItemText(fh->hlistview,index2,fh->col,str2,sizeof(str2));
		if(isdigit(str1[0]) && isdigit(str2[0])){
			int a,b;
			a=atoi(str1);
			b=atoi(str2);
			if(a<b)
				result=-1;
			else if(a==b)
				result=0;
			else
				result=1;
		}
		else
			result=_stricmp(str1,str2);
		if(fh->dir)
			result=-result;
		return result;
	}
	return 0;
}

int fit_win_to_data(HWND hlistview,HWND hwnd)
{
	int col_count,row_count;
	col_count=lv_get_column_count(hlistview);
	if(col_count>0){
		int width=0;
		int height=0;
		row_count=ListView_GetItemCount(hlistview);
		if(row_count>0){
			RECT rect={0};
			ListView_GetItemRect(hlistview,0,&rect,LVIR_BOUNDS);
			width=rect.right-rect.left;
			height=(rect.bottom-rect.top+2)*row_count;
		}
		if(width>100){
			RECT rect={0};
			int flags=SWP_NOZORDER|SWP_NOMOVE;
			int x=0,y=0;
			width+=8;
			height+=100;
			if(GetWindowRect(hwnd,&rect)!=0){
				RECT nrect={0};
				if(get_nearest_monitor(rect.left,rect.top,rect.right-rect.left,rect.bottom-rect.top,&nrect)){
					if(width>(nrect.right-nrect.left)){
						width=nrect.right-nrect.left;
						x=nrect.left;
						y=rect.top;
						flags=SWP_NOZORDER;
					}
					if(height>(nrect.bottom-nrect.top)){
						height=nrect.bottom-nrect.top;
						x=nrect.left;
						y=rect.top;
						flags=SWP_NOZORDER;
					}
				}
			}
			SetWindowPos(hwnd,NULL,x,y,width,height,flags);
		}

	}
	return TRUE;
}
int copy_cols_clip(HWND hlistview,int include_header)
{
	int count,cols,buf_size=0x10000,str_size=0x10000;
	char *buf,*str;
	int *widths;
	int rows_copied=0;
	count=ListView_GetItemCount(hlistview);
	buf=malloc(buf_size);
	str=malloc(str_size);
	cols=lv_get_column_count(hlistview);
	widths=malloc(cols*sizeof(int));
	if(buf!=0 && widths!=0 && str!=0){
		int i,buf_full=FALSE;
		char *out=buf;
		int out_len=buf_size;
		for(i=0;i<cols;i++){
			int j;
			widths[i]=0;
			for(j=0;j<count;j++){
				int w;
				str[0]=0;
				ListView_GetItemText(hlistview,j,i,str,str_size);
				str[str_size-1]=0;
				w=strlen(str)+2;
				if(w>widths[i])
					widths[i]=w;
			}
			if(include_header){
				int w;
				str[0]=0;
				lv_get_col_text(hlistview,i,str,str_size);
				str[str_size-1]=0;
				w=strlen(str)+2;
				if(w>widths[i])
					widths[i]=w;
			}
		}
		buf[0]=0;
		for(i=0;i<count;i++){
			int j,stored;
			int selected=FALSE;
			if(i==0 && include_header){
				for(j=0;j<cols;j++){
					str[0]=0;
					lv_get_col_text(hlistview,j,str,str_size);
					str[str_size-1]=0;
					stored=_snprintf(out,out_len,"%-*s%s",widths[j],str,j==(cols-1)?"\n":"");
					if(stored>0){
						out+=stored;
						out_len-=stored;
						if(out_len<0)
							out_len=0;
					}
					else if(stored<0){
						out_len=0;
						buf_full=TRUE;
					}
				}
			}
			for(j=0;j<cols;j++){
				if(ListView_GetItemState(hlistview,i,LVIS_SELECTED)==LVIS_SELECTED){
					str[0]=0;
					ListView_GetItemText(hlistview,i,j,str,str_size);
					str[str_size-1]=0;
					stored=_snprintf(out,out_len,"%-*s%s",widths[j],str,j==(cols-1)?"\n":"");
					selected=TRUE;
					if(stored>0){
						out+=stored;
						out_len-=stored;
						if(out_len<0)
							out_len=0;
					}
					else if(stored<0){
						out_len=0;
						buf_full=TRUE;
					}
				}
				else
					break;
			}
			if(selected)
				rows_copied++;
			if(buf_full)
				break;
		}
		buf[buf_size-1]=0;
		copy_str_clipboard(buf);
	}
	if(buf!=0)
		free(buf);
	if(widths!=0)
		free(widths);
	if(str!=0)
		free(str);
	return rows_copied;
}
int update_sort_col(HWND hlistview,int dir,int column)
{
	int i,count,expanded=0;
	count=lv_get_column_count(hlistview);
	for(i=0;i<count;i++){
		WCHAR str[80]={0};
		LV_COLUMN col={0};
		int j,len;
		col.mask=LVCF_TEXT|LVCF_WIDTH;
		col.pszText=str;
		col.cchTextMax=sizeof(str)/sizeof(WCHAR);
		SendMessageW(hlistview,LVM_GETCOLUMNW,i,&col);
		len=wcslen(col.pszText);
		{
			int index=0;
			for(j=0;j<len;j++){
				WCHAR c=str[j];
				if(c==0x25BC || c==0x25B2)
					c=c;
				else
					str[index++]=str[j];
			}
			str[index]=0;
		}
		if(column==i){
			int width;
			if( len<=((sizeof(str)/sizeof(WCHAR))-2) ){
				str[len+1]=0;
				for(j=len;j>0;j--){
					str[j]=str[j-1];
				}
				str[0]=dir?0x25BC:0x25B2;
			}
			width=get_string_width_wc(hlistview,str,TRUE)+14;
			if(width>col.cx){
				expanded+=width-col.cx;
				col.cx=width;
			}
		}
		col.pszText=str;
		col.cchTextMax=sizeof(str)/sizeof(WCHAR);
		SendMessageW(hlistview,LVM_SETCOLUMNW,i,&col);
	}
	if(expanded>0){
		HWND hparent=GetParent(hlistview);
		if(hparent){
			RECT rect={0};
			GetWindowRect(hparent,&rect);
			SetWindowPos(hparent,NULL,0,0,rect.right-rect.left+expanded,rect.bottom-rect.top,SWP_NOMOVE|SWP_NOZORDER);
		}
	}
	return expanded;
}
int sort_listview(HWND hlistview,int dir,int column)
{
	struct find_helper fh;
	fh.hlistview=hlistview;
	fh.dir=dir;
	fh.col=column;
	ListView_SortItems(hlistview,compare_func,&fh);
	update_sort_col(hlistview,dir,column);
	return TRUE;
}
LRESULT CALLBACK col_info_proc(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam)
{
	static HWND grippy=0,hlistview=0;
	static int sort_dir=0;

	if(FALSE)
	if(msg!=WM_NCHITTEST&&msg!=WM_SETCURSOR&&msg!=WM_ENTERIDLE&&msg!=WM_MOUSEMOVE&&msg!=WM_NCMOUSEMOVE)
	{
		static DWORD tick=0;
		if((GetTickCount()-tick)>500)
			printf("--\n");
		printf("ci");
		print_msg(msg,lparam,wparam,hwnd);
		tick=GetTickCount();
	}
	switch(msg){
	case WM_INITDIALOG:
		hlistview=CreateWindow(WC_LISTVIEW,"",WS_TABSTOP|WS_CHILD|WS_CLIPSIBLINGS|WS_VISIBLE|LVS_REPORT|LVS_SHOWSELALWAYS,
                                     0,0,
                                     0,0,
                                     hwnd,
                                     IDC_LIST1,
                                     ghinstance,
                                     NULL);
		ListView_SetExtendedListViewStyle(hlistview,LVS_EX_FULLROWSELECT);
		SendMessage(hlistview,WM_SETFONT,GetStockObject(get_font_setting(IDC_LISTVIEW_FONT)),0);
		populate_col_info(hwnd,hlistview,lparam);
		fit_win_to_data(hlistview,hwnd);
		SetFocus(hlistview);
		grippy=create_grippy(hwnd);
		resize_col_info(hwnd);
		sort_dir=0;
		break;
	case WM_SIZE:
		resize_col_info(hwnd);
		grippy_move(hwnd,grippy);
		break;
	case WM_NOTIFY:
		if(LOWORD(wparam)==IDC_LIST1){
			NMHDR *nmhdr=lparam;
			if(lparam==0)
				break;
			switch(nmhdr->code){
			case LVN_KEYDOWN:
				{
				LV_KEYDOWN *lvk=lparam;
				switch(lvk->wVKey){
				case 'A':
					if(GetKeyState(VK_CONTROL)&0x8000){
						int i,count;
						count=ListView_GetItemCount(hlistview);
						for(i=0;i<count;i++)
							ListView_SetItemState(hlistview,i,LVIS_FOCUSED|LVIS_SELECTED,LVIS_FOCUSED|LVIS_SELECTED);
					}
					break;
				case 'C':
					if(!(GetKeyState(VK_CONTROL)&0x8000))
						break;
					copy_cols_clip(hlistview,GetKeyState(VK_SHIFT)&0x8000);
					break;
				}
				}
				break;
			case  LVN_COLUMNCLICK:
				{
					NMLISTVIEW *nmlv=lparam;
					if(nmlv!=0){
						sort_listview(hlistview,sort_dir,nmlv->iSubItem);
						sort_dir=!sort_dir;
					}

				}
				break;
			}
		}
		break;
	case WM_COMMAND:
		switch(LOWORD(wparam)){
		case IDCANCEL:
			EndDialog(hwnd,0);
			break;
		}
		break;
	}
	return 0;
}