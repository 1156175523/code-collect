#ifndef __CODE_CONVERTER_H__
#define __CODE_CONVERTER_H__
/***********************************
功能：
实现字符串编码转换类
目前只支持utf-8,gbk编码的检查和转换
平台：
WINDOW/Linux
************************************/

#include <cstdio>
#include <cstdlib>
#include <cstring>
#ifdef _WIN32
#include <windows.h>
#else
#include <iconv.h>
#endif
#include <string>

#define CODE_MAX_DATA_LEN (512)

typedef enum {
    CODE_GBK,
    CODE_UTF8,
    CODE_UNKOWN
} CODING;

class CodeConverter{
public:
    int preNUm(unsigned char byte) {
        unsigned char mask = 0x80;
        int num = 0;
        for (int i = 0; i < 8; i++) {
            if ((byte & mask) == mask) {
                mask = mask >> 1;
                num++;
            } else {
                break;
            }
        }
        return num;
    }

    bool isUtf8(const char* pData, int len) {
        int num = 0;
        int i = 0;
        unsigned char* data = (unsigned char*)pData;
        while (i < len) {
            if ((data[i] & 0x80) == 0x00) {
                // 0XXX_XXXX
                i++;
                continue;
            }
            else if ((num = preNUm(data[i])) > 2) {
                // 110X_XXXX 10XX_XXXX
                // 1110_XXXX 10XX_XXXX 10XX_XXXX
                // 1111_0XXX 10XX_XXXX 10XX_XXXX 10XX_XXXX
                // 1111_10XX 10XX_XXXX 10XX_XXXX 10XX_XXXX 10XX_XXXX
                // 1111_110X 10XX_XXXX 10XX_XXXX 10XX_XXXX 10XX_XXXX 10XX_XXXX
                // preNUm() 返回首个字节8个bits中首�?0bit前面1bit的个数，该数量也是该字符所使用的字节数        
                i++;
                 for(int j = 0; j < num - 1; j++) {
                   //判断后面num - 1 个字节是不是都是10开
                    if ((data[i] & 0xc0) != 0x80) {
                        return false;
                    }
                    i++;
                }
            } else {
                //其他情况说明不是utf-8
                return false;
            }
        } 
        return true;
    }

    bool isGBK(const char* pData, int len)  {
        int i  = 0;
        unsigned char* data = (unsigned char*)pData;
        while (i < len)  {
            if (data[i] <= 0x7f) {
                //编码小于等于127,只有一个字节的编码，兼容ASCII
                i++;
                continue;
            } else {
                //大于127的使用双字节编码
                if  (data[i] >= 0x81 &&
                    data[i] <= 0xfe &&
                    data[i + 1] >= 0x40 &&
                    data[i + 1] <= 0xfe &&
                    data[i + 1] != 0xf7) {
                    i += 2;
                    continue;
                } else {
                    return false;
                }
            }
        }
        return true;
    }

    //需要说明的是，isGBK()是通过双字节是否落在gbk的编码范围内实现的，
    //而utf-8编码格式的每个字节都是落在gbk的编码范围内�?
    //所以只有先调用isUtf8()先判断不是utf-8编码，再调用isGBK()才有意义
    //获取编码格式
    CODING GetCoding(std::string srcStr){
        CODING coding = CODE_UNKOWN;
        if (srcStr.empty()){
            return coding;
        }

        if (isUtf8(srcStr.c_str(), (int)(srcStr.size())) == true) {
            coding = CODE_UTF8;
        } else if (isGBK(srcStr.c_str(), (int)(srcStr.size())) == true) {
            coding = CODE_GBK;
        }
        return coding;
    }

