////////////////////////////////////////////////////////////////////////////////////////////////////
/// 
///  XUNZIP - common zip file extractor
///
/// @author   park
/// @date     Friday, May 14, 2010  12:27:28 PM
/// 
/// Copyright(C) 2010 Bandisoft, All rights reserved.
/// 
////////////////////////////////////////////////////////////////////////////////////////////////////

/*
	���� �ϴ� ����
		- RAW ����Ÿ
		- DEFLATE ����
		- �޸� ���۷� ���� ����
		- ���� �ڵ�� ���� ����

	���� ���ϴ� ����
		- ZIP64
		- 2GB �̻��� ����
		- ���ϸ�Ͽ� ���丮�� �������� ����
		- �޺κ��� �ջ�� ���� ���� ����
		- ���� zip ����
		- ��ȣ �ɸ� ����
		- UTF ���ϸ� ���� ����
		- File comment �� �ִ� ����
		- DEFLATE64, LZMA, BZIP2 PPMD, ... ��� �˰���
*/

#pragma once

#include "XStream.h"

// ���� �ڵ�
enum XUNZIP_ERR
{
	XUNZIP_ERR_OK,							// ���� ����
	XUNZIP_ERR_CANT_OPEN_FILE,				// ���� ���� ����
	XUNZIP_ERR_CANT_READ_FILE,				// ���� �б� ����
	XUNZIP_ERR_BROKEN_FILE,					// �ջ�� ������ (Ȥ�� �������� �ʴ� Ȯ�� ������ zip ����)
	XUNZIP_ERR_INVALID_ZIP_FILE,			// ZIP ������ �ƴ�
	XUNZIP_ERR_UNSUPPORTED_FORMAT,			// �������� �ʴ� ���� ����
	XUNZIP_ERR_ALLOC_FAILED,				// �޸� alloc ����
	XUNZIP_ERR_INVALID_PARAM,				// �߸��� �Է� �Ķ����
	XUNZIP_ERR_CANT_WRITE_FILE,				// ���� ����� ���� �߻�
	XUNZIP_ERR_INFLATE_FAIL,				// inflate ���� �߻�
	XUNZIP_ERR_INVALID_CRC,					// crc ���� �߻�
	XUNZIP_ERR_MALLOC_FAIL,					// �޸� alloc ����
	XUNZIP_ERR_INSUFFICIENT_OUTBUFFER,		// ��� ���۰� ���ڶ�
};

// ���� ���
enum XUNZIP_COMPRESSION_METHOD
{
	XUNZIP_COMPRESSION_METHOD_STORE		=	0,
	XUNZIP_COMPRESSION_METHOD_DEFLATE	=	8,
};

///////////////////////////////////
//
// ���� ���� ����ü
//
struct XUnzipFileInfo
{
	CHAR*	fileName;						// ���ϸ� 
	int		compressedSize;					// ����� ũ��
	int		uncompressedSize;				// ���� �ȵ� ũ��
	UINT32	crc32;
	XUNZIP_COMPRESSION_METHOD	method;		// ���� �˰���
	BOOL	encrypted;						// ��ȣ �ɷȳ�?
	int		offsetLocalHeader;				// local header�� �ɼ�
	int		offsetData;						// ���� ����Ÿ�� �ɼ�
};

class XInflate;
class XUnzip
{
public :
	XUnzip();
	~XUnzip();

	BOOL					Open(LPCWSTR szPathFileName);
	BOOL					Open(BYTE* data, int dataLen);
	BOOL					Open(XReadStream* stream);
	void					Close();
	XUNZIP_ERR				GetError() const { return m_err; }
	int						GetFileCount() { return m_fileCount; }
	const XUnzipFileInfo*	GetFileInfo(int index);
	BOOL					ExtractTo(int index, HANDLE hFile);
	BOOL					ExtractTo(int index, XWriteStream* outFile);
	BOOL					ExtractTo(int index, XBuffer& buf, BOOL addNull=FALSE);
	BOOL					ExtractTo(LPCWSTR fileName, XBuffer& buf);
	int						FindIndex(LPCSTR fileName);
	int						FindIndex(LPCWSTR fileName, int codePage=CP_OEMCP);


private :
	enum SIGNATURE			// zip file signature
	{
		SIG_ERROR								= 0x00000000,
		SIG_EOF									= 0xffffffff,
		SIG_LOCAL_FILE_HEADER					= 0x04034b50,
		SIG_CENTRAL_DIRECTORY_STRUCTURE			= 0x02014b50,
		SIG_ENDOF_CENTRAL_DIRECTORY_RECORD		= 0x06054b50,
	};

	enum ZIP_FILE_ATTRIBUTE
	{
		ZIP_FILEATTR_READONLY	= 0x1,
		ZIP_FILEATTR_HIDDEN		= 0x2,
		ZIP_FILEATTR_DIRECTORY	= 0x10,
		ZIP_FILEATTR_FILE		= 0x20,			
	};

private :
	BOOL			_Open(XReadStream* pInput);
	BOOL			SearchCentralDirectory();
	BOOL			ReadCentralDirectoryStructure();
	BOOL			_ExtractToStream(int index, XWriteStream* stream);
	XUnzipFileInfo*	_GetFileInfo(int index);
	BOOL			ReadLocalHeader(XUnzipFileInfo* pFileInfo);

private :
	BOOL			FileListAdd(XUnzipFileInfo* pFileInfo);
	void			FileListClear();

private :
	BOOL			ReadUINT32(UINT32& value);
	BOOL			ReadUINT8(BYTE& value);
	BOOL			Read(void* data, int len);
	BOOL			Skip(int len);

private :
	enum {DEFAULT_FILE_LIST_SIZE = 1024 };		// �⺻ ���� ��� �迭 ũ��
	enum {INPUT_BUF_SIZE = 1024*32};			// ���൥��Ÿ �ѹ��� �д� ũ��
	enum {INVALID_OFFSET = -1};					// ���൥��Ÿ �ѹ��� �д� ũ��

private :
	XReadStream*		m_pInput;
	BOOL				m_bDeleteInput;			// m_pInput �� ���� delete ���� ����.
	XUNZIP_ERR			m_err;

	XUnzipFileInfo**	m_ppFileList;
	int					m_fileCount;
	int					m_fileListSize;
	BOOL				m_checkCRC32;

	XInflate*			m_inflate;
};
