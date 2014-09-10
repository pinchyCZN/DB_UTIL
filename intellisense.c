#include <windows.h>
#include <commctrl.h>
#include <richedit.h>
#include "resource.h"
#include "structs.h"
extern HANDLE ghtreeview,ghmdiclient;


HANDLE intellisense_event;
char tab_word[20]={0};
int tab_continue=FALSE,tab_pos=0;
#define TABLE_MODE 1
#define FIELD_MODE 0
enum {
	MSG_ADD_DB,
	MSG_ADD_TABLE,
	MSG_ADD_FIELD,
	MSG_DEL_DB,
	MSG_DEL_TABLE,
	MSG_DEL_FIELD
};
typedef struct{
	char *table;
	int field_count;
	char *fields;
	void *prev;
	void *next;
}TABLE_INFO;
typedef struct{
	char *name;
	int table_count;
	TABLE_INFO *table_info;
	void *prev;
	void *next;
}DB_INFO;
DB_INFO *top=0;

int populate_intel(TABLE_WINDOW *win,char *src,int cur_pos,int mode,int *width)
{
	int max_width=0;
	if(win!=0 && src!=0){
		int src_len=cur_pos;
		if(src_len<=0)
			src_len=1; //strlen(src);
		SendMessage(win->hintel,LB_RESETCONTENT,0,0);
		//if(mode==TABLE_MODE)
		{
			DB_INFO *db=0;
			TABLE_INFO *t=0;
			if(find_db_node(win->name,&db))
				t=db->table_info;
			while(t!=0){
				int index;
				char *str=t->table;
				if(strnicmp(str,src,src_len)==0){
					index=SendMessage(win->hintel,LB_FINDSTRINGEXACT,-1,str);
					if(index==LB_ERR){
						int w;
						SendMessage(win->hintel,LB_ADDSTRING,0,str);
						w=get_str_width(win->hintel,str);
						if(w>max_width)
							max_width=w;
					}
				}
				else{
					index=SendMessage(win->hintel,LB_FINDSTRINGEXACT,-1,str);
					if(index!=LB_ERR)
						SendMessage(win->hintel,LB_DELETESTRING,index,0);
				}
				t=t->next;
			}
		}
		//else
		{ //field mode
			DB_INFO *db=0;
			TABLE_INFO *t=0;
			if(find_db_node(win->name,&db))
				t=db->table_info;
			while(t!=0){
				if(win->table==0 || win->table[0]==0){
					t=0;
					break;
				}
				if(stricmp(t->table,win->table)==0)
					break;
				t=t->next;
			}
			if(t!=0 && t->fields!=0){
				int i,len,index;
				char *sptr=t->fields;
				char str[80]={0};
				len=safe_strlen(sptr);
				index=0;
				for(i=0;i<len;i++){
					if(sptr[i]!='\n')
						if(index < (sizeof(str)-1))
							str[index++]=sptr[i];

					if(sptr[i]=='\n' || sptr[i+1]==0){
						int lbindex;
						str[index]=0;
						if(index==0)
							continue;
						else
							index=0;
						if(strnicmp(str,src,src_len)==0){
							lbindex=SendMessage(win->hintel,LB_FINDSTRINGEXACT,-1,str);
							if(lbindex==LB_ERR){
								int w;
								SendMessage(win->hintel,LB_ADDSTRING,0,str);
								w=get_str_width(win->hintel,str);
								if(w>max_width)
									max_width=w;
							}
						}
						else{
							lbindex=SendMessage(win->hintel,LB_FINDSTRINGEXACT,-1,str);
							if(lbindex!=LB_ERR)
								SendMessage(win->hintel,LB_DELETESTRING,lbindex,0);
						}
					}
				}
			}
		}
		//if nothing so far add reserved words if they match
		if(SendMessage(win->hintel,LB_GETCOUNT,0,0)<=0){
			static char *sql_major_words[]={
				"SELECT","FROM","WHERE","INNER","JOIN","ORDER","UPDATE","INSERT","DELETE"
			};
			int i;
			for(i=0;i<sizeof(sql_major_words)/sizeof(char *);i++){
				char *str=sql_major_words[i];
				if(str==0)
					continue;
				if(strnicmp(str,src,src_len)==0){
					int lbindex;
					lbindex=SendMessage(win->hintel,LB_FINDSTRINGEXACT,-1,str);
					if(lbindex==LB_ERR){
						int w;
						SendMessage(win->hintel,LB_ADDSTRING,0,str);
						w=get_str_width(win->hintel,str);
						if(w>max_width)
							max_width=w;
					}
				}
			}
		}
		if(SendMessage(win->hintel,LB_GETCOUNT,0,0)<=0)
			ShowWindow(win->hintel,SW_HIDE);
		else{
			int index=SendMessage(win->hintel,LB_FINDSTRING,-1,src);
			if(index<0)
				index=0;
			SendMessage(win->hintel,LB_SETCURSEL,index,0);
			*width=max_width;
			return TRUE;
		}
	}
	return FALSE;
}

