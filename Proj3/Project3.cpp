#include <stdio.h>
#include <iostream>
#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
using namespace std;
struct msg{
  int iFrom; /*who sent the message (0 ... number-of-threads)*/
  int value; /*Its value*/
  int cnt; /*Count of operations (not needed by all msgs)*/
  int tot; /*Total time (not needed by all msgs)*/
};
struct Node{
  int iTo;
  int value;
  Node* next;
};

Node* queue=NULL;
Node* mailboxList=NULL;
#define MAXTHREAD 10
struct msg mailbox[MAXTHREAD]; /*Needs to be the length of max thread*/
sem_t mailboxP[MAXTHREAD];
sem_t mailboxC[MAXTHREAD];
bool nb=false;

bool isEmpty(Node* head){

  bool result=true;
  if(head==NULL){
    result=false;
    //printf("Linked list is full\n");
  }else{
    //printf("Linked list is empty\n");
    result=true;
  }
  return result;
}
int listSize(Node *head)
{
  struct Node *temp=head;
  if (head == NULL)
  {
    return 0;
  }
  int size=0;
  while (temp != NULL) {
    size++;
    temp=temp->next;
  }
  return size;
}
void printList(Node *head)
{
  while (head != NULL) {
    printf("(%d %d)", head->value, head->iTo);
    head = head->next;
  }
  printf("\n");
}
struct Node *push(struct Node **head, int iTo, int value)
{
  Node* temp = new Node();
  temp->iTo = iTo;
  temp->value=value;
  temp->next = *head;
  (*head) = temp;
  return *head;
}

void deleteNode(Node* head, int iTo, int value){
  //printf("Deleting Node\n");
  struct Node *temp=new struct Node;
  temp->value=value;
  temp->iTo=iTo;
  if(head == temp)
  {
    if(head->next == NULL)
    {
      return;
    }
    head->iTo = head->next->iTo;
    head->value = head->next->value;
    temp = head->next;
    head->next = head->next->next;
    free(temp);
    return;
  }
  Node *prev = head;
  while(prev->next != NULL && prev->next != temp)
  prev = prev->next;
  if(prev->next == NULL)
  {
    return;
  }
  prev->next = prev->next->next;
  free(temp);
  return;
}


bool checkQueue(Node* head, int iTo, int value){
  //printf("Searching Queue\n");
  bool result=false;
  while (head != NULL) {
    if(head->iTo==iTo){
      result=true;
    }
    head = head->next;
  }
  return result;
}
void removeDuplicates(Node *head){
  struct Node *ptr1, *ptr2, *dup;
  ptr1 = head;
  while (ptr1 != NULL && ptr1->next != NULL) {
    ptr2 = ptr1;
    while (ptr2->next != NULL) {
      if (ptr1->iTo == ptr2->next->iTo) {
        dup = ptr2->next;
        ptr2->next = ptr2->next->next;
        delete (dup);
      }
      else{
        ptr2 = ptr2->next;
      }
    }
    ptr1 = ptr1->next;
  }
}
int NBSendMsg(int iTo, struct msg *Msg){
  int result=0;
  if(iTo==0){
    //printf("Sending to 0\n");
  }
  if(sem_trywait(&mailboxP[iTo])==0){
    //printf("Sending...\n");
    mailbox[iTo]=*Msg;
    mailboxList=push(&mailboxList,iTo,Msg->value);
    //printf("Message Sent\n");
    //sem_post(&mailboxC[iTo]);
  }else{
    result=-1;
    if(Msg->value!=-1){
      mailboxList=push(&mailboxList,iTo,Msg->value);
    }
      queue=push(&queue,iTo,Msg->value);
  }
  return result;
}
void SendMsg(int iTo, struct msg *Msg){ //Producer
  sem_wait(&mailboxP[iTo]);
  mailbox[iTo]=*Msg;
  sem_post(&mailboxC[iTo]);
}

void RecvMsg(int iRecv, struct msg *Msg){//Consumer

  /*if(iRecv==0){
    //printf("Receving from thread 0\n");
  }else{
    printf("Receving from thread %d\n", iRecv);
  }*/
  sem_wait(&mailboxC[iRecv]);
  *Msg=mailbox[iRecv];
  //printf("Value %d\n", Msg->value);
  sem_post(&mailboxP[iRecv]);
}


void *adder(void* arg){
  int index=*(int*)arg;
  //printf("addder\n");
  struct msg Msg;
  struct msg *finalMsg=new struct msg;

  time_t startTime=time(NULL);
  finalMsg->cnt=0;
  finalMsg->value=0;
  while(true){
    //printf("Thread %d\n", index);

    RecvMsg(index+1, &Msg);

    //printf("Counting incremented\n");
    if(Msg.value==-1){
      //printf("Final Msg Received\n");
      break;
    }else{
      finalMsg->value+=Msg.value;
      finalMsg->cnt++;
      sleep(1);

    }
    //printf("Total Value %d\n",finalMsg->value);
  }
  time_t endTime=time(NULL);
  finalMsg->tot=endTime - startTime;
  finalMsg->iFrom=index+1;
  SendMsg(0,finalMsg);
  //printf("Thread %d Exiting\n", index+1);
  pthread_exit(0);
}

