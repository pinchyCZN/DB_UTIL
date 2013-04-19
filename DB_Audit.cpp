#include "stdafx.h"
#include "afxdb.h"
#include "afxdao.h"

#include <conio.h>

#ifndef LockTypeEnum
#define LockTypeEnum _LockTypeEnum
#define FieldAttributeEnum _FieldAttributeEnum
#define DataTypeEnum _DataTypeEnum
#define RecordStatusEnum _RecordStatusEnum
#define EditModeEnum _EditModeEnum
#define ParameterDirectionEnum _ParameterDirectionEnum
#endif



#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

typedef struct{
	char name[80];
	int field_count;
} TABLE_DESCRIPTOR;

typedef struct   {
	char name[80];
	int  type;
	int	 prec;
	char def[80];
	char index; //A=asc D=desc
	int key;
	int required;
	int allowzlen;
	int autoinc;
} FIELD_DESCRIPTOR;

typedef struct {
	char	name[80];
	char	field[80];
	BOOL	descending;
	int		primary;
} INDEX_DESCRIPTOR;

/////////////////////////////////////////////////////////////////////////////
// The one and only application object

CWinApp theApp;

using namespace std;

char *g_dbpassword=0;
FILE *flog=0;

#define MAX_TABLE_COUNT 200
#define MAX_FIELD_COUNT 200

int key_ctrl=FALSE;
int key_shift=FALSE;
int extended_key=FALSE;
int getkey()
{
	int i=0;
	key_ctrl=FALSE;
	key_shift=FALSE;
	extended_key=FALSE;
	i=getch();
	if (GetKeyState(VK_SHIFT) < 0)
		key_shift=TRUE;
	if(GetKeyState(VK_CONTROL) < 0)
		key_ctrl=TRUE;
	if(i==0 || i==0xE0)
	{
		i=getch();
		extended_key=TRUE;
	}
	return i&0xFF;

}
int getkey2()
{
	int i=0;
	key_ctrl=FALSE;
	key_shift=FALSE;
	extended_key=FALSE;
	if(kbhit())
	{
		i=getch();
		if(i==0 || i==0xE0)
		{
			i=getch();
			extended_key=TRUE;
		}
		if (GetKeyState(VK_SHIFT) < 0)
			key_shift=TRUE;
		if(GetKeyState(VK_CONTROL) < 0)
			key_ctrl=TRUE;

	}
	return i&0xFF;

}
template <typename CHAR_TYPE>
CHAR_TYPE *stristr
(
   CHAR_TYPE         *  szStringToBeSearched, 
   const CHAR_TYPE   *  szSubstringToSearchFor
)
{
   CHAR_TYPE   *  pPos = NULL;
   CHAR_TYPE   *  szCopy1 = NULL;
   CHAR_TYPE   *  szCopy2 = NULL;

   // verify parameters
   if ( szStringToBeSearched == NULL || 
        szSubstringToSearchFor == NULL ) 
   {
      return szStringToBeSearched;
   }

   // empty substring - return input (consistent with strstr)
   if ( _tcslen(szSubstringToSearchFor) == 0 ) {
      return szStringToBeSearched;
   }

   szCopy1 = _tcslwr(_tcsdup(szStringToBeSearched));
   szCopy2 = _tcslwr(_tcsdup(szSubstringToSearchFor));

   if ( szCopy1 == NULL || szCopy2 == NULL  ) {
      // another option is to raise an exception here
      free((void*)szCopy1);
      free((void*)szCopy2);
      return NULL;
   }

   pPos = strstr(szCopy1, szCopy2);

   if ( pPos != NULL ) {
      // map to the original string
      pPos = szStringToBeSearched + (pPos - szCopy1);
   }

   free((void*)szCopy1);
   free((void*)szCopy2);

   return pPos;
}

