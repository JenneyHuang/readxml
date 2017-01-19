#include <iostream>
#include <time.h>
#include <assert.h>
#include <Windows.h>
#include <sstream>
#include <fstream>
#include <string>
//rapid core readxml
#include "rapidxml-1.13/rapidxml.hpp"
#include "rapidxml-1.13/rapidxml_utils.hpp"
#include "rapidxml-1.13/rapidxml_print.hpp"

//XUnzip unzip zte's zip
#include "XUnzip-master\unzip\stdafx.h"
#include "XUnzip-master\src\XUnzip.h"
#include "XUnzip-master\unzip\myutil.h"
//read single zip or gz
//#pragma comment(lib, "zlibwapi.lib")
#include "unzip.h"
#include "zconf.h"
#include "zlib.h"
//makedir
#include "traverse_dir.h"
#include "direct.h"


//#include "rapidxml-1.13/rapidxml_iterators.hpp"
//#include <io.h>
//#include <mbstring.h>
//#include <tchar.h> 
//#include <stdio.h>
//#include <strsafe.h>
//#include <vector>

using namespace rapidxml;
#define GZ_BUF_SIZE 1024
#define MAX_LEN 1024*1024*100  //100M
//#define compressRate 25  //ѹ����Ĭ�����Ϊ4% ��25��
#define numOfManufacture 5

const char* manufacture[numOfManufacture] = {"ALCATEL","DATANG","HUAWEI","NSN","ZTE"};
enum manufactureName { ALCATEL,DATANG,HUAWEI,NSN,ZTE };
std::string  filepath = ".\\";
std::string outpath = ".\\gsz\\output";
const char* configFileName = "readXML.config";

std::string covertCI(std::string &orgci, int &manufacture);
int getManufacture(std::string &str);

std::string & replace_all(std::string& str, const std::string& old_value, const std::string& new_value);
std::string getFileName(std::string & filename,std::string & pathName);
int readMROData(std::string &filename,std::string &output);
void replaceFirstWS(std::string &nvstr);
void replaceLastWS(std::string &nvstr);
void readConfig(const char* cfilename, std::string & filepath, std::string & outputPath);
void writeConfig(std::string strPath,std::string outputPath);
int isFileExist(const char* filename);
bool gzipLoad(const char* gzfn, ::std::string &out);
bool zipload(const char* zipfn, std::string & out);
void readZData(std::string &filename, std::string &outputpath);
int readXMLchar(char * c, std::string & filename,std::string & outputpath);
int BigZipLoad(const char* zipfn);
int ExtractAllTo(XUnzip& unzip, CString targetFolder);
int Unzip(CString zipPathName, CString targetFolder);
void Wchar_tToString(std::string & szDst, wchar_t *wchar);
void formatPath(std::string &pathname);

/*****    unused functions     ********/
//std::string int2str(int int_temp);
//std::string & replace_all_distinct(std::string& str, const std::string &old_value, const std::string& new_value);
//int dir(std::string & path);
//unsigned char* gzLoad(const char * gzfileName, uLong &decmpLen);
//int gzdecompress(Byte *zdata, uLong nzdata,Byte *data, uLong *ndata);
//std::string getBigZipFilepath(const char * orgFName);

int main(int argc, char* argv[])
{
	
	if (argv[1] != '\0' && argv[2] != '\0')
	{	//��ȡ����ĵ�1������ �� д�������ļ�  readXml.exe  "filepathֵ"
		filepath= argv[1];
		outpath = argv[2];
		writeConfig(filepath,outpath);
	}
	if (isFileExist(configFileName))
		//��������ļ��Ѵ������ȡ�����ļ�
		readConfig(configFileName, filepath, outpath);
	else
		writeConfig(filepath, outpath);
	
	clock_t start, finish;
	double totalTime;
	int num = 0;

	//int num= dir(filepath);
	std::string strDir = filepath;
	std::vector<std::string> vec;
	int ret = traverse_dir(strDir, vec, true, true);
	if (ret < 0)
		std::cout << "·�������뽫mroѹ���ļ���·��д�������ļ�(readxml.config)�е�valueֵ" << std::endl;
	else if (ret == 0)
	{
		std::cout << "����·��/����ǰ·������MROѹ���ļ�" << std::endl;
	}
	else
	{
		start = clock();
		for (unsigned int i = 0; i<vec.size(); ++i)
		{
			//if (vec[i].find(".gz", 0) != std::string::npos)
			//{
				std::cout << i+1 << "/" << vec.size() << " \n" << vec[i] << std::endl;
				num += readMROData(vec[i],outpath);
			//}
		}
		finish = clock();
		totalTime = (double)(finish - start) / CLOCKS_PER_SEC;
		std::cout << "�������� " << totalTime << " �롣" << "ƽ��ÿ�� " << num / totalTime << " ��" << std::endl;
	}
	system("PAUSE");
	return EXIT_SUCCESS;
}