    //gbk转utf-8
    std::string ConvertToUtf8(std::string srcStr){
        std::string retStr = "";
        if (srcStr.empty()){
            return retStr;
        }

        char dstutf8[CODE_MAX_DATA_LEN];
        memset(dstutf8, 0x00, sizeof(dstutf8));

        int minLen = srcStr.size() > CODE_MAX_DATA_LEN ? CODE_MAX_DATA_LEN : srcStr.size();
        char* pSrcTmp = (char*)malloc(sizeof(char)*(minLen+1));
        if (pSrcTmp == NULL){
            return retStr;
        }

        char* pSrc = pSrcTmp;
        size_t srcLen = (size_t)(minLen);
        memset(pSrc,0x00, minLen+1);
        strncpy(pSrc, srcStr.c_str(), minLen);

#ifdef _WIN32
        wchar_t * pUnicodeBuff = NULL;
        int rlen = 0;
        rlen = MultiByteToWideChar(936, 0, pSrc, -1, NULL, NULL);
        pUnicodeBuff =  new WCHAR[rlen + 1];  //为Unicode字符串空间
        rlen = MultiByteToWideChar(936, 0, pSrc, -1, pUnicodeBuff, rlen); 
        rlen = WideCharToMultiByte(CP_UTF8, 0, pUnicodeBuff, -1, NULL, NULL, NULL, NULL);
        WideCharToMultiByte(CP_UTF8, 0, pUnicodeBuff, -1, dstutf8, rlen, NULL, NULL);
        delete[] pUnicodeBuff;
#else
        iconv_t  cd;
        size_t ret = 0;
        char* pUTFOUT = dstutf8;
        size_t outLenUTF = sizeof(dstutf8);
        cd = iconv_open("UTF-8", "GBK");
        if (cd == (iconv_t)-1) {
            //printf("iconv_open error\n");
            free(pSrcTmp);
            pSrcTmp = NULL;
            return retStr;
        }
        ret = iconv(cd, &pSrc, &srcLen, &pUTFOUT, &outLenUTF);
        iconv_close(cd);
#endif

        if (pSrcTmp){
            free(pSrcTmp);
            pSrcTmp = NULL;
        }
        retStr = dstutf8;
        return retStr;
    }

    //utf-8转gbk
    std::string ConvertToGbk(std::string srcStr){
        std::string retStr = "";
        if (srcStr.empty()){
            return retStr;
        }

        char dstgbk[CODE_MAX_DATA_LEN];
        memset(dstgbk, 0x00, sizeof(dstgbk));

        int minLen = srcStr.size() > CODE_MAX_DATA_LEN ? CODE_MAX_DATA_LEN : srcStr.size();
        char* pSrcTmp = (char*)malloc(sizeof(char)*(minLen+1));
        if (pSrcTmp == NULL){
            return retStr;
        }

        char* pSrc = pSrcTmp;
        size_t srcLen = (size_t)(minLen);
        memset(pSrc,0x00, minLen+1);
        strncpy(pSrc, srcStr.c_str(), minLen);

#ifdef _WIN32
        wchar_t * pUnicodeBuff = NULL;
        int rlen = 0;
        rlen = MultiByteToWideChar(CP_UTF8, 0, pSrc, -1, NULL ,NULL);
        pUnicodeBuff =  new WCHAR[rlen + 1];  //为Unicode字符串空间
        rlen = MultiByteToWideChar(CP_UTF8, 0, pSrc, -1, pUnicodeBuff, rlen); 
        rlen = WideCharToMultiByte(936, 0, pUnicodeBuff, -1, NULL, NULL, NULL, NULL); //936 为windows gb2312代码页码
        WideCharToMultiByte(936, 0, pUnicodeBuff ,-1, dstgbk, rlen, NULL ,NULL);
        delete[] pUnicodeBuff;
#else
        iconv_t  cd;
        size_t ret = 0;
        char* pGBKOUT = dstgbk;
        size_t outLenGBK = sizeof(dstgbk);

        cd = iconv_open("GBK", "UTF-8");
        if (cd == (iconv_t)-1) {
            //printf("iconv_open error\n");
            free(pSrcTmp);
            pSrcTmp = NULL;
            return retStr;
        }
        ret = iconv(cd, (char**)&pSrc, &srcLen, &pGBKOUT, &outLenGBK);
        iconv_close(cd);
#endif

        if (pSrcTmp){
            free(pSrcTmp);
            pSrcTmp = NULL;
        }
        retStr = dstgbk;
        return retStr;
    }
};

#endif