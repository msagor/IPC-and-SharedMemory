//<SEE DETAILS BELOW HOW THIS PROGRAM WORKS AND HOW TO COMPILE AND EXECUTE>
#include <stdio.h>
#include <unistd.h>
#include <cstdlib>
#include <mutex>
#include <iostream>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdlib.h>
#include <string>
#include <signal.h>
#include <cstring>
#include <sys/wait.h>
#include <sstream>
#include <time.h>
#include <pthread.h>

using namespace std;

//matrix A variables
int* mat_A;//pointer to attach with mat A shared mem
key_t key_A;//key for shared memory id for mat A
int shmid_A;//shared memory id for mat A
int** matA;//2d matrix (parent local cache)

//matrix B variables
int* mat_B;
key_t key_B;
int shmid_B;
int** matB;

//matrix C variables
int* mat_C;
key_t key_C;
int shmid_C;
int** matC;

//more variables
int mat_A_row, mat_A_col, mat_B_row, mat_B_col, mat_C_row, mat_C_col = -1;
std::mutex mu;//mutex

//struct for matrix computation
struct v{
  int i;//row;
  int j;//column
};

//function that actually does matrix mult
void* runner(void* param){
   struct v* data = (struct v*) param;
   int n, sum =0;
   
   //row multiplied by column
   int husband = -1;
   int wife = -1;
   for(n=0; n<mat_A_col; n++){
      mu.lock();
      husband = matA[data->i][n];
      mu.unlock();
      mu.lock();
      wife =  matB[n][data->j];
      mu.unlock(); 
      sum+= husband*wife;
   }

   //assign the sum to its coordinate
   mu.lock();
   matC[data->i][data->j] = sum;
   mu.unlock();
    
   //exit the thread
   pthread_exit(0); 
}