int handle_intellisense(TABLE_WINDOW *win,char *str,int pos,int mode)
{
	char tab_word[80]={0};
	tab_word[0]=0;
	if(get_substr(str,pos,tab_word,sizeof(tab_word),&tab_pos)){
		tab_continue=TRUE;
		printf("substr=%s\n",tab_word);
		if(win->hintel!=0){
			int width=80;
			if(populate_intel(win,tab_word,pos-tab_pos,mode,&width)){
				POINT p={0};
				int cpos=0;
				SendMessage(win->hedit,EM_GETSEL,&cpos,0);
				cpos-=pos-tab_pos;
				if(cpos<0)
					cpos=0;
				SendMessage(win->hedit,EM_POSFROMCHAR,&p,cpos);
				
				if(width<100 || width>640)
					width=100;
				else
					width+=7;
				SetWindowPos(win->hintel,HWND_TOP,p.x,p.y+16,width,200,SWP_SHOWWINDOW);
			}
		}
	}
	else{
		ShowWindow(win->hintel,SW_HIDE);
	}
	return tab_continue;
}

int find_word_start(unsigned char *str,int pos,int *start)
{
	int i,found=FALSE;
	if(is_word_boundary(str[pos])){
		pos--;
		if(pos<0)
			pos=0;
	}

	if(str[pos]<=' ')
		return FALSE;
	for(i=pos;i>=0;i--){
		if(is_word_boundary(str[i]))
			break;
		else
			found=TRUE;
	}
	i++;
	if(i<0)
		i=0;
	*start=i;
	return found;
}
int find_word_end(char *str,int pos,int *end)
{
	int i;
	for(i=pos;i<pos+255;i++){
		if(is_word_boundary(str[i]))
			break;
	}
	*end=i;
	return TRUE;
}

int is_word_boundary(unsigned char a)
{
	if(a<='/')
		return TRUE;
	else if(a=='_')
		return FALSE;
	else if(a>=':' && a<='@')
		return TRUE;
	else if(a>='[' && a<='^')
		return TRUE;
	else if(a=='`')
		return TRUE;
	else if(a>='{')
		return TRUE;
	else
		return FALSE;
}

int get_substr(unsigned char *str,int start,char *substr,int size,int *pos)
{
	int i,index,len,found=FALSE;
	len=strlen(str);
	if(start<=len){
		if(start>0 && str[start-1]<=' ' && str[start]<=' ')
			return found;
		if(start==0 && str[0]<=' ')
			return found;

		while(start>0){
			start--;
			if(is_word_boundary(str[start])){ //if(str[start]<=' ' || (str[start]>='!' && str[start]<='/')){
				start++;
				break;
			}
		}
		*pos=start;
		len=strlen(str+start);
		index=0;
		for(i=0;i<len;i++){
			if(is_word_boundary(str[start+i])){ //str[start+i]<=' ' || (str[start+i]>='!' && str[start+i]<='/')){
				break;
			}
			else
				substr[index++]=str[start+i];
			if(index>=(size-1))
				break;
		}
		substr[index++]=0;
		if(substr[0]!=0)
			found=TRUE;
	}
	return found;
}