std::string covertCI(std::string &orgci, int &manufacture)
{
	std::string newci = "460-00-";
	if (orgci.find(":", 0) != std::string::npos)
	{
		if (manufacture == HUAWEI || manufacture == ZTE)   //HUAWEI 1019537-16:38350:2  ZTE version="1.0.0"
		{
			return newci += (orgci.substr(0, orgci.find(':', 0)));
		}
		else if (manufacture == ALCATEL)  //ALCATEL  110650626:37900:7
		{
			return covertCI(orgci.substr(0, orgci.find(":", 0)), manufacture);
		}
		else 
			return "";
	}
	else
	{
		char c[8];
		_itoa_s(std::stoi(orgci) >> 8 ,c ,10);   //��256ȡ�̵�����
		newci += c;
		newci.push_back('-');
		_itoa_s(std::stoi(orgci) & 255, c, 10);  //��256ȡ���� 
		//newci+=(itoa(std::stoi(orgci) % 256);
		newci += c;
		return newci;
		//return "460-00-" + int2str(std::stoi(orgci) / 256) + '-'+ int2str(std::stoi(orgci) % 256);
	}
}
/*
std::string int2str( int int_temp)
{
	std::stringstream stream;
	stream << int_temp;
	return stream.str();   //�˴�Ҳ������ stream>>string_temp  
}*/
void Wchar_tToString(std::string & szDst, wchar_t *wchar)
{
	wchar_t * wText = wchar;

	DWORD dwNum = WideCharToMultiByte(CP_OEMCP, NULL, wText, -1, NULL, 0, NULL, FALSE);
	char *psText;
	psText = new char[dwNum];
	WideCharToMultiByte(CP_OEMCP, NULL, wText, -1, psText, dwNum, NULL, FALSE);
	szDst = psText;
	delete[] psText; 
	/*std::stringstream stream;
	stream << wchar;
	szDst = stream.str();*/
}

