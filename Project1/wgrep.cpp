#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
using namespace std;

//check if buff contains str
bool find(char* buff, char* str)
{
  int str_len = strlen(str);
  for(int i = 0; buff[i]; i++){
    if(!strncmp(buff + i, str, str_len)){
      return true;
    }
  }
  return false;
}

void grep(int fileDescriptor, char* str)
{
  int cap = 100;
  char* data = (char*)malloc(cap);
  int len = 0;

  //read file to data buffer
  char buffer[100];
  while(true){
    int bytesRead = read(fileDescriptor, buffer, sizeof(buffer));
    if(bytesRead == 0) break;

    //if cap is not large enough, malloc a larger one
    if(len + bytesRead > cap){
      char* data2 = (char*)malloc(cap * 2);
      memcpy(data2, data, len);
      free(data);
      data = data2;
      cap *= 2;
    }

    memcpy(data + len, buffer, bytesRead);
    len += bytesRead;
  }

  //check line by line
  int prev = 0; //start as prev
  for(int i = 0; i < len; i++){
    if(data[i] == '\n') { //end of the line
      data[i] = 0;
      if(find(data + prev, str)){ //check
        write(STDOUT_FILENO, data + prev, i - prev);
        write(STDOUT_FILENO, "\n", 1);
      }
      prev = i + 1; //next start position
    }
  }
  //for the last line
  if(find(data + prev, str)){
    write(STDOUT_FILENO, data + prev, len - prev);
    write(STDOUT_FILENO, "\n", 1);
  }
  free(data);
}

int main(int argc, char* argv[])
{
  if(argc < 2){
    cout << "wgrep: searchterm [file ...]" << endl;
    exit(1);
  }

  if(argc == 2){
    grep(STDIN_FILENO, argv[1]);
  }

  else{
    for(int i = 2; i < argc; i++){
      int fileDescriptor = open(argv[i], O_RDONLY);
      if(fileDescriptor < 0) {
        cout << "wgrep: cannot open file" << endl;
        exit(1);
      }
      grep(fileDescriptor, argv[1]);

      close(fileDescriptor);
    }
  }
  return 0;
}