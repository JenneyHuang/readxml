////////////////////////////////////////////////////////////////////////////////////////////////////
/// 
/// fast huff table �и�
/// 
/// @author   parkkh
/// @date     Sun Jul 24 02:03:46 2016
/// 
/// Copyright(C) 2016 Bandisoft, All rights reserved.
/// 
////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once


/////////////////////////////////////////////////////
//
// ������ Ʈ���� ���� Ž���� �� �ְ� �ϱ� ���ؼ�
// Ʈ�� ��ü�� �ϳ��� �迭�� ����� ������.
//   

#define HUFFTABLE_VALUE_NOT_EXIST	-1

struct XFastHuffItem									// XFastHuffTable �� ������
{
	// ���� ����Ÿ���� ���� �����÷ο츦 ���� ���ؼ� �׻� �ʱ�ȭ�� ����� �Ѵ� ��.��
	XFastHuffItem()
	{
		bitLen = 0;
		symbol = HUFFTABLE_VALUE_NOT_EXIST;
	}
	SHORT	bitLen;										// ��ȿ�� ��Ʈ��
	SHORT	symbol;										// ��ȿ�� �ɺ�
};



class XFastHuffTable
{
public:
	XFastHuffTable()
	{
		pItems = NULL;
	}
	~XFastHuffTable()
	{
		if (pItems) { delete[] pItems; pItems = NULL; }
	}
	// �迭 ����
	void	Create(int _bitLenMin, int _bitLenMax)
	{
		if (pItems) ASSERT(0);							// �߻� �Ұ�

		bitLenMin = _bitLenMin;
		bitLenMax = _bitLenMax;
		mask = (1 << bitLenMax) - 1;					// ����ũ
		itemCount = 1 << bitLenMax;						// ���� ������ �ִ� ���̺� ũ��
		pItems = new XFastHuffItem[itemCount];		// 2^maxBitLen ��ŭ �迭�� �����.
	}
	// �ɺ� ����Ÿ�� ������ ��ü �迭�� ä���.
	__forceinline BOOL SetValue(int symbol, int bitLen, UINT bitCode)
	{
		/*
		���� ������ Ʈ�� ��忡�� 0->1->1 �̶�� ����Ÿ�� 'A' ���
		symbol = 'A',   bitLen = 3,   bitCode = 3  �� �Ķ���ͷ� ���޵ȴ�.

		* ���� bitstream �� �������� ������ ������ ���߿� ������ ���� �ϱ� ���ؼ�
		0->1->1 �� 1<-1<-0 ���� �����´�.

		* ���� bitLenMax �� 5 ��� �������� 1<-1<-0 �� �� �� 2bit �� ���� ������
		00110, 01110, 10110, 11110 �� ���ؼ��� ������ �ɺ��� ������ �� �ֵ��� �����.
		*/
		//::Ark_DebugW(L"SetValue: %d %d %d", symbol, bitLen, bitCode);
		if (bitCode >= ((UINT)1 << bitLenMax))
		{
			ASSERT(0); return FALSE;
		}			// bitLenmax �� 3 �̶��.. 111 ������ �����ϴ�


		UINT revBitCode = 0;
		// ������
		int i;
		for (i = 0; i<bitLen; i++)
		{
			revBitCode <<= 1;
			revBitCode |= (bitCode & 0x01);
			bitCode >>= 1;
		}

		int		add2code = (1 << bitLen);		// bitLen �� 3 �̶�� add2code ���� 1000(bin) �� ����

												// �迭 ä���
		for (;;)
		{
#ifdef _DEBUG
			if (revBitCode >= itemCount) ASSERT(0);

			if (pItems[revBitCode].symbol != HUFFTABLE_VALUE_NOT_EXIST)
			{
				ASSERT(0); return FALSE;
			}
#endif
			pItems[revBitCode].bitLen = bitLen;
			pItems[revBitCode].symbol = symbol;

			// ���� ������ bit code �� ó���ϱ� ���ؼ� ���� ��� ���Ѵ�.
			revBitCode += add2code;

			// ���� ������ ������ ��� ��� ������
			if (revBitCode >= itemCount)
				break;
		}
		return TRUE;
	}

	__forceinline int HUFFLOOKUP(int& bitLen, BITSTREAM& bits)
	{
		/*
		// ����Ÿ ���� 											
		if (this->bitLenMin > bitLen)
		{
			//ASSERT(0);
			return -1;
		}
		else
		*/
		{
			XFastHuffItem* pItem = &(this->pItems[this->mask & bits]);
			// ����Ÿ ���� 
			if (pItem->bitLen > bitLen)
			{
				//ASSERT(0);
				return -1;
			}
			else
			{
				//BS_REMOVEBITS(pItem->bitLen);
				bits >>= (pItem->bitLen);
				bitLen -= (pItem->bitLen);
				ASSERT(!(bitLen < 0));
				return pItem->symbol;
			}
		}
	}


	XFastHuffItem*	pItems;							// (huff) code 2 symbol ������, code�� �迭�� ��ġ ������ �ȴ�.
	int				bitLenMin;						// ��ȿ�� �ּ� ��Ʈ��
	int				bitLenMax;						// ��ȿ�� �ִ� ��Ʈ��
	UINT			mask;							// bitLenMax �� ���� bit mask
	UINT			itemCount;						// bitLenMax �� ���� ������ �ִ� ������ ����
};


inline int HUFFLOOKUP_FAST(XFastHuffItem* pItems, int mask, int& bitLen, BITSTREAM& bits)
{
	XFastHuffItem* pItem = &(pItems[mask & bits]);
	//BS_REMOVEBITS(pItem->bitLen);
	bits >>= (pItem->bitLen);
	bitLen -= (pItem->bitLen);
	return pItem->symbol;
}

