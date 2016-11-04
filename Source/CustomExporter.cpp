#include "CustomExporter.h"

CCustomExporter::CCustomExporter( void )
: m_name( "CustomExporter" ), m_dllFilePath( "" ), m_dllHandle( NULL ), m_referenceCount( NULL ),
  m_GetName( NULL ), m_Export( NULL )
{

}

CCustomExporter::CCustomExporter( CCustomExporter const& other )
: m_name( "CustomExporter" ), m_dllFilePath( "" ), m_dllHandle( NULL ), m_referenceCount( NULL ),
  m_GetName( NULL ), m_Export( NULL )
{
	//copy everything over
	m_referenceCount = other.m_referenceCount;
	m_dllHandle = other.m_dllHandle;
	m_name = other.m_name;
	m_dllFilePath = other.m_dllFilePath;
	m_GetName = other.m_GetName;
	m_Export = other.m_Export;

	//increment reference count of dll being copied
	incReferenceCount();
}

CCustomExporter& CCustomExporter::operator=( CCustomExporter const& other )
{
	//guard against self assignment 
	if( this == &other ) return *this;

	//decrement reference count of currently held dll.
	decReferenceCount();
	
	//copy everything over
	m_referenceCount = other.m_referenceCount;
	m_dllHandle = other.m_dllHandle;
	m_name = other.m_name;
	m_dllFilePath = other.m_dllFilePath;
	m_GetName = other.m_GetName;
	m_Export = other.m_Export;

	//increment reference count of dll being copied
	incReferenceCount();

	return *this;
}

void CCustomExporter::incReferenceCount( void )
{
	if( NULL != m_referenceCount )
	{
		( *m_referenceCount)++;
	}
	else
	{
		m_referenceCount = new int( 1 );
	}
}

void CCustomExporter::decReferenceCount( void )
{
	if( NULL != m_referenceCount )
	{
		( *m_referenceCount)--;

		if( *m_referenceCount == 0 )
		{
			if( NULL != m_dllHandle )
			{
				FreeLibrary( m_dllHandle );
				m_dllHandle = NULL;
			}

			delete m_referenceCount;
			m_referenceCount = NULL;
		}
	}
}

CCustomExporter::~CCustomExporter( void )
{
	decReferenceCount();
}

CString const& CCustomExporter::getName( void ) const
{
	return m_name;
}

bool CCustomExporter::load( CString FileName )
{
	if( NULL != m_dllHandle )
	{
		FreeLibrary( m_dllHandle );
	}

	m_dllHandle = LoadLibraryA( FileName );
	m_dllFilePath = FileName;
	incReferenceCount();

	if( NULL != m_dllHandle )
	{
		m_GetName = (const char* (__cdecl *)( void ))GetProcAddress( m_dllHandle, "GetName" );

		if( NULL != m_GetName )
		{
			m_name = m_GetName();
		}
		else
		{
			decReferenceCount();
			return false;
		}

		m_Export = (bool (__cdecl *)( CFamiTrackerDocInterface const*, LPCSTR fileName ))GetProcAddress( m_dllHandle, "Export" );

		if( NULL == m_Export )
		{
			decReferenceCount();
			return false;
		}
	}

	return true;
}

bool CCustomExporter::Export( CFamiTrackerDocInterface const* doc, LPCSTR fileName ) const
{
	if( NULL != m_Export )
	{
		return m_Export( doc, fileName );
	}
	else
	{
		return false;
	}
}