int testit(TABLE_WINDOW *win)
{
	int start,end;
	int line;

	SendMessage(win->hedit,EM_GETSEL,&start,&end);
	if(end<start)
		start=end;
	line=SendMessage(win->hedit,EM_LINEFROMCHAR,start,0);
	if(line>=0){
		char s[1024];
		int len,lindex,linestart;
		((WORD*)s)[0]=sizeof(s);
		len=SendMessage(win->hedit,EM_GETLINE,line,s);
		lindex=SendMessage(win->hedit,EM_LINEINDEX,line,0);
		linestart=start-lindex;
		if(linestart>=0 && len>0){
			int wordstart=linestart;
			s[len-1]=0;
			if(find_word_start(s,linestart,&wordstart))
				printf("wordstart=%s\n",s+wordstart);
		}
	}
	return TRUE;
}
int replace_current_word(TABLE_WINDOW *win,char *str)
{
	int start=0,end=0,line;
	if(win==0)
		return FALSE;
	if(win->hedit==0)
		return FALSE;
	SendMessage(win->hedit,EM_GETSEL,&start,&end);
	line=SendMessage(win->hedit,EM_LINEFROMCHAR,start,0);
	if(line>=0){
		char s[1024];
		int len,lindex,linestart;
		((WORD*)s)[0]=sizeof(s);
		len=SendMessage(win->hedit,EM_GETLINE,line,s);
		lindex=SendMessage(win->hedit,EM_LINEINDEX,line,0);
		linestart=start-lindex;
		if(linestart>=0 && len>0){
			int wordstart=linestart,wordend=linestart;
			s[len-1]=0;
			if(find_word_start(s,linestart,&wordstart)){
				int wstart=lindex+wordstart;
				find_word_end(s,linestart,&wordend);
				if((wstart-start)==0) //if cursor is at start of word replace whole word
					end=lindex+wordend;
				start=wstart;
				SendMessage(win->hedit,EM_SETSEL,start,end);
				SendMessage(win->hedit,EM_REPLACESEL,TRUE,str);
				return TRUE;
			}
		}
	}
	return FALSE;
}
int insert_selection(TABLE_WINDOW *win)
{
	if(win!=0 && IsWindowVisible(win->hintel)){
		int sel=SendMessage(win->hintel,LB_GETCURSEL,0,0);
		if(sel>=0){
			int len;
			char str[256]={0};
			len=SendMessage(win->hintel,LB_GETTEXTLEN,sel,0);
			if(len<sizeof(str)){
				SendMessage(win->hintel,LB_GETTEXT,sel,str);
				if(str[0]!=0){
					return replace_current_word(win,str);
				}
			}
		}
	}
	return FALSE;
}

