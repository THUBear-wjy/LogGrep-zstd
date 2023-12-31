#include "Encoder.h"
#include "Coffer.h"
#include <vector>
#include <cmath>
#include <cstring>
#include <algorithm>
#include "constant.h"
#include "union.h"
#include <zstd.h>
using namespace std;
Encoder::Encoder(string cp_mode, string zip_mode, int compression_level){
    data.clear();
    _meta_out = (zip_mode == "O") ? true : false;
    _cp_mode = cp_mode;
    cp_level = compression_level;
}

bool sortCoffer(Coffer* co1, Coffer* co2){
    return co1 -> type < co2 -> type;
}

string Encoder::compress(){
    string meta = "";
    sort(data.begin(), data.end(), sortCoffer);
    int nowOffset = 0;
    for(auto &temp: data){
        if(temp -> srcLen == 0){
            meta += temp -> filenames + " 0 " +  to_string(nowOffset) + " 0 0 0 " + to_string(temp->eleLen) + "\n";
            continue;
        }
        int destLen = temp -> compress(_cp_mode, cp_level);
        meta += temp ->filenames + " " + to_string(temp -> compressed) + " " +  to_string(nowOffset) + " " + to_string(destLen) + " " + to_string(temp -> srcLen) + " " + to_string(temp -> lines) + " " + to_string(temp -> eleLen) + "\n";
        
        nowOffset += destLen;
    }
   return meta;
}

string Encoder::padding(string filename, int Idx, int maxIdx){
    int totLen = 1 + log10(maxIdx + 1);
    int nowLen = 1 + log10(Idx + 1);
    if (totLen < nowLen){
        printf("%s Int Padding Error: totLen: %d, nowLen:%d!\n", filename.c_str(), totLen, nowLen);
        return to_string(Idx);
    }
    int padSize = totLen - nowLen;
    if(padSize == 0) return to_string(Idx);
    char* PADS = new char[padSize];
    for(int i = 0; i < padSize; i++) PADS[i] = ' ';
    PADS[padSize] = '\0';
    return string(PADS) + to_string(Idx);
}

void Encoder::padding(string filename, char* buffer, int startPos, int padSize, int typ){
    char PAD = ' ';
    for(int i = 0; i < padSize; i++){
        buffer[startPos + i] = PAD;
    }
}

string Encoder::padding(string filename, string target, int maxLen, int typ){
    char PAD = ' ';
    int t_size = target.size();
    if(maxLen < t_size){
        printf("%s String Padding Error: maxLen: %d, target size: %d\n", filename.c_str(), maxLen, t_size);
        cout << target << endl;
        return target;
    }
    int padSize = maxLen - target.size();
    if(padSize == 0) return target;
    char* PADS = new char[padSize + 5];
    for(int i = 0; i < padSize; i ++) PADS[i] = PAD;
    PADS[padSize] = '\0';
    return string(PADS) + target;
}

void Encoder::serializeTemplate(string zip_path, LengthParser* parser){
    char* longStr = NULL;
    int ll = parser -> getTemplate(&longStr);
    if(_meta_out){
        FILE* test = fopen((zip_path + ".templates").c_str(), "w");
        fprintf(test, "%s", longStr);
        fclose(test);
    }
    int srcL = strlen(longStr);
    Coffer* nCoffer = new Coffer(to_string(TYPE_TEMPLATE << POS_TYPE), longStr, srcL, ll, 0, -1);
    data.push_back(nCoffer);
}

void Encoder::serializeTemplateOutlier(char** failed_log, int failLine){
    string longStr = "";
    int count = 0;
    for(int i = 0; i < failLine; i++, count++){
        string temp(failed_log[i]);
        if(longStr.size() + temp.size() > MAXCOMPRESS){
            Coffer* nCoffer = new Coffer(to_string(TYPE_OUTLIER << POS_TYPE), longStr, longStr.size(), count, 1, -1);
            data.push_back(nCoffer);
            longStr = "";
            count = 0;
        }
        longStr += temp + "\n";
        
    }
    Coffer* nCoffer = new Coffer(to_string(TYPE_OUTLIER << POS_TYPE), longStr, longStr.size(), count, 1, -1);
    data.push_back(nCoffer);
}

void Encoder::serializeVar(string filename, char* globuf, VarArray* var, int maxLen){
    int length = var ->nowPos;
    int tot = length * (maxLen+1);
    char* temp = new char[tot + 5];
    int nowPtr = 0;
    for(int i=0; i< length; i++)
    {
        int varLen = var->len[i];
        int padSize = maxLen - varLen;
        if (padSize < 0) SysWarning("Error pad size < 0\n");
        padding(filename, temp, nowPtr, padSize, 0);
        nowPtr += padSize;
        strncpy(temp + nowPtr, globuf + var->startPos[i], varLen);
        nowPtr += varLen;
    }

    Coffer* nCoffer = new Coffer(filename, temp, nowPtr, length, 5, maxLen);
    data.push_back(nCoffer);
}

void Encoder::serializeEntry(string filename, int* entry, int maxEntry, int total){
   string longStr = "";
   int paddingSize = 1 + log10(maxEntry);
   for(int i = 0; i < total; i++){
       longStr += padding(filename, to_string(entry[i]), paddingSize, 0);
   }
   Coffer* nCoffer = new Coffer(filename, longStr, longStr.size(), total, 4, paddingSize);
   data.push_back(nCoffer);
}

