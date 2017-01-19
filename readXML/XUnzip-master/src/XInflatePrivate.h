////////////////////////////////////////////////////////////////////////////////////////////////////
/// 
/// 
/// 
/// @author   parkkh
/// @date     Sun Jul 24 02:19:48 2016
/// 
/// Copyright(C) 2016 Bandisoft, All rights reserved.
/// 
////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

/**********************************************************************************

-= history =-

* 2010/4/5 �ڵ� ����
* 2010/4/9 1�� �ϼ�
- �ӵ� �� (34.5MB ¥�� gzip ����, Q9400 CPU)
. zlib : 640ms
. xinflate : 1900ms

* 2010/4/29
- �ӵ� ��� ����ȭ
. �Ϻ� �Լ� ȣ���� ��ũ��ȭ ��Ű�� : 1900ms
. ��Ʈ ��Ʈ���� ���� ����ȭ ��Ű��: 1800ms
. Write �Լ� ��ũ��ȭ: 1750ms
. Window ��� ����ȭ: 1680ms
. Window ��ũ��ȭ + ���� ����ȭ : 1620ms
. InputBuffer ���� ����ȭ : 1610ms
. ��� ������ ���� ����ȭ : 1500ms;
. while() �������� m_bCompleted �� ���� : 1470ms;
. while() ���������� m_error �� ���� : 1450ms
. m_state ���� ����ȭ : 1450ms
. m_windowLen, m_windowDistCode ���� ����ȭ : 1390ms

- ���� ����ȭ �� �ӵ� ���� �� (34.5MB ¥�� gzip ����, Q9400 CPU)
. zlib : 640MS
. Halibut : 1750ms
. XInflate : 1370ms

* 2010/4/30
- �ڵ� ����
- v1.0 ���� ����

* 2010/5/11
- static table �� �ʿ��Ҷ��� �����ϵ��� ����
- ����ȭ
. ����ȭ �� : 1340ms
. CHECK_TABLE ���� : 1330ms
. FILLBUFFER() ����: 1320ms
. table�� ��ũ�� ����Ʈ���� �迭�� ���� : 1250ms

* 2010/5/12
- ����ȭ ���
. STATE_GET_LEN �� break ���� : 1220ms
. STATE_GET_DISTCODE �� break ���� : 1200ms
. STATE_GET_DIST �� break ���� : 1170ms

* 2010/5/13
- ����ȭ ���
. FastInflate() �� �и��� FILLBUFFER_FAST, HUFFLOOKUP_FAST ���� : 1200ms
. ������ Ʈ�� ó�� ��� ����(���� ���̺� ������ Ʈ�� Ž���� ���ְ� �޸� ��뷮�� ���̰�) : 900ms
. HUFFLOOKUP_FAST �ٽ� ���� : 890ms
. WRITE_FAST ���� : 810ms
. lz77 ������ ��½� while() �� do-while �� ���� : 800ms

* 2010/5/19
- ��� ���۸� ���� alloc ��� �ܺ� ���۸� �̿��� �� �ֵ��� ��� �߰�
- m_error ���� ����

* 2010/5/25
- �ܺι��� ��� ����� ���� ����
- direct copy �� �ణ ����

* 2010/08/31
- ���̺� ������ �̻��Ѱ� ������ ���������ϵ��� ����

* 2011/08/01
- STATE_GET_LEN ������ FILLBUFFER �� ���ϰ� STATE_GET_DISTCODE �� �Ѿ�鼭 ������ ����� Ǯ�� ���ϴ� ��찡 �ִ�
���� ���� ( Inflate() �� FastInflate() �ΰ� �� )

* 2011/08/16
- coderecord �� ����ü�� ������� �ʰ�, len �� dist�� �����ϵ��� ����
142MB ���� : 1820ms -> 1720ms �� ������
- HUFFLOOKUP_FAST ���� m_pCurrentTable �� �������� �ʰ�, pItems �� �����ϵ��� ����
1720ms -> 1700ms �� ������ (zlib �� 1220ms)

* 2012/01/28
- windowDistCode �� 32, 33 ���� �ɶ� ���� ó�� �ڵ� ����
- _CreateTable ���� ���̺��� �ջ�� ��� ���� ó�� �ڵ� ���

* 2012/02/7
- XFastHuffItem::XFastHuffItem() �� �ּ����� ���Ҵ��� Ǯ����.

* 2016/7/21
- ������ zlib ��Ÿ���� ���� ����� �ݹ� ȣ�� ������� ����
- ���� ���۵� ���� �Ҵ����� �ʰ�, ��� ���۸� ���� ����ϵ��� ����

* 2016/7/22
- BYPASS �Ҷ� ���� ��� WRITE_BLOCK �� memcpy �� �ӵ� ���

***********************************************************************************/


// DEFLATE �� ���� Ÿ��
enum BTYPE
{
	BTYPE_NOCOMP = 0,
	BTYPE_FIXEDHUFFMAN = 1,
	BTYPE_DYNAMICHUFFMAN = 2,
	BTYPE_RESERVED = 3		// ERROR
};


#define LENCODES_SIZE		29
#define DISTCODES_SIZE		30

static int lencodes_extrabits[LENCODES_SIZE] = { 0,   0,   0,   0,   0,   0,   0,   0,   1,   1,   1,   1,   2,   2,   2,   2,   3,   3,   3,   3,   4,   4,   4,   4,   5,   5,   5,   5,   0 };
static int lencodes_min[LENCODES_SIZE] = { 3,   4,   5,   6,   7,   8,   9,  10,  11,  13,  15,  17,  19,  23,  27,  31,  35,  43,  51,  59,  67,  83,  99, 115, 131, 163, 195, 227, 258 };
static int distcodes_extrabits[DISTCODES_SIZE] = { 0, 0, 0, 0, 1, 1,  2,  2,  3,  3,  4,  4,  5,   5,   6,   6,   7,   7,   8,    8,    9,    9,   10,   10,   11,   11,    12,    12,    13,    13 };
static int distcodes_min[DISTCODES_SIZE] = { 1, 2, 3, 4, 5, 7,  9, 13, 17, 25, 33, 49, 65,  97, 129, 193, 257, 385, 513,  769, 1025, 1537, 2049, 3073, 4097, 6145,  8193, 12289, 16385, 24577 };


// code length 
static const unsigned char lenlenmap[] = { 16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15 };


// ��ƿ ��ũ��
#ifndef ASSERT
#	define ASSERT(x) {}
#endif
#define SAFE_DEL(x) if(x) {delete x; x=NULL;}
#define SAFE_FREE(x) if(x) {free(x); x=NULL;}

////////////////////////////////////////////////
//
// ������ ��ũ��
//
#ifdef _DEBUG
#	define ADD_INPUT_COUNT			m_inputCount ++
#	define ADD_OUTPUT_COUNT			m_outputCount ++
#	define ADD_OUTPUT_COUNTX(x)		m_outputCount += x
#else
#	define ADD_INPUT_COUNT	
#	define ADD_OUTPUT_COUNT
#	define ADD_OUTPUT_COUNTX(x)
#endif

#define	DOERR(x) { ASSERT(0); ret = x; goto END; }



