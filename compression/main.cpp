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
#include "template.h"
#include "constant.h"
#include <map>
#include <cmath>
#include <vector>

#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#include "LengthParser.h"
#include "util.h"
#include "sampler.h"
#include "union.h"
#include "SubPattern.h"
#include "Encoder.h"
#include "timer.h"
#include "Coffer.h"


using namespace std;

const char* delim = " \t:=,";

//return length of file data, -1 or 0: open file failed 
int LoadFileToMem(const char *varname, char **mbuf)
{
	int fd = open(varname,O_RDONLY);
	int len =0;
	if(fd != -1)
	{
		len = lseek(fd,0,SEEK_END);
		if(len > 0) 
		{
			*mbuf = (char *)mmap(NULL,len,PROT_READ,MAP_PRIVATE,fd,0);
		}
	}
	close(fd);
	return len;
}

//need to cache hot data?
int LoadcVars(const char *varname, char *vars)
{
	char *mbuf =NULL;
	int len = LoadFileToMem(varname, &mbuf);
	if(len <=0)
	{
		return 0;
	}
	int index=0;int offset=0;
	for (char *p= mbuf;*p && p-mbuf< len; p++)  
	{
        if (*p == '\n')
		{
			vars[index * MAX_VALUE_LEN + offset] = '\0';
			index++;
			offset = 0;
		}
        else
		{
			vars[index * MAX_VALUE_LEN + offset++] = *p; 
		}
	}
	return index;
}

//clove rewrite 20220415,  use mmap
int matchFile(string input_path, LengthParser* parser, string zip_mode, int * Eid, int* failed_num, char** failed_log, map<int, VarArray*>& variables, int& nowLine)
{
    char *mbuf =NULL;
	int len = LoadFileToMem(input_path.c_str(), &mbuf);
	if(len <=0)
	{
		return 0;
	}
//    char buffer[MAX_VALUE_LEN]={'\0'};//如果MAX_VALUE_LEN值太大，还是需要在堆上申请
    SegTag segArray[MAXTOCKEN*100];
    int segSize=0;
    int segStart=0; int lineStart=0;
    int failLine =0;
    for (int i=0; i<len; i++)
	{
        if (mbuf[i] == '\n')
		{
            //判断最后是否还有字符串的遗留，也需要处理
            if(i > segStart)
            {
                segArray[segSize].tag = 0;
                segArray[segSize].startPos = segStart;
                segArray[segSize].segLen=i-segStart;
                segSize++;
            }
            //执行处理程序
            int eid = parser -> SearchTemplate(mbuf, segArray, segSize, variables, true);
			Eid[nowLine] = eid;
            if(eid == -1)
            {
                // lineLen = lineEnd-lineStart;
				failed_num[failLine] = nowLine;
                failed_log[failLine] = (char*)malloc(sizeof(char) * (i-lineStart+1));
                strncpy(failed_log[failLine], mbuf + lineStart, i-lineStart);
                failed_log[failLine][i-lineStart] = '\0';
			    //SysDebug("Failed log: %s\n", failed_log[failLine]);
                failLine++;
			}
            nowLine++;
            segSize=0;
            segStart =i+1;
            lineStart=i+1;
		}
        else if(strchr(delim,mbuf[i]))//记录分割符号
        {
            if(i > segStart)//delim前有字符串
            {
                segArray[segSize].tag = 0;
                segArray[segSize].startPos = segStart;
                segArray[segSize].segLen=i-segStart;
                segSize++;

            }
            //记录分隔符本身
            segArray[segSize].tag = mbuf[i];
            segArray[segSize].segLen=1;
            segArray[segSize].startPos = i;
            segStart =i+1;
            segSize++;
        }
	}
	//SysDebug("Failed line: %d\n", failLine);
    if(zip_mode != "Z") cout << "Failed rate: " << (double)failLine / nowLine << endl;
    //free(buffer);
	return failLine;
}


