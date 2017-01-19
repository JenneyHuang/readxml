#include "stdafx.h"
#include "../src/XUnzip.h"
#include "myutil.h"
#include <iostream>
#include <string>
void PrintUsage()
{
	printf("xunzip.exe [zip_path_name] [folder_path_to_extract]\n");
}


BOOL ExtractAllTo(XUnzip& unzip, CString targetFolder)
{
	int count = unzip.GetFileCount();

	for (int i = 0; i < count; i++)
	{
		const XUnzipFileInfo* fileInfo = unzip.GetFileInfo(i);
		CString fileName = fileInfo->fileName;
		CString filePathName = AddPath(targetFolder, fileName, L"\\");
		CString fileFolder = ::GetParentPath(filePathName);
		::DigPath(fileFolder);

		XFileWriteStream outFile;
		XBuffer szReadBuffer;
		XUnzip Bufferzip;

		if (outFile.Open(filePathName) == FALSE)
		{
			wprintf(L"ERROR: Can't open %s\n", filePathName.GetString());
			return FALSE;
		}
		
		wprintf(L"Extracting: %s..", filePathName.GetString());
		
		if (unzip.ExtractTo(i, szReadBuffer))
		{
			wprintf(L"done\n");
			if (filePathName.Right(3) == "zip")
			{
				Bufferzip.Open(szReadBuffer.data, szReadBuffer.dataSize);
				BOOL ret = ExtractAllTo(Bufferzip, targetFolder);
			}
			else
				std::cout << szReadBuffer << std::endl;
		}
		else
		{
			wprintf(L"Error\n");
			return FALSE;
		}
		
		
		
	}
	return TRUE;
}



BOOL Unzip(CString zipPathName, CString targetFolder)
{
	XUnzip		unzip;

	if (unzip.Open(zipPathName) == FALSE)
	{
		wprintf(L"ERROR: Can't open %s\n", zipPathName.GetString());
		return FALSE;
	}

	INT64 tick = ::GetTickCount64();
	BOOL ret = ExtractAllTo(unzip, targetFolder);
	INT64 elapsed = GetTickCount64() - tick;
	if (ret)
		wprintf(L"Elapsed time : %dms\n", (int)elapsed);
	return ret;
}




int wmain(int argc, WCHAR* argv[])
{
	/*if (argc < 3)
	{
		PrintUsage();
		return 0;
	}*/

	//CString zipPathName = argv[1];
	//CString targetFolder = argv[2];
	CString zipPathName = "C:\\Users\\huangyx\\Downloads\\XUnzip-master\\XUnzip-master\\x64\\Debug\\bigzip.zip";
	CString targetFolder = "C:\\Users\\huangyx\\Downloads\\XUnzip-master\\XUnzip-master\\x64\\Debug\\";
	BOOL ret = Unzip(zipPathName, targetFolder);

    return ret ? 0 : 1;
}


