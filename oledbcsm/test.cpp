#include <stdio.h>
#include <windows.h>
#include <string.h>
#include <oledb.h>

#include <oledberr.h>      // OLE DB Errors

#include <msdasc.h>        // OLE DB Service Component header
#include <msdaguid.h>      // OLE DB Root Enumerator
#include <msdasql.h>       // MSDASQL - Default provider

#include <conio.h>         // getch, putch
#include <locale.h>        // setlocale
#define ROUNDUP_AMOUNT  8
#define ROUNDUP_(size,amount)           (((ULONG)(size)+((amount)-1))&~((amount)-1))
#define ROUNDUP(size)                           ROUNDUP_(size, ROUNDUP_AMOUNT)
int chk(HRESULT hr)
{
	switch ( hr ) 
	{
	case REGDB_E_CLASSNOTREG :
		printf("REGDB_E_CLASSNOTREG \n");
		break;

	case CLASS_E_NOAGGREGATION :
		printf("CLASS_E_NOAGGREGATION \n");
		break;

	case E_NOINTERFACE  :
		printf("E_NOINTERFACE  \n");
		break;

	case S_OK :
		printf("S_OK\n");
		return 0;
		break;
	
	case E_FAIL :
		printf("E_FAIL\n");
		break;

	case E_UNEXPECTED :
		printf("E_UNEXPECTED\n");
		break;

	case DB_E_MISMATCHEDPROVIDER :
		printf("DB_E_MISMATCHEDPROVIDER\n");
		break;

	case DB_E_NOAGGREGATION :
		printf("DB_E_NOAGGREGATION\n");
		break;

	case E_INVALIDARG   :
		printf("E_INVALIDARG\n");
		break;

	case DB_E_ERRORSOCCURRED  :
		printf("DB_E_ERRORSOCCURRED\n");
		break;

	case  DB_E_BADINITSTRING :
		printf("DB_E_BADINITSTRING\n");
		break;

	default:
		printf("hr %ld\n", hr);
		break;
	}
	return 1;
}

void disp_clsid ( CLSID &mID )
{
	unsigned int id1;
	unsigned short id2,id3,id4;
	unsigned char id5[6];
	int n;

	memcpy(&id1, &mID, sizeof(id1));
	memcpy(&id2, &((unsigned char*)&mID)[sizeof(id1)], sizeof(id2));
	memcpy(&id3, &((unsigned char*)&mID)[sizeof(id1)+sizeof(id2)], sizeof(id3));
	memcpy(&id4, &((unsigned char*)&mID)[sizeof(id1)+sizeof(id2)+sizeof(id3)], sizeof(id4));
	memcpy(&id5, &((unsigned char*)&mID)[sizeof(id1)+sizeof(id2)+sizeof(id3)+sizeof(id4)], sizeof(id5));

	printf("%08X-%04X-%04X-", id1,id2,id3);
	for ( n = 0 ; n < sizeof(id4); n++ )
		printf("%02X", ((unsigned char*)&mID)[n+8]);

	printf("-");

	for ( n = 0 ; n < sizeof(id5); n++ )
		printf("%02X", ((unsigned char*)&mID)[n+10]);
	
	printf("\n");
}

