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
//#define new DEBUG_NEW
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

#define MAX_TABLE_COUNT 300
#define MAX_FIELD_COUNT 300

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

	char str[255];
	strcpy(str,db->GetConnect());
	strlwr(str);
	if(strstr(str,".mdb")!=0)
	{
		sscanf(db->GetConnect(),"ODBC;DBQ=%[^;];",str);
		dbpath.Format("%s",str);
		if(g_dbpassword!=0)
			sprintf(str,";PWD=%s",g_dbpassword);
		else
			sprintf(str,";");
	}
	else
	{
		goto trysql;
		dbpath.Format("%s",db->GetConnect());
		dbpath.Format("ODBC;DRIVER={SQL Server};Server=TEST_G_XP\\SQLEXPRESS;DATABASE=fm1;Trusted_Connection=yes;");
		log_printf("%s\n",dbpath);
	}

	TRY{
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
				if(fi.m_nType!=10) //not varchar
					strupr(fields[i].def);
			}
			//get index info for the table
			*index_count=tabledef.GetIndexCount();
			for(i=0;i<*index_count;i++)
			{
				CDaoIndexInfo indexi;
//	printf("get index info\n");
				tabledef.GetIndexInfo(i,indexi,AFX_DAO_SECONDARY_INFO);
//	printf("get index info succeed\n");
				strncpy(indexes[i].name,indexi.m_strName,sizeof(indexes[i].name)-1);
				strncpy(indexes[i].field,indexi.m_pFieldInfos->m_strName,sizeof(indexes[i].field)-1);
				indexes[i].descending=indexi.m_pFieldInfos->m_bDescending;
				indexes[i].primary=indexi.m_bPrimary;
			}

 			tabledef.Close();
		}
		CATCH(CDaoException,e)
		{
		printf("%s\n",db->GetConnect());
		printf("err:%s\n%s\n%s\n",e->m_pErrorInfo->m_strDescription,
			e->m_pErrorInfo->m_strSource,
			e->m_pErrorInfo->m_strHelpFile);
		}
		END_CATCH
		daodb.Close();
		return TRUE;

	}
	CATCH(CDaoException,e)
	{
		log_printf("error: get_fields->Dao:%s\n",e->m_pErrorInfo->m_strDescription);
	}
	END_CATCH
	//****************************************************************
trysql:
	//Use CDatabase/CRecordset
	CRecordset rec1( db );

	TRY{
		SqlString.Format("{CALL sp_columns (%s)}",table);
		rec1.Open(CRecordset::snapshot,SqlString,CRecordset::readOnly);
	}
	CATCH(CDBException, e){
		log_printf("error: sp_columns:\n");
		rec1.Close();
		return TRUE;
	}
	END_CATCH
		/*
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
		*/
	*field_count=0;
	for(i=0;i<MAX_FIELD_COUNT;i++)
	{
		int type,prec,j;
		CDBVariant var;
		CString str;
		rec1.GetFieldValue("COLUMN_NAME",str);
		strncpy(fields[i].name,str,sizeof(fields[i].name)-1);
		//rec1.GetFieldValue("COLUMN_DEF",fields[i].allowzlen);
		rec1.GetFieldValue("DATA_TYPE",str);
		j=type=atol(str);
		rec1.GetFieldValue("PRECISION",str);
		prec=atol(str);

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
			//prec=finfo.m_nPrecision;
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
			log_printf("->>fail:unknown type %i %i field %s<<-\n",type,prec,fields[i].name);
			break;
		}
		fields[i].type=type;
		fields[i].prec=prec;
		rec1.GetFieldValue("NULLABLE",str);
		j=atol(str);
		if(j==0)
			fields[i].required=TRUE;
		rec1.GetFieldValue("COLUMN_DEF",str);
		if(str.GetLength()>0)
		{
			str.Replace("(","");
			str.Replace(")","");
			if(type==10)
				str.Replace("'","\"");
			else
				str.MakeUpper();
			strncpy(fields[i].def,str,sizeof(fields[i].def)-1);
		}

		rec1.MoveNext();
		if(rec1.IsEOF())
			break;
	}
	*field_count=i;
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
	
	SQLAllocStmt(db->m_hdbc, &hStmt);
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

