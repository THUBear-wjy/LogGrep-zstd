#include "Coffer.h"
#include<cstring>
#include<cstdio>
#include<cstdlib>
#include<zstd.h>
using namespace std;
Coffer::Coffer(string filename, char* srcData, int srcL, int line, int typ, int ele){
    data = srcData;
    filenames = filename;
    cdata = NULL;
    srcLen = srcL;
    lines = line;
    type = typ;
    eleLen = ele;
    compressed = 0;
}

Coffer::Coffer(string filename, string srcData, int srcL, int line, int typ, int ele){
//cout << "Build coffer" << filename << " " << line << endl;
    data = new char[srcL + 5];
    memcpy(data, srcData.c_str(), sizeof(char)*srcL);
    data[srcL] = '\0';
    filenames = filename;
    cdata = NULL;
    srcLen = srcL;
    lines = line;
    eleLen = ele;
    type = typ;
    compressed = 0;
}

Coffer::Coffer(string metaStr){
    data = NULL;
    cdata = NULL;
    type = -1;
   // cout << "Build based: " << metaStr << endl;
    char filename[128];
    int _compressed, _offset, _destLen, _srcLen, _lines, _eleLen;
    sscanf(metaStr.c_str(), "%s %d %d %d %d %d %d", filename, &_compressed, &_offset, &_destLen, &_srcLen, &_lines, &_eleLen);
    //cout << filename << endl;
    //cout << _compressed << endl;
    //cout << _compressed << _offset << _destLen << _srcLen << _lines << _eleLen << endl;
    filenames = filename;
    compressed = _compressed;
    offset = _offset;
    destLen = _destLen;
    srcLen = _srcLen;
    lines = _lines;
    eleLen = _eleLen;
}

Coffer::~Coffer()
{
    if(data){
        delete[] data;
        data = NULL;
    }
    if(cdata && strlen((char*)cdata) >6){
        delete[] cdata;
        cdata = NULL;
    }
}

int Coffer::compress(string compression_method, int compression_level){
    size_t com_space_size = ZSTD_compressBound(srcLen);
    cdata = new Byte[com_space_size];
    destLen = ZSTD_compress(cdata, com_space_size, data, srcLen, compression_level);
    compressed = 1;
    return destLen;
}

int Coffer::readFile(FILE* zipFile, int fstart){
    int totOffset = fstart + this->offset;
    fseek(zipFile, totOffset, SEEK_SET);
    cdata = new unsigned char[destLen + 5];
    return fread(cdata, sizeof(Byte), destLen, zipFile);
}


int Coffer::decompress(){
    if(compressed == 0){
        if(srcLen == 0) return 0;
        data = new char[srcLen + 5];
        memcpy(data ,cdata , sizeof(char)*srcLen);
        return srcLen;
    }
    size_t decom_buf_size = ZSTD_getFrameContentSize(cdata, destLen);
    data = new char[decom_buf_size];
    int res = ZSTD_decompress(data, decom_buf_size, cdata, destLen);
    if(res != srcLen){
        printf("varName: %s decompress failed with %d\n", filenames.c_str(), res);
        return -1;
    }
    return srcLen;
}

void Coffer::output(FILE* zipFile, int typ){
    if(cdata == NULL || zipFile == NULL){
        cout << "coffer: " + filenames + " output failed" << endl;
        return;
    }
    if(compressed){
        fwrite(cdata, sizeof(Byte), destLen, zipFile);
    }else{
        fwrite(data, sizeof(Byte), srcLen, zipFile);
    }
}

string Coffer::print(){
    string name = filenames;
    return name;
}

void Coffer::printFile(string output_path){
    FILE* pFile = fopen((output_path + filenames).c_str(), "w");
    fwrite(data, sizeof(char), srcLen, pFile);
    fclose(pFile);
}