int main(){
   cout << "P:Beginning of solution..."<< endl;
   
   //parent asking what is the dimension of matrices
   cout << "P:Enter num of rows of matrix A: ";
   cin >> mat_A_row;
   cout << "P:Enter num of columns of matrix A: ";
   cin >> mat_A_col;
   mat_B_row = mat_A_col;
   cout << "P:Matrix B has " << mat_B_row << " rows." << endl;
   cout <<"P:Enter num of columns of matrix B: ";
   cin >> mat_B_col;
   mat_C_row = mat_A_row;
   mat_C_col = mat_B_col;
   
   //parent creating shared memory for matrix A
   mu.lock();
   key_A  = ftok("gda", 'p');
   shmid_A = shmget(key_A, ((mat_A_row*mat_A_col)*sizeof(int)), 0666|IPC_CREAT);
   mu.unlock();

   //parent creating shared memory for matrix B
   mu.lock();
   key_B  = ftok("heb", 'k');
   shmid_B = shmget(key_B, ((mat_B_row*mat_B_col)*sizeof(int)), 0666|IPC_CREAT);
   mu.unlock();

   //parent creating shared memory for matrix C
   mu.lock();
   key_A  = ftok("ifc", 'd');
   shmid_C = shmget(key_C, ((mat_C_row*mat_C_col)*sizeof(int)), 0666|IPC_CREAT);   cout <<"P:shared memories has been created by parent." << endl;
   mu.unlock();

   //THIS BLOCK IS USED FOR DESTROYING DEAD SHARED MEMEORIES.SEE DETAILS BELOW* 
   //parent destroys shared memory for cleaning
   /* shmctl(shmid_A, IPC_RMID, NULL);
   shmctl(shmid_B, IPC_RMID, NULL);
   shmctl(shmid_C, IPC_RMID, NULL);
   cout << "P:Shared memory had been destroyed by parent." << endl;
   exit(0); */

   //parent forking to create child process
   cout <<"P:Forking..." << endl;   
   pid_t pid = fork();
   
   if(pid < 0){
      
      cout << "P:fork failed...restart the program." << endl;
   
   }else if(pid > 0){
     
      //parent waits until child changes state
      int ret;
      waitpid(pid,&ret,WUNTRACED | WCONTINUED);
      if(WIFSTOPPED(ret)){ /*CONTINUE*/ }

       
      //execution returns to parent for the first time
      cout <<"P:Parent starts execution..." << endl;
     
      //parent printing matrix A from shared memory before computing
      cout <<"P:Parent printing matrix A from shared memory" << endl;
      cout <<"P:Matrix A- " << endl;
      int countttt=1;
      mat_A =(int*) shmat(shmid_A, 0, 0);
      for(int i=0; i<(mat_A_row*mat_A_col); i++){
         mu.lock();
         cout<< mat_A[i] << " ";
         mu.unlock();
         if(countttt==mat_A_col){cout << endl; countttt=0;}
         countttt++;

      }
      cout << endl;
      shmdt(mat_A);

      //parent printing matrix B from shared memory before computing
      cout <<"P:Parent printing matrix B from shared memory" << endl;
      cout <<"P:Matrix B- " << endl;
      countttt=1;
      mat_B =(int*) shmat(shmid_B, 0, 0);
      for(int i=0; i<(mat_B_row*mat_B_col); i++){
         mu.lock();
         cout<< mat_B[i] << " ";
         mu.unlock();
         if(countttt==mat_B_col){cout << endl; countttt=0;}
         countttt++;

      }
      cout << endl;
      shmdt(mat_B);

      //parent creates local cache for matrix A,B,C
      matA = new int*[mat_A_row];
      for(int i=0; i<mat_A_row; ++i){matA[i] = new int[mat_A_col];}
      matB = new int*[mat_B_row];
      for(int i=0; i<mat_B_row; ++i){matB[i] = new int[mat_B_col];}
      matC = new int*[mat_C_row];
      for(int i=0; i<mat_C_row; ++i){matC[i] = new int[mat_C_col];}
      //cout <<"P:Parent created local cache" << endl;

      //copying matrix A from shared memory to local cache
      mat_A = (int*) shmat(shmid_A, 0,0);
      int count = 0;
      for(int i=0; i< mat_A_row; i++){
         for(int j=0; j<mat_A_col; j++){
            matA[i][j] = mat_A[count];
            count++; 
         }
      }
      shmdt(mat_A);
      
      //copying matrix B from shared memory to local cache
      mat_B = (int*) shmat(shmid_B, 0,0);
      count = 0;
      for(int i=0; i< mat_B_row; i++){
         for(int j=0; j<mat_B_col; j++){
            matB[i][j] = mat_B[count];
            count++; 
         }
      }
      shmdt(mat_B);
      
      //copying matrix C from shared memory to  local cache
      mat_C = (int*) shmat(shmid_C, 0,0);
      count = 0;
      for(int i=0; i< mat_C_row; i++){
         for(int j=0; j<mat_C_col; j++){
            matC[i][j] = mat_C[count];
            count++; 
         }
      }
      shmdt(mat_C);

      //parent does matrix multiplication
      cout <<"P:Parent starts matrix mult" << endl;
      int i,j,countt=0;
      for(i =0; i<mat_A_row; i++){
         for(j=0;j<mat_B_col; j++){
            struct v* data = (struct v*) malloc(sizeof(struct v));
            data->i = i;
            data->j = j;
            pthread_t tid;
            pthread_attr_t attr;
            pthread_attr_init(&attr);
            pthread_create(&tid,&attr,runner,data);
            pthread_join(tid,NULL);
            countt++;
         }
      }
      cout << "P:Parent ends matrix mult" << endl;

      //parent transfers matrix C from local cache to shared mem
      mu.lock();
      int counttt=0;
      mat_C = (int*) shmat(shmid_C, 0,0);
      for(int i=0; i<mat_C_row; i++){
         for(int j=0; j<mat_C_col; j++){
            mat_C[counttt] = matC[i][j];
            counttt++;
         }
      }
      shmdt(mat_C);
      mu.unlock();

      //parent awakes child to print the computed matrix
      cout <<"P:Parent giving control back to child" << endl;
      kill(pid, SIGCONT);
      sleep(1);
 
      //parent waits for child finish printing matrix C and gain back ctrl
      //std::system("kill -STOP " + getpid());
      int ret1;
      waitpid(pid,&ret1,WUNTRACED | WCONTINUED);
      if(WIFSTOPPED(ret1)){}

      //parent gets control back for second time
      cout <<"P:Parent starts execution again...." << endl;
      
      //parent kills shared memory and exits
      shmctl(shmid_A, IPC_RMID, NULL);
      shmctl(shmid_B, IPC_RMID, NULL);
      shmctl(shmid_C, IPC_RMID, NULL);
      cout <<"P:Parent destroyed shared memeory." << endl;
      cout <<"P:Parent is dying...dead" << endl;

      
   }else if(pid == 0){
      
      cout <<"C:fork succeeded...child executions first" << endl;

      //child takes input for matrices
      cout <<"C:Enter <Y> to auto populate matrices A and B." << endl;
      std::string input="";
      while( input.compare("Y")!=0 ){
         cout <<"C:press Y: ";
         cin >> input;
      }
   
      //child populates matrix A with random numbers 
      srand(time(NULL)); 
      mu.lock();
      mat_A = (int*) shmat(shmid_A, 0,0);
      for(int i=0; i<(mat_A_row*mat_A_col); i++){
         mat_A[i] = rand() % 10;
      }
      shmdt(mat_A);
      mu.unlock();
      cout<<"C:Child populated matrix A in shared memory." << endl;
      
      //child populates matrix B with random numbers
      mu.lock();
      mat_B = (int*) shmat(shmid_B, 0,0);
      for(int i=0; i<(mat_B_row*mat_B_col); i++){
         mat_B[i] = rand() % 10;
      }
      shmdt(mat_B);
      mu.unlock();
      cout <<"C:Child populated matrix B in shared memory" << endl;
      
      //child populates matrix C with -1
      mu.lock();
      mat_C = (int*) shmat(shmid_C, 0,0);
      for(int i=0; i<(mat_C_row*mat_C_col); i++){
         mat_C[i] = -1;
      }
      shmdt(mat_C);
      mu.unlock();
     
      //child giving control back to parent for the first time
      cout <<"C:Passing control to parent for first time" << endl;
      kill(getpid(), SIGSTOP); 

      //child got control back
      cout<<"C:Child got control back..." <<endl;

      //child printing computed matrix C from shared memory
      cout <<"C:Child printing final matrix C from shared memory" << endl;
      cout <<"C:Matrix C- " << endl;
      int countttt=1;
      mat_C =(int*) shmat(shmid_C, 0, 0);
      for(int i=0; i<(mat_C_row*mat_C_col); i++){
         mu.lock();
         cout<< mat_C[i] << " ";
         mu.unlock();
         if(countttt==mat_C_col){cout << endl; countttt=0;}
         countttt++;

      }
      cout << endl;
      shmdt(mat_C);

      //child is exiting
      cout << "C:Child is dying...dead" << endl;
   }



   return 0;
}