int getManufacture(std::string &str)
{
	
	//ZTE NSN HW  DATANG ALCATEL 
	// 4  3   2     1       0
	
	if (str.length() > 0)
	{
		int ret = 0;
		for (unsigned int i =0; i < numOfManufacture; i++)
		{
			if (str.find(manufacture[i], 0) != std::string::npos)
				ret = i;
		}
		return ret;
	}
	else
		return -1;
}
//std::string & replace_all_distinct(std::string& str, const std::string & old_value, const std::string & new_value)
//{
//	for (std::string::size_type pos(0); pos != std::string::npos; pos += new_value.length()) {
//		if ((pos = str.find(old_value, pos)) != std::string::npos)
//			str.replace(pos, old_value.length(), new_value);
//		else break;
//	}
//	return str;
//}
std::string& replace_all(std::string& str, const std::string& old_value, const std::string& new_value)
{
	while (true) {
		std::string::size_type pos(0);
		if ((pos = str.find(old_value)) != std::string::npos)
			str.replace(pos, old_value.length(), new_value);
		else break;
	}
	return str;
}
std::string getFileName(std::string &filename, std::string &pathName)
{
	//std::string tmp = filename.substr(0, filename.find_last_of("_", filename.size()) + 9) + ".csv";
	//����pathName��filename���
	std::string tmp;
	if (filename.find("/") != std::string::npos)
	{
		//��ȡ���һ��"_"���ų��ֵ�λ��ǰ����ǰ��9λ���ֺ�csv���������ļ���
		if(filename.find(manufacture[0])!=std::string::npos)
			tmp = filename.substr(0, filename.find_last_of("_", filename.size()-10) + 9) + ".csv";
		else
			tmp = filename.substr(0, filename.find_last_of("_", filename.size()) + 9) + ".csv";
		//ȡ���һ��"//"����ַ���
		tmp = tmp.substr(tmp.find_last_of("//", tmp.size()), tmp.size());
	}
	else
	{
		tmp = filename.substr(0, filename.find_last_of("_", filename.size()) + 9) + ".csv";
	}
	return pathName + tmp;
}
// ��ȡXML ZIP GZ�ȸ�ʽ�ĵ���MRO�ļ���
int readMROData(std::string &filename,std::string &outputpath)
{
	int iNum = 0;  //ͳ�ƴ��������
	//�ȶ��ļ�
	char* zchar="";
	std::string out;
	readZData(filename, out);
	const int len = out.length();
	if (len == 0 && filename.find(".zip") != std::string::npos)
		iNum += BigZipLoad(filename.c_str());
	if (len == 0)
		return iNum;
	//std::string ��char*ת��
	zchar = new char[len + 1];
	strcpy(zchar, out.c_str());

	iNum += readXMLchar(zchar, filename,outputpath);
	return iNum;
	//return 0;
}
int readXMLchar(char * zchar, std::string &filename,std::string &outputpath)
{
	int iNum = 0;
	//file<> fdoc(filename.c_str());
	//doc.parse<0>(fdoc.data());    // 0 means default parse flags
	xml_document<> doc;    // character type defaults to char
						   //����xml����
	doc.parse<0>(zchar);
	int writeFlag = 0;  //�ļ������м�¼
	std::string outfileName = getFileName(filename, outputpath);
	if (isFileExist(outfileName.c_str()))
		writeFlag++;
	//���outfile
	std::ofstream outfile(outfileName.c_str(), std::ios::app | std::ios::out);

	//��ȡ����
	int mfacture = getManufacture(filename);
	//�������
	std::string fileFormatVersion;
	std::string startTime;
	std::string endTime;
	std::string CI;
	std::string MMEUES1APID;
	std::string MmeGroupId;
	std::string MmeCode;
	std::string TimeStamp;

	//��ȡ���ڵ� bulkPmMrDataFile
	xml_node<>* root = doc.first_node();

	//��ȡ���ڵ��һ���ڵ� fileHeader
	xml_node<>* node1 = root->first_node();

	//��ȡfileHeader������ ��ʼʱ��ͽ���ʱ��
	xml_attribute<char> * attr = node1->first_attribute();
	fileFormatVersion = node1->first_attribute()->value();
	if (mfacture == HUAWEI)
	{   //��Ϊ���������ҵĿ�ʼ����ʱ�䲻ͬ
		//startTime = node1->first_attribute()->next_attribute()->next_attribute()->next_attribute()->next_attribute()->value();
		startTime = node1->first_attribute("startTime")->value();
		endTime = node1->first_attribute("endTime")->value();
	}
	else
	{
		startTime = node1->first_attribute("startTime")->value();
		endTime = node1->first_attribute("endTime")->value();
	}

	//��ȡ���ڵ����һ���ڵ� eNB
	xml_node<>* node2 = root->last_node();

	//����ÿ��measurement ����ֻȡ����RSRP�Ľڵ�
	for (rapidxml::xml_node<char> * node = node2->first_node("measurement");
	node != NULL; node = node->next_sibling("measurement"))
	{
		//����ÿ��<smr><object>
		//��measurement��smr����RSRPʱչ����һ��Object�ڵ�
		for (rapidxml::xml_node<char> * nodeSMR = node->first_node();
		nodeSMR != NULL; nodeSMR = nodeSMR->next_sibling())
		{
			//��ȡsmr��object�ֶα������ÿһ��SMR��OBJECT
			//�����Ҳ��ǻ�Ϊ�ͱ���ʱ��SMR��VALUEֵ����RIP��QCIʱ����
			std::string tmpHead = nodeSMR->value();
			if (mfacture != HUAWEI && mfacture != ALCATEL && (mfacture != ZTE && fileFormatVersion=="V1.0.0")
				&&(tmpHead.find("MR.LteScRIP") != std::string::npos
					|| tmpHead.find("MR.LteScPlrULQci") != std::string::npos))
				continue;

			if (strcmp(nodeSMR->name(), "object") == 0
				|| 
				(tmpHead.find("MR.LteScRSRP", 0) != std::string::npos && strcmp(nodeSMR->name(),"smr") == 0))
			{
				if (mfacture == ZTE && fileFormatVersion == "V1.0.0") replaceFirstWS(tmpHead);
				replace_all(tmpHead, " ", ",");
				tmpHead = "StartTime,EndTime,CI,MMEUES1APID,MmeGroupId,MmeCode,TimeStamp," + tmpHead;
				if (writeFlag == 0)
				{
					outfile << tmpHead << std::endl;
					writeFlag++;
				}
				//��ȡ���ڵ����һ���ڵ���ĵ�һ���ڵ���ĵ�2���ڵ� object �������⣩
				xml_node<>* nodeObject = nodeSMR->next_sibling();
				
				if (nodeObject!= NULL && strcmp(nodeObject->name(), "object") == 0)
				{
				    //object 
					//xml_attribute<>* attr1 = nodeObject->first_attribute("id");
					CI = nodeObject->first_attribute("id")->value();
					CI = covertCI(CI, mfacture);
					if(CI == "") break;
					MMEUES1APID = nodeObject->first_attribute("MmeUeS1apId")->value();
					MmeGroupId = nodeObject->first_attribute("MmeGroupId")->value();
					MmeCode = nodeObject->first_attribute("MmeCode")->value();
					TimeStamp = nodeObject->first_attribute("TimeStamp")->value();
					
					//v
					for (rapidxml::xml_node<char> * nodeV = nodeObject->first_node("v");
					nodeV != NULL; nodeV = nodeV->next_sibling("v"))
					{
						if (MMEUES1APID == "NIL") break; //��MMEUES1APIDΪNIL����Ϊ��������Ч����Ҫ���� ---RIP��������
						std::string nv = nodeV->value();
						if (nv.length() <= 73) break;  //QCI�������Ϊ73���ַ�
						replaceFirstWS(nv);
						replaceLastWS(nv);
						if (mfacture == ZTE && fileFormatVersion == "V1.0.0")  
							//���˵�1.0.0�汾���ݱȽ����⣬��������ո���SMRͷ����һ���ո�
							replace_all(nv, "  ", " ");
						replace_all(nv, " ", ",");
						replace_all(nv, "NIL", "");

						std::string outstr;
						startTime.replace(10, 1, " ");
						if (mfacture == ALCATEL) startTime = startTime.substr(0, 23);  //��+08:00ɾ��
						outstr += startTime;
						outstr.push_back(',');
						endTime.replace(10, 1, " ");
						if (mfacture == ALCATEL) endTime = endTime.substr(0, 23);
						outstr += endTime;
						outstr.push_back(',');
						outstr += CI;
						outstr.push_back(',');
						outstr += MMEUES1APID;
						outstr.push_back(',');
						outstr += MmeGroupId;
						outstr.push_back(',');
						outstr += MmeCode;
						outstr.push_back(',');
						TimeStamp.replace(10, 1, " ");
						if (mfacture == ALCATEL) TimeStamp = TimeStamp.substr(0, 23);
						outstr += TimeStamp;
						outstr.push_back(',');
						outstr += nv;
						//if (mfacture == 0) replace_all(outstr, "+08:00", "");

						//replace_all(outstr, "T", " ");
						//replace_all(outstr, "NIL", "");
						outfile << outstr << std::endl;
						iNum++;
					}
				}
			}
		}
	}
	outfile.close();
	doc.clear();
	delete[] zchar;
	zchar = NULL;
	return iNum;
}
//int dir(std::string &path)
//{
//	long hFile = 0;
//	struct _finddata_t fileInfo;
//	std::string pathName, exdName;
//	int iNum = 0; //�ļ�������
//	int i = 0;
//	if ((hFile = _findfirst(pathName.assign(path).append("\\*.gz").c_str(), &fileInfo)) == -1) {
//		std::cout << "�Ҳ���XML�ļ�..." << std::endl << "��ȷ�������ļ��е���д��filepath��value·���Ƿ����ļ�" << std::endl;
//		return 0;
//	}
//	
//	do {
//		//std::cout << fileInfo.name  << (fileInfo.attrib&_A_SUBDIR ? "[folder]" : "[file]") << std::endl;
//		if (fileInfo.attrib ==32)  //������ļ� 
//		{
//			std::string fname = path + fileInfo.name;
//			char *p = strstr(fileInfo.name, "MRO");
//			if (p != NULL)
//			{
//				int num =readMROData(fname,outpath);
//				iNum += num;
//				std::cout << fileInfo.name << "\t"<< ++i <<"���ļ��ѽ���"<< std::endl;
//				
//			}
//		}
//		else if (fileInfo.attrib == 16)
//		{
//			//������ļ���
//			std::string strname = fileInfo.name;
//			return dir(strname);
//		}
//	} while (_findnext(hFile, &fileInfo) == 0);
//	_findclose(hFile);
//
//	//if (!i) std::cout << "�Ҳ���MRO�ļ�" << std::endl;
//	return iNum;
//}
void replaceFirstWS(std::string &nvstr)
{   //��ͷΪ�ո��ɾ��
	if (nvstr.find(" ",0)==0) 
		nvstr.replace(0, 1, "");  
}
void replaceLastWS(std::string &nvstr)
{    //����Ϊ�ո��ɾ��
	if (nvstr.find(" ", nvstr.size() - 1) == nvstr.size() - 1)
		nvstr.replace(nvstr.size() - 1, nvstr.size(), "");  
}