int check_if_fmledger(CString *dbname)
{
	CString str;
	str.Format("%s",*dbname);
	str.MakeLower();
	if(str.Find("fmledger",0)>=0)
	{
		g_dbpassword="1";
		return TRUE;
	}
	return FALSE;

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
		if(g_dbpassword!=0)
			sDsn.Format("ODBC;DRIVER={MICROSOFT ACCESS DRIVER (*.mdb)};DSN='';DBQ=%s;PWD=%s",*dbname,g_dbpassword);
		else
			sDsn.Format("ODBC;DRIVER={MICROSOFT ACCESS DRIVER (*.mdb)};DSN='';DBQ=%s",*dbname);
		break;
	case 1: //SQL
		//sDsn.Format("ODBC;DRIVER={SQLServer};SERVER=%s;DATABASE=%s;Trusted_Connection=yes",*dbname);
		//sDsn.Format("ODBC;DRIVER={SQL Server};%s;Integrated Security=SSPI",*dbname);
		sDsn.Format("Provider=MSDASQL;DRIVER={SQL Server};%s;User Id=testuser;Password=1",*dbname);
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
	return db->IsOpen();
}
int get_db_version(CString *dbname,CString *ver)
{
	CDatabase db;
	CString SqlString;

	if(!open_database(&db,dbname))
		return FALSE;

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

int drop_column(CDatabase *db,char *table,char *column)
{
	CString SqlString;
	log_printf("dropping column %s from table %s\n",column,table);
	TRY{
		SqlString.Format("ALTER TABLE [%s] DROP COLUMN [%s];",table,column);
		db->ExecuteSQL(SqlString);
	}
	CATCH(CDBException, e){
		log_printf("failed to drop column %s from table %s\n",column,table);
		return FALSE;
	}
	END_CATCH

	return TRUE;
}

int alter_column(CDatabase *db,char *table,char *column,int type,int len)
{
	CString SqlString;
	CString typesyntax;

	switch(type)
	{
	case 1: //yes/no
		typesyntax.Format("BIT");
		log_printf("altering bit type: %s %s %i %i\n",table,column,type,len);
		break;
	case 3: //integer
		typesyntax.Format("smallint");
		log_printf("altering smallint type: %s %s %i %i\n",table,column,type,len);
		break;
	case 4: //long integer
		typesyntax.Format("int");
		log_printf("altering int type: %s %s %i %i\n",table,column,type,len);
		break;
	case 10: //varchar
		typesyntax.Format("varchar(%i)",len);
		log_printf("altering varchar type: %s %s %i %i\n",table,column,type,len);
		break;
//	case 8: //float
//		typesyntax.Format("float");
//		log_printf("altering float type: %s %s %i %i\n",table,column,type,len);
//		break;
	case 8: //date/time
		typesyntax.Format("DATETIME");
		log_printf("altering datetime type: %s %s %i %i\n",table,column,type,len);
		break;
	default:
		log_printf("failed:unsupported, unable to alter type: %s %s %i %i\n",table,column,type,len);
		return FALSE;
		break;


	}

	TRY{
		SqlString.Format("ALTER TABLE [%s] ALTER COLUMN %s %s;",table,column,typesyntax);
		db->ExecuteSQL(SqlString);
	}
	CATCH(CDBException, e){
		log_printf("failed to modify column %s in table %s\n",column,table);
		return FALSE;
	}
	END_CATCH

	return TRUE;
}
int compare_indexes(INDEX_DESCRIPTOR index1[],int icount1,INDEX_DESCRIPTOR index2[],int icount2,int verbose)
{

	int diff=0;


	int max_index=icount1<icount2 ? icount2:icount1;
	if(max_index!=0)
	{
		log_printf("   indexes:\n");
		for(int i=0;i<max_index;i++)
		{
			if((strcmp(index1[i].name,index2[i].name)!=0) ||
				(index1[i].descending!=index2[i].descending) ||
				(index1[i].primary!=index2[i].primary) ||
				(strcmp(index1[i].field,index2[i].field)!=0) )
			{
				diff++;
				if(verbose==0)
					continue;
				else
					log_printf("~~~");
			}
			else //there the same
			{
				if(verbose<=1)
					continue;
				if(verbose>=2)
					log_printf("   ");
			}
			
			char str1[80],str2[80];
			sprintf(str1,"%s %s (%s,%s)",index1[i].name,index1[i].field,
				index1[i].descending ? "D":"A",index1[i].primary ? "PRI":"");
			sprintf(str2,"%s %s (%s,%s)",index2[i].name,index2[i].field,
				index2[i].descending ? "D":"A",index2[i].primary ? "PRI":"");

			log_printf("%-38s%s\n",str1,str2);
		}
		if(diff!=0)
			log_printf("   diff=%i\n",diff);
		else
			log_printf("\n");
	}
	return diff;
}
int restore_indices(CDatabase *db,char *table,
					INDEX_DESCRIPTOR indices1[],int index_count1,
					INDEX_DESCRIPTOR indices2[],int index_count2)
{
	CString SqlString;
	int err=0;

	log_printf("restoring indices on table %s\n",table);

	//go ahead and drop all indices
	if(index_count1>0)
	{
		log_printf("dropping old indices\n",table);
		for(int i=0;i<index_count1;i++)
		{
			SqlString.Format("DROP INDEX [%s] ON [%s];",indices1[i].name,table);
			TRY{ db->ExecuteSQL(SqlString); }
			CATCH(CDBException, e){
				log_printf("failed to drop index %s on table [%s]\n",indices1[i].name,table);
				log_printf("%s\n",SqlString);
				log_printf("error:%s\n",e->m_strError);
				err++;
			}
			END_CATCH
		}
	}
	//restore like new
	log_printf("making new indices\n",table);
	for(int i=0;i<index_count2;i++)
	{
		log_printf("%s (%s,%s)\n",indices2[i].name,indices2[i].descending ? "D":"A",indices2[i].primary ? "PRI":"");
		if(indices2[i].primary)
		{

			SqlString.Format("ALTER TABLE [%s] ADD CONSTRAINT [%s] PRIMARY KEY ([%s]);",table,indices2[i].name,indices2[i].field);
			TRY{ db->ExecuteSQL(SqlString); }
			CATCH(CDBException, e){
				log_printf("failed to create primary key %s on table [%s]\n",indices2[i].field,table);
				log_printf("%s\n",SqlString);
				log_printf("error:%s\n",e->m_strError);
				err++;
			}
			END_CATCH
		}
		else
		{
		//CREATE INDEX [blah] ON [table] ([col name] ASC,[col2 name] ASC....)
			SqlString.Format("CREATE INDEX [%s] ON [%s] ([%s] %s);",
				indices2[i].name,
				table,
				indices2[i].field,
				indices2[i].descending ? "DESC":"ASC");
			TRY{ db->ExecuteSQL(SqlString); }
			CATCH(CDBException, e){
				log_printf("failed to create index %s on table [%s]\n",indices2[i].name,table);
				log_printf("%s\n",SqlString);
				log_printf("error:%s\n",e->m_strError);
				err++;
			}
			END_CATCH

		}
	}
	if(err==0)
	{
		log_printf("index restore successful\n");
		return TRUE;
	}
	else
	{
		log_printf("index restore failed\n");
		return FALSE;
	}
}
int get_string_type(CString *typestr,int type,int prec)
{

	switch(type)
	{
	case 1:
		*typestr="BIT";
		break;
	case 2:
		*typestr="BYTE";
		break;
	case 3:
		*typestr="SMALLINT";
		break;
	case 4: //(syn INTEGER,LONG)
		*typestr="INT";
		break;
	case 6: //(syn SINGLE,FLOAT4)
		*typestr="REAL";
		break;
	case 7: //(syn DOUBLE,NUMBER,NUMERIC)
		*typestr="FLOAT";
		break;
	case 8: //(syn DATE,TIME,TIMESTAMP)
		*typestr="DATETIME";
		break;
	case 10: //(syn TEXT,CHAR,STRING)
		typestr->Format("varchar(%i)",prec);
		break;
	case 11: //(syn OLEOBJECT,GENERAL)
		*typestr="LONGBINARY";
		break;
	case 12: //(syn LONGCHAR,MEMO,NOTE)
		*typestr="LONGTEXT";
		break;
	default:
		log_printf("invalid type %i\n",type);
		return FALSE;
		break;
	}
	return TRUE;
}

int drop_table(CDatabase *db,char *table)
{
	CString SqlString;

	TRY{
		SqlString.Format("DROP TABLE [%s];",table);
		db->ExecuteSQL(SqlString);
		log_printf("dropped table %s\n",table);
	}
	CATCH(CDBException, e){
		log_printf("failed to drop table %s\n",table);
		return FALSE;
	}
	END_CATCH

	return TRUE;
}
int create_temp_table_name(CString *tempname,char *table_name)
{
	char time[20],date[20];
	SYSTEMTIME systime;
	
	GetSystemTime(&systime);
	GetTimeFormat(LOCALE_USER_DEFAULT,NULL,&systime,"hhmmss",time,20);
	GetDateFormat(LOCALE_USER_DEFAULT,NULL,&systime,"yyyyMd",date,20);
	tempname->Format("temp%s%s%s",table_name,date,time);
	return TRUE;
}

#include "create_table.h"
#include "combine_tables.h"

int restore_table(CDatabase *db_bad, CDatabase *db_good,char *table_name,
				  FIELD_DESCRIPTOR field_bad[],FIELD_DESCRIPTOR field_good[],
				  int field_bad_count,int field_good_count)
{
	CString SqlString;
	CString tempname;
	char db_bad_path[MAX_PATH],db_good_path[MAX_PATH];
	int err=0;

	err+=sscanf(db_bad->GetConnect(),"ODBC;DBQ=%[^;]",db_bad_path);
	if(err==0)
	{
		err+=sscanf(strstr(db_bad->GetConnect(),"DATABASE="),"DATABASE=%[^;]",db_bad_path);
		sprintf(db_bad_path,"dbo");
	}
	err+=sscanf(db_good->GetConnect(),"ODBC;DBQ=%[^;]",db_good_path);
	if(err==0)
	{
		err+=sscanf(strstr(db_bad->GetConnect(),"DATABASE="),"DATABASE=%[^;]",db_good_path);
		sprintf(db_good_path,"dbo");
	}
	if(err!=2)
	{
		log_printf("unable to sscanf %s\n",db_bad->GetConnect());
		return FALSE;
	}

	log_printf("attempting to restore table %s\n",table_name);


	create_temp_table_name(&tempname,table_name);

	//drop temp table if it exists
	//log_printf("dropping initial temp table %s if it exists\n",tempname);
	//drop_table(db_bad,tempname.GetBuffer(tempname.GetLength()));


	//copy bad table to temp table
	log_printf("moving bad table to temp table %s\n",tempname);
	SqlString.Format("SELECT * INTO [%s].[%s] FROM [%s].[%s];",db_bad_path,tempname,db_bad_path,table_name);

	TRY{db_bad->ExecuteSQL(SqlString);}
	CATCH(CDBException, e){
		log_printf("failed to copy from table %s to table %s\n",table_name,tempname);
		log_printf("error:%s\n",e->m_strError);
		log_printf(SqlString);
		return FALSE;
	}
	END_CATCH

	//drop original bad table
	log_printf("dropping original bad table\n");
	if( !drop_table(db_bad,table_name))
		return FALSE;

	CString dbpath;
	sscanf(db_bad->GetConnect(),"ODBC;DBQ=%[^;];",dbpath);

	//close database so create table using ADO connection works
	db_bad->Close();

	//create new original table from good database
	log_printf("creating new good table from template db\n");
	if(create_table(db_bad,table_name,field_good,field_good_count)==FALSE)
	{
		open_database(db_bad,&dbpath);
		log_printf("failed to create new table %s\n",table_name);
		return FALSE;
	}
	open_database(db_bad,&dbpath);


	//insert values from bad table into new good table in the correct order
	log_printf("copying data from temp table to new good table\n");
	int min_field_count= field_bad_count<field_good_count ? field_bad_count : field_good_count;

	SqlString.Format("INSERT INTO [%s].[%s] (",db_bad_path,table_name);
	int begin_comma=FALSE;
	for(int i=0;i<min_field_count;i++)
	{
		char field_name[255];
		strncpy(field_name,field_good[i].name,sizeof(field_name));
		if(field_good[i].autoinc)
		{
			log_printf("skipping possible auto field %s (insert)\n",field_good[i].name);
			continue;
		}
		if(begin_comma)
			SqlString+=",";
		SqlString+="[";
		SqlString+=field_name;
		SqlString+="]";
		begin_comma=TRUE;
	}

	SqlString+=") SELECT ";
	begin_comma=FALSE;
	for(i=0;i<min_field_count;i++)
	{
		char field_name[255];
		strncpy(field_name,field_bad[i].name,sizeof(field_name));

		if(field_bad[i].autoinc)
		{
			log_printf("skipping possible auto field %s (select)\n",field_bad[i].name);
			continue;
		}
		if(stristr(field_good[i].name,field_name)==0) //if fields are different name find a match
		{
			char match_str[255];
			strncpy(match_str,field_good[i].name,sizeof(match_str));
			if( !field_good[i].autoinc ) //if good field is not autonum find match
			{
				for(int j=0;j<field_bad_count;j++)
				{
					strncpy(match_str,field_bad[j].name,sizeof(match_str)); //search all bad fields for a match
					if(stristr(field_good[i].name,match_str)!=0)
					{
						strncpy(field_name,match_str,sizeof(field_name));
						break;
					}
				}
			}
		}
		if(begin_comma)
			SqlString+=",";
		SqlString+="[";
		SqlString+=field_name;
		SqlString+="]";
		begin_comma=TRUE;
	}
	SqlString+=" FROM [";
	SqlString+=tempname;
	SqlString+="];";

//	log_printf("%s\n",SqlString);

	TRY{db_bad->ExecuteSQL(SqlString);}
	CATCH(CDBException, e){
		log_printf("failed to copy from table %s to table %s\n",tempname,table_name);
		log_printf("error:%s\n",e->m_strError);
		if(flog!=0) fputs(SqlString,flog);
		return FALSE;
	}
	END_CATCH


	//we could drop temp table at this point
	log_printf("restore table %s successful\n",table_name);
	
	return TRUE;
}

//return DB version if successfull 0 otherwise
int update_db_version(CDatabase *db,CDatabase *db_target)
{
	int target_version=0;
	int source_version=0;
	CString SqlString;
	CRecordset rec(db);
	CRecordset rec_target(db_target);

	SYSTEMTIME systime;
	char date[20];
	GetSystemTime(&systime);
	GetDateFormat(LOCALE_USER_DEFAULT,NULL,&systime,"M'/'d'/'yyyy",date,sizeof(date));

	SqlString = "SELECT * FROM [DBVersionStamp];";
	TRY{
		rec_target.Open(CRecordset::snapshot,SqlString,CRecordset::readOnly);
	}
	CATCH(CDBException, e){
		log_printf("failed to get target DB version\n");
		rec_target.Close();
		return 0;
	}
	END_CATCH

	CString result;
	rec_target.GetFieldValue("ThisDatabaseVersion",result);
	rec_target.Close();
	target_version=strtoul(result,NULL,10);

	log_printf("target DB version=%i\n",target_version);

	SqlString = "SELECT * FROM [DBVersionStamp];";
	TRY{
		rec.Open(CRecordset::snapshot,SqlString,CRecordset::readOnly);
	}
	CATCH(CDBException, e){
		log_printf("failed to get source DB version\n");
		rec.Close();
		return 0;
	}
	END_CATCH

	rec.GetFieldValue("ThisDatabaseVersion",result);
	rec.Close();
	source_version=strtoul(result,NULL,10);
	log_printf("source DB version=%i\n",source_version);

	if(target_version<source_version)
	{
		SqlString.Format("DELETE FROM [DBVersion] WHERE [DBVERSION] > %d;",target_version);
		log_printf("reverting DBVersion table to %i\n",target_version);
	}
	else if(target_version>source_version)
	{
		SqlString.Format("INSERT INTO [DBVersion] ([DBVERSION],[LASTVERSION],[CONVERTDATE]) VALUES (%d,%d,'%s') ;",target_version,source_version,date);
		log_printf("adding row to DBVersion table: %i,%i,%s\n",target_version,source_version,date);
	}
	else
	{
		log_printf("no DB version change made\n");
		return 0;
	}

	TRY{
		db->ExecuteSQL(SqlString);
	}
	CATCH(CDBException, e){
		log_printf("failed to modify source DBVersion table\n");
		return 0;
	}
	END_CATCH



	SqlString.Format("UPDATE [DBVersionStamp] SET [ThisDatabaseVersion]=%d,[Comment]='Modified by DB cleaner',[DateWasCreated]='%s',[FMversion]='1.0'",target_version,date);
	TRY{
		db->ExecuteSQL(SqlString);
	}
	CATCH(CDBException, e){
		log_printf("failed to modify source DBVersionStamp table\n");
		return 0;
	}
	END_CATCH

	log_printf("updated DBVersionStamp table to version %i\n",target_version);


	return target_version;
}
int compare_db(CString *dbname1,CString *dbname2,int modify_database,int verbosity)
{
	int i;
	CDatabase db1,db2;
	open_database(&db1,dbname1);
	open_database(&db2,dbname2);


	TABLE_DESCRIPTOR tnames1[MAX_TABLE_COUNT];
	TABLE_DESCRIPTOR tnames2[MAX_TABLE_COUNT];
	int db1_table_count=0;
	int db2_table_count=0;
	memset(tnames1,0,sizeof(tnames1));
	memset(tnames2,0,sizeof(tnames2));
	db1_table_count=get_tables(&db1,tnames1);
	db2_table_count=get_tables(&db2,tnames2);
	
	db1_table_count+=combine_tables(tnames1,db1_table_count,tnames2,db2_table_count);

	int max_table_count = db1_table_count<db2_table_count ? db2_table_count : db1_table_count;

	for(i=0;i<max_table_count;i++)
	{
		FIELD_DESCRIPTOR fields1[MAX_FIELD_COUNT];
		FIELD_DESCRIPTOR fields2[MAX_FIELD_COUNT];
		int field_count1=0;
		int field_count2=0;
		int max_field_count=0;
		INDEX_DESCRIPTOR indices1[MAX_FIELD_COUNT];
		INDEX_DESCRIPTOR indices2[MAX_FIELD_COUNT];
		int index_count1=0;
		int index_count2=0;
		char *cur_table_name;
		int table_needs_restore=FALSE;
//		log_printf("db1 %-30s db2 %-30s\n",tnames1[i].name,tnames2[i].name);
		cur_table_name=tnames1[i].name;
		if(cur_table_name[0]==0)
			continue;
		if(!strstr(cur_table_name,"Security_Operator"))
			continue;

		log_printf("%s::\n",cur_table_name);

		memset(fields1,0,sizeof(fields1));
		memset(fields2,0,sizeof(fields2));
		memset(indices1,0,sizeof(indices1));
		memset(indices2,0,sizeof(indices2));

		get_fields(&db1,cur_table_name,fields1,&field_count1,indices1,&index_count1);
		get_fields(&db2,cur_table_name,fields2,&field_count2,indices2,&index_count2);
		max_field_count= field_count1<field_count2 ? field_count2 : field_count1;
		if((field_count1!=0) && (field_count2!=0))
		{
			for(int j=0;j<max_field_count;j++)
			{
				if((fields1[j].name[0]!=0) && (fields2[j].name[0]!=0)) //if both fields exist
				{
					if(memcmp(&fields1[j],&fields2[j],sizeof(fields1[j]))!=0) //if fields are not the same
					{
						//modified field

						if(strcmp(fields1[j].name,fields2[j].name)!=0)
						{
							log_printf("error column names are not the same [%s] [%s] (field %i)\n",
								fields1[j].name,
								fields2[j].name,j);
							table_needs_restore=TRUE;
						}
						else
						{
							if((fields1[j].type!=fields2[j].type) || (fields1[j].prec!=fields2[j].prec))
							{
								log_printf("different field type/prec (field1=%-s %i %i) (field2=%-s %i %i)\n",
									fields1[j].name,fields1[j].type,fields1[j].prec,
									fields2[j].name,fields2[j].type,fields2[j].prec);
								if(modify_database && (!table_needs_restore))
								{
									alter_column(&db1,
										cur_table_name,fields1[j].name,
										fields2[j].type,
										fields2[j].prec);
									//might need to check if this fails and try a table restore instead
								}
							}
							if(strcmp(fields1[j].def,fields2[j].def)!=0)
							{
								log_printf("different default values %-s (%s) %-s (%s)\n",
									fields1[j].name,fields1[j].def,
									fields2[j].name,fields2[j].def);
								if(modify_database)
									table_needs_restore=TRUE;
									/*
									restore_default(&db1,cur_table_name,fields1[j].name,
									fields2[j].def,
									fields1[j].type,
									fields1[j].prec);
									*/
							}
						}
					}
					else
					{
						//identical fields
						if(verbosity>=3)
						{
							log_printf("===%-s %i %i %s %-s %i %i %s\n",
								fields1[j].name,fields1[j].type,fields1[j].prec,fields1[j].def,
								fields2[j].name,fields2[j].type,fields2[j].prec,fields2[j].def);
						}
					}
				}
				else //one of the fields doesnt exist
				{
					char f1[80],f2[80];
					if(fields1[j].name[0]==0)
						sprintf(f1,"(field is blank)");
					else
						sprintf(f1,"%s %i %i %s",fields1[j].name,fields1[j].type,fields1[j].prec,fields1[j].def);
					if(fields2[j].name[0]==0)
						sprintf(f2,"(field is blank)");
					else
						sprintf(f2,"%s %i %i %s",fields2[j].name,fields2[j].type,fields2[j].prec,fields2[j].def);

					log_printf("+++%-37s %s\n",f1,f2);

					char col[255];
					sscanf(fields1[j].name[0]!=0 ? fields1[j].name : fields2[j].name,"%s",col);
					if(modify_database && (!table_needs_restore))
					{
						if(fields1[j].name[0]!=0) //if left side has column but right doesnt then drop it
						{
							if(!drop_column(&db1,cur_table_name,col))
								table_needs_restore=TRUE; // restore table due to possibly unable to drop autonum
						}
						else //right side has extra column, restore table to get it
						{
							table_needs_restore=TRUE;
						}

					}

				}
			}// end for check all fields
			if(verbosity>0)
			{
				if(compare_indexes(indices1,index_count1,indices2,index_count2,verbosity)!=0)
				{
					if(modify_database && (!table_needs_restore))
						restore_indices(&db1,cur_table_name,indices1,index_count1,indices2,index_count2);
				}
			}

			//done reading all fields restore table if needed
			if(table_needs_restore && modify_database)
			{
				if(restore_table(&db1,&db2,cur_table_name,fields1,fields2,field_count1,field_count2))
					restore_indices(&db1,cur_table_name,NULL,0,indices2,index_count2);

			}

			log_printf("\n");
			
		}
		else
		{
			//one of the field counts is zero , table doesnt exist
			int whichzero=0;
			whichzero=field_count1==0 ? 1 : 2;
			log_printf("table %s doesnt exist in db %i\n",cur_table_name,whichzero);
			if(modify_database)
			{
				if(stristr(cur_table_name,"CustomExport")!=0)
				{
					log_printf("skipping drop of customexport table\n");
				}
				else
				{
					if(whichzero==2)
						drop_table(&db1,cur_table_name);
					else if(whichzero==1)
					{
						log_printf("need to add table %s to DB1\n",cur_table_name);
						db1.Close(); //close connection so buffer is flushed in ADO create table call
						if(create_table(&db1,cur_table_name,fields2,field_count2))
						{
							open_database(&db1,dbname1);
							restore_indices(&db1,cur_table_name,NULL,0,indices2,index_count2);
						}
						else
							open_database(&db1,dbname1);

					}
				}
			}
			log_printf("\n");
		}

		if(getkey2()==0x1b)
			break;
	}

	if(modify_database)
		update_db_version(&db1,&db2);

	db1.Close();
	db2.Close();

	return 0;

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
static int record_count_check(int a[],int b[])
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
	
	printf("parsing tables\n");
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

		printf("%i\\%i        \r",i+1,db_table_count);
		if(getkey2()==0x1b)
			break;
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
	qsort(tempcounts,db_table_count,8,(int (__cdecl *)(const void *,const void *))record_count_check);
	log_printf("qsort by record count\n");
	for(i=0;i<db_table_count;i++)
	{
		char str1[80],str2[80];
		sprintf(str1,"table:%s",tables[tempcounts[i][0]].name);
		sprintf(str2,"rec count:%i  (fc=%i)",tempcounts[i][1],tables[tempcounts[i][0]].field_count);
		log_printf("%-40s %s\n",str1,str2);
	}
	return 0;

}
int debug_delete_temps(CString *dbname)
{
	int i;
	CDatabase db1;
	open_database(&db1,dbname);

	TABLE_DESCRIPTOR tables[MAX_TABLE_COUNT];

	int db_table_count=0;
	memset(tables,0,sizeof(tables));
	db_table_count=get_tables(&db1,tables);
	for(i=0;i<db_table_count;i++)
	{
		strlwr(tables[i].name);
		if(strstr(tables[i].name,"temp")!=0)
		{
			drop_table(&db1,tables[i].name);
		}
	}
	db1.Close();
	return 0;
}
int usage()
{
	printf("dbtool [-audit] [-compare] [-modify] [-verbose=<>] [-?] [db1] [db2]\n");
	printf("if comparing and modifiying databases then DB2 will be reference DB that DB1 is changed against\n");
	return TRUE;
}
int parse_args(int argc,TCHAR* argv[],CString *db1,CString *db2,int *audit,int *modify,int *verbosity)
{
	int i;
	int db_count=0;


	*audit=FALSE;
	*modify=FALSE;
	*verbosity=0;
	for(i=1;i<argc;i++)
	{
		if(strcmp("-debug",argv[i])==0)
			*audit=0xDEADBEEF;
		else if(strcmp("-audit",argv[i])==0)
			*audit=TRUE;
		else if(strcmp("-compare",argv[i])==0)
		{
			printf("comparing DB\n");
			*audit=FALSE;
		}
		else if(strcmp("-modify",argv[i])==0)
		{
			printf("modify DB enabled\n");
			*modify=TRUE;
		}
		else if(strncmp("-verbose=",argv[i],strlen("-verbose="))==0)
		{
			if(strlen(argv[i])>strlen("-verbose="))
			{
				*verbosity=atol(argv[i]+strlen("-verbose="));
				printf("verbose level=%i\n",*verbosity);
			}
			else
				printf("warning:verbose option given but not paramter\n");

		}
		else if(strcmp("-?",argv[i])==0)
		{
			usage();
			exit(0);
		}
		else
		{
			if(db_count==0)
			{
				db1->Format(argv[i]);
				cout<<"DB1:" << (LPCTSTR)*db1 <<"\n";
				db_count++;
			}
			else if(db_count==1)
			{
				db2->Format(argv[i]);
				cout<<"DB2:" << (LPCTSTR)*db2 <<"\n";
				db_count++;
			}
			else 
				printf("unknown arg %s\n",argv[i]);
		}

	}
	if(db_count==0)
	{
		if(*audit==TRUE)
		{
			cout<<"audit DB:";
			if(!OpenDB(db1,"Open Database for audit"))
			{
				printf("canceled db open\n");
				exit(1);
			}
		}
		else
		{
			cout<<"DB1:";
			if(!OpenDB(db1,"Open Database 1"))
				exit(1);

			cout<<"DB2:";
			if(!OpenDB(db2,"Open Database 2"))
				exit(1);
		}

	}
	else if(db_count==1)
	{
		if(*audit==FALSE)
		{
			cout<<"DB2:";
			if(!OpenDB(db2,"Open Database 2"))
			{
				printf("canceled db open\n");
				exit(1);
			}
		}
	}
	else if(db_count==2)
	{
		if(*audit==TRUE)
			printf("parse error:trying to audit 2 apparent databases\n");
	}
	return TRUE;
}
int _tmain(int argc, TCHAR* argv[], TCHAR* envp[])
{
	int nRetCode = 0;

	int audit=FALSE;
	int modify=FALSE;
	int verbosity=0;

	// initialize MFC and print and error on failure
	if (!AfxWinInit(::GetModuleHandle(NULL), NULL, ::GetCommandLine(), 0))
	{
		// TODO: change error code to suit your needs
		cerr << _T("Fatal Error: MFC initialization failed") << endl;
		nRetCode = 1;
	}
	else
	{

		CString db1,db2;
		CString input;
		parse_args(argc,argv,&db1,&db2,&audit,&modify,&verbosity);
		if(audit==0xDEADBEEF)
		{
			debug_delete_temps(&db1);
			cout << "\nDONE";
			getkey();
			return 0;
		}

		flog=fopen("log.txt","wb");

		check_if_fmledger(&db1);

		CString ver;
		log_printf("db1:");

//		get_db_version(&db1,&ver);
//		log_printf(ver);
		if(audit)
			audit_db(&db1,verbosity);
		else
		{
			log_printf("db2:");
//			get_db_version(&db2,&ver);
//			log_printf(ver);
			cout	<< "Is this correct? press ESC to quit\n";
//			input+=getkey();
//			input.MakeUpper();
//			if(input==0x1b)
//				return 0;
			compare_db(&db1,&db2,modify,verbosity);
		}

		AfxDaoTerm();

		if(flog!=0)	{fclose(flog); flog=0;}

		cout << "\nDONE";
		getkey();


	}

	return nRetCode;
}