/*
1.The program starts by parent process creating shared memory.
2.Then the parent process forks a child process and the child process and child process takes over execution. 
3.child process then takes user input for two matrices A and B, and stores them in shared memory. 
4.The child then gives execution back to parent and parent prints the matrices A and B from shared memory. 
5.Parent then copies the matrices from shared memory to its local cache. 
6.Parent then passes the local caches as arguments for matrix multiplication and the resultant matrix C is stored in local cache. 
7.Parent then copies the resultant matrix C from local cache to shared memory.
8.Then parent gives execution back to child.
9.Child wakes up and prints the resultant matrix from the shared memory. 
10.Child then dies and gives control back to parent. 
11.Parent finally destroys the shared memory and dies too.
*/

/*There is a block of code in the middle which is currently commented. This block is needed for destroying shared memory if for any case the shared memory crashes while creation. Because once a shared memory is created, the memory is forever there until destroyed manually. So, if any case the program crashes, the shared memory will be there next time the program will be executed and will give segmentation fault every time the program is executed in future in the same machine. In this case, you must remember for what matrix size the shared memory failed. Then uncomment the piece of code and compile-execute the program again, create the shared memory of the same dimension. This will erase the shared memory that crashed but existed in the system. Now that the shared memory is deleted, comment out the piece of code and recompile and execute the program normally. */

/*HOW TO RUN THIS PROGRAM: 
//-use linux based system
//-must have c++ v11 and g++ compiler
//type "g++ -std=c++11 -pthread solution1.cpp -o sol" to create an executable named "sol"
//then type "./sol" to execute the sol executable.*/