void readConfig(const char* cfilename,std::string & filepath,std::string & outputPath)
{
	
	file<> fdoc(cfilename);
	xml_document<>  doc;
	doc.parse<0>(fdoc.data());
	
	//std::cout << fdoc.data() << std::endl;
	//! ��ȡ���ڵ�  
	xml_node<>* root = doc.first_node();
	//! ��ȡ���ڵ��һ���ڵ�  
	xml_node<>* node1 = root->first_node();

	filepath = node1->first_attribute()->next_attribute()->value();
	//wchar_t *tmp = (wchar_t *)"D:\mro\fzzteindoor\test\���";
	//Wchar_tToString(filepath,tmp);
	formatPath(filepath);
	xml_node<>* node2 = root->last_node();
	outputPath = node2->first_attribute()->next_attribute()->value();
	
	formatPath(outputPath);
	//�������Ŀ¼
	if (_access(outputPath.c_str(), 6) == -1)
		_mkdir(outputPath.c_str());

	doc.clear();
}
void formatPath(std::string &pathname)
{
	//��֤·��������
	if (pathname == "")
		pathname += ".\\";
	if (pathname.back() != '\\')
		pathname += "\\";
}
void writeConfig(std::string inputPath,std::string outputPath)
{
	//std::string strPath = "C:\\";
	if (inputPath == "")
	{
		std::cout << "���޸������ļ��е�filepath��valueֵΪ�������ļ�����·������" << std::endl;;
	}
	if (outputPath == "")
	{
		std::cout << "���Ŀ¼Ϊ��,Ĭ�Ͻ�����ļ��ŵ���ǰĿ¼��outputĿ¼��" << std::endl;;
	}
	//if (!isFileExist(configFileName))
	//{
		xml_document<> doc;
		xml_node<> *node1 = doc.allocate_node(node_declaration);
		doc.append_node(node1);
		node1->append_attribute(doc.allocate_attribute("version", "1.0"));
		node1->append_attribute(doc.allocate_attribute("encoding", "utf-8"));
		xml_node<> *node2 = doc.allocate_node(node_element, "Configuration", "1");
		doc.append_node(node2);
		xml_node<> *nodeadd1 = doc.allocate_node(node_element, "add");
		node2->append_node(nodeadd1);

		xml_attribute<> *attr = doc.allocate_attribute("key", "filepath");
		nodeadd1->append_attribute(attr);
		//���������ļ�����
		nodeadd1->append_attribute(doc.allocate_attribute("value", inputPath.c_str()));

		xml_node<> *nodeadd2 = doc.allocate_node(node_element, "add");
		node2->append_node(nodeadd2);
		xml_attribute<> *attradd2 = doc.allocate_attribute("key", "outPutPath");
		nodeadd2->append_attribute(attradd2);
		//���������ļ�����
		nodeadd2->append_attribute(doc.allocate_attribute("value", outputPath.c_str()));

		//ֱ�����  
		std::cout << "print:readXml.config\n" << doc << std::endl;

		//�ñ��浽string  
		//std::string strxml;
		//print(std::back_inserter(strxml), doc, 0);
		std::ofstream out(configFileName);
		out << doc;
		doc.clear();
		//std::cout << "print:strxml" << strxml << std::endl;
	//}	
	
}
int isFileExist(const char* filename)
{
	std::fstream _file;
	_file.open(filename, std::ios::in);
	if (!_file)
	{
		//std::cout << filename << "û�б�����";
		return 0;
	}
	else
	{
		//std::cout << filename << "�Ѿ�����";
		return 1;
	}
	//return 0;
}