int check_LR(TABLE_WINDOW *win,int vkey)
{
	int start=0,end=0,line;
	if(win==0)
		return FALSE;
	if(win->hedit==0)
		return FALSE;
	SendMessage(win->hedit,EM_GETSEL,&start,&end);
	if(end<start)
		start=end;
	if(vkey==VK_LEFT)
		start--;
	else if(vkey==VK_RIGHT)
		start++;
	if(start<0)
		start=0;
	line=SendMessage(win->hedit,EM_LINEFROMCHAR,start,0);
	if(line>=0){
		unsigned char s[1024];
		int len,lindex,linestart;
		((WORD*)s)[0]=sizeof(s);
		len=SendMessage(win->hedit,EM_GETLINE,line,s);
		lindex=SendMessage(win->hedit,EM_LINEINDEX,line,0);
		linestart=start-lindex;
		if(linestart>=0 && len>0){
			s[len-1]=0;
			printf("current char:%c\n",s[linestart]);
			if(linestart>0 && s[linestart]<=' ')
				ShowWindow(win->hintel,SW_HIDE);
		}
	}
	return FALSE;
}
int seek_word_edge(HWND hedit,int vkey)
{
	int start=0,end=0;
	int line,lineindex;
	char str[512];
	SendMessage(hedit,EM_GETSEL,&start,&end);
	line=SendMessage(hedit,EM_LINEFROMCHAR,start,0);
	lineindex=SendMessage(hedit,EM_LINEINDEX,line,0);
	((WORD*)str)[0]=sizeof(str);
	SendMessage(hedit,EM_GETLINE,line,str);
	str[sizeof(str)-1]=0;
	if(vkey==VK_END){
		find_word_end(str,start-lineindex,&end);
		SendMessage(hedit,EM_SETSEL,lineindex+end,lineindex+end);
	}
	else{
		find_word_start(str,start-lineindex,&start);
		SendMessage(hedit,EM_SETSEL,lineindex+start,lineindex+start);
	}
	return TRUE;
}
int display_line_pos(TABLE_WINDOW *win)
{
	extern HWND ghstatusbar;
	if(win && win->hedit){
		CHARRANGE range={0};
		int line;
		SendMessage(win->hedit,EM_EXGETSEL,0,&range);
		line=SendMessage(win->hedit,EM_EXLINEFROMCHAR,0,range.cpMin);
		if(line>=0){
			set_status_bar_text(ghstatusbar,1,"line=%i",line+1);
		}
	}
	return TRUE;
}
static WNDPROC wporigtedit=0;
static LRESULT APIENTRY sc_edit(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam)
{
	static int last_insert=FALSE;
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
	case WM_CHAR:
		switch(wparam){
		case VK_RETURN:
			if(last_insert)
				return TRUE;
			break;
		case VK_ESCAPE:
			break;
		default:
			{
			int ctrl=GetKeyState(VK_CONTROL)&0x8000;
			if((!ctrl) || wparam==VK_SPACE)
				PostMessage(hwnd,WM_USER,wparam,lparam);
			if(wparam==VK_SPACE && ctrl)
				return 0;
			break;
			}
		}
		break;
	case WM_KEYUP:
		{
		TABLE_WINDOW *win=0;
		find_win_by_hedit(hwnd,&win);
		if(win==0)
			break;
		switch(wparam){
		case VK_F5:
			break;
		}
		}
		break;
	case WM_SYSKEYDOWN:
		if(wparam==VK_RETURN){
			TABLE_WINDOW *win=0;
			if(find_win_by_hedit(hwnd,&win)){
				if(GetKeyState(VK_MENU)&0x8000){
					int message=WM_MDIMAXIMIZE;
					if(IsZoomed(win->hwnd))
						message=WM_MDIRESTORE;
					SendMessage(ghmdiclient,message,win->hwnd,0);
				}
			}
		}
		break;
	case WM_KEYFIRST:
		{
		TABLE_WINDOW *win=0;
		find_win_by_hedit(hwnd,&win);
		if(win==0)
			break;
		switch(wparam){
		case VK_HOME:
		case VK_END:
			if(IsWindowVisible(win->hintel)){
				seek_word_edge(hwnd,wparam);
				post_intel_msg(WM_USER,win,wparam);
				return TRUE;
			}
			else
				PostMessage(hwnd,WM_APP,win,0);
			break;
		case VK_PRIOR:
		case VK_NEXT:
		case VK_UP:
		case VK_DOWN:
			if(IsWindowVisible(win->hintel)){
				SendMessage(win->hintel,msg,wparam,lparam);
				return 0;
			}
			else
				PostMessage(hwnd,WM_APP,win,0);
			break;
		case VK_RETURN:
			{
				int result=insert_selection(win);
				ShowWindow(win->hintel,SW_HIDE);
				if(result){
					last_insert=TRUE;
					return TRUE;
				}
			}
			break;
		case VK_ESCAPE:
			if(IsWindowVisible(win->hintel)){
				ShowWindow(win->hintel,SW_HIDE);
				return 0;
			}
			else{
				SendMessage(win->hwnd,WM_USER,0,IDC_MDI_EDIT);
				SetFocus(win->hlistview);
			}
			break;
		case VK_LEFT:
		case VK_RIGHT:
			check_LR(win,wparam);
			if(IsWindowVisible(win->hintel))
				post_intel_msg(WM_USER,win,wparam);
			break;
		default:
			break;

		}
		}
		break;
	case WM_HELP:
		{
			static char *help="ODBC Datetime format:\r\n"
				"{ ts '1998-05-02 01:23:56.123' }\r\n"
				"{ d '1990-10-02' }\r\n"
				"{ t '13:33:41' }\r\n";
		MessageBox(hwnd,help,"SQL HELP",MB_OK);
		}
		break;
	case WM_APP:
		{
			TABLE_WINDOW *win=wparam;
			display_line_pos(win);
		}
		break;
	case WM_USER:
		{
		TABLE_WINDOW *win=0;
		find_win_by_hedit(hwnd,&win);
		if(win!=0)
			post_intel_msg(WM_USER,win,wparam);
		}
		break;
	}
	
    return CallWindowProc(wporigtedit,hwnd,msg,wparam,lparam);
}
int subclass_edit(HWND hedit)
{
	wporigtedit=SetWindowLong(hedit,GWL_WNDPROC,(LONG)sc_edit);
	printf("subclass=%08X\n",wporigtedit);
	return wporigtedit;
}
static int gintellisense_tid=0;
int assert()
{
	return 0;
}
int get_left_char(int pos,unsigned char *str)
{
	if(pos>0)
		return str[pos-1];
	else
		return 0;
}
void __cdecl intellisense_thread(void)
{
	void *pParser=0; //ParseAlloc(malloc);
#define sizeof_str 0x7FFF //MAXWORD largest em_getline allows
	static unsigned char str[sizeof_str];

	while(TRUE){
		MSG msg;
		int result;
		//printf("intellisense_thread waiting for msg\n");
		result=GetMessage(&msg,NULL,0,0);
		if(result>0){
			//printf("int");
			//print_msg(msg.message,msg.lParam,msg.wParam,msg.hwnd);
			switch(msg.message){
			case WM_USER:
				{
				int line=0,pos=0;
				static unsigned char lastleftchar=0;
				TABLE_WINDOW *win=msg.wParam;
				//wparam=win lparam=key
				str[0]=0;
				SendMessage(win->hedit,EM_GETSEL,&pos,NULL);
				line=SendMessage(win->hedit,EM_LINEFROMCHAR,pos,NULL);
				((WORD*)str)[0]=sizeof_str; //MAXWORD is largest allowed
				SendMessage(win->hedit,EM_GETLINE,line,str);
				str[sizeof_str-1]=0;
				line=SendMessage(win->hedit,EM_LINEINDEX,-1,0);
				pos=pos-line;
				if(pos<0)
					pos=0;
				else if(pos>=sizeof_str-1)
					pos=sizeof_str-1;

				//check edge cases where we dont want the intelli window to appear
				if(msg.lParam==VK_SPACE && (GetKeyState(VK_CONTROL)&8000)){
					lastleftchar=get_left_char(pos,str);
				}
				else if(msg.lParam==VK_BACK && win!=0 && win->hintel!=0 && (0==IsWindowVisible(win->hintel))){
					int do_break=FALSE;
					if(pos>0){
						if(!is_word_boundary(str[pos-1])){
							if(is_word_boundary(lastleftchar))
								do_break=TRUE;
						}
						else
							do_break=TRUE;
					}
					else
						do_break=TRUE;

					lastleftchar=get_left_char(pos,str);
					if(do_break)
						break;
				}
				else{
					lastleftchar=get_left_char(pos,str);
				}

				if(str[0]!=0){
					int yv,mode;
					printf("str=%40s\n",str);
					if(pParser!=0){
						strupr(str);
						yy_scan_string(str);
						Parse(pParser,0,0);
						while( (yv=yylex()) != 0){
							Parse(pParser,yv,0);
						}
						Parse(pParser,yv,0);
					}
					mode=get_sql_mode();
					if(mode==0){
						handle_intellisense(win,str,pos,mode);
					}
					else if(mode==1){ //table mode
						handle_intellisense(win,str,pos,mode);
					}
				}
				}
				break;
			case WM_USER+1:
				if(msg.lParam==0)
					break;
				switch(msg.wParam){
				case MSG_ADD_DB:
					add_db_node(msg.lParam);
					free((void*)msg.lParam);
					break;
				case MSG_DEL_DB:
					del_db_node(msg.lParam);
					free((void*)msg.lParam);
					break;
				case MSG_ADD_TABLE:
					add_table(msg.lParam);
					free((void*)msg.lParam);
					break;
				case MSG_ADD_FIELD:
					add_field(msg.lParam);
					free((void*)msg.lParam);
					break;
				default:
					if(msg.lParam!=0)
						free((void*)msg.lParam);
				}
				break;
			default:
				DispatchMessage(&msg);
				break;
			}
		}
		else if(result==0){
			printf("received wm_quit\n");
		}
		else
			printf("get message %i\n",result);
	}
	ParseFree(pParser,free);
	_endthreadex(0);
}
int start_intellisense_thread()
{
	_beginthreadex(NULL,0,intellisense_thread,NULL,0,&gintellisense_tid);
	return gintellisense_tid;
}

