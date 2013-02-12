// x86or64.cpp : コンソール アプリケーションのエントリ ポイントを定義します。
//

#include "stdafx.hpp"

class FileMappingForRead
{
private:
	HANDLE filehandle;
	HANDLE maphandle;
	LPVOID baseaddress;
	FileMappingForRead() {}
public:
	FileMappingForRead(LPCTSTR filename)
	{
		filehandle = CreateFile(filename, GENERIC_READ, 0, nullptr, OPEN_EXISTING, 0, nullptr);
		if (filehandle == INVALID_HANDLE_VALUE)
			throw;
		try
		{
			maphandle = CreateFileMapping(filehandle, nullptr, PAGE_READONLY, 0, 0, nullptr);
			if (maphandle <= 0)
				throw;
			try
			{
				baseaddress = MapViewOfFile(maphandle, FILE_MAP_READ, 0, 0, 0);
				if (baseaddress == NULL)
					throw;
			}
			catch (...)
			{
				CloseHandle(maphandle);
				maphandle = INVALID_HANDLE_VALUE;
				throw;
			}
		}
		catch (...)
		{
			CloseHandle(filehandle);
			filehandle = INVALID_HANDLE_VALUE;
			throw;
		}
	}

	~FileMappingForRead()
	{
		UnmapViewOfFile(baseaddress);
		CloseHandle(maphandle);
		CloseHandle(filehandle);
		baseaddress = NULL;
		maphandle = INVALID_HANDLE_VALUE;
		filehandle = INVALID_HANDLE_VALUE;
	}

	HANDLE GetFileHandle() const {return filehandle;}
	HANDLE GetMappingHandle() const {return maphandle;}
	LPVOID GetBaseAddress() const {return baseaddress;}
};

class PEHeaderReaderError
{
private:
	PEHeaderReaderError() {}
public:
	enum Info {
		NotExecutableFile = 0,
		NotPEFile = 1
	} info;
	PEHeaderReaderError(Info info) {this->info = info;}
};

// ファイルのMZ及びPEヘッダー情報へのアクセス機能を提供します。
class PEHeaderReader
{
private:
	FileMappingForRead filemap;
	PEHeaderReader() : filemap(nullptr) {}
public:
	PEHeaderReader(LPCTSTR filename)
		: filemap(filename)
	{
		const IMAGE_DOS_HEADER* pdosheader = static_cast<const IMAGE_DOS_HEADER*>(filemap.GetBaseAddress());
		if (pdosheader->e_magic != 0x5a4d)
			throw PEHeaderReaderError(PEHeaderReaderError::NotExecutableFile);
		// IMAGE_OPTIONAL_HEADERは32と64があるが、SignatureとIMAGE_FILE_HEADERはそのまま。
		// 便宜上IMAGE_NT_HEADERSでまとめて扱う。
		const IMAGE_NT_HEADERS* pntheaders = reinterpret_cast<const IMAGE_NT_HEADERS*>(
			static_cast<LPCBYTE>(filemap.GetBaseAddress()) + pdosheader->e_lfanew);
		if (pntheaders->Signature != MAKELONG(MAKEWORD('P', 'E'), MAKEWORD('\0', '\0')))
			throw PEHeaderReaderError(PEHeaderReaderError::NotPEFile);
	}

	const IMAGE_DOS_HEADER& GetDOSHeader() const
	{
		return *static_cast<const IMAGE_DOS_HEADER*>(filemap.GetBaseAddress());
	}

	const IMAGE_NT_HEADERS32& GetNTHeaders32() const
	{
		return *reinterpret_cast<const IMAGE_NT_HEADERS32*>(
			static_cast<LPCBYTE>(filemap.GetBaseAddress())
				+ static_cast<const IMAGE_DOS_HEADER*>(filemap.GetBaseAddress())->e_lfanew);
	}

	const IMAGE_NT_HEADERS64& GetNTHeaders64() const
	{
		return *reinterpret_cast<const IMAGE_NT_HEADERS64*>(
			static_cast<LPCBYTE>(filemap.GetBaseAddress())
				+ static_cast<const IMAGE_DOS_HEADER*>(filemap.GetBaseAddress())->e_lfanew);
	}

	DWORD GetPESignature() const
	{
		return GetNTHeaders32().Signature;
	}

	const IMAGE_FILE_HEADER& GetPEFileHeader() const
	{
		return GetNTHeaders32().FileHeader;
	}
};

using namespace std;

#if defined(UNICODE) || defined(_UNICODE)
#define tcout wcout
#else
#define tcout cout
#endif

int _tmain()
{
	PEHeaderReader peheaderreader(_T("C:\\Windows\\Notepad.exe"));
	switch (peheaderreader.GetPEFileHeader().Machine)
	{
	case IMAGE_FILE_MACHINE_I386:
		tcout << _T("x86") << endl;
		break;
	case IMAGE_FILE_MACHINE_IA64:
		tcout << _T("Intel Itanium") << endl;
		break;
	case IMAGE_FILE_MACHINE_AMD64:
		tcout << _T("x64") << endl;
		break;
	default:
		break;
	}

	return 0;
}