bool gzipLoad(const char* gzfn, ::std::string &out)
{
	//open .gz file
	gzFile gzfp = gzopen(gzfn, "rb");
	if (!gzfp)
	{
		return false;
	}

	//read and add it to out
	unsigned char buf[GZ_BUF_SIZE];
	int have;
	while ((have = gzread(gzfp, buf, GZ_BUF_SIZE)) > 0)
	{
		out.append((const char*)buf, have);
	}
	//delete[] buf;
	
	//close .gz file
	gzclose(gzfp);
	return true;
}

bool zipload(const char* zipfn, std::string & out)
{
	int nReturnValue;

	//��zip�ļ�
	unzFile unzfile = unzOpen(zipfn);
	//std::string ziplongFn = getFilepath(zipfn);
	if (unzfile == NULL)
	{
		cout << "��zip�ļ�ʧ��" << endl;
		return false;
	}

	//��ȡzip�ļ�����Ϣ
	unz_global_info* pGlobalInfo = new unz_global_info;
	nReturnValue = unzGetGlobalInfo(unzfile, pGlobalInfo);
	if (nReturnValue != UNZ_OK)
	{
		cout << "��ȡ�ļ���Ϣʧ��" << endl;
		return false;
	}
	if (pGlobalInfo->number_entry > 1)
	{
		cout << "It's a big zip contrains many zips" << endl;
		return false;
	}
		
	//����zip�ļ�
	unz_file_info* pFileInfo = new unz_file_info;
	char szZipFName[MAX_PATH];                            //��Ŵ�zip�н����������ڲ��ļ���
	for (unsigned int i = 0; i<pGlobalInfo->number_entry; i++)
	{
		//�����õ�zip�е��ļ���Ϣ
		nReturnValue = unzGetCurrentFileInfo(unzfile, pFileInfo, szZipFName, MAX_PATH,
			NULL, 0, NULL, 0);
		if (nReturnValue != UNZ_OK)
		{
			cout << "�����ļ�ʧ��" << endl;
			return false;
		}

		//�ж����ļ��л����ļ�
		switch (pFileInfo->external_fa)
		{
		case FILE_ATTRIBUTE_DIRECTORY:                    //�ļ���
		{
			//std::string strDiskPath = strTempPath + _T("//") + szZipFName;
			//CreateDirectory(strDiskPath, NULL);
		}
		break;
		default:                                        //�ļ�
		{
			//�����ļ�
			//std::string strDiskFile = strTempPath +"//" + szZipFName;
			//���ļ�
			nReturnValue = unzOpenCurrentFile(unzfile);
			if (nReturnValue != UNZ_OK)
			{
				std::cout << "���ļ�ʧ��!" << std::endl;
				//CloseHandle(hFile);
				return false;
			}

			//��ȡ�ļ�
			const int BUFFER_SIZE = 4096;
			char szReadBuffer[BUFFER_SIZE];
			while (TRUE)
			{
				memset(szReadBuffer, 0, BUFFER_SIZE);
				int nReadFileSize = unzReadCurrentFile(unzfile, szReadBuffer, BUFFER_SIZE);
				if (nReadFileSize < 0)                //��ȡ�ļ�ʧ��
				{
					std::cout << "��ȡ�ļ�ʧ��!" << std::endl;
					unzCloseCurrentFile(unzfile);
					return false;
				}
				else if (nReadFileSize == 0)            //��ȡ�ļ����
				{
					unzCloseCurrentFile(unzfile);
					break;
				}
				else                                //д���ȡ������
				{
					out.append((const char*)szReadBuffer, nReadFileSize);
				}
			}
			//delete[] szReadBuffer;
			
		}
		break;
		}
		unzGoToNextFile(unzfile);
	}

	//�ر�
	if (unzfile)
	{
		unzClose(unzfile);
	}
	return true;
}