int post_intel_msg(int msg,WPARAM wparam,LPARAM lparam)
{
	if(gintellisense_tid!=0)
		return PostThreadMessage(gintellisense_tid,msg,wparam,lparam);
	else
		return 0;
}


int find_table(DB_INFO *db,char *table,TABLE_INFO **ti)
{
	int result=FALSE;
	TABLE_INFO *t=0;
	if(db==0 || table==0 || table[0]==0)
		return FALSE;
	t=db->table_info;
	if(t==0 && ti!=0){ //add it if none exist
		if(db->name!=0){
			add_table_node(db,table);
		}
		if(db->table_info!=0){
			*ti=db->table_info;
			result=TRUE;
		}
	}
	else{
		while(t!=0){
			if(stricmp(t->table,table)==0){
				if(ti!=0)
					*ti=t;
				result=TRUE;
				break;
			}
			t=t->next;
		}
	}
	return result;
}
// str format field1\nfield2\nfield3\n etc...
int find_field(char *str,char *field)
{
	int result=FALSE;
	int i,len,flen,index;
	if(str==0 || str[0]==0)
		return result;
	if(field==0 || field[0]==0)
		return result;
	len=strlen(str);
	flen=strlen(field);
	index=0;
	for(i=0;i<len;i++){
		if(field[index]==str[i])
			index++;
		else
			index=0;
		if(index==flen){
			if(str[i]=='\n' || str[i]==0){
				result=TRUE;
				break;
			}
		}
		if(str[i]=='\n')
			index=0;
	}
	return result;
}
int safe_strlen(char *str)
{
	if(str==0 || str[0]==0)
		return 0;
	else
		return strlen(str);
}
/*
str format: db_name\ntable\nfield
*/
int add_field(char *str)
{
	int result=FALSE;
	char *db_name=0;
	char *table=0;
	char *field=0;
	if(str==0 || str[0]==0)
		return result;
	table=strchr(str,'\n');
	if(table!=0){
		table[0]=0;
		table++;
		field=strchr(table,'\n');
		if(field!=0){
			field[0]=0;
			field++;
		}
		db_name=str;
	}
	if(table!=0 && table[0]!=0 && field!=0 && field[0]!=0){
		DB_INFO *db=0;
		TABLE_INFO *ti=0;
		if(find_db_node(db_name,&db) && find_table(db,table,&ti)){
			if(!find_field(ti->fields,field)){
				char *sptr=0;
				int len=safe_strlen(ti->fields)+1+strlen(field)+1;
				sptr=realloc(ti->fields,len);
				if(sptr!=0){
					if(ti->fields==0)
						sptr[0]=0;
					_snprintf(sptr,len,"%s\n%s",sptr,field);
					sptr[len-1]=0;
					ti->fields=sptr;
					ti->field_count++;
					result=TRUE;
				}
			}
		}
	}
//	dump_db_nodes();
	return result;
}
int add_table_node(DB_INFO *db,char *table)
{
	int result=FALSE;
	TABLE_INFO *t=0;
	t=malloc(sizeof(TABLE_INFO));
	if(t!=0){
		int len;
		memset(t,0,sizeof(TABLE_INFO));
		len=strlen(table)+1;
		t->table=malloc(len);
		if(t->table!=0){
			strncpy(t->table,table,len);
			t->table[len-1]=0;
			if(db->table_info==0){
				db->table_count++;
				db->table_info=t;
				result=TRUE;
			}
			else{
				TABLE_INFO *tnode=db->table_info;
				while(tnode!=0){
					if(tnode->next==0){
						db->table_count++;
						tnode->next=t;
						t->prev=tnode;
						result=TRUE;
						break;
					}
					else{
						tnode=tnode->next;
					}
				}
			}
			if(result==FALSE){
				free(t->table);
				free(t);
			}
		}
		else{
			free(t);
			result=FALSE;
		}
	}
	return result;
}
/*
format: db_name\ntable
*/
int add_table(char *str)
{
	int result=FALSE;
	char *db_name=0;
	char *table=0;
	if(str==0 || str[0]==0)
		return result;
	table=strchr(str,'\n');
	if(table!=0 && table[1]!=0){
		DB_INFO *db=0;
		table[0]=0;
		table++;
		db_name=str;
		if(find_db_node(db_name,&db)){
			if(find_table(db,table,0))
				result=TRUE;
			else{
				result=add_table_node(db,table);
			}
		}
	}
//	dump_db_nodes();
	return result;
}
int dump_db_nodes()
{
	DB_INFO *n=top;
	int i=0;
	printf(">>dumping db nodes\n");
	while(n!=0){
		TABLE_INFO *t;
		printf("\t%i %s\n",i,n->name);
		t=n->table_info;
		while(t!=0){
			printf("\t%s\n",t->table);
			if(t->fields!=0)
				printf("\tfields:\n%s\n",t->fields);
			t=t->next;
		}
		n=n->next;
		i++;
	}
	printf("<<\n");
	return 0;
}
int find_db_node(char *name,DB_INFO **db)
{
	DB_INFO *n=top;
	while(n!=0){
		if(n->name!=0){
			if(strcmp(n->name,name)==0){
				if(db!=0)
					*db=n;
				return TRUE;
			}
		}
		n=n->next;
	}
	return FALSE;
}
int add_db_node(char *name)
{
	int result=FALSE;
	int len;
	DB_INFO *db=0;
	char *n=0;
	if(name==0 || name[0]==0)
		return result;
	if(find_db_node(name,0))
		return TRUE;
	db=malloc(sizeof(DB_INFO));
	len=strlen(name)+1;
	n=malloc(len);
	if(db!=0 && n!=0){
		memset(db,0,sizeof(DB_INFO));
		db->name=n;
		strncpy(n,name,len);
		n[len-1]=0;
		if(top==0)
			top=db;
		else{
			DB_INFO *node=top;
			while(node->next!=0)
				node=node->next;
			node->next=db;
			db->prev=node;
		}
		result=TRUE;
	}else{
		if(db!=0)
			free(db);
		if(n!=0)
			free(n);
	}
//	dump_db_nodes();
	return result;
}

