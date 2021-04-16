#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
using namespace std;

//open file, output: stdout
int main(int argc, char* argv[])
{
  if(argc < 2){
    return 0;
  }

  for(int i = 1; i < argc; i++) {
    char buffer[100];
  	int fileDescriptor = open(argv[i], O_RDONLY);
  	if(fileDescriptor < 0){
    	cout << "wcat: cannot open file" << endl;
    	exit(1);
  	}

  	while(true){
    	int bytesRead = read(fileDescriptor, buffer, sizeof(buffer));
    	if(bytesRead == 0){
    		break;
    	}
    	write(STDOUT_FILENO, buffer, bytesRead);
  	}
  	close(fileDescriptor);
  }
  return 0;
}