UINT CALLBACK OFNHookProc(HWND hDlg,UINT Msg,WPARAM wParam,LPARAM lParam)
{

	HWND hWnd;
	RECT rect;
	static int init_size=TRUE,init_details=TRUE;
	static int scroll_pos=0;
	static int last_selection=0;

	switch(Msg)
	{
	case WM_INITDIALOG:
		init_details=TRUE;
		SetForegroundWindow(hDlg);
		return 0; //0=dialog process msg, nonzero=ignore
	case WM_NOTIFY:
        PostMessage( hDlg, WM_APP + 1, 0, 0 ); 
		return 0;
	case WM_APP + 1:
		{ 
			int i;
			HWND const dlg      = GetParent( hDlg ); 
			HWND defView  = GetDlgItem( dlg, 0x0461 ); 
			HWND list = GetDlgItem(defView,1);
			if(init_details)
			{
				SendMessage( defView, WM_COMMAND, 28716, 0 ); //details view
				ListView_EnsureVisible(list,last_selection,FALSE);
				ListView_SetItemState(list,last_selection,LVIS_SELECTED|LVIS_FOCUSED,LVIS_SELECTED|LVIS_FOCUSED);
				if(ListView_GetItemCount(list)>0)
				{
					ListView_SetColumnWidth(list,0,LVSCW_AUTOSIZE);
					ListView_SetColumnWidth(list,1,LVSCW_AUTOSIZE);
					ListView_SetColumnWidth(list,2,LVSCW_AUTOSIZE);
					ListView_SetColumnWidth(list,3,LVSCW_AUTOSIZE);
				}
				SetFocus(list);
				init_details=FALSE;
			}
			i=ListView_GetNextItem(list,-1,LVNI_SELECTED);
			if(i>=0)
				last_selection=i;

			hWnd=GetDesktopWindow();
			if(init_size && GetWindowRect(hWnd, &rect)) //only do at start , later resizing operations remain
			{
				SetWindowPos(dlg,HWND_TOP,0,0,rect.right*.75,rect.bottom*.75,NULL);
				init_size=FALSE;
			}
		} 
		return TRUE;
	case WM_DESTROY:
		return 0;
		break;
	default:  
		return 0;
	}

}



static TCHAR szTitleName [MAX_PATH];
char * OpenFileR(char *title)
{
	static TCHAR szFilter[] = TEXT ("mdb Files\0*.mdb;\0\0") ;
	static TCHAR szFileName [MAX_PATH],  startpath[MAX_PATH];
	static OPENFILENAME ofn ;
	memset(&ofn,0,sizeof (OPENFILENAME));
	memset(szFileName,0,sizeof(szFileName));

	ofn.lStructSize       = sizeof (OPENFILENAME) ;
	ofn.lpstrFilter       = szFilter ;
	ofn.lpstrFile         = szFileName ;          // Set in Open and Close functions
	ofn.nMaxFile          = MAX_PATH ;
	ofn.lpstrFileTitle    = szTitleName ;          // Set in Open and Close functions
	ofn.nMaxFileTitle     = MAX_PATH ;
	ofn.lpfnHook		  = OFNHookProc;
	ofn.Flags			  = OFN_ENABLEHOOK|OFN_EXPLORER|OFN_ENABLESIZING;
	if(title!=0)
		ofn.lpstrTitle=title;

	strcpy(startpath,".");
	ofn.lpstrInitialDir   = startpath;


	GetOpenFileName (&ofn) ;
	return ofn.lpstrFile;
}

int OpenDB(CString *dbfile,char *opentitle) 
{

	*dbfile=OpenFileR(opentitle);
	if(*dbfile=="")
		return FALSE;
	cout <<  (LPCTSTR)*dbfile << endl;
	return TRUE;
}