void InitMailBox(){
  /*Mailbox set up code*/
  for(int i=0; i<MAXTHREAD;i++){
    mailbox[i].iFrom=0;
    mailbox[i].value=0;
    mailbox[i].cnt=0;
    mailbox[i].tot=0;
  }
  /*Main thread is thread 0 and the main thread will record the
  thread id stored in the first arg of pthread_create*/
}
int main(int argc, char* argv[]){
  int numThreads=0;
  if(argc<2){
    perror("Not enough arguments\n");
    exit(1);
  }else if(argc==3){
    if(strcmp(argv[2],"nb")==0){
      nb=true;
    }else{
      perror("Error with arguments\n");
      exit(1);
    }
  }else if(argc>3){
    perror("Too many arguments\n");
    exit(1);
  }
  numThreads=atoi(argv[1]);
  printf("Number of threads %d\n",numThreads);
  if(numThreads>MAXTHREAD){
    perror("Sorry, that's too many threads.\n");
    exit(1);
  }else if(numThreads<=0){
    perror("Sorry, that's too little threads.\n");
    exit(1);
  }
  InitMailBox();
  //Semaphore creation
  for(int i=0; i<numThreads+1; i++){
    sem_init(&mailboxP[i], 0, 1);
  }
  for(int i=0; i<numThreads+1; i++){
    sem_init(&mailboxC[i], 0, 0);
  }
  pthread_t threadID[numThreads];
  for(int j=1; j<=numThreads+1; j++){
    if(pthread_create(&threadID[j-1], NULL, adder, (void*)new int (j-1))!=0){
      perror("pthread_create");
      exit(1);
    }
  }
  //printf("All threads created\n");
  struct msg *input=new struct msg;
  //Msg.value=9;
  int value,iTo;
  string str;
  while(true){
    cout<<"Please specify your value and index: "<<endl;
    getline(cin, str);
    if(str.length()<3){
      if(queue!=NULL && nb){
      /*  printf("Sending blocked messages\n");
        printf("Queue: ");
        printList(queue);
        printf("\n");*/
        struct msg *qMsg=new struct msg;
        qMsg->value=queue->value;
        struct Node *q=queue;
        while(q!=NULL){
          q=q->next;
          qMsg->value=queue->value;
          NBSendMsg(queue->iTo,qMsg);
          deleteNode(queue, queue->iTo, queue->value);
          printList(queue);
          queue=q;
          sleep(1);

        }
        //printf("Queue: %d\n", listSize(queue));
      }
      break;
    }
    value=str[0]-'0';

    iTo=str[2]-'0';

    input->value=value;
    input->iFrom=0;
    //printf("%d",numThreads);
    if((value<0)||(iTo<1)||(iTo>numThreads)){
      printf("There's a problem with your inputs, please try again\n");
    }else{
      //printf("Value: %d\niTo: %d\nFrom: %d\n", temp->value, iTo, temp->iFrom);
      if(nb){
        //printf("Checking nb for blocks\n");
        int checker=NBSendMsg(iTo, input);

      }else{
        SendMsg(iTo, input);
      }
    }
  }
  //printf("\n\n\nOutside of main loop\n");
  if(queue!=NULL && nb){
    //printList(queue);
    //printf("Thread termination complete\n");
    /*printf("MailboxList: ");
    printList(mailboxList);
    printf("\n");*/
    removeDuplicates(mailboxList);
    /*printf("MailboxList: ");
    printList(mailboxList);
    printf("\n");*/
    struct msg *mMsg=new struct msg;
    struct Node* mbL=mailboxList;
    while(mbL!=NULL){
      mbL=mbL->next;
      mMsg->value=-1;
      NBSendMsg(mailboxList->iTo,mMsg);
      deleteNode(mailboxList, mailboxList->iTo, mailboxList->value);
      mailboxList=mbL;
      sleep(1);
    }
  //  printf("\nMailboxList Size: %d\n", listSize(mailboxList));
    //printf("\nQueue Size: %d\n", listSize(queue));

    //removeDuplicates(queue);
    //printf("Mailbox 0: %d", mailbox[0].value);
    struct msg finalMsg;
    struct Node *q2=queue;
    //printList(q2);
    struct msg *qMsg=new struct msg;
    while(q2!=NULL){
      q2=q2->next;
      qMsg->value=queue->value;
      NBSendMsg(queue->iTo,qMsg);
      //printf("Here\n");
      RecvMsg(0,&finalMsg);
      printf("The result from thread %d is %d from %d opperations during %d seconds\n", finalMsg.iFrom, finalMsg.value, finalMsg.cnt, finalMsg.tot);

      deleteNode(queue, queue->iTo, queue->value);
      printList(queue);
      queue=q2;
      sleep(1);
    }
    //printf("Done.\n");
  }else if(!nb){
    for(int i=1;i<numThreads+1;i++){

      input->value=-1;
      input->iFrom=0;
      SendMsg(i,input);
      //printf("Final Msg Sent to %d\n", i);
    }
    struct msg finalMsg;
    for(int i=1; i<numThreads+1; i++){
      RecvMsg(0,&finalMsg);
      printf("The result from thread %d is %d from %d opperations during %d seconds\n", finalMsg.iFrom, finalMsg.value, finalMsg.cnt, finalMsg.tot);
    }
  }

  for(int i=0;i<numThreads;i++){
    int fail=pthread_join(threadID[i],NULL);
    if(fail!=0){
      perror("Failed to join with NULL\n");
      exit(1);
    }
  }
  for(int i=0; i<MAXTHREAD; i++){
    sem_destroy(&mailboxC[i]);
  }

  return 0;
}