int main(int argc, char *argv[])
{
	HRESULT  hr;
	WCHAR wszProgID[100], clstr[100];
	char cstr[200];
	LPCWSTR wcstr;
	int n;
	
	CLSID mID;
   IDataInitialize *       pIDataInitialize       = NULL;
   IDBPromptInitialize *   pIDBPromptInitialize   = NULL;
   IDBInitialize *         pIDBInitialize         = NULL;
   IDBCreateSession        *pIDBCreateSession =NULL;
  //CLSID                   db_id                  = CLSID_MSDASQL;
  CLSID                   db_id;
   CLSID                   init_id                  = CLSID_MSDAINITIALIZE;

	//hr = CLSIDFromProgID( L"DataLinks", &mID);
	//if ( chk(hr) ) return 0;
	//printf("roundup 33 %d\n", ROUNDUP(33));
	const char *my_con = "Provider=MSDASQL;Location=Pubs";
	CoInitialize(NULL);

	goto PROMPT;
	printf("CoCreateInstance CLSID_MSDAINITIALIZE ");
	disp_clsid(init_id);
	hr = CoCreateInstance(init_id, NULL, CLSCTX_INPROC_SERVER, 
		IID_IDataInitialize,(void**)&pIDataInitialize);
	if ( chk(hr) ) return 0;
	
	LPWSTR tmpstr = new WCHAR [3*strlen(my_con)];
	printf("converted multi %d\n", MultiByteToWideChar(CP_ACP, 0,  my_con, -1, tmpstr, 3*strlen(my_con))); 
	printf("GetDataSource\n");
	hr = pIDataInitialize->GetDataSource( NULL, CLSCTX_INPROC_SERVER, tmpstr, IID_IDBInitialize,
   		reinterpret_cast<IUnknown **>(&pIDBInitialize));
	if ( chk(hr) ) return 0;

	printf("IID_IDBCreateSession\n");
	hr = pIDBInitialize->QueryInterface(IID_IDBCreateSession,(void**)&pIDBCreateSession);
	if ( chk(hr) ) return 0;

	printf("CLSIDFromProgID Microsoft.Jet.OLEDB.4.0\n");
	hr = CLSIDFromProgID( L"Microsoft.Jet.OLEDB.4.0", &db_id);
	if ( chk(hr) ) return 0;
	printf("CoCreateInstance Microsoft.Jet.OLEDB.4.0");
	disp_clsid(db_id);
	hr = CoCreateInstance(db_id, NULL, CLSCTX_INPROC_SERVER,
                 IID_IDBInitialize,(void**)&pIDBInitialize);
	if ( chk(hr) ) return 0;

	printf(" CLSIDFromProgID OraOLEDB.Oracle\n");
	hr = CLSIDFromProgID( L"OraOLEDB.Oracle", &db_id);
	if ( chk(hr) ) return 0;
	printf(" CreateDBInstance OraOLEDB.Oracle\n");

	hr = pIDataInitialize->CreateDBInstance(db_id, NULL, CLSCTX_INPROC_SERVER, 
		NULL, IID_IDBInitialize, (IUnknown**)&pIDBInitialize);
	if ( chk(hr) ) return 0;

PROMPT:
	printf(" CoCreateInstance CLSID_DataLinks\n");
      hr = CoCreateInstance(
               CLSID_DataLinks,                 // clsid -- Data Links UI
               NULL,                            // pUnkOuter
               CLSCTX_INPROC_SERVER,            // dwClsContext
               IID_IDBPromptInitialize,         // riid
               (void**)&pIDBPromptInitialize    // ppvObj
               );

	if ( chk(hr) ) return 0;
      // Invoke the Data Links UI to allow the user to select
      // the provider and set initialization properties for
      // the data source object that this will create.
      // See IDBPromptInitialize and IDBInitialize
      hr = pIDBPromptInitialize->PromptDataSource(
               NULL,                             // pUnkOuter
               GetDesktopWindow(),               // hWndParent
               DBPROMPTOPTIONS_PROPERTYSHEET,    // dwPromptOptions
               0,                                // cSourceTypeFilter
               NULL,                             // rgSourceTypeFilter
               NULL,                             // pwszszzProviderFilter
               IID_IDBInitialize,                // riid
               (IUnknown**)&pIDBInitialize       // ppDataSource
               );

	printf(" PromptDataSource ht %d\n", hr);
	if ( chk(hr) ) return 0;
      // We've obtained a data source object from the Data Links UI. This
      // object has had its initialization properties set, so all we
      // need to do is Initialize it.
      hr = pIDBInitialize->Initialize();
	printf("Initialize ht %d\n", hr);
	if ( chk(hr) ) return 0;
}
