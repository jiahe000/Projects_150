#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
using namespace std;

int cap = 100;
char* data = (char*)malloc(cap);
int len = 0;

//read file to buffer
void load(int fileDescriptor)
{
	char buffer[100];
	while(true){
		int byteReads = read(fileDescriptor, buffer, sizeof(buffer));
		if(byteReads == 0) break;
		
		//make sure cap is large enough
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
}

//compress file to stdout
void zip()
{
	int prev = 256; //current character
	int count = 0; //count number

	for(int i = 0; i < len; i++){
		if(data[i] == prev){ //if same, count increases by 1
			count++;
		}
		else{ //for new ch
			if(prev < 256){ //output zip
				write(STDOUT_FILENO, &count, 4);
				write(STDOUT_FILENO, &prev, 1);
			}
			prev = data[i];
			count = 1;
		}
	}
	//last ch
	write(STDOUT_FILENO, &count, 4);
	write(STDOUT_FILENO, &prev, 1);	

	free(data);
}

int main(int argc, char* argv[])
{
	if(argc < 2){
		cout << "wzip: file1 [file2 ...]" << endl;
		exit(1);
	}
	
	for(int i = 1; i < argc; i++){
		int fileDescriptor = open(argv[i], O_RDONLY);
		if(fileDescriptor < 0) {
			free(data);
			cout << "wzip: cannot open file" << endl;
			exit(1);
		}
		load(fileDescriptor);
		close(fileDescriptor);
	}
	zip();
	return 0;
}