bool outputVar(string fileName, vector<string>* temp){
	FILE* fo = fopen(fileName.c_str(), "w");
	if (fo == NULL){
		cout << "Open output log file: " << fileName + " failed" << endl;
		return false;
	}
	for(vector<string>::iterator iit = temp ->begin(); iit != temp ->end(); iit++){
		fprintf(fo, "%s\n", (*iit).c_str());
	}
	fclose(fo);
	return true;
}

bool outputEntry(string fileName, int* entry, int maxIdx, int size){
    FILE* fo = fopen(fileName.c_str(), "w");
    if(fo == NULL){
       cout << "Open output log file: " << fileName + " failed" << endl;
       return false;
    }
    int totLen = 1 + log10(maxIdx);
    for(int i = 0; i < size; i ++){
        string nowStr = "";
        int nowEntry = entry[i];
        int nowLen = 1 + log10(nowEntry);
//cout << nowLen << " " << totLen << endl;
        while(nowLen < totLen){
            nowStr += "0";
            nowLen += 1;
        }
//cout << nowStr + to_string(nowEntry) << endl;
        fprintf(fo, "%s%d\n", nowStr.c_str(), nowEntry);
    }
    fclose(fo);
    return true;
}

/*
主处理函数入口
./Input A.log -> A.zip

*/
void proc(string input_path, string output_path, string cp_mode, string zip_mode, int compression_level, double threashold){
    
    timeval stime_s = ___StatTime_Start();    
	
    bool dict = true;
    bool sub = true;
    
    char* mbuf = NULL;
    int len = LoadFileToMem(input_path.c_str(), &mbuf);
    if (len <= 0){
        SysWarning("Read file failed!\n");
        return;
    }
    SegTag segArray[MAXTOCKEN * 100];
    int segSize = 0;
    int segStart = 0, lineStart = 0;

    int sampleRange = 100;

    int nowLine = 0, nowSample = 0;
    bool sampled = false;
    LengthParser parser(threashold);

    for (int i = 0; i < len; i++){
        if(segSize == MAXTOCKEN*100 - 1){
            SysWarning("segSize out of bound\n");
        }
        if(mbuf[i] == '\n'){
            if(sampled && i > segStart){
                segArray[segSize].tag = 0;
                segArray[segSize].startPos = segStart;
                segArray[segSize].segLen = i - segStart;
                segSize++;
            }
            if(sampled){
                nowSample++;
                parser.parseTemplate(mbuf, segArray, segSize);
                //execute parsing process
            }
            //SysDebug("Remaining: %d\n", remaining);
            if(rand() % sampleRange == 1){
                sampled = true;
            }else{
                sampled = false;
            }
            nowLine++;
            segSize = 0;
            segStart = i + 1;
            lineStart = i + 1;
            if(i - lineStart > MAX_VALUE_LEN){
                SysWarning("[WARNING] line length out of bound: %d\n", i - lineStart);
            }
        }else if(sampled && strchr(delim, mbuf[i])){
            if (i > segStart){
                segArray[segSize].tag = 0;
                segArray[segSize].startPos = segStart;
                segArray[segSize].segLen = i-segStart;
                segSize++;
            }
            segArray[segSize].tag = mbuf[i];
            segArray[segSize].segLen = 1;
            segArray[segSize].startPos = i;
            segStart = i + 1;
            segSize++;
        }
    }
    if(zip_mode != "Z") printf("Tot Size: %d, Sampler Size: %d\n", nowLine, nowSample);

    int expendTime = sampleRange * 4;
    
    //build varaible_mapping;
    map<int, VarArray*> variable_mapping;  
    for(auto &pool:parser.LengthTemplatePool){
        vector<templateNode*>* nowPool = pool.second;
        for(auto &temp: *nowPool){
            for(int i = 0; i < temp->varLength; i++){
                int nowTag = ((temp->Eid)<<POS_TEMPLATE) | (i<<POS_VAR);
                variable_mapping[nowTag] = new VarArray(nowTag, parser.STC[temp->Eid] * expendTime);
            }
        }
    }
    

    if(zip_mode != "Z") parser.TemplatePrint();
    if(zip_mode != "Z") cout << "start match" << endl;
	//Streaming processing(match, match again, extract varibales and output)
	double stime = ___StatTime_End(stime_s);

    timeval mtime_s = ___StatTime_Start();
    int * Eid = new int[nowLine];
    

    
    int failed_num[MAXLOG * 2];
	char* failed_log[MAXLOG * 2];
	
    int nowline = 0;
    int failLine = matchFile(input_path, &parser, zip_mode, Eid, failed_num, failed_log, variable_mapping, nowline);
    

    double mtime = ___StatTime_End(mtime_s);
///*    
    Encoder* encoder = new Encoder(cp_mode, zip_mode, compression_level);
    encoder -> serializeTemplate(output_path, &parser);
    encoder -> serializeTemplateOutlier(failed_log, failLine);

    //Here variable strings get, need to split, and 
	//For each variable, build union -> split -> build sub-pattern
    string SUBPATTERN = "";
    int SUBCOUNT = 0;
    
    timeval vtime_s = ___StatTime_Start();
    if(zip_mode != "Z") cout << "start variable process" << endl;
    char tempArray[MAX_VALUE_LEN];
	for(map<int, VarArray*>::iterator it = variable_mapping.begin(); it != variable_mapping.end(); it++){
        

		bool debug = false;
        //if(it ->first == ((12<<POS_TEMPLATE) + (1<<POS_VAR))) debug = true;//E12_V1s
        VarArray * temp = it -> second;
        int varTag = it ->first;
        int templateNum = varTag >> POS_TEMPLATE;
        int variableNum = (varTag >> POS_VAR) & 0xff;

		if ((!sub && !dict) || temp ->nowPos < 100){
            //TODO(var)		outputVar(fileName, it ->second);
            int maxLen = 0;
            int varType = 0;
            for(int tempIndex=0; tempIndex< temp ->nowPos; tempIndex++)
            {
                int varLen = temp ->len[tempIndex];
                maxLen = max(maxLen, varLen);
                varType |= getType(mbuf + temp->startPos[tempIndex], varLen);                
            }
            SUBPATTERN += to_string(varTag)  + " V ";
            SUBPATTERN += to_string(varType) + " ";
            SUBPATTERN += to_string(maxLen);
            SUBPATTERN += "\n";
            SUBCOUNT++;
            string fileName = to_string((varTag | (TYPE_VAR << POS_TYPE)));
            encoder -> serializeVar(fileName, mbuf, temp, maxLen);
			continue;
		}
       
		Union** tree = new Union* [MAXUNION];
		int nowStart = 0, nowEnd = 0;
		Union* root = new Union(mbuf, temp, 0.0001); //Sample to create the first Union
		//Get UniqueRate over sample
        if(root ->dictionary.size() < root -> _tot * root ->uniqueRate && dict){ //Build dictionary
			//Extract global dictionary
			int* entry = new int[temp ->nowPos];
			int idx = 0;
			//Extract dictionary from original variables
			root -> buildMapping(temp);
            for(int i=0; i< temp->nowPos; i++)
            {
                
				entry[idx++] = root -> dictionary[root ->HashValue[i]];
			}
			encoder -> serializeEntry(to_string(varTag | (TYPE_ENTRY << POS_TYPE)), entry, root -> dicMax, idx);
            encoder -> serializeDic(to_string(varTag | (TYPE_DIC << POS_TYPE)), mbuf,temp, root);    
		    SUBPATTERN += to_string(varTag) + " D " + to_string(root -> patCount) + " ";
            for(int i = 0; i < root -> patCount; i++){
                SUBPATTERN += root -> nowFormat[i] + " " + to_string(root -> nowPaddingSize[i]) + " " + to_string(root -> nowCounter[i]) + " ";
            }
            SUBPATTERN += "\n";
            SUBCOUNT++;
		
		}else if (sub){ //Build sub-pattern
			vector<int> container;

            tree[nowEnd++] = root;
			while(nowStart < nowEnd){
				    //tree[nowStart] -> output();
				tree[nowStart] -> execute(tree, nowStart, nowEnd);
			    //	cout << "Now End: " << nowEnd << endl;
			    nowStart++;
		    }

			    //Sorted union according to their num
			int nowNum = 0;
		    while(true){
			    bool find = false;
				for(int count = 0; count < nowEnd; count++){
				    if(tree[count] ->num == nowNum){
					    find = true;
					    nowNum++;
						if(tree[count] ->type >= 0){
						    container.push_back(count);
					    }
				    }
			    }      	
			    if(!find) break;
		    }

			    //std::sort(tree.begin(), tree.end(), Union::unionCmp);
			
			    //Clean up

			//Build sub-pattern
			vector <SubPattern*> subPatternPool; //Push all subPattern

            // if(debug){
			//     cout << "Union state: " << endl;
			//     for(vector<int>:: iterator itc = container.begin(); itc != container.end(); itc++){
			// 	    int i = *itc;
			// 	    cout << "#" << tree[i] -> num << " " << tree[i] -> type <<  endl;
			//     }
            // }

			for(vector<int>:: iterator itc = container.begin(); itc != container.end(); itc++){
				int i = *itc;
				//if(debug) cout << "Now process: " << tree[i] -> num << endl;
				if (tree[i] -> type < 0) break;
                int idx = 1;
                int t = (itc + idx == container.end()) ? nowEnd: *(itc+1); 
				//string nextConstant = (t == nowEnd) ? "": tree[t] -> constant;
                Union * next = (t == nowEnd) ? NULL : tree[t];
		        string nextConstant = ""; 
                while(next && next -> type == 0){    
                    nextConstant += next -> constant;
                    idx++;
                    t = (itc + idx == container.end()) ? nowEnd: *(itc+idx); 
                    next = (t == nowEnd) ? NULL : tree[t];
                }
                
                //if(debug) cout << nextConstant << endl;    	
                SubPattern* pat = new SubPattern(tree[i], nextConstant, temp ->nowPos);
				subPatternPool.push_back(pat);
			}
            


            //Finish Extract
			
            if(debug){
                for(vector<SubPattern*>::iterator pit = subPatternPool.begin(); pit != subPatternPool.end(); pit++){
				    (*pit) -> output();
			    }
            }
			
            
			set<int> outlier_idx;
			vector<pair<int, string> > outlier;
            for(int i=0; i< temp->nowPos; i++)
			{
				int strIdx = 0; //The reading point
				bool jump = false;
                int varLen = temp->len[i];
                strncpy(tempArray,mbuf + temp->startPos[i], varLen);
                tempArray[varLen] = '\0';
				string nowStr = tempArray;
                //if(debug) cout << "count: " << count << endl;
				bool success = true;
                for(vector<SubPattern*>::iterator pit = subPatternPool.begin(); pit != subPatternPool.end(); pit++){
					if(!(*pit) ->extract(nowStr, strIdx, jump)){
						//if(debug) cout << "count: " << count << " outlier: " << nowStr << " strIdx: " << strIdx << endl;
                        outlier.push_back(make_pair(i, nowStr));
						outlier_idx.insert(i);
						success = false;
                        break;
					}
					if(debug) cout << " success " << nowStr << " strIdx: " << strIdx  << "varLen" << varLen << endl;
				}
                if(success && strIdx != varLen) {
                    //cout << "here: " << nowStr << endl;
                    outlier.push_back(make_pair(i, nowStr));
					outlier_idx.insert(i);
                    success = false;
                }
                for(vector<SubPattern*>::iterator pit = subPatternPool.begin(); pit != subPatternPool.end(); pit++){ 
                    (*pit) -> add(success); 
           //         if(debug && (*pit) -> type != 0) cout << "success: " << success << "now add: " << (*pit) ->data[(*pit) -> data_count - 1] << endl;
                }
			}

            string subPattern = "";

            for(vector<SubPattern*>::iterator pit = subPatternPool.begin(); pit != subPatternPool.end(); pit++){
                subPattern += (*pit) -> getPattern();
            }
           // if(debug) cout << subPattern << endl;
            //Output subvaribales
            int varCount = 0;
			
            for(vector<SubPattern*>:: iterator pit = subPatternPool.begin(); pit != subPatternPool.end(); pit++){
				if ((*pit) ->length == 0 || (*pit) -> type == 0) continue;
               // if(debug) (*pit) -> output_var(output_path + ".E1_V2");
                //if(debug) cout << (*pit) -> data_count << endl;
                string file_path = to_string(varTag | ((varCount++) << POS_SUBVAR) | (TYPE_SVAR << POS_TYPE));
                encoder -> serializeSvar(file_path, *pit); 
                //TODO(subvar) bool res = (*pit) ->output_var(now_path);
			}
//			cout << " outlier rate: " << outlier_idx.size() / (double)temp ->size() << endl;
			
            encoder -> serializeOutlier(to_string(varTag | (TYPE_OUTLIER << POS_TYPE)), outlier);
			//for(vector<pair<int, string> >::iterator itOut = outlier.begin(); itOut != outlier.end(); itOut++){
			    //TODO(outlier)
                //	fprintf(fow, "%d %s\n", itOut ->first, (itOut ->second).c_str());
			//}
            SUBPATTERN += to_string(varTag) + " S " + subPattern + "\n";
            SUBCOUNT++;
            //fprintf(fw, "%s S %s\n", (it ->first).c_str(), subPattern.c_str());


        }
//		cout << "$$$$$$$$$$$$$Now end process: " << it -> first << endl;
	}
    double vtime = ___StatTime_End(vtime_s);
    
    timeval ctime_s = ___StatTime_Start();
    if(zip_mode != "Z") cout << "start compression" << endl;
    encoder -> serializeSubpattern(output_path, SUBPATTERN, SUBCOUNT);	
    
    //int output_type = (zip_mode == "O") ? 1: 0;
    printf("start output\n");
    encoder -> output(output_path, 0);
    double ctime = ___StatTime_End(ctime_s);
	
    free(Eid);
    for(auto &temp: variable_mapping){
        free(temp.second);
    }

    if(zip_mode != "Z") printf("stime: %lfs, mtime: %lfs, vtime:%lfs, ctime:%lfs\n", stime, mtime, vtime, ctime);
//*/
}

