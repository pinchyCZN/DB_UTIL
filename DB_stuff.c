#define VC_EXTRALEAN

#include <windows.h>
#include <sql.h>
#include <sqlext.h>
#include <stdio.h>
#include <conio.h>

#include <stdlib.h>




int sql_test(char *pwszConnStr)
{
    SQLHENV     hEnv = NULL;
    SQLHDBC     hDbc = NULL;
    if(SQLAllocHandle(SQL_HANDLE_ENV,SQL_NULL_HANDLE,&hEnv)==SQL_ERROR)
		return 0;
	if(SQLSetEnvAttr(hEnv,SQL_ATTR_ODBC_VERSION,(SQLPOINTER)SQL_OV_ODBC3,0)==SQL_SUCCESS){
		if(SQLAllocHandle(SQL_HANDLE_DBC, hEnv, &hDbc)==SQL_SUCCESS){
			char str[1024]={0};

			if(SQLDriverConnect(hDbc,
							 GetDesktopWindow(),
							 (SQLCHAR*)pwszConnStr,
							 SQL_NTS,
							 (SQLCHAR*)str,
							 sizeof(str),
							 NULL,
							 SQL_DRIVER_COMPLETE)==SQL_SUCCESS){
				if(str[0]!=0){
					//write_ini(
				}
			}
		}
	}

    if (hDbc)
    {
        SQLDisconnect(hDbc);
        SQLFreeHandle(SQL_HANDLE_DBC, hDbc);
    }

    if (hEnv)
    {
        SQLFreeHandle(SQL_HANDLE_ENV, hEnv);
    }
}
int open_db(void **db,char *dbname)
{
	char str[1024];
	_snprintf(str,sizeof(str),"ODBC;",dbname);
	sql_test(str);
	return 0;
	/*
	int result=FALSE;
	CString connect;
	*db= new CDatabase;
	if(*db==0)
		return FALSE;

	connect.Format("ODBC;UID=dba;PWD=sql;DBN=Journal;DBF=%s;ASTOP=YES;DSN=Journal;INT=NO;DBG=YES;DMRF=NO;LINKS=SharedMemory;COMP=NO",dbname);
	TRY{
		if((*db)->Open(NULL,FALSE,FALSE,connect,FALSE)!=0)
			result=TRUE;
	}
	CATCH(CDBException, e){
		printf("Error opening database:%s\n",e->m_strError);
	}
	END_CATCH
	return result;
	*/
}
int close_db(void *db)
{
	if(db!=0){
	}
	return TRUE;
}
int free_db(void *db)
{
	if(db!=0){
	}
	return TRUE;
}
int get_db_info(void *db,void **info)
{

//SQLTables

	return 0;
}

/*
int get_field_count(CRecordset *rec)
{
	if(rec!=0){
		TRY{
			return rec->GetODBCFieldCount();
		}CATCH(CDBException, e){
			printf("Error getting field count:%s\n",e->m_strError);
		}
		END_CATCH
		return 0;
	}
	else
		return 0;
}

int get_field_name(CRecordset *rec,int index,char *str,int size)
{
	if(index<rec->GetODBCFieldCount()){
		CODBCFieldInfo info;
		rec->GetODBCFieldInfo(index,info);
		if(str!=0 && size>0){
			strncpy(str,info.m_strName,size);
			str[size-1]=0;
		}
		return TRUE;
	}
	return FALSE;
}
int get_fields(CDatabase *db,char *table,char *list,int size)
{
	CString str,SqlString;
	int i;
	int result=FALSE;
	CRecordset rec1( db );

	SqlString = 
	"SELECT TOP 1 * FROM [";
	SqlString+=table;
	SqlString+="]; ";

	TRY{
		rec1.Open(CRecordset::snapshot,SqlString,CRecordset::readOnly);
		if(size>0 && list!=0){
			list[0]=0;
			for(i=0;i<rec1.GetODBCFieldCount();i++)
			{
				CODBCFieldInfo finfo;
				rec1.GetODBCFieldInfo((short)i,finfo);
				if(_snprintf(list,size,"%s%s,",list,finfo.m_strName)>0)
					result=TRUE;
				else
					result=FALSE;
			}
		}
		rec1.Close();
	}CATCH(CDBException, e){
		if(rec1.IsOpen())
			rec1.Close();
	}END_CATCH
	return result;
}

*/