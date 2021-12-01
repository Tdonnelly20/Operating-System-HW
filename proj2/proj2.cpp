#include <iostream>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/resource.h>
using namespace std;
int searchForChar(int len, int off, char* argv, char c){
  int counter=0;
  for(int k=off; k<len; k++){
    //  cout<<argv[i];
    if(argv[k]==c){
      counter++;
    }
  }
  //printf("\n");
  return counter;
}
void results(char* filename, int len, char* farray, char* mapped, char c, int farrayCount, int mappedCount, bool map, bool parallel){
  if(map){
    printf("Testing mmap() now\n");
    if(!parallel){
      mappedCount=searchForChar(len, 0, mapped, c);
    }
    printf("Mapped found %d '%c's\n", mappedCount, c);
    if(farrayCount==mappedCount){
      printf("Testing completed, matching farray and mapped was successful\n");
      printf("There are %d '%c's in %s\n", mappedCount, c, filename);
    }else{
      printf("Testing completed, matching farray and mapped was unsuccessful\n\n\n");
    }
  }else{
    printf("There are %d '%c's in %s\n", farrayCount, c, filename);
  }
}
int mmapLoop(char* filename, int fd, int len, int processNum, char c){
  int f;
  int mappedCount=0;
  //int len;
  if((f=open(filename, O_RDONLY))<0){
    perror("Could not open file\n");
    exit(1);
  }
  int off=0;
  char* mapped;
  mapped= (char*)mmap(NULL, len, PROT_READ, MAP_SHARED, f, off);
  if(mapped == (char*)-1){
    perror("Could not mmap file");
    exit(1);
  }

  int status=0;
  long div=((len)/processNum)+len%processNum;
  int countPerP=0;
  long temp=div;
  if(len>8192){
    perror("File is too large");
    exit(1);
  }
  printf("File size: %d\n", len);
  for(int i=0; i<processNum;i++){
    mappedCount+=countPerP;
    countPerP=0;
    //printf("Total Count: %d\n\n", mappedCount);
    int pid=fork();
    if(pid<0){
      printf("Fork error");
      exit(1);
    }else if(pid==0){//Child
      /*printf("#%d Div %ld\n", i,div);
      printf("#%d Off %d\n", i,off);
      printf("#%d Temp %ld\n", i,temp);*/
      if(i==0){
        off=0;
      }else{
        off+=div;
      }
      if((((len%processNum))>0) && (i==0 || i==processNum)){
        //cout<<"First child has "<<(len%processNum)<<" extra byte(s)"<<endl;
        div=(len/processNum)+((len%processNum));
        temp=off+div;
      }else{
        div=(len/processNum);
        temp=off+div;
      }
      if(temp>len){
        temp=len;
      }
      countPerP=searchForChar(temp, off, mapped, c);
      printf("Parallel Process #%d has %d '%c's\n", i, countPerP, c);
      /*cout<<"Temporary Map #"<<i<<": ";
      for(int j=off; j<temp; j++){
        cout<<mapped[j];
      }
      printf("\n");*/

      exit(0);
    }else{//Parent
      if(waitpid(pid, &status, 0)==0){
        mapped=(char*)mmap(NULL, len,  PROT_READ, MAP_SHARED, fd, 0);
        mappedCount+=countPerP;
      }
    }
  }
  if(munmap(mapped, len)<0){
    perror("Could not unmap memory");
    exit(1);
  }
  //printf("Total Count: %d\n\n", mappedCount);
  close(f);
  return mappedCount;
}
int main(int argc, char* argv[]){
  char* filename=argv[1];
  char* c=argv[2];
  int processNum=0;
  int farrayCount=0;
  int mappedCount=0;
  int len=0;
  char* farray;
  char* mapped;
  bool map=false;
  bool parallel=false;
  if(argc<3){
    perror("Not enough arguments\n");
    exit(1);
  }else if(argc>4){
    perror("Too many arguments\n");
    exit(1);
  }
  if(argc==4){
    char* temp3=argv[3];
    if(strcmp(temp3,"mmap")==0){
      map=true;
    }else if(temp3[0]=='p'){
      char* str=&temp3[1];
      processNum=atoi(str);
      if(processNum>16){
        perror("Too many processes\n");
        exit(1);
      }
      cout<<"There are "<<processNum<<" parallel processes"<<endl;
      map=true;
      parallel=true;
    }else{
      len=strtod(argv[3],NULL);
      if(len>8192){
        perror("File size is over 8192 bytes\n");
        exit(1);
      }
    }
    if(!map){
      int fd;
      if((fd=open(filename, O_RDONLY))<0){
        perror("Could not open file\n");
        exit(1);
      }
      if(len>8192){
        perror("File size is over 8192 bytes\n");
        exit(1);
      }
      cout<<"File size: "<<len<<endl;
      farray=new char[len];
      read(fd, farray, len);
      farrayCount=searchForChar(len, 0, farray, c[0]);
      close(fd);

    }else if (!parallel){
      int prot=PROT_READ;
      int flags=MAP_PRIVATE;
      struct stat sb;
      int fd;
      if((fd=open(filename, O_RDONLY))<0){
        perror("Could not open file\n");
        exit(1);
      }
      if(fstat(fd, &sb)<0){
        perror("Could not stat file to obtain size");
        exit(1);
      }
      len=sb.st_size;
      if(len>8192){
        perror("File size is over 8192 bytes\n");
        exit(1);
      }
      cout<<"File size: "<<len<<endl;
      farray=new char[len];
      read(fd, farray, len);
      farrayCount=searchForChar(len, 0, farray, c[0]);
      off_t off=0;
      mapped=(char*)mmap(NULL,len, prot, flags, fd, off);
      close(fd);
    }else{
      struct stat sb;
      int fd;
      if((fd=open(filename, O_RDONLY))<0){
        perror("Could not open file\n");
        exit(1);
      }
      if(fstat(fd, &sb)<0){
        perror("Could not stat file to obtain size");
        exit(1);
      }
      len= sb.st_size;
      if(len>8192){
        perror("File size is over 8192 bytes\n");
        exit(1);
      }
      mappedCount=mmapLoop(filename, fd, len,processNum, c[0]);
      farray=new char[len];
      read(fd, farray, len);
      farrayCount=searchForChar(len, 0, farray, c[0]);
      cout<<"Farray Count: "<<farrayCount<<endl;
      close(fd);
      exit(0);
    }
  }else{ // 3 arguments
    int fd;
    struct stat sb;
    if((fd=open(filename, O_RDONLY))<0){
      perror("Could not open file\n");
      exit(1);
    }
    if(fstat(fd, &sb)<0){
      perror("Could not stat file to obtain size");
      exit(1);
    }
    len= sb.st_size;
    if(len>8192){
      perror("File size is over 8192 bytes\n");
      exit(1);
    }
    cout<<"File size: "<<len<<endl;
    if(len<=1024){
      farray=new char[len];
      read(fd, farray, len);
      farrayCount=searchForChar(len, 0, farray, c[0]);
    }else{
      char buf[1024];
      size_t nbytes=sizeof(buf);
      ssize_t bytes_read;
      while((bytes_read=read(fd, buf, nbytes))>0){
        farrayCount+=searchForChar(bytes_read, 0, buf, c[0]);
      }
    }
    close(fd);
  }
  results(filename, len, farray, mapped, c[0], farrayCount, mappedCount, map, parallel);
  return 0;
}
