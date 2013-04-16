#define VC_EXTRALEAN

#include <afx.h>

#include "afxdb.h"
#include <conio.h>
#include <assert.h>

extern "C"{

int open_db(CDatabase **db,char *dbname)
{
	int result=FALSE;
	CString connect;
	*db= new CDatabase;
	if(*db==0)
		return FALSE;

	connect.Format("ODBC;UID=dba;PWD=sql;DBN=Journal;DBF=C:\\Journal Manager\\Journal.db;ASTOP=YES;DSN=Journal;INT=NO;DBG=YES;DMRF=NO;LINKS=SharedMemory;COMP=NO",*dbname);
	TRY{
		if((*db)->Open(NULL,FALSE,FALSE,connect,FALSE)!=0)
			result=TRUE;
	}
	CATCH(CDBException, e){
		printf("Error opening database:%s\n",e->m_strError);
	}
	END_CATCH
	return result;
}
int get_db_info(CDatabase *db,CRecordset **info)
{
	CRecordset *rec=new CRecordset(db);



	return 0;
}

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


} //extern "C"