//std::string getBigZipFilepath(const char * orgFName)
//{
//	std::string outFPstr = orgFName;
//	outFPstr = outFPstr.substr(0, outFPstr.find_last_of(".")) + "//";
//	return outFPstr;
//}
void readZData(std::string &filename, std::string &out)
{	
	//std::string out;
	if (filename.find(".gz") != std::string::npos)
	{
		if (gzipLoad(filename.c_str(), out))
		{
			std::cout << "gzload Success" << std::endl;
		}
		else
		{
			std::cout << "gzload error " << std::endl;
			return ;
		}
	}
	else if (filename.find(".zip") != std::string::npos)
	{
		if (zipload(filename.c_str(), out))
		{
			std::cout << "zipload Success" << std::endl;
		}
		else {
			std::cout << "zipload error " << std::endl;
			return ;
		}
	}
	else if (filename.find(".xml", 0) != std::string::npos)
	{
		file<> fdoc(filename.c_str());
		out = fdoc.data();
	}
	//return out;
}


int BigZipLoad(const char* zipfn)
{
	CString scstr = zipfn;
	CString tcstr = outpath.c_str();

	int iNum = Unzip(scstr, tcstr);
	return iNum;
}
	/*
	int nReturnValue;
	int iNum=0;
	//��zip�ļ�
	int ret = 0;
	unzFile unzfile = unzOpen(zipfn);
	std::string zipPathName = getBigZipFilepath(zipfn);
	if (unzfile == NULL)
	{
		cout << "��zip�ļ�ʧ��" << endl;
		return -1;
	}

	//��ȡzip�ļ�����Ϣ
	unz_global_info* pGlobalInfo = new unz_global_info;
	nReturnValue = unzGetGlobalInfo(unzfile, pGlobalInfo);
	if (nReturnValue != UNZ_OK)
	{
		cout << "��ȡ�ļ���Ϣʧ��" << endl;
		return -2;
	}

	//����zip�ļ�
	unz_file_info* pFileInfo = new unz_file_info;
	char szZipFName[MAX_PATH];                            //��Ŵ�zip�н����������ڲ��ļ���
	//char* filename = "TD-LTE_MRO_ZTE_OMC1_21434_20161124174500.zip";
	if (UNZ_OK == unzLocateFile(unzfile, filename, 0))
	{
		cout << "1b" << endl;
	}
	for (unsigned int i = 0; i<pGlobalInfo->number_entry; i++)
	{
		//�����õ�zip�е��ļ���Ϣ
		nReturnValue = unzGetCurrentFileInfo(unzfile, pFileInfo, szZipFName, MAX_PATH,
			NULL, 0, NULL, 0);
		if (nReturnValue != UNZ_OK)
		{
			cout << "�����ļ�ʧ��" << endl;
			return -3;
		}
		
		//�ж����ļ��л����ļ�
		switch (pFileInfo->external_fa)
		{
			case FILE_ATTRIBUTE_DIRECTORY:                    //�ļ���
			{
				//std::string strDiskPath = strTempPath + _T("//") + szZipFName;
				//CreateDirectory(strDiskPath, NULL);
			}
			break;
			default:                                        //�ļ�
			{
				//�����ļ�
				//std::string strDiskFile = strTempPath +"//" + szZipFName;
				
				nReturnValue = unzOpenCurrentFile(unzfile);
				
				if (nReturnValue != UNZ_OK) 
				{
					std::cout << "���ļ�ʧ��!" << std::endl;
					//CloseHandle(hFile);
					return false;
				}
				
				if (strstr(szZipFName, ".zip") != NULL)
				{
					//��ȡ�ļ�
					
					const int BUFFER_SIZE = 4096;
					char szReadBuffer[BUFFER_SIZE];
					if (_access(zipPathName.c_str(), 6) == -1)
						_mkdir(zipPathName.c_str());

					FILE* fp4 = NULL;
					fp4 = fopen((zipPathName + szZipFName).c_str(), "wb");
					while (TRUE)
					{
						memset(szReadBuffer, 0, BUFFER_SIZE);
						int nReadFileSize = unzReadCurrentFile(unzfile, szReadBuffer, BUFFER_SIZE);
						if (nReadFileSize < 0)                //��ȡ�ļ�ʧ��
						{
							std::cout << "��ȡ�ļ�ʧ��!" << std::endl;
							unzCloseCurrentFile(unzfile);
							return false;
						}
						else if (nReadFileSize == 0)            //��ȡ�ļ����
						{
							unzCloseCurrentFile(unzfile);
							break;
						}
						else                                //д���ȡ������
						{
							fwrite(szReadBuffer, 1, nReadFileSize, fp4);
						}
					}
					fclose(fp4);
					//ѭ������ ��ȡ��һ�ν�ѹ���ZIP�ļ� ���еڶ��ν�ѹ��ȡ
					std::string out;
					
					readZData(zipPathName + szZipFName, out);
					std::cout << i+1 << "/" << pGlobalInfo->number_entry << endl;
					std::cout << zipPathName + szZipFName << std::endl;
					char* zchar = "";
					const int len = out.length();
					if (len == 0)
						return 0;
					zchar = new char[len + 1];
					strcpy(zchar, out.c_str());
					iNum += readXMLchar(zchar, zipPathName + szZipFName,outpath);
				}
			}
			break;
		}
		unzGoToNextFile(unzfile);
	}

	//�ر�
	if (unzfile)
	{
		unzClose(unzfile);
	}
	return iNum;
	*/
	

