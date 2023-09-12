#include <iostream>
#include <fstream>
#include <unistd.h>
#include <string>
#include <set>
#include <ctime>
#include <string.h>
#include <algorithm>
#include <regex>
#include <stdio.h>
#include <cstdio>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <map>
#include <cmath>
#include <vector>
#include <zstd.h>
#include "../constant.h"
#include "../util.h"
#include "../Coffer.h"
#include "../LZMA/LzmaLib.h"
using namespace std;


int main(int argc, char *argv[]){
//TODO:
//1. Fix parser bugs
//2. Integrate compression methods
//3. Batch small files

	// clock_t start = clock();
	int o;
	const char *optstring = "HhI:O:T:";
	srand(4);
	//Input Content
	string input_path; string output_path; string type;
//Input A.log -> A.zip
	while ((o = getopt(argc, argv, optstring)) != -1){ //input_path -> .zip file, output_path -> dir
		switch (o)
		{
		case 'I':
			input_path = optarg;
			//strcpy(input,optarg);
			printf("input file path: %s\n", input_path.c_str()); //clove++ 20200919: path
			break;
		case 'O':
			output_path = optarg;
			printf("output path : %s\n", output_path.c_str());
			break;
        case 'h':
		case 'H':
			printf("-I input path\n");	//clove## 20200919:
			printf("-O output path\n");
			return 0;
			break;
		case '?':
			printf("error:wrong opt!\n");
			printf("error optopt: %c\n", optopt);
			printf("error opterr: %c\n", opterr);
			return 1;
		}
	}
	
	//Basic input check
	if (input_path == ""){
		printf("error : No input file\n");
		return -1;
	}
	
	if (output_path == ""){
		printf("error : No output\n");
		return -1;
	}
    
    FILE* zipFile = fopen(input_path.c_str(), "rb");
    if(zipFile == NULL){
        printf("uncompression read file error!\n");
        return -1;
    }
    
    //Uncompression meta file
    size_t destLen;
    size_t srcLen;

    fread(&destLen, sizeof(size_t), 1, zipFile);
    fread(&srcLen, sizeof(size_t), 1, zipFile);
    cout << "DestLen: " << destLen << " SrcLen: " << srcLen << endl;
    unsigned char* pLzma = new unsigned char[destLen + 5];
    fread(pLzma, sizeof(char), destLen, zipFile);
    size_t decom_buf_size = ZSTD_getFrameContentSize(pLzma, destLen);
    char* meta = new char[decom_buf_size];
    size_t res = ZSTD_decompress(meta, decom_buf_size, pLzma, destLen);
    if(res != srcLen){
        printf("Meta uncompress failed with %lu \n", res);
        return -1;
    }
    delete [] pLzma;
    int fstart = sizeof(size_t) + sizeof(size_t) + destLen; //real file start
    
    vector<Coffer*> coffer_buffer;
    map<string, Coffer*> coffer_map;
    char* meta_buffer = new char[128];
    int meta_end = 0;
    int metaLen = strlen(meta);
    for(int i = 0; i < metaLen; i++){
        if(meta[i] == '\n'){ //New coffer
            meta_buffer[meta_end] = '\0';
            Coffer* newCoffer = new Coffer(string(meta_buffer));
            cout << "Build new coffer: " << newCoffer -> print() << endl;
            coffer_buffer.push_back(newCoffer);
            coffer_map[newCoffer -> filenames] = newCoffer;
            meta_end = 0;
            continue;
        }
        meta_buffer[meta_end++] = meta[i];
    }
    
    for(map<string, Coffer*>::iterator iit = coffer_map.begin(); iit != coffer_map.end(); iit++){
       cout << "Now decompress: " << iit -> first << endl;
       Coffer* nowCoffer = iit -> second;
       int res = nowCoffer -> readFile(zipFile, fstart);
       if(res < 0){
           cout << "read file failed" << endl;
           continue;
       }
       res = nowCoffer -> decompress();
       //char * query = nowCoferr -> data;
       if(res < 0){
           cout << "decompress failed" << endl;
           continue;
       }
       nowCoffer -> printFile(output_path);
       //cout << "decompress: " << iit -> first << " successed" << endl; 
    }
    //cout << string(meta) << endl;
    
    //size_t outputLen = 0;
    //size_t inputLen = 0;
    //cout << s << endl;
    
}
