#include "..\unzip\stdafx.h"
#include "XInflate.h"
#include "XFastHuffTable.h"
#include "XInflatePrivate.h"




////////////////////////////////////////////////
//
// ��Ʈ ��Ʈ�� ��ũ��ȭ
//
#define BS_EATBIT()								\
	bits & 0x01;								\
	bitLen --;									\
	bits >>= 1;									\
	if(bitLen<0) ASSERT(0);

#define BS_EATBITS(count)						\
	(bits & ((1<<(count))-1));					\
	bitLen -= count;							\
	bits >>= count;								\
	if(bitLen<0) ASSERT(0);

#define BS_REMOVEBITS(count)					\
	bits >>= (count);							\
	bitLen -= (count);							\
	if(bitLen<0) ASSERT(0); 

#define BS_MOVETONEXTBYTE						\
	BS_REMOVEBITS((bitLen % 8));

#define BS_ADDBYTE(byte)						\
	bits |= (BITSTREAM(byte)) << bitLen;		\
	bitLen += 8;
//
// ��Ʈ ��Ʈ�� ��ũ�� ó��
//
////////////////////////////////////////////////



////////////////////////////////////////////////
//
// �ݺ� �۾� ��ũ��ȭ
//
#define FILLBUFFER()							\
	while(bitLen<=BITSLEN2FILL)					\
	{											\
		if(inBufferRemain==0)					\
			break;								\
		BS_ADDBYTE(*inBuffer);					\
		inBufferRemain--;						\
		inBuffer++;								\
		ADD_INPUT_COUNT;						\
	}											\
	if(bitLen<=0)								\
		goto END;


//#define HUFFLOOKUP(result, pTable)								\
//		/* ����Ÿ ���� */											\
//		if(pTable->bitLenMin > bitLen)							\
//		{														\
//			result = -1;										\
//		}														\
//		else													\
//		{														\
//			pItem = &(pTable->pItem[pTable->mask & bits]);		\
//			/* ����Ÿ ���� */									\
//			if(pItem->bitLen > bitLen)							\
//			{													\
//				result = -1;									\
//			}													\
//			else												\
//			{													\
//				result = pItem->symbol;							\
//				BS_REMOVEBITS(pItem->bitLen);					\
//			}													\
//		}

// ��� ���ۿ� �ѹ���Ʈ ����
#define WRITE(byte)												\
	ADD_OUTPUT_COUNT;											\
	WIN_ADD;													\
	CHECK_AND_FLUSH_OUT_BUFFER									\
	*outBufferCur = byte;										\
	outBufferCur++;


// ��� ���۰� ��á���� -> ���� + ��� ���� ��ġ �̵� �ʱ�ȭ + ���� ����Ÿ�� ��� ���� ������ ����
#define CHECK_AND_FLUSH_OUT_BUFFER								\
	if(outBufferCur>=outBufferEnd)								\
	{															\
		if(stream->Write(windowStartPos, (int)(outBufferCur - windowStartPos))==FALSE)		\
			return XINFLATE_ERR_USER_STOP;						\
		int _dist = (int)(outBufferCur - windowCurPos);			\
		memcpy(windowStartPos-DEFLATE_WINDOW_SIZE, outBufferCur - DEFLATE_WINDOW_SIZE, DEFLATE_WINDOW_SIZE);	\
		windowCurPos = windowStartPos-_dist ;					\
		outBufferCur = windowStartPos;							\
	}															\
	


////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//                                              FastInflate
//


// �Է� ���۰� ������� üũ���� �ʴ´�.
#define FILLBUFFER_FAST()								\
	while(bitLen<=BITSLEN2FILL)									\
	{													\
		BS_ADDBYTE(*inBuffer);							\
		inBufferRemain--;								\
		inBuffer++;										\
		ADD_INPUT_COUNT;								\
	}													\