int zipiNum = 0;
int ExtractAllTo(XUnzip& unzip, CString targetFolder)
{
	int count = unzip.GetFileCount();
	//int iNUm = 0;
	for (int i = 0; i < count; i++)
	{
		const XUnzipFileInfo* fileInfo = unzip.GetFileInfo(i);
		CString fileName = fileInfo->fileName;
		CString filePathName = AddPath(targetFolder, fileName, L"\\");
		CString fileFolder = ::GetParentPath(filePathName);
		::DigPath(fileFolder);

		//XFileWriteStream outFile;
		XBuffer szReadBuffer;
		
		USES_CONVERSION;
		LPCWSTR P = A2CW((LPCSTR)filePathName);
		/*if (outFile.Open(P) == FALSE)
		{
			wprintf(L"ERROR: Can't open %s\n", filePathName.GetString());
			return FALSE;
		}*/
		//wprintf(L"Extracting: %s..", filePathName.GetString());

		if (unzip.ExtractTo(i, szReadBuffer))
		{
			XUnzip Bufferzip;
			if (filePathName.Right(3) == "zip")
			{
				//wprintf(L"unzip in zip done\n");
				Bufferzip.Open(szReadBuffer.data, szReadBuffer.dataSize);
				ExtractAllTo(Bufferzip, targetFolder);
				std::cout << i + 1 << "/" << count << std::endl;
			}
			else
			{
				//Byte*��char*ת��
				char *zchar = new char[szReadBuffer.dataSize+1];
				memset(zchar, 0, szReadBuffer.dataSize+1);
				memcpy(zchar, szReadBuffer.data, szReadBuffer.dataSize);
				//CString -> std::stringת��
				std::string fileNamestr = fileName.GetBuffer(0);
				std::string tagPathStr = targetFolder.GetBuffer(0);

				zipiNum +=readXMLchar(zchar, fileNamestr, tagPathStr);
				cout << fileName <<" analyze is done"<< endl;
			}
		}
		else
		{
			wprintf(L"Error\n");
			return 0;
		}
	}
	return zipiNum;
}

int Unzip(CString zipPathName, CString targetFolder)
{
	XUnzip  unzip;
	USES_CONVERSION;
	LPCWSTR P = A2CW((LPCSTR)zipPathName);
	if (unzip.Open(P) == FALSE)
	{
		wprintf(L"ERROR: Can't open %s\n", zipPathName.GetString());
		return FALSE;
	}

	//INT64 tick = ::GetTickCount64();
	int ret = ExtractAllTo(unzip, targetFolder);
	//INT64 elapsed = GetTickCount64() - tick;
	//if (ret)
	//	wprintf(L"Elapsed time : %dms\n", (int)elapsed);
	return ret;
}