// ./THULR -I /apsarapangu/disk9/LogSeg/Android/0.log -O /apsarapangu/disk9/PillBox_test/Android.zip
int main(int argc, char *argv[]){
//TODO:
//1. Fix parser bugs
//2. Integrate compression methods
//3. Batch small files

	// clock_t start = clock();
	int o;
	const char *optstring = "HhI:O:C:Z:L:";
	srand(4);
	//Input Content
	string input_path; string output_path;
    string cp_mode, zip_mode;
    int compression_level = 1;
    //Input A.log -> A.zip
	while ((o = getopt(argc, argv, optstring)) != -1)
	{
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
        case 'C':
            cp_mode = optarg;
            printf("Compression Mode(Lzma, Zstd): %s\n", cp_mode.c_str());
            break;
        case 'Z':
            zip_mode = optarg;
            printf("ZipMode(Z, O): %s\n", zip_mode.c_str());
            break;
        case 'L':
            compression_level = atoi(optarg);
            printf("Compression level: %d\n", compression_level);
            break;
        case 'h':
		case 'H':
			printf("-I input path\n");
			printf("-O output path\n");
			printf("-T template path\n");
			printf("-C compression methods(Zstd or Lzma)\n");
			printf("-L compression_level\n");
			printf("-Z compression_mode\n");
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

    if(cp_mode == ""){
        cp_mode = "Zstd";
    }
    if(zip_mode == ""){
        zip_mode = "O"; 
    }
    double threashold = 0.5;
    
    proc(input_path, output_path, cp_mode, zip_mode, compression_level, threashold); 
} 