// ��Ʈ��Ʈ�� ����Ÿ(bits + bitLen)�� ������� üũ���� �ʴ´�.
//#define HUFFLOOKUP_FAST(result)							\
//		pItem = &(pItems[mask & bits]);					\
//		result = pItem->symbol;							\
//		BS_REMOVEBITS(pItem->bitLen);					


// ��� ���۰� ������� üũ���� �ʴ´�.
#define WRITE_FAST(byte)							\
	ADD_OUTPUT_COUNT;								\
	WIN_ADD;										\
	*outBufferCur = byte;							\
	outBufferCur++;												


#define WRITE_BLOCK(in, len)						\
	ADD_OUTPUT_COUNTX(len);							\
	memcpy(outBufferCur, in, len);					\
	windowCurPos +=len;								\
	outBufferCur +=len;

////////////////////////////////////////////////
//
// ������ ��ũ��ȭ
//

#define WIN_ADD				(windowCurPos++)
#define WIN_GETBUF(dist)	(windowCurPos - dist)

//
// ������ ��ũ��ȭ
//
////////////////////////////////////////////////




static const char* xinflate_copyright = 
"[XInflate - Copyright(C) 2016, by kippler]";

XInflate::XInflate()
{
	m_pStaticInfTable = NULL;
	m_pStaticDistTable = NULL;
	m_pDynamicInfTable = NULL;
	m_pDynamicDistTable = NULL;
	m_pCurrentTable = NULL;
	m_pCurrentDistTable = NULL;

	m_pLenLenTable = NULL;
	m_dicPlusOutBuffer = NULL;
	m_outBuffer = NULL;
	m_inBuffer = NULL;
	m_copyright = NULL;
}

XInflate::~XInflate()
{
	Free();
}

// ���� �޸� alloc ����
void XInflate::Free()
{
	SAFE_DEL(m_pStaticInfTable);
	SAFE_DEL(m_pStaticDistTable);
	SAFE_DEL(m_pDynamicInfTable);
	SAFE_DEL(m_pDynamicDistTable);
	SAFE_DEL(m_pLenLenTable);

	SAFE_FREE(m_dicPlusOutBuffer);
	SAFE_FREE(m_inBuffer);
}


