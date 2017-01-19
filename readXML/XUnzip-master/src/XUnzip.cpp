#include "..\unzip\stdafx.h"
#include "XUnzip.h"
#include "XInflate.h"
#include "fast_crc32.h"

/**********************************************************************************

                                    -= history =-

* 2010/05/14
	- �۾� ����. ���� ��ǥ�Ѵ�� ���� ���� �Ϸ�

* 2010/05/19
	- ���̷�Ʈ �޸� ���� ���� ���� �Ϸ�

* 2010/05/24
	- �޸� ���� �Է±�� �߰�

* 2010/05/25
	- v 1.0 ����

* 2010/06/08
	- ��θ��� / �� \ �� �ٲٵ��� ��� �߰�

* 2010/12/31
	- extra field ���� ���� ����

* 2015/10/23
	- ��/����� XStream �� ����ϰ�, XUnzip2 �� �̸� �ٲ�

* 2016/7/22
	- XInflate 2.0 ����

***********************************************************************************/

#undef DOERR
#define DOERR(err) {m_err = err; ASSERT(0); return FALSE;}
#define DOERR2(err, ret) {m_err = err; ASSERT(0); return ret;}
#define DOFAIL(err) {m_err = err; ASSERT(0); goto END;}


static LPSTR Unicode2Ascii(LPCWSTR szInput, int nCodePage)
{
	LPSTR	ret=NULL;
	int		wlen = (int)(wcslen(szInput)+1)*3;
	int		asciilen;
	asciilen = wlen;					// asciilen �� wlen ���� ũ�� �ʴ�.
	ret = (LPSTR)malloc(asciilen);		// ASCII �� ����
	if(ret==NULL) {ASSERT(0); return NULL;}
	if(WideCharToMultiByte(nCodePage, 0, szInput,  -1, ret, asciilen, NULL, NULL)==0)	// UCS2->ascii
	{ASSERT(0); free(ret); return NULL;}
	return ret;
}


////////////////////////////////////////////////////////////////////////////
//
//    ZIP ���� ����ü ����
//

#pragma pack(1)
struct SEndOfCentralDirectoryRecord
{
	SHORT	numberOfThisDisk;
	SHORT	numberOfTheDiskWithTheStartOfTheCentralDirectory;
	SHORT	centralDirectoryOnThisDisk;
	SHORT	totalNumberOfEntriesInTheCentralDirectoryOnThisDisk;
	DWORD	sizeOfTheCentralDirectory;
	DWORD	offsetOfStartOfCentralDirectoryWithREspectoTotheStartingDiskNumber;
	SHORT	zipFileCommentLength;
};

union _UGeneralPurposeBitFlag
{
	SHORT	data;
	struct 
	{
		BYTE bit0 : 1;	// If set, indicates that the file is encrypted.
		BYTE bit1 : 1;
		BYTE bit2 : 1;
		BYTE bit3 : 1;	// If this bit is set, the fields crc-32, compressed  
						// size and uncompressed size are set to zero in the  
						// local header.  The correct values are put in the   
						// data descriptor immediately following the compressed data. 
		BYTE bit4 : 1;			  
		BYTE bit5 : 1;			   
		BYTE bit6 : 1;
		BYTE bit7 : 1;			   
		BYTE bit8 : 1;
		BYTE bit9 : 1;			   
		BYTE bit10 : 1;			   
		BYTE bit11 : 1;			   
	};
};

struct SLocalFileHeader
{
	SHORT	versionNeededToExtract;
	_UGeneralPurposeBitFlag	generalPurposeBitFlag;
	SHORT	compressionMethod;
	DWORD	dostime;		// lastModFileTime + lastModFileDate*0xffff
	DWORD	crc32;
	DWORD	compressedSize;
	DWORD	uncompressedSize;
	SHORT	fileNameLength;
	SHORT	extraFieldLength;
};