int free_table_info(TABLE_INFO *t)
{
	if(t==0)
		return FALSE;
	if(t->table!=0)
		free(t->table);
	if(t->fields!=0)
		free(t->fields);
	t->field_count=0;
	free(t);
	return TRUE;
}
int free_db_info(DB_INFO *db)
{
	if(db==0)
		return FALSE;
	if(db->name!=0)
		free(db->name);
	if(db->table_info!=0){
		TABLE_INFO *t=db->table_info;
		while(t!=0){
			TABLE_INFO *current=t;
			t=t->next;
			free_table_info(current);
			db->table_count--;
		}
	}
	free(db);
	return TRUE;
}
int del_db_node(char *name)
{
	int result=FALSE;
	DB_INFO *node=top;
	if(name==0 || name[0]==0)
		return result;
	if(node==0)
		return result;
	do{
		if(strcmp(node->name,name)==0){
			DB_INFO *p,*n;
			p=node->prev;
			n=node->next;
			if(p!=0)
				p->next=n;
			if(n!=0)
				n->prev=p;
			free_db_info(node);
			if(node==top)
				top=n;
			result=TRUE;
			break;
		}
		node=node->next;
	}while(node!=0);
//	dump_db_nodes();
	return result;
}
static int post_thread_msg(int msg,int wparam,int lparam)
{
	int result=FALSE;
	if(gintellisense_tid!=0){
		if(PostThreadMessage(gintellisense_tid,msg,wparam,lparam)!=0)
			result=TRUE;
	}
	return result;
}
int intelli_string_msg(char *str,int msg)
{
	int result=FALSE;
	if(str==0 || str[0]==0)
		return result;
	else{
		char *s;
		int len=strlen(str)+1;
		s=malloc(len);
		if(s==0)
			return result;
		strncpy(s,str,len);
		s[len-1]=0;
		if(!post_thread_msg(WM_USER+1,msg,s))
			free(s);
		else
			result=TRUE;
	}
	return result;
}
int intelli_add_db(char *name)
{
	return intelli_string_msg(name,MSG_ADD_DB);
}
int intelli_del_db(char *name)
{
	return intelli_string_msg(name,MSG_DEL_DB);
}
int intelli_add_table(char *dbname,char *table)
{
	char *s;
	int len;
	int result=FALSE;
	if(dbname==0 && dbname[0]==0)
		return result;
	if(table==0 && table[0]==0)
		return result;
	len=strlen(dbname)+1+strlen(table)+1;
	s=malloc(len);
	if(s!=0){
		_snprintf(s,len,"%s\n%s",dbname,table);
		result=intelli_string_msg(s,MSG_ADD_TABLE);
		free(s);
	}
	return result;
}
int intelli_add_field(char *dbname,char *table,char *field)
{
	char *s;
	int len;
	int result=FALSE;
	if(dbname==0 && dbname[0]==0)
		return result;
	if(table==0 && table[0]==0)
		return result;
	if(field==0 && field[0]==0)
		return result;
	len=strlen(dbname)+1+strlen(table)+1+strlen(field)+1;
	s=malloc(len);
	if(s!=0){
		_snprintf(s,len,"%s\n%s\n%s",dbname,table,field);
		result=intelli_string_msg(s,MSG_ADD_FIELD);
		free(s);
	}
	return result;

}