int log_printf(const char *str,...)
{
	char buffer[80*40];
	va_list list;

	buffer[sizeof(buffer)-1]=0;
	va_start(list,str);
	_vsnprintf(buffer,sizeof(buffer),str,list);
	printf(buffer);
	if(flog!=0)
		fputs(buffer,flog);
	va_end(list);

	return TRUE;
}
int increment_time(SYSTEMTIME *time, int hour, int min, int sec)
{
	FILETIME ft;               
	SystemTimeToFileTime(time,&ft);
	LARGE_INTEGER li = {ft.dwLowDateTime,ft.dwHighDateTime};
	li.QuadPart += (LONGLONG)1000*1000*10 * (LONGLONG)sec; //*10nano2micro *1000micro2mili *1000mili2seconds
	li.QuadPart += (LONGLONG)60*1000*1000*10 * (LONGLONG)min; //*10nano2micro *1000micro2mili *1000mili2seconds *60minutes
	li.QuadPart += (LONGLONG)60*60*1000*1000*10 * (LONGLONG)hour; //*10nano2micro *1000micro2mili *1000mili2seconds *60mins*60secs
	ft.dwHighDateTime = li.HighPart;
	ft.dwLowDateTime = li.LowPart;
	FileTimeToSystemTime(&ft, time);
	return TRUE;
}
int get_column_default(CDatabase *db,char *table,char *column,char field_default[])
{
	CString dbpath;
	CString connect;
	CDaoDatabase daodb;

	sscanf(db->GetConnect(),"ODBC;DBQ=%[^;];",dbpath);
	connect.Format("ODBC;",dbpath);
	TRY{ 
		daodb.Open(dbpath);
	}
	CATCH(CDaoException,e)
	{
		printf("err:%s\n",e->m_pErrorInfo->m_strDescription);
		return 0;
	}
	END_CATCH
	CDaoRecordset rec(&daodb);
	CString StrSql;
	StrSql.Format("SELECT TOP 1 * FROM [%s];",table);
	rec.Open(dbOpenDynaset,StrSql);
	for(int i=0;i<rec.GetFieldCount();i++)
	{
		CDaoFieldInfo fi;
		rec.GetFieldInfo(i,fi,AFX_DAO_ALL_INFO);
		if(strcmp(fi.m_strName,column)==0)
		{
			if(strlen(fi.m_strDefaultValue)!=0)
			{
				CString str;
				str.Format(" %s",fi.m_strDefaultValue);
				strcat(field_default,str);
			}
			break;
		}
	}
	rec.Close();
	daodb.Close();
	//CDaoFieldInfo
	return 0;
}

int get_field_count(CDatabase *db,char *table)
{
	CString SqlString;
	int rec_count;

	CRecordset rec( db );

	SqlString = 
	"SELECT TOP 1 * FROM ";
	SqlString+=table;
	SqlString+="; ";

	rec.Open(CRecordset::snapshot,SqlString,CRecordset::readOnly);
	rec_count=rec.GetODBCFieldCount();
	rec.Close();
	if(rec_count<=0)
		cout<<"error getting rec count\n";
	return rec_count;

}
int get_fields(CDatabase *db,char *table,FIELD_DESCRIPTOR fields[],int *field_count,INDEX_DESCRIPTOR indexes[],int *index_count)
{
	CString SqlString;
	int i;
	//Try DAO first
	//****************************************************************
	CDaoDatabase daodb;
	CString dbpath;


	TRY{
		char str[255];
		sscanf(db->GetConnect(),"ODBC;DBQ=%[^;];",str);
		//strcpy(str,db->GetConnect());
		dbpath.Format("%s",str);
		if(g_dbpassword!=0)
			sprintf(str,";PWD=%s",g_dbpassword);
		else
			sprintf(str,";");

		daodb.Open(dbpath,FALSE,FALSE,str);
		CDaoRecordset rec(&daodb);
		TRY{
			CDaoTableDef tabledef(&daodb);
			tabledef.Open(table);
			*field_count=tabledef.GetFieldCount();
			for(int i=0;i<*field_count;i++)
			{
				CDaoFieldInfo fi;
				tabledef.GetFieldInfo(i,fi,AFX_DAO_ALL_INFO);
				strncpy(fields[i].name,fi.m_strSourceField,sizeof(fields[i].name)-1);
				if(fi.m_strDefaultValue!="")
					strncpy(fields[i].def,fi.m_strDefaultValue,sizeof(fields[i].def)-1);
				fields[i].type=fi.m_nType;
				fields[i].prec=fi.m_lSize;
				fields[i].allowzlen=fi.m_bAllowZeroLength;
				fields[i].required=fi.m_bRequired;
				fields[i].autoinc= ((fi.m_lAttributes&dbAutoIncrField)!=0) ? TRUE:FALSE;
			}
			//get index info for the table
			*index_count=tabledef.GetIndexCount();
			for(i=0;i<*index_count;i++)
			{
				CDaoIndexInfo indexi;
				tabledef.GetIndexInfo(i,indexi,AFX_DAO_ALL_INFO);
				strncpy(indexes[i].name,indexi.m_strName,sizeof(indexes[i].name)-1);
				strncpy(indexes[i].field,indexi.m_pFieldInfos->m_strName,sizeof(indexes[i].field)-1);
				indexes[i].descending=indexi.m_pFieldInfos->m_bDescending;
				indexes[i].primary=indexi.m_bPrimary;
			}

 			tabledef.Close();
		}
		CATCH(CDaoException,e)
		{
		//printf("err:%s\n",e->m_pErrorInfo->m_strDescription);
		}
		END_CATCH
		daodb.Close();
		return TRUE;

	}
	CATCH(CDaoException,e)
	{
//		log_printf("error: get_fields->Dao:%s\n",e->m_pErrorInfo->m_strDescription);
	}
	END_CATCH
	//****************************************************************

	log_printf("using crecordset to get fields\n");
	//Use CDatabase/CRecordset
	CRecordset rec1( db );

	TRY{
		SqlString.Format("SELECT TOP 1 * FROM [%s];",table);
		rec1.Open(CRecordset::snapshot,SqlString,CRecordset::readOnly);
	}
	CATCH(CDBException, e){
		rec1.Close();
		return TRUE;
	}
	END_CATCH
	*field_count=rec1.GetODBCFieldCount();
	for(i=0;i<*field_count;i++) //output field names 
	{
		CODBCFieldInfo finfo;
		rec1.GetODBCFieldInfo((short)i,finfo);
		strncpy(fields[i].name,finfo.m_strName,sizeof(fields[i].name)-1);
		fields[i].type=finfo.m_nSQLType;
		fields[i].prec=finfo.m_nPrecision;
		int j=finfo.m_nSQLType;
		int type=0;
		int prec=0;
		switch(j) 
		{
		case -7: //1 yes no bit
			type=1;
			prec=1;
			break;
		case -6: //3 byte
			type=2;
			prec=1;
			break;
		case 5: //5 integer
			type=3;
			prec=2;
			break;
		case 4: //10 long int
			type=4;
			prec=4;
			break;
		case 6: //6,15 float USED in SQL DB
		case 7: //7 single
			type=6;
			prec=4;
			break;
		case 8: //15 double
			type=7;
			prec=8;
			break;
		case 11: //19 date time
			type=8;
			prec=8;
			break;
		case -9: //text type used in SQL DB 1073741823
		case 1: //another text type
		case 12: //(len) text
			type=10;
			prec=finfo.m_nPrecision;
			break;
		case -4: //1073741823 (0x3FFFFFFF) OLE
			type=11;
			prec=0;
			break;
		case -10: //-10 memo type (ntext) used in SQL
		case -1: //1073741823 (0x3FFFFFFF) memo
			type=12;
			prec=0;
			break;
		default:
			log_printf("->>fail:unknown type %i %i field %s<<-\n",finfo.m_nSQLType,finfo.m_nPrecision,fields[i].name);
			break;
		}
		fields[i].type=type;
		fields[i].prec=prec;
	}
	rec1.Close();
	return TRUE;
}