struct SCentralDirectoryStructureHead
{
	SHORT	versionMadeBy;
	SHORT	versionNeededToExtract;
	_UGeneralPurposeBitFlag	generalPurposeBitFlag;
	SHORT	compressionMethod;
	UINT	dostime;
	DWORD	crc32;
	DWORD	compressedSize;
	DWORD	uncompressedSize;
	SHORT	fileNameLength;
	SHORT	extraFieldLength;
	SHORT	fileCommentLength;
	SHORT	diskNumberStart;
	SHORT	internalFileAttributes;
	DWORD	externalFileAttributes;
	DWORD	relativeOffsetOfLocalHeader;
};

#pragma pack()


////////////////////////////////////////////////////////////////////////////
//
//       XUNZIP
//

XUnzip::XUnzip()
{
	m_pInput = NULL;
	m_bDeleteInput = FALSE;
	m_ppFileList = NULL;
	m_fileCount = 0;
	m_fileListSize = 0;
	m_inflate = NULL;
	m_err = XUNZIP_ERR_OK;
	m_checkCRC32 = FALSE;
}


XUnzip::~XUnzip()
{
	Close();

	if(m_inflate)
		delete m_inflate;

}

void XUnzip::Close()
{
	if(m_pInput && m_bDeleteInput)
	{
		delete m_pInput;
		m_pInput = NULL;
	}
	m_bDeleteInput = FALSE;

	FileListClear();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
///         ���� ����
/// @param  
/// @return 
/// @date   Friday, May 14, 2010  1:00:26 PM
////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL XUnzip::Open(LPCWSTR szPathFileName)
{
	Close();

	XFileReadStream* pInput = new XFileReadStream();
	if(pInput->Open(szPathFileName)==FALSE)
	{
		delete pInput;
		DOERR(XUNZIP_ERR_CANT_OPEN_FILE);
	}
	m_bDeleteInput = TRUE;
	return _Open(pInput);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
///         �޸� ���۷� ����Ÿ ���� - �޸� ���۴� Close() �Ҷ����� ��ȿ�ؾ� �Ѵ�.
/// @param  
/// @return 
/// @date   Monday, May 24, 2010  12:56:12 PM
////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL XUnzip::Open(BYTE* data, int dataLen)
{
	Close();

	XMemoryReadStream* pInput = new XMemoryReadStream();
	pInput->Attach(data, dataLen);
	m_bDeleteInput = TRUE;
	return _Open(pInput);
}


////////////////////////////////////////////////////////////////////////////////////////////////////
///         stream�� delete �� xunzip �� ���Ѵ�!
/// @param  
/// @return 
/// @date   Tuesday, February 24, 2015  5:09:29 PM
////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL XUnzip::Open(XReadStream* stream)
{
	return _Open(stream);
}

BOOL XUnzip::_Open(XReadStream* pInput)
{
	m_pInput = pInput;
	SIGNATURE	sig;

	// ���� ��� Ȯ���ϱ�
	if(ReadUINT32((UINT32&)(sig))==FALSE)
		DOERR(XUNZIP_ERR_CANT_OPEN_FILE);
	if(sig!=SIG_LOCAL_FILE_HEADER)
		DOERR(XUNZIP_ERR_INVALID_ZIP_FILE);

	// ���� ���� ��ġ ã��
	if(SearchCentralDirectory()==FALSE)
		DOERR(XUNZIP_ERR_BROKEN_FILE);

	// ���� ���� �б�
	for(;;)
	{
		// signature �б�
		if(ReadUINT32((UINT32&)(sig))==FALSE)
			DOERR(XUNZIP_ERR_CANT_OPEN_FILE);

		if(sig==SIG_CENTRAL_DIRECTORY_STRUCTURE)
		{
			if(ReadCentralDirectoryStructure()==FALSE)
				return FALSE;
		}
		else if(sig==SIG_ENDOF_CENTRAL_DIRECTORY_RECORD)
		{
			// ���� �����ϱ�
			break;
		}
		else
		{
			DOERR(XUNZIP_ERR_BROKEN_FILE);
		}
	}
	return TRUE;
}

BOOL XUnzip::ReadUINT32(UINT32& value)
{
	return m_pInput->Read((BYTE*)&value, sizeof(UINT32));
}

BOOL XUnzip::ReadUINT8(BYTE& value)
{
	return m_pInput->Read((BYTE*)&value, sizeof(BYTE));
}

BOOL XUnzip::Read(void* data, int len)
{
	return m_pInput->Read((BYTE*)data, len);
}

BOOL XUnzip::Skip(int len)
{
	if(len==0) return TRUE;
	INT64 newPos = m_pInput->GetPos() + len;
	return (m_pInput->SetPos(newPos)==newPos) ? TRUE : FALSE;	// �׻� TRUE �̴�. -.-;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
///         ��Ʈ�� ���丮 ã�� - zip comment �� ������� ó������ ���Ѵ�.
/// @param  
/// @return 
/// @date   Friday, May 14, 2010  1:27:26 PM
////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL XUnzip::SearchCentralDirectory()
{
	// end of central directory record �б�
	int size = sizeof(SEndOfCentralDirectoryRecord) + sizeof(UINT32);
	m_pInput->SetPos(-(int)(sizeof(SEndOfCentralDirectoryRecord) + sizeof(UINT32)), FILE_END);

	// end of central directory record ���� �б�
	SIGNATURE	sig;
	if(ReadUINT32((UINT32&)(sig))==FALSE)
		DOERR(XUNZIP_ERR_CANT_OPEN_FILE);
	if(sig!=SIG_ENDOF_CENTRAL_DIRECTORY_RECORD)
		DOERR(XUNZIP_ERR_INVALID_ZIP_FILE);

	SEndOfCentralDirectoryRecord	rec;

	if(Read(&rec, sizeof(rec))==FALSE)
		DOERR(XUNZIP_ERR_CANT_READ_FILE);

	// central directory ��ġ�� ã�Ҵ�!
	m_pInput->SetPos(rec.offsetOfStartOfCentralDirectoryWithREspectoTotheStartingDiskNumber, FILE_BEGIN);

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
///         ���� ���� �б�
/// @param  
/// @return 
/// @date   Friday, May 14, 2010  2:24:39 PM
////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL XUnzip::ReadCentralDirectoryStructure()
{
	CHAR*				fileName=NULL;
	int					fileOffset=0;
	BOOL				ret = FALSE;
	XUnzipFileInfo*		pFileInfo = NULL;
	SCentralDirectoryStructureHead head;
	
	// ���� �а�
	if(Read(&head, sizeof(head))==FALSE)
		DOERR(XUNZIP_ERR_BROKEN_FILE);

	// �����ϴ� �����ΰ�?
	if(	head.compressionMethod!=XUNZIP_COMPRESSION_METHOD_STORE &&
		head.compressionMethod!=XUNZIP_COMPRESSION_METHOD_DEFLATE)
		DOERR(XUNZIP_ERR_UNSUPPORTED_FORMAT);

	// ��ȣ�� �������� �ʴ´�
	if(head.generalPurposeBitFlag.bit0)
		DOERR(XUNZIP_ERR_UNSUPPORTED_FORMAT);


	// read file name
	if(head.fileNameLength)
	{
		fileName = (char*)malloc(head.fileNameLength+sizeof(char));
		if(fileName==NULL)
			DOERR(XUNZIP_ERR_BROKEN_FILE);

		if(Read(fileName,  head.fileNameLength)==FALSE)
			DOFAIL(XUNZIP_ERR_BROKEN_FILE);

		fileName[head.fileNameLength] = NULL;
	}

	// extra field;
	if(Skip(head.extraFieldLength)==FALSE)
		DOFAIL(XUNZIP_ERR_BROKEN_FILE);

	// file comment;
	if(Skip(head.fileCommentLength)==FALSE)
		DOFAIL(XUNZIP_ERR_BROKEN_FILE);

	// ���丮�� ��Ͽ� �߰����� �ʴ´� (�����Ƽ�)
	if(head.externalFileAttributes & ZIP_FILEATTR_DIRECTORY)
	{ret = TRUE; goto END;}

	// ���� ��Ͽ� �߰��ϱ�.
	pFileInfo = new XUnzipFileInfo;

	pFileInfo->fileName = fileName;									// ���ϸ�
	pFileInfo->compressedSize = head.compressedSize;
	pFileInfo->uncompressedSize = head.uncompressedSize;
	pFileInfo->crc32 = head.crc32;
	pFileInfo->method = (XUNZIP_COMPRESSION_METHOD)head.compressionMethod;
	pFileInfo->encrypted = head.generalPurposeBitFlag.bit0 ? TRUE : FALSE;
	pFileInfo->offsetLocalHeader = head.relativeOffsetOfLocalHeader;
	pFileInfo->offsetData = INVALID_OFFSET;							// ���� ���� ����Ÿ ��ġ - ���߿� ����Ѵ�.

	if(FileListAdd(pFileInfo)==FALSE)
		goto END;

	// ����
	pFileInfo = NULL;
	fileName = NULL;
	ret = TRUE;

END :
	if(fileName) free(fileName);
	if(pFileInfo) delete pFileInfo;
	return ret;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
///         ���� ��Ͽ� �߰��ϱ�
/// @param  
/// @return 
/// @date   Friday, May 14, 2010  3:13:53 PM
////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL XUnzip::FileListAdd(XUnzipFileInfo* pFileInfo)
{
	// ���ϸ��� '/' �� \ �� �ٲٱ�
	char* p = pFileInfo->fileName;
	while(*p)
	{
		if(*p=='/') *p='\\';
		p++;
	}

	// ���� ȣ��?
	if(m_ppFileList==NULL)
	{
		m_fileListSize = DEFAULT_FILE_LIST_SIZE;
		m_ppFileList = (XUnzipFileInfo**)malloc (m_fileListSize * sizeof(void*));
		if(m_ppFileList==NULL)
			DOERR(XUNZIP_ERR_ALLOC_FAILED);
	}

	// ��á��?
	if(m_fileCount>=m_fileListSize)
	{
		m_fileListSize = m_fileListSize*2;		// �ι�� ���
		XUnzipFileInfo** temp;
		temp = (XUnzipFileInfo**)realloc(m_ppFileList, (m_fileListSize * sizeof(void*) ));
		if(temp==NULL)
			DOERR(XUNZIP_ERR_ALLOC_FAILED);

		m_ppFileList = temp;
	}

	// �迭�� �߰�
	m_ppFileList[m_fileCount] = pFileInfo;
	m_fileCount++;

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
///         ���� ����Ʈ �����
/// @param  
/// @return 
/// @date   Friday, May 14, 2010  3:17:02 PM
////////////////////////////////////////////////////////////////////////////////////////////////////
void XUnzip::FileListClear()
{
	if(m_ppFileList==NULL) return;

	int	i;
	for(i=0;i<m_fileCount;i++)
	{
		XUnzipFileInfo* pFileInfo = *(m_ppFileList + i);
		//ASSERT(pFileInfo);
		free(pFileInfo->fileName);
		delete pFileInfo;
	}

	free(m_ppFileList);
	m_ppFileList = NULL;
	m_fileCount = 0;
	m_fileListSize =0 ;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
///         ���� ���� �����ϱ�
/// @param  
/// @return 
/// @date   Friday, May 14, 2010  3:51:17 PM
////////////////////////////////////////////////////////////////////////////////////////////////////
const XUnzipFileInfo* XUnzip::GetFileInfo(int index)
{
	// �Ķ���� �˻�
	if(m_ppFileList==NULL ||
		index<0 || index >=m_fileCount)
		DOERR2(XUNZIP_ERR_INVALID_PARAM, NULL);

	return m_ppFileList[index];
}
XUnzipFileInfo* XUnzip::_GetFileInfo(int index)
{
	// �Ķ���� �˻�
	if(m_ppFileList==NULL ||
		index<0 || index >=m_fileCount)
		DOERR2(XUNZIP_ERR_INVALID_PARAM, NULL);

	return m_ppFileList[index];
}

////////////////////////////////////////////////////////////////////////////////////////////////////
///         ���� �ڵ�� ���� Ǯ��
/// @param  
/// @return 
/// @date   Friday, May 14, 2010  3:54:47 PM
////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL XUnzip::ExtractTo(int index, HANDLE hFile)
{
	XFileWriteStream output;
	output.Attach(hFile);

	return ExtractTo(index, &output);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
///         ���� alloc �� ���� Ǯ��
/// @param  
/// @return 
/// @date   Monday, November 05, 2012  4:36:19 PM
////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL XUnzip::ExtractTo(int index, XBuffer& buf, BOOL addNull)
{
	XUnzipFileInfo* info = _GetFileInfo(index);
	if(info==NULL) DOERR2(XUNZIP_ERR_INVALID_PARAM, FALSE);
	if(buf.Alloc(info->uncompressedSize + (addNull?2:0))==FALSE)
		DOERR(XUNZIP_ERR_MALLOC_FAIL);

	if(addNull)				// ���ڿ� �Ľ̽� ó���� �����ϱ� ���ؼ� ���� null �ϳ� �־��ֱ�.
	{
		buf.data[buf.allocSize -1] = 0;
		buf.data[buf.allocSize -2] = 0;
	}
	buf.dataSize = info->uncompressedSize;	// ũ�� ����

	XMemoryWriteStream stream;
	stream.Attach(buf.data, buf.dataSize);

	return ExtractTo(index, &stream);
}

BOOL XUnzip::ExtractTo(LPCWSTR fileName, XBuffer& buf)
{
	return ExtractTo(FindIndex(fileName), buf);
}

int	XUnzip::FindIndex(LPCSTR fileName)
{
	for(int i=0;i<m_fileCount;i++)
	{
		if(_stricmp(fileName, m_ppFileList[i]->fileName)==0)
			return i;
	}
	ASSERT(0); return -1;
}

int	XUnzip::FindIndex(LPCWSTR fileName, int codePage)
{
	int index = -1;
	char* fileNameA = Unicode2Ascii(fileName, codePage);
	if(fileNameA==NULL)
		DOERR2(XUNZIP_ERR_INVALID_PARAM, -1);
	index = FindIndex(fileNameA);
	free(fileNameA);
	return index;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
///         ���� ��� ���� �а� ���� ����Ÿ ��ġ �ľ��ϱ�
/// @param  
/// @return 
/// @date   Friday, May 14, 2010  5:07:36 PM
////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL XUnzip::ReadLocalHeader(XUnzipFileInfo* pFileInfo)
{
	SIGNATURE	sig;
	if(ReadUINT32((UINT32&)(sig))==FALSE)
		DOERR(XUNZIP_ERR_CANT_READ_FILE);

	// SIGNATURE üũ
	if(sig!=SIG_LOCAL_FILE_HEADER)
		DOERR(XUNZIP_ERR_BROKEN_FILE);

	SLocalFileHeader	head;

	// ��� �а�
	if(Read(&head, sizeof(head))==FALSE)
		DOERR(XUNZIP_ERR_CANT_READ_FILE);

	// ���ϸ�
	if(Skip(head.fileNameLength)==FALSE)
		DOERR(XUNZIP_ERR_CANT_READ_FILE);

	// extraFieldLength
	if(Skip(head.extraFieldLength)==FALSE)
		DOERR(XUNZIP_ERR_CANT_READ_FILE);


	// ���൥��Ÿ ��ġ �ľ� �Ϸ�
	pFileInfo->offsetData = (int)m_pInput->GetPos();

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
///         ���� Ǯ�� 
/// @param  
/// @return 
/// @date   Friday, May 14, 2010  4:09:31 PM
////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL XUnzip::ExtractTo(int index, XWriteStream* output)
{
	XUnzipFileInfo* pFileInfo = _GetFileInfo(index);
	if(pFileInfo==NULL) return FALSE;


	// ����Ÿ offset ������ �߸��� ���
	if(pFileInfo->offsetData==INVALID_OFFSET)
	{
		// ��� ��ġ�� ��ġ ���
		if(m_pInput->SetPos(pFileInfo->offsetLocalHeader, FILE_BEGIN)!=pFileInfo->offsetLocalHeader)
			DOERR(XUNZIP_ERR_CANT_READ_FILE);

		// data ��ġ�� ã�Ƴ���.
		if(ReadLocalHeader(pFileInfo)==FALSE)
			return FALSE;
	}


	// ��ġ ���
	if(m_pInput->SetPos(pFileInfo->offsetData, FILE_BEGIN)==FALSE)
		DOERR(XUNZIP_ERR_CANT_READ_FILE);


	BYTE*	bufIn;
	int		inputRemain;
	int		outputRemain;
	int		toRead;
	DWORD	crc32=0;
	BOOL	ret=FALSE;

	bufIn = (BYTE*)malloc(INPUT_BUF_SIZE);

	inputRemain = pFileInfo->compressedSize;
	outputRemain = pFileInfo->uncompressedSize;

	// ���� Ǯ��
	if(pFileInfo->method==XUNZIP_COMPRESSION_METHOD_STORE)
	{
		while(inputRemain)
		{
			toRead = min(INPUT_BUF_SIZE, inputRemain);

			// �о
			if(m_pInput->Read(bufIn, toRead)==FALSE)
				DOFAIL(XUNZIP_ERR_CANT_READ_FILE);

			// �׳� ����
			if(output->Write(bufIn, toRead)==FALSE)
				DOFAIL(XUNZIP_ERR_CANT_WRITE_FILE);

			// crc
			crc32 = fast_crc32(crc32, bufIn, toRead);

			inputRemain -= toRead;
			outputRemain -= toRead;
		}
	}
	else if(pFileInfo->method==XUNZIP_COMPRESSION_METHOD_DEFLATE)
	{
		/////////////////////////////////////
		// deflate data ó��
		class MyStream : public IDecodeStream
		{
		public:
			virtual int		Read(BYTE* buf, int len)
			{
				DWORD read = 0;
				len = (int)min(m_inputRemain, len);
				m_input->Read(buf, len, &read);
				m_inputRemain -= read;
				return read;
			}
			virtual BOOL	Write(BYTE* buf, int len)
			{
				if(m_checkCRC32)
					m_crc32 = fast_crc32(m_crc32, buf, len);
				m_outputRemain -= len;
				return m_output->Write(buf, len);
			}
			XReadStream*	m_input;
			XWriteStream*	m_output;
			INT64			m_inputRemain;
			INT64			m_outputRemain;
			DWORD			m_crc32;
			BOOL			m_checkCRC32;
		};

		// ó�� ȣ���ΰ�?
		if(m_inflate==NULL)
			m_inflate = new XInflate;

		// �ʱ�ȭ
		MyStream mystream;
		mystream.m_input = m_pInput;
		mystream.m_inputRemain = inputRemain;
		mystream.m_output = output;
		mystream.m_outputRemain = outputRemain;
		mystream.m_crc32 = 0;
		mystream.m_checkCRC32 = m_checkCRC32;

		// ���� Ǯ��
		XINFLATE_ERR errInflate = m_inflate->Inflate(&mystream);
		if (errInflate != XINFLATE_ERR_OK)
			DOFAIL(XUNZIP_ERR_INFLATE_FAIL);

		crc32 = mystream.m_crc32;
		outputRemain = (int)mystream.m_outputRemain;

		// ���������� ������ Ǯ�ȴ��� �׳� Ȯ�ο�.
		if ((mystream.m_inputRemain || outputRemain))
			ASSERT(0);
	}
	else
		ASSERT(0);

	// crc �˻�
	if(m_checkCRC32 && crc32!=pFileInfo->crc32)
		DOFAIL(XUNZIP_ERR_INVALID_CRC);				// crc ���� �߻�

	// �Ϸ�Ǿ���
	if(outputRemain!=0) 
		DOFAIL(XUNZIP_ERR_INFLATE_FAIL);			// �߻� �Ұ�.

	// ����
	ret = TRUE;

END :
	if(bufIn)
		free(bufIn);

	return ret;
}