bool pairCmp (pair<string, int>&t1, pair<string, int>& t2){
    return (t1.second < t2.second);
}

void Encoder::serializeDic(string filename, char* globuf, VarArray* varMapping, Union* root){
    vector<pair<unsigned int, int> >* container = root -> getContainer();
    int count = 0;
    int paddingSize = root -> nowPaddingSize[0];
    int patIdx = 0;
    int nowPtr = 0;
    int bufferSize = (root -> dicMax >= 10000) ? MAXBUFFER : MAX_VALUE_LEN * root ->dicMax;
    char* temp = new char[bufferSize];
    for(vector<pair<unsigned int, int> >::iterator it = container -> begin(); it != container -> end(); it++,count++){

        if(count == root -> nowCounter[patIdx]){
            count = 0;
            patIdx++;
            paddingSize = root -> nowPaddingSize[patIdx]; 
        }
        int nowPos = root ->posDictionary[it ->first];
        int nowLen = varMapping->len[nowPos];
        padding(filename, temp, nowPtr, paddingSize - nowLen, 0);
        nowPtr += paddingSize - nowLen;
        strncpy(temp + nowPtr, globuf + varMapping->startPos[nowPos], nowLen);
        nowPtr += varMapping->len[nowPos];
    } 
    Coffer* nCoffer = new Coffer(filename, temp, nowPtr, root -> dicMax, 3, -2);
    data.push_back(nCoffer);
}

void Encoder::serializeSvar(string filename, SubPattern* subPattern){
    bool debug = false;
    if(debug) cout << subPattern -> type << endl;
    
    if(subPattern -> type == 1){  //fint, fstr
        string longStr = "";
        for(int i = 0; i < subPattern -> data_count; i++){
            string tempStr = subPattern -> data[i];
            if(tempStr.size() == 0) tempStr = padding(filename, subPattern ->data[i], subPattern -> length, subPattern -> type);
            longStr += tempStr;
        }
        Coffer* nCoffer = new Coffer(filename, longStr, longStr.size(), subPattern -> data_count, 6, subPattern -> length);
        data.push_back(nCoffer);
    }
    if(subPattern -> type == 2){ //int, str
        string longStr = "";
        for(int i = 0; i < subPattern -> data_count; i++){
            string tempStr = padding(filename, subPattern -> data[i], subPattern -> maxLen, subPattern -> type);
            int ssize = subPattern ->data[i].size();
            if(ssize  < subPattern ->maxLen && debug) cout << subPattern -> data[i] << " " << tempStr << endl;
            longStr += tempStr;
        } 
        int paddingSize = subPattern -> maxLen;
        if(debug) cout << longStr << endl;
        Coffer* nCoffer = new Coffer(filename, longStr, longStr.size(), subPattern -> data_count, 6, paddingSize);
        data.push_back(nCoffer);
    }
}

void Encoder::serializeOutlier(string filename, vector<pair<int, string> > outliers){
    string longStr = "";
    for(auto &temp: outliers){
        longStr += to_string(temp.first) + " " + temp.second + "\n";
    }
    Coffer* nCoffer = new Coffer(filename, longStr, longStr.size(), outliers.size(), 7, -3);
    data.push_back(nCoffer);
}

void Encoder::serializeSubpattern(string zip_path, string SUBPATTERN, int SUBCOUNT){
    if(_meta_out){
        FILE* test = fopen((zip_path + ".variables").c_str(), "w");
        fprintf(test, "%s", SUBPATTERN.c_str());
        fclose(test);
    }
    Coffer* nCoffer = new Coffer(to_string(TYPE_VARIABLELIST << POS_TYPE), SUBPATTERN, SUBPATTERN.size(), SUBCOUNT, 2, -1);
    data.push_back(nCoffer);
}

void Encoder::output(string zip_path, int typ){
    if(typ == 1){
       for(auto &temp: data){
           temp -> printFile(zip_path);
       }
       return;
    }
    FILE* zipFile = fopen(zip_path.c_str(), "w");
    if (zipFile == NULL){
        cout << "open zip file failed" << endl;
        return;
    }

    string meta = compress();//Build meta
    
    if(_meta_out){
        FILE* test = fopen((zip_path + ".meta").c_str(),"w");
        fprintf(test, "%s", meta.c_str());
        fclose(test);
    }

	size_t com_space_size = ZSTD_compressBound(meta.size());
	Byte* pZstd = new Byte[com_space_size];
	size_t srcLen = meta.size();
	size_t destLen = ZSTD_compress(pZstd, com_space_size, meta.c_str(), srcLen, cp_level);
	fwrite(&destLen, sizeof(size_t), 1, zipFile);
	fwrite(&srcLen, sizeof(size_t), 1, zipFile);
	fwrite(pZstd, sizeof(Byte), destLen, zipFile);
	delete [] pZstd;

    //Output templates
    printf("cp_mode:%s\n", _cp_mode.c_str());
    for(auto &temp: data){
        if(temp -> lines == 0) continue;
        temp -> output(zipFile, typ);
    }
    fclose(zipFile);
}