////////////////////////////////////////////////////////////////////////////////////////////////////
///         ���� ���� �ʱ�ȭ
/// @param  
/// @return 
/// @date   Thursday, April 08, 2010  4:17:46 PM
////////////////////////////////////////////////////////////////////////////////////////////////////
XINFLATE_ERR XInflate::Init()
{
	SAFE_DEL(m_pLenLenTable);
	SAFE_DEL(m_pDynamicInfTable);
	SAFE_DEL(m_pDynamicDistTable);

	m_pCurrentTable = NULL;
	m_pCurrentDistTable = NULL;

	m_bFinalBlock = FALSE;
	m_copyright = xinflate_copyright;

	// ��� ���� ���
	if (m_dicPlusOutBuffer == NULL)
	{
		m_dicPlusOutBuffer = (BYTE*)malloc(DEFAULT_OUTBUF_SIZE + DEFLATE_WINDOW_SIZE);
		if (m_dicPlusOutBuffer == NULL){ASSERT(0); return XINFLATE_ERR_ALLOC_FAIL;}

		/*
		���� ���� alloc �� ��ġ���� ������ ũ�⸸ŭ ���ʿ� ���� �����Ѵ�.

		alloc ��ġ
		|		outBuffer ��ġ
		��       ��
		+-------+-----------------------------+
		| ����  |                             |
		+-------+-----------------------------+
		*/

		// ������ ������ ������ ���۽�����ġ
		m_outBuffer = m_dicPlusOutBuffer + DEFLATE_WINDOW_SIZE;
	}

	if (m_inBuffer == NULL)
	{
		m_inBuffer = (BYTE*)malloc(DEFAULT_INBUF_SIZE);
		if (m_inBuffer == NULL){ASSERT(0); return XINFLATE_ERR_ALLOC_FAIL;}
	}

#ifdef _DEBUG
	m_inputCount = 0;
	m_outputCount = 0;
#endif

	return XINFLATE_ERR_OK;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
///         ���� ���� - ȣ���� Init() �� �ݵ�� ȣ���� ��� �Ѵ�.
/// @param  
/// @return 
/// @date   Thursday, April 08, 2010  4:18:55 PM
////////////////////////////////////////////////////////////////////////////////////////////////////
XINFLATE_ERR XInflate::Inflate(IDecodeStream* stream)
{
	XINFLATE_ERR	ret = XINFLATE_ERR_OK;

	// �ʱ�ȭ
	ret = Init();
	if (ret != XINFLATE_ERR_OK) return ret;

	BITSTREAM	bits=0;
	int			bitLen=0;
	STATE		state= STATE_START;
	int			windowLen=0;
	int			windowDistCode=0;
	int			symbol=0;
	BYTE*		inBuffer = m_inBuffer;
	int			inBufferRemain = 0;
	UINT		uncompRemain = 0;

	BYTE*		outBufferCur = m_outBuffer;
	BYTE*		outBufferEnd = m_outBuffer + DEFAULT_OUTBUF_SIZE;
	BYTE*		windowStartPos = m_outBuffer;
	BYTE*		windowCurPos = m_outBuffer;

	// ���� ����
	int					extrabits;
	int					dist;
	//XFastHuffItem*		pItem;			// HUFFLOOKUP() ���� ���
	BYTE				byte;			// �ӽ� ����
	XINFLATE_ERR		err;

	// ���� ���鼭 ���� ����
	for(;;)
	{
		// ���� ä���
		if (inBufferRemain == 0)
		{
			inBuffer = m_inBuffer;
			inBufferRemain = stream->Read(inBuffer, DEFAULT_INBUF_SIZE);
			if (inBufferRemain == 0 && bitLen==0) 
			{
				if(state!= STATE_COMPLETED)ASSERT(0); 
				break;
			}
		}
		
		FILLBUFFER();

		switch(state)
		{
			// ��� �м� ����
		case STATE_START :
			if (bitLen < 3)
				continue;

			// ������ �� ����
			m_bFinalBlock = BS_EATBIT();

			// ��Ÿ�� ����� 2bit 
			{
				int bType = BS_EATBITS(2);

				if(bType==BTYPE_DYNAMICHUFFMAN)
					state = STATE_DYNAMIC_HUFFMAN;
				else if(bType==BTYPE_NOCOMP)
					state = STATE_NO_COMPRESSION;
				else if(bType==BTYPE_FIXEDHUFFMAN)
					state = STATE_FIXED_HUFFMAN;
				else
					DOERR(XINFLATE_ERR_HEADER);
			}
			break;

			// ���� �ȵ�
		case STATE_NO_COMPRESSION :
			BS_MOVETONEXTBYTE;
			state = STATE_NO_COMPRESSION_LEN;
			break;

		case STATE_NO_COMPRESSION_LEN :
			// LEN
			if(bitLen<16) 
				continue;
			uncompRemain = BS_EATBITS(16);
			state = STATE_NO_COMPRESSION_NLEN;

			break;

		case STATE_NO_COMPRESSION_NLEN :
			// NLEN
			if(bitLen<16) 
				continue;
			{
				UINT32 nlen = BS_EATBITS(16);
				// one's complement 
				if( (nlen^0xffff) != uncompRemain) 
					DOERR(XINFLATE_ERR_INVALID_NOCOMP_LEN);
			}
			state = STATE_NO_COMPRESSION_BYPASS;
			break;

		case STATE_NO_COMPRESSION_BYPASS :
			// ����Ÿ ��������
			if(bitLen<8) 
				continue;

			//////////////////////////////////////////////
			// ���� �ڵ�
			/*
			{
				byte = BS_EATBITS(8);
				WRITE(byte);
			}
			uncompRemain--;
			*/
			if(bitLen%8!=0) ASSERT(0);			// �߻��Ұ�, �׳� Ȯ�ο�

			//////////////////////////////////////////////
			//
			// �Ʒ��� ������ �ڵ�. �׷��� �� ���̰� ���� ��.��
			//

			// ��Ʈ ��Ʈ���� ���� ����
			while(bitLen && uncompRemain)
			{
				byte = BS_EATBITS(8);
				WRITE(byte);
				uncompRemain--;
			}

			// ������ ����Ÿ�� ����Ʈ �״�� ����
			{
				int	toCopy, toCopy2;
				toCopy = toCopy2 = min((int)uncompRemain, inBufferRemain);

				/* �����ڵ�
				while(toCopy)
				{
					WRITE(*inBuffer);
					inBuffer++;
					toCopy--;
				}
				*/

				if(outBufferEnd - outBufferCur > toCopy)
				{
					// ��� ���۰� ����� ��� - ��� ���۰� ������� ���θ� üũ���� �ʴ´�.
					/*
					while(toCopy)
					{
						WRITE_FAST(*inBuffer);
						inBuffer++;
						toCopy--;
					}
					*/
					WRITE_BLOCK(inBuffer, toCopy);
					inBuffer += toCopy;
					toCopy = 0;
				}
				else
				{
					while(toCopy)
					{
						WRITE(*inBuffer);
						inBuffer++;
						toCopy--;
					}
				}

				uncompRemain-=toCopy2;
				inBufferRemain-=toCopy2;
			}
			//
			// ������ �ڵ� ��
			//
			//////////////////////////////////////////////

			if(uncompRemain==0)
			{
				if(m_bFinalBlock)
					state = STATE_COMPLETED;
				else
					state = STATE_START;
			}
			break;

			// ���� ������
		case STATE_FIXED_HUFFMAN :
			if(m_pStaticInfTable==NULL)
				CreateStaticTable();

			m_pCurrentTable = m_pStaticInfTable;
			m_pCurrentDistTable = m_pStaticDistTable;
			state = STATE_GET_SYMBOL;
			break;

			// ���� ��������
		case STATE_GET_LEN :
			// zlib �� inflate_fast ȣ�� ���� �䳻����
			/*
			if(inBufferRemain>FASTINFLATE_MIN_BUFFER_NEED)
			{
				if(symbol<=256)	ASSERT(0);

				XINFLATE_ERR result = FastInflate(stream, inBuffer, inBufferRemain, 
										outBufferCur, 
										outBufferEnd,
										windowStartPos, windowCurPos,
										bits, bitLen, state, windowLen, windowDistCode, symbol);

				if(result!=XINFLATE_ERR_OK)
					return result;

				break;
			}
			*/

			extrabits = lencodes_extrabits[symbol - 257];
			if (bitLen < extrabits) 
				continue;

			// RFC 1951 3.2.5
			// �⺻ ���̿� extrabit ��ŭ�� ��Ʈ�� ������ ���ϸ� ��¥ ���̰� ���´�
			windowLen = lencodes_min[symbol - 257] + BS_EATBITS(extrabits);

			state = STATE_GET_DISTCODE;
			FILLBUFFER();
			//break;	�ʿ� ����..

			// �Ÿ� �ڵ� ��������
		case STATE_GET_DISTCODE :
			//HUFFLOOKUP(windowDistCode, m_pCurrentDistTable);
			windowDistCode = m_pCurrentDistTable->HUFFLOOKUP(bitLen, bits);
			if(windowDistCode<0)
				continue;

			//if(windowDistCode==30 || windowDistCode==31)	// 30 �� 31�� ���� �� ����. RFC1951 3.2.6
			if(windowDistCode>=30)							
				DOERR(XINFLATE_ERR_INVALID_DIST);

			state = STATE_GET_DIST;

			FILLBUFFER();
			//break;		// �ʿ����

			// �Ÿ� ��������
		case STATE_GET_DIST:
			extrabits = distcodes_extrabits[windowDistCode];

			// DIST ���ϱ�
			if(bitLen<extrabits)
				continue;

			dist = distcodes_min[windowDistCode] + BS_EATBITS(extrabits);

			// lz77 ���
			while(windowLen)
			{
				byte = *WIN_GETBUF(dist);
				WRITE(byte);
				windowLen--;
			}
	
			state = STATE_GET_SYMBOL;

			FILLBUFFER();
			//break;		// �ʿ� ����

			// �ɺ� ��������.
		case STATE_GET_SYMBOL :
			//HUFFLOOKUP(symbol, m_pCurrentTable);
			symbol = m_pCurrentTable->HUFFLOOKUP(bitLen, bits);
			if(symbol<0) 
				continue;

			if(symbol<256)
			{
				byte = (BYTE)symbol;
				WRITE(byte);
				break;
			}
			else if(symbol==256)	// END OF BLOCK
			{
				if(m_bFinalBlock)
					state = STATE_COMPLETED;
				else
					state = STATE_START;
				break;
			}
			else if(symbol<286)
			{
				state = STATE_GET_LEN;
			}
			else
				DOERR(XINFLATE_ERR_INVALID_SYMBOL);		// �߻� �Ұ�

			break;

			// ���̳��� ������ ����
		case STATE_DYNAMIC_HUFFMAN :
			if (bitLen < 5 + 5 + 4)
				continue;

			// ���̺� �ʱ�ȭ
			SAFE_DEL(m_pDynamicInfTable);
			SAFE_DEL(m_pDynamicDistTable);

			m_literalsNum  = 257 + BS_EATBITS(5);		// �ִ� 288 (257+11111)
			m_distancesNum = 1   + BS_EATBITS(5);		// �ִ� 32
			m_lenghtsNum   = 4   + BS_EATBITS(4);

			if(m_literalsNum > 286 || m_distancesNum > 30)
				DOERR(XINFLATE_ERR_INVALID_LEN);

			memset(m_lengths, 0, sizeof(m_lengths));
			m_lenghtsPtr = 0;

			state = STATE_DYNAMIC_HUFFMAN_LENLEN ;
			break;

			// ������ ���� ��������.
		case STATE_DYNAMIC_HUFFMAN_LENLEN :
			if(bitLen<3) 
				continue;

			// 3bit �� �ڵ� ������ �ڵ� ���� ���� ��������.
			while (m_lenghtsPtr < m_lenghtsNum && bitLen >= 3) 
			{
				if(m_lenghtsPtr>sizeof(lenlenmap))						// �Է°� üũ..
					DOERR(XINFLATE_ERR_INVALID_LEN);
#ifdef _DEBUG
				if(lenlenmap[m_lenghtsPtr]>=286+32) ASSERT(0);
				if(m_lenghtsPtr>=sizeof(lenlenmap)) ASSERT(0);
#endif

				m_lengths[lenlenmap[m_lenghtsPtr]] = BS_EATBITS(3);
				m_lenghtsPtr++;
			}

			// �� ����������..
			if (m_lenghtsPtr == m_lenghtsNum)
			{
				// ���̿� ���� ������ ���̺� �����
				if(CreateTable(m_lengths, 19, m_pLenLenTable, err)==FALSE)
					DOERR(err);
				state = STATE_DYNAMIC_HUFFMAN_LEN;
				m_lenghtsPtr = 0;
			}
			break;

			// ����� ���� ������ m_pLenLenTable �� ���ļ� ��������.
		case STATE_DYNAMIC_HUFFMAN_LEN:

			// �� ����������
			if (m_lenghtsPtr >= m_literalsNum + m_distancesNum) 
			{
				// ���� ���̳��� ���̺� ���� (literal + distance)
				if(	CreateTable(m_lengths, m_literalsNum, m_pDynamicInfTable, err)==FALSE ||
					CreateTable(m_lengths + m_literalsNum, m_distancesNum, m_pDynamicDistTable, err)==FALSE)
					DOERR(err);

				// lenlen ���̺��� ���� ���̻� �ʿ����.
				SAFE_DEL(m_pLenLenTable);

				// ���̺� ����
				m_pCurrentTable = m_pDynamicInfTable;
				m_pCurrentDistTable = m_pDynamicDistTable;

				// ��¥ ���� ���� ����
				state = STATE_GET_SYMBOL;
				break;
			}

			{
				// ���� ���� �ڵ� ��������
				int code=-1;
				//HUFFLOOKUP(code, m_pLenLenTable);
				code = m_pLenLenTable->HUFFLOOKUP(bitLen, bits);

				if (code == -1)
					goto END;

				if (code < 16) 
				{
					if(m_lenghtsPtr>sizeof(m_lengths))		// �� üũ
						DOERR(XINFLATE_ERR_INVALID_LEN);

					m_lengths[m_lenghtsPtr] = code;
					m_lenghtsPtr++;
				} 
				else 
				{
					m_lenExtraBits = (code == 16 ? 2 : code == 17 ? 3 : 7);
					m_lenAddOn = (code == 18 ? 11 : 3);
					m_lenRep = (code == 16 && m_lenghtsPtr > 0 ? m_lengths[m_lenghtsPtr - 1] : 0);
					state = STATE_DYNAMIC_HUFFMAN_LENREP;
				}
			}
			break;

		case STATE_DYNAMIC_HUFFMAN_LENREP:
			if (bitLen < m_lenExtraBits)
				continue;

			{
				int repeat = m_lenAddOn + BS_EATBITS(m_lenExtraBits);

				while (repeat > 0 && m_lenghtsPtr < m_literalsNum + m_distancesNum) 
				{
					m_lengths[m_lenghtsPtr] = m_lenRep;
					m_lenghtsPtr++;
					repeat--;
				}
			}

			state = STATE_DYNAMIC_HUFFMAN_LEN;
			break;

		case STATE_COMPLETED :
			goto END;
			break;

		default :
			ASSERT(0);
		}
	}

END :
	if (stream->Write(windowStartPos, (int)(outBufferCur - windowStartPos)) == FALSE)
		return XINFLATE_ERR_USER_STOP;

	if(state!= STATE_COMPLETED)			// ���� �߸����.
	{ASSERT(0); return XINFLATE_ERR_STREAM_NOT_COMPLETED;}

	if (inBufferRemain) ASSERT(0);

	return ret;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
///         length ������ ������ code �� �����Ѵ�. 
//          RFC 1951 3.2.2
/// @param  
/// @return 
/// @date   Wednesday, April 07, 2010  1:53:36 PM
////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL XInflate::CreateCodes(BYTE* lengths, int numSymbols, int* codes)
{
	int		bits;
	int		code = 0;
	int		bitLenCount[MAX_CODELEN+1];
	int		next_code[MAX_CODELEN+1];
	int		i;
	int		n;
	int		len;

	memset(bitLenCount, 0, sizeof(bitLenCount));
	for(i=0;i<numSymbols;i++)
	{
		bitLenCount[lengths[i]] ++;
	}

	bitLenCount[0] = 0;

	for(bits=1;bits<=MAX_CODELEN;bits++)
	{
		code = (code + bitLenCount[bits-1]) << 1;
		next_code[bits] = code;
	}

	for(n=0; n<numSymbols; n++)
	{
		len = lengths[n];
		if(len!=0)
		{
			codes[n] = next_code[len];
			next_code[len]++;
		}
	}

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
///         codes + lengths ������ ������ ���̺��� �����.
/// @param  
/// @return 
/// @date   Wednesday, April 07, 2010  3:43:21 PM
////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL XInflate::CreateTable(BYTE* lengths, int numSymbols, XFastHuffTable*& pTable, XINFLATE_ERR& err)
{
	int			codes[MAX_SYMBOLS];
	// lengths ������ ������ �ڵ����� codes ������ �����Ѵ�.
	if(CreateCodes(lengths, numSymbols, codes)==FALSE) 
	{	
		err = XINFLATE_ERR_CREATE_CODES;
		ASSERT(0); 
		return FALSE;
	}
	if(_CreateTable(codes, lengths, numSymbols, pTable)==FALSE)
	{
		err = XINFLATE_ERR_CREATE_TABLE;
		ASSERT(0); 
		return FALSE;
	}
	return TRUE;
}
BOOL XInflate::_CreateTable(int* codes, BYTE* lengths, int numSymbols, XFastHuffTable*& pTable)
{
	int		bitLenCount[MAX_CODELEN+1];
	int		symbol;
	int		bitLen;

	// bit length ���ϱ�
	memset(bitLenCount, 0, sizeof(bitLenCount));
	for(symbol=0;symbol<numSymbols;symbol++)
		bitLenCount[lengths[symbol]] ++;


	// ������ Ʈ������ ��ȿ�� �ּ� bitlen �� �ִ� bitlen ���ϱ�
	int	bitLenMax = 0;
	int	bitLenMin = MAX_CODELEN;

	for(bitLen=1;bitLen<=MAX_CODELEN;bitLen++)
	{
		if(bitLenCount[bitLen])
		{
			bitLenMax = max(bitLenMax, bitLen);
			bitLenMin = min(bitLenMin, bitLen);
		}
	}

	// ���̺� ����
	pTable = new XFastHuffTable;
	if(pTable==0) {ASSERT(0); return FALSE;}			// �߻� �Ұ�.
	pTable->Create(bitLenMin, bitLenMax);


	// ���̺� ä���
	for(symbol=0;symbol<numSymbols;symbol++)
	{
		bitLen = lengths[symbol];
		if(bitLen)
		{
			if(pTable->SetValue(symbol, bitLen, codes[symbol])==FALSE)
				return FALSE;
		}
	}

//#ifdef _DEBUG
	// ����Ÿ�� �ջ�� ��� ���̺��� ���������� ��������� ���� �� �����Ƿ� ���� ó�� �ʿ�.
	for(UINT i=0;i<pTable->itemCount;i++)
	{
		if(pTable->pItems[i].symbol==HUFFTABLE_VALUE_NOT_EXIST)
		{
			ASSERT(0);
			return FALSE;
		}
	}
//#endif

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
///         static �������� ����� ���̺� �����
/// @param  
/// @return 
/// @date   Thursday, April 08, 2010  4:17:06 PM
////////////////////////////////////////////////////////////////////////////////////////////////////
void XInflate::CreateStaticTable()
{
	BYTE		lengths[MAX_SYMBOLS];

	// static symbol ���̺� �����
	// RFC1951 3.2.6
    memset(lengths, 8, 144);
    memset(lengths + 144, 9, 256 - 144);
    memset(lengths + 256, 7, 280 - 256);
    memset(lengths + 280, 8, 288 - 280);

	XINFLATE_ERR err;
	CreateTable(lengths, MAX_SYMBOLS, m_pStaticInfTable, err);

	// static dist ���̺� �����
	// RFC1951 3.2.6
	memset(lengths, 5, 32);
	CreateTable(lengths, 32, m_pStaticDistTable, err);
}


////////////////////////////////////////////////////////////////////////////////////////////////////
///         �Է� ���۰� ����� ��� ���� ���ڵ��� �����Ѵ�.
/// @param  
/// @return 
/// @date   Thursday, May 13, 2010  1:43:34 PM
////////////////////////////////////////////////////////////////////////////////////////////////////
XINFLATE_ERR XInflate::FastInflate(IDecodeStream* stream, LPBYTE& inBuffer, int& inBufferRemain,
		LPBYTE& outBufferCur, 
		LPBYTE& outBufferEnd,
		LPBYTE& windowStartPos, LPBYTE& windowCurPos,
		BITSTREAM&		bits, int& bitLen,
		STATE&		state,
		int& windowLen, int& windowDistCode, int& symbol)
{
	XINFLATE_ERR	ret = XINFLATE_ERR_OK;
	int				extrabits;
	int				dist;
	//XFastHuffItem*	pItem;							// HUFFLOOKUP() ���� ���
	XFastHuffItem*	pItems = m_pCurrentTable->pItems;	// HUFFLOOKUP ���� ���� �����Ҽ��ְ� ���� ������ �����س���
	int				mask = m_pCurrentTable->mask;		// ...��������.
	BYTE			byte;								// �ӽ� ����

	// ���� ���鼭 ���� ����
	while(inBufferRemain>FASTINFLATE_MIN_BUFFER_NEED)
	{
		FILLBUFFER_FAST();

		/////////////////////////////////////
		// ���� ��������
		extrabits = lencodes_extrabits[symbol - 257];

		// RFC 1951 3.2.5
		// �⺻ ���̿� extrabit ��ŭ�� ��Ʈ�� ������ ���ϸ� ��¥ ���̰� ���´�
		windowLen = lencodes_min[symbol - 257] + BS_EATBITS(extrabits);

		FILLBUFFER_FAST();

		
		/////////////////////////////////////
		// �Ÿ� �ڵ� ��������
		//HUFFLOOKUP(windowDistCode, m_pCurrentDistTable);
		windowDistCode = m_pCurrentDistTable->HUFFLOOKUP(bitLen, bits);

		//if(windowDistCode==30 || windowDistCode==31)	// 30 �� 31�� ���� �� ����. RFC1951 3.2.6
		if(windowDistCode>=30)							// �̷л� 32, 33 � ���� �� �ִµ�?
			DOERR(XINFLATE_ERR_INVALID_DIST);
		if(windowDistCode<0)
			DOERR(XINFLATE_ERR_INVALID_DIST);											// fast inflate ����..

		FILLBUFFER_FAST();


		/////////////////////////////////////
		// �Ÿ� ��������
		
		//rec = &distcodes[windowDistCode];
		extrabits = distcodes_extrabits[windowDistCode];

		// DIST ���ϱ�
		dist = distcodes_min[windowDistCode] + BS_EATBITS(extrabits);


		/////////////////////////////////////
		// lz77 ���
		if(outBufferEnd - outBufferCur > windowLen)
		{
			// ��� ���۰� ����� ��� - ��� ���۰� ������� ���θ� üũ���� �ʴ´�.
			do
			{
				byte = *WIN_GETBUF(dist);
				WRITE_FAST(byte);
			}while(--windowLen);
		}
		else
		{
			// ��� ���۰� ������� ���� ���
			do
			{
				byte = *WIN_GETBUF(dist);
				WRITE(byte);
			}while(--windowLen);
		}


		/////////////////////////////////////
		// �ɺ� ��������.
		for(;;)
		{
			// �Է� ���� üũ
			if(!(inBufferRemain>FASTINFLATE_MIN_BUFFER_NEED)) 
			{
				state = STATE_GET_SYMBOL;
				goto END;
			}

			FILLBUFFER_FAST();

			//HUFFLOOKUP_FAST(symbol);
			//symbol = HUFFLOOKUP_FAST(pItems, mask, bitLen, bits);
			XFastHuffItem*  pItem = &pItems[mask & bits];
			symbol = pItem->symbol;
			bits >>= (pItem->bitLen);
			bitLen -= (pItem->bitLen);


			if(symbol<0) 
			{ASSERT(0); goto END;}					// �߻� �Ұ�.
			else if(symbol<256)
			{
				byte = (BYTE)symbol;
				WRITE(byte);
			}
			else if(symbol==256)	// END OF BLOCK
			{
				if(m_bFinalBlock)
					state = STATE_COMPLETED;
				else
					state = STATE_START;
				// �Լ� ����
				goto END;
			}
			else if(symbol<286)
			{
				// ���� ��������� �����Ѵ�.
				state = STATE_GET_LEN;
				break;
			}
			else
				DOERR(XINFLATE_ERR_INVALID_SYMBOL);		// �߻� �Ұ�
		}
	}

END :
	return ret;
}