int check_table_exist(CDatabase *db,char *table)
{
	CString SqlString;

	TRY{
		SqlString = 
			"SELECT TOP 1 * FROM ";
		SqlString+=table;
		SqlString+="; ";
		db->ExecuteSQL(SqlString);
	}
	CATCH(CDBException, e){
		return FALSE;
	}
	END_CATCH

	return TRUE;
}



int get_tables(CDatabase *db,TABLE_DESCRIPTOR tnames[])
{
	HSTMT hStmt;
	char pcName[256];
	long lLen;
	int count=0;
	
	SQLAllocStmt (db->m_hdbc, &hStmt);
	if (SQLTables (hStmt, NULL, SQL_NTS, NULL, SQL_NTS, NULL, SQL_NTS, (unsigned char*)"'TABLE'", SQL_NTS) != SQL_ERROR)
	{ /* OK */
		if (SQLFetch (hStmt) != SQL_NO_DATA_FOUND)
		{ /* Data found */
			while (!SQLGetData (hStmt, 3, SQL_C_CHAR, pcName, 256, &lLen))
			{ /* We have a name */
				if (pcName[0] != '~')
				{
					// Do something with the name here
					//printf("%i %s\n",count,pcName);
					if(count>=MAX_TABLE_COUNT)
						break;
					if(tnames!=0)
					{
					memset(tnames[count].name,0,sizeof(tnames[0].name));
					strncpy(tnames[count].name,pcName,sizeof(tnames[0].name)-1);
					}
					count++;
				}
				SQLFetch(hStmt);
			}
		}
	}
	SQLFreeStmt (hStmt, SQL_CLOSE);

	return count;
}
int get_table_indexes(CDatabase *db,char *table,INDEX_DESCRIPTOR field_indexes[])
{
	HSTMT hStmt;
	char pcName[256];
	long lLen;
	int count=0;
	
	SQLAllocStmt (db->m_hdbc, &hStmt);
	if (SQLStatistics (hStmt, NULL, SQL_NTS, NULL, SQL_NTS,(unsigned char*) table, SQL_NTS, SQL_INDEX_ALL, SQL_QUICK) != SQL_ERROR)
	{ /* OK */
		while(SQLFetch (hStmt) != SQL_NO_DATA_FOUND)
		{ /* Data found */

			for(int i=1;i<=13;i++)
			{
				unsigned short sqlshort=0;
				int sqlint=0;
				lLen=0;
				memset(pcName,0,sizeof(pcName));
				switch(i)
				{
/*
				case 12:
				case 11:
					SQLGetData (hStmt, i, SQL_C_LONG, &sqlint, sizeof(sqlint), &lLen);
					sprintf(pcName,"%i",sqlint);
					break;
				case 8:
				case 7: //SQL_INDEX_OTHER=3
				case 4:
					SQLGetData (hStmt, i, SQL_C_SHORT, &sqlshort, sizeof(sqlshort), &lLen);
					sprintf(pcName,"%i",sqlshort);
					break;
*/
				case 10: //ASC/DES
				case 9: //field name
				case 6: //index name
					SQLGetData (hStmt, i, SQL_C_CHAR, pcName, 256, &lLen);
					break;
				default:
//					SQLGetData (hStmt, i, SQL_C_CHAR, pcName, 256, &lLen);
					break;

				}
				char str[255];
				if(strlen(pcName)>0)
				{
					sprintf(str,"%s ",pcName);
					strcat(field_indexes[count].name,str);
				}

			}
			if(strlen(field_indexes[count].name)>0)
				count++;
			if(count>=MAX_FIELD_COUNT)
				break;

		}
	}
	SQLFreeStmt (hStmt, SQL_CLOSE);

	return count;
}
int open_database(CDatabase *db,CString *dbname)
{
	int type=0;
	CString sDsn;
	CString tstr;
	tstr=*dbname;
	tstr.MakeLower();
	if(tstr.Find(".mdb")>=0)
		type=0;
	else
		type=1;
	switch(type)
	{
	default:
	case 0: //ACCESS
		sDsn.Format("ODBC;DRIVER={MICROSOFT ACCESS DRIVER (*.mdb)};DSN='';DBQ=%s",*dbname);
		break;
	case 1: //SQL
		//sDsn.Format("ODBC;DRIVER={SQLServer};SERVER=%s;DATABASE=%s;Trusted_Connection=yes",*dbname);
		sDsn.Format("ODBC;DRIVER={SQL Server};%s;Integrated Security=SSPI",*dbname);
		break;
	}
	TRY{
		db->Open(NULL,false,false,sDsn,FALSE);
	}
	CATCH(CDBException, e){
		CString error = CString("ERROR: ") + e->m_strError + CString("\nODBC: ") + e->m_strStateNativeOrigin;
		cout<< (LPCSTR)error;
	}
	END_CATCH
	return TRUE;
}
int get_db_version(CString *dbname,CString *ver)
{
	CDatabase db;
	CString SqlString;

	open_database(&db,dbname);

	CRecordset rec( &db );

	SqlString = "SELECT ThisDatabaseVersion,DateWasCreated FROM DBVersionStamp ;";

	rec.Open(CRecordset::snapshot,SqlString,CRecordset::readOnly);

	cout<<"database version ";
	while(!rec.IsEOF())
	{
		CString result;
		rec.GetFieldValue("ThisDatabaseVersion",result);
		if(ver==0)
			cout<<(LPCTSTR)result << "      ";
		else
			*ver=result+"      ";
		rec.GetFieldValue("DateWasCreated",result);
		if(ver==0)
			cout<<(LPCTSTR)result;
		else
			*ver+=result;
		rec.MoveNext();
		if(ver==0)
			cout<<"\n";
		else
			*ver+="\n";
	}
	rec.Close();
	db.Close();
	return TRUE;
}


