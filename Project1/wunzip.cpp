#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
using namespace std;

char buffer[100];

//uncompress file
void unzip(int fileDescriptor)
{
	int cap = 100;
	char* data = (char*)malloc(cap);
	int len = 0;
	
	//read file to buffer
	char buffer[100];
	while(true){
		int byteReads = read(fileDescriptor, buffer, sizeof(buffer));
		if(byteReads == 0) break;
		
		if(len + byteReads > cap){
			char* data2 = (char*)malloc(cap * 2);
			memcpy(data2, data, len);
			free(data);
			data = data2;
			cap *= 2;
		}	
		memcpy(data + len, buffer, byteReads);
		len += byteReads;
	}
	
	//unzip, 5 bytes as a group
	for(int i = 0; i < len / 5; i++){
		int count = *(int*)(data + 5 * i);
		char ch = data[5 * i + 4];
		
		while(count > 0){
			int l = min(count, (int)sizeof(buffer));
			for(int j = 0; j < l; j++){
				buffer[j] = ch;
			}
			write(STDOUT_FILENO, buffer, l);
			count -= l;
		}
	}
	free(data);
}

int main(int argc, char* argv[])
{
	if(argc < 2){
		cout << "wunzip: file1 [file2 ...]" << endl;
		exit(1);
	}
	
	for(int i = 1; i < argc; i++){
		int fileDescriptor = open(argv[i], O_RDONLY);
		if(fileDescriptor < 0){
			cout << "wunzip: cannot open file" << endl;
			exit(1);
		}
		unzip(fileDescriptor);
		close(fileDescriptor);
	}
	return 0;
}