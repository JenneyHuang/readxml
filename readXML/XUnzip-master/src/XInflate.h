////////////////////////////////////////////////////////////////////////////////////////////////////
/// 
/// XINFLATE - DEFLATE �˰����� ���� ����(inflate) Ŭ����
///
/// - ���� : v 2.0 (2016/7/21)
///
/// - ���̼��� : zlib license (https://kippler.com/etc/zlib_license/ ����)
///
/// - ���� �ڷ�
///  . RFC 1951 (http://www.http-compression.com/rfc1951.pdf)
///  . Halibut(http://www.chiark.greenend.org.uk/~sgtatham/halibut/)�� deflate.c �� �Ϻ� �����Ͽ���
/// 
/// @author   kippler@gmail.com
/// @date     Monday, April 05, 2010  5:59:34 PM
/// 
/// Copyright(C) 2010-2016 Bandisoft, All rights reserved.
/// 
////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#include <windows.h>


// ���� �ڵ�
enum XINFLATE_ERR
{
	XINFLATE_ERR_OK,						// ����
	XINFLATE_ERR_STREAM_NOT_COMPLETED,		// ��Ʈ���� ����ϰ� ������ �ʾҴ�.
	XINFLATE_ERR_HEADER,					// ����� ���� ����
	XINFLATE_ERR_INVALID_NOCOMP_LEN,		// no compressio ����� ���� ����
	XINFLATE_ERR_ALLOC_FAIL,				// �޸� ALLOC ����
	XINFLATE_ERR_INVALID_DIST,				// DIST �������� ���� �߻�
	XINFLATE_ERR_INVALID_SYMBOL,			// SYMBOL �������� ���� �߻�
	XINFLATE_ERR_BLOCK_COMPLETED,			// ���� ������ �̹� �Ϸ�Ǿ���.
	XINFLATE_ERR_CREATE_CODES,				// code ������ ���� �߻�
	XINFLATE_ERR_CREATE_TABLE,				// table ������ ���� �߻�
	XINFLATE_ERR_INVALID_LEN,				// LEN ������ ���� ����
	XINFLATE_ERR_INSUFFICIENT_OUT_BUFFER,	// �ܺ� ��� ���۰� ���ڶ��.
	XINFLATE_ERR_USER_STOP,					// ����� ���
	XINFLATE_ERR_INTERNAL,
};


struct IDecodeStream
{
	virtual int		Read(BYTE* buf, int len) = 0;
	virtual BOOL	Write(BYTE* buf, int len) = 0;
};


//#ifdef _WIN64
//typedef UINT64 BITSTREAM;
//#define BITSLEN2FILL	(7*8)			// 3bytes*8
//#else
typedef UINT BITSTREAM;
#define BITSLEN2FILL	24				// 3bytes*8
//#endif



class XFastHuffTable;
class XInflate
{
public :
	XInflate();
	~XInflate();
	void					Free();													// ���ο� ���� ��� ��ü�� �����Ѵ�. ���̻� �� Ŭ������ ������� ������ ȣ���ϸ� �ȴ�.
	XINFLATE_ERR			Inflate(IDecodeStream* stream);							// �������� �۾�. ū �޸� �� ���� ������ ������ ��� ȣ���ϸ� �ȴ�.

private :
	XINFLATE_ERR			Init();													// �ʱ�ȭ: �Ź� ���� ������ �����Ҷ����� ȣ���� ��� �Ѵ�.
	void					CreateStaticTable();
	BOOL					CreateCodes(BYTE* lengths, int numSymbols, int* codes);
	BOOL					CreateTable(BYTE* lengths, int numSymbols, XFastHuffTable*& pTable, XINFLATE_ERR& err);
	BOOL					_CreateTable(int* codes, BYTE* lengths, int numSymbols, XFastHuffTable*& pTable);

private :
	enum	STATE						// ���� ����
	{
		STATE_START,

		STATE_NO_COMPRESSION,
		STATE_NO_COMPRESSION_LEN,
		STATE_NO_COMPRESSION_NLEN,
		STATE_NO_COMPRESSION_BYPASS,

		STATE_FIXED_HUFFMAN,

		STATE_GET_SYMBOL_ONLY,
		STATE_GET_LEN,
		STATE_GET_DISTCODE,
		STATE_GET_DIST,
		STATE_GET_SYMBOL,

		STATE_DYNAMIC_HUFFMAN,
		STATE_DYNAMIC_HUFFMAN_LENLEN,
		STATE_DYNAMIC_HUFFMAN_LEN,
		STATE_DYNAMIC_HUFFMAN_LENREP,

		STATE_COMPLETED,
	};


private :
	XINFLATE_ERR FastInflate(IDecodeStream* stream, LPBYTE& inBuffer, int& inBufferRemain,
		LPBYTE& outBufferCur,
		LPBYTE& outBufferEnd,
		LPBYTE& windowStartPos, 
		LPBYTE& windowCurPos,
		BITSTREAM& bits,
		int& bitLen,
		STATE& state,
		int& windowLen, int& windowDistCode, int& symbol);


private :
	XFastHuffTable*			m_pStaticInfTable;		// static huffman table
	XFastHuffTable*			m_pStaticDistTable;		// static huffman table (dist)
	XFastHuffTable*			m_pDynamicInfTable;		// 
	XFastHuffTable*			m_pDynamicDistTable;	// 
	XFastHuffTable*			m_pCurrentTable;		// 
	XFastHuffTable*			m_pCurrentDistTable;	// 

	BYTE*					m_dicPlusOutBuffer;		// ���� + ��� ����
	BYTE*					m_outBuffer;			// ��� ���� ��ġ
	BYTE*					m_inBuffer;				// �Էµ���Ÿ�� ������ ����

	BOOL					m_bFinalBlock;			// ���� ������ ���� ó�����ΰ�?

	int						m_literalsNum;			// dynamic huffman - # of literal/length codes   (267~286)
	int						m_distancesNum;			// "               - # of distance codes         (1~32)
	int						m_lenghtsNum;			//                 - # of code length codes      (4~19)
	BYTE					m_lengths[286+32];		// literal + distance �� length �� ���� ����.
	int						m_lenghtsPtr;			// m_lengths ���� ���� ��ġ
	int						m_lenExtraBits;
	int						m_lenAddOn;
	int						m_lenRep;
	XFastHuffTable*			m_pLenLenTable;
	const char*				m_copyright;

private :
	enum { DEFLATE_WINDOW_SIZE = 32768 };			// RFC �� ���ǵ� WINDOW ũ��
	enum { DEFAULT_OUTBUF_SIZE = 1024 * 1024 };		// �⺻ ��� ���� ũ��
	enum { DEFAULT_INBUF_SIZE = 1024*128 };			// �⺻ �Է� ���� ũ��
	enum { MAX_SYMBOLS = 288 };						// deflate �� ���ǵ� �ɺ� ��
	enum { MAX_CODELEN = 16 };						// deflate ������ Ʈ�� �ִ� �ڵ� ����
	enum { FASTINFLATE_MIN_BUFFER_NEED = 6 };		// FastInflate() ȣ��� �ʿ��� �ּ� �Է� ���ۼ�

#ifdef _DEBUG
	int						m_inputCount;
	int						m_outputCount;
#endif

};