int get_record_count(CDatabase *db,char *table)
{
	int count=0;
	CString SqlString;
	CRecordset rec( db );
	SqlString.Format("SELECT COUNT(*) FROM [%s];",table);
	rec.Open(CRecordset::snapshot,SqlString,CRecordset::readOnly);
	if(!rec.IsEOF())
	{
		CString val;
		rec.GetFieldValue((short)0,val);
		count=atol(val);
	}
	rec.Close();
	return count;

}
static int record_check(int a[],int b[])
{
	int result;
	if(a[1]<b[1])
		result=-1;
	else if(a[1]==b[1])
		result=0;
	else
		result=1;
	return result;
}
int audit_db(CString *dbname,int verbosity)
{
	int i;
	CDatabase db1;
	open_database(&db1,dbname);


	TABLE_DESCRIPTOR tables[MAX_TABLE_COUNT];
	int record_counts[MAX_TABLE_COUNT];

	int db_table_count=0;
	memset(tables,0,sizeof(tables));
	memset(record_counts,0,sizeof(record_counts));
	db_table_count=get_tables(&db1,tables);
	if(db_table_count>=MAX_TABLE_COUNT)
		log_printf("warning:max table count exceeded time for recompile\n");
	
	for(i=0;i<db_table_count;i++)
	{
		FIELD_DESCRIPTOR fields[MAX_FIELD_COUNT];
		int field_count=0;
		INDEX_DESCRIPTOR indices[MAX_FIELD_COUNT];
		int index_count=0;

		char *cur_table_name;
		cur_table_name=tables[i].name;
		if(cur_table_name[0]==0)
			continue;

//		log_printf("%s::\n",cur_table_name);

		memset(fields,0,sizeof(fields));

		get_fields(&db1,cur_table_name,fields,&field_count,indices,&index_count);
		tables[i].field_count=field_count;
		if(field_count>=MAX_FIELD_COUNT)
			log_printf("warning:max field count exceeded time for recompile\n");
		record_counts[i]=get_record_count(&db1,cur_table_name);
//		log_printf("\trecord_count:%i\n",record_counts[i]);

		log_printf(".");
	}
	db1.Close();

	log_printf("\n\n");
	log_printf("table_count=%i\n",db_table_count);

	int tempcounts[MAX_TABLE_COUNT][2];
	memset(tempcounts,0,sizeof(tempcounts));
	for(i=0;i<db_table_count;i++)
	{
		tempcounts[i][0]=i;
		tempcounts[i][1]=record_counts[i];
	}
	qsort(tempcounts,db_table_count,8,(int (__cdecl *)(const void *,const void *))record_check);
	log_printf("after qsort\n");
	for(i=0;i<db_table_count;i++)
	{
		char str1[80],str2[80];
		sprintf(str1,"table:%s",tables[tempcounts[i][0]].name);
		sprintf(str2,"rec count:%i  (fc=%i)",tempcounts[i][1],tables[tempcounts[i][0]].field_count);
		log_printf("%-40s %s\n",str1,str2);
	}
	return 0;

}


int _tmain(int argc, TCHAR* argv[], TCHAR* envp[])
{
	int nRetCode = 0;


	// initialize MFC and print and error on failure
	if (!AfxWinInit(::GetModuleHandle(NULL), NULL, ::GetCommandLine(), 0))
	{
		// TODO: change error code to suit your needs
		cerr << _T("Fatal Error: MFC initialization failed") << endl;
		nRetCode = 1;
	}
	else
	{

		CString db1;
		CString input;

		if(argc>=2)
		{
			db1.Format(argv[1]);
		}
		else
		{
			cout<<"db1:";
			if(!OpenDB(&db1,"Open Database 1"))
				return FALSE;

		}
		cout	<< "Is this correct? press ESC to quit\n";
//		input+=getkey();
//		input.MakeUpper();
//		if(input==0x1b)
//			return 0;
		
		//g_dbpassword="l_xLA95X_";
		flog=fopen("log.txt","wb");


		CString ver;

		log_printf("db1:");
		get_db_version(&db1,&ver);
		log_printf(ver);




		audit_db(&db1,1);
		AfxDaoTerm();

		if(flog!=0)	{fclose(flog); flog=0;}

		cout << "\nDONE\npress any key\n";
		getkey();


	}

	return nRetCode;
}


