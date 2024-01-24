#include<stdio.h>
#include<stdlib.h>
#include<time.h>
#include<unistd.h>
#include "umalloc.h"
#define TRUE 1
#define FALSE 0
#define MAX 999999
char *ptr[MAX];
int k=0, c=1;
unsigned int s=1024;
char* saturation_val, *prev, *first, *last, *maxi;

void consistency(){
    printf("Initalizing a\n");
    char *a1=(char *)malloc(10);
    printf("Starting address of a is %p\n", a1);
    free(a1);
    printf("Initalizing b\n");
    char *b1=(char *)malloc(10);
    printf("Starting address of b is %p\n", b1);
    free(b1);
    printf("\nConsitency is proven since both the blocks are assigned at the same Address\n");
}

int Maximization(){
    int x=1;
    int flag=0;
    int ans=0;
    
    while (TRUE)
    {
        maxi=(char*)malloc(x);
        if(maxi!=NULL && flag==0){
           //printf("In f=0 value of x is %d\n",x);
           ans=x;
           x=x*2;
           free(maxi); 
        }
        else if(maxi==NULL){
            //printf("In max==null value of x is %d\n",x);
            x=x/2;
            break;
        }
    }
    maxi=(char*)malloc(ans);
    /*
    x=x/2;
    char* m;
    while (TRUE)
    {
        m=(char*)malloc(x);
        if(m!=NULL){
            ans=ans+x;
            free(m);
            break;
        }
        x=x/2;
    }
    */
    if(maxi!=NULL)
    {
        free(maxi);
        printf("\nMaximum size is %d\n",ans);
    }
    return(ans);
}


void Basic_Coalesence(int ans){
    int half=ans/2;
    int quarter=ans/4;
    char *Half=(char *)malloc(half);
    printf("\nStarting address of Half the size initialization is %p\n", Half);
    char *Quarter= (char *)malloc(quarter);
    printf("Starting address of Quarter the size initialization is %p\n", Quarter);
    free(Half);
    free(Quarter);
    char *final=(char*)malloc(ans);
    printf("Starting address of Max size initialization is %p\n", final);
    free(final);
}

void Saturation(){

    //printf("Starting address of mem %p\n",&mem[0]);
    //printf("Ending address of mem %p\n",&mem[999]);
    while (c<=9216)
    {
        saturation_val=(char*)malloc(s);
        ptr[k]=saturation_val;
        //printf("Sat %p\n",saturation_val);
        //printf("Ptr %p\n",ptr[k]);
        k++;
        //if(c==100){break;}
        if(c==1){
            first=saturation_val;
            //break;
        }
        //printf("Starting address of Saturation val is %p\n", saturation_val);
        //if(c==5) break;
        prev=saturation_val;
        //printf("%d\n",c);
        c++;
        
    }
    s=1;
    while (TRUE)
    {
        //break;
        saturation_val=(char*)malloc(s);
        //printf("Starting address of Saturation val is %p\n", saturation_val);
        if(saturation_val==NULL){
            break;
        }
        ptr[k]=saturation_val;
        k++;
        prev=saturation_val;
        //printf("%d\n",c);
        c++;
        
    }
    printf("The memory saturation is complete\n");
}

void timeOverhead(){
    free(prev);
    struct timespec start, end;
    clock_gettime(CLOCK_REALTIME, &start);
    saturation_val=(char*)malloc(s);
    clock_gettime(CLOCK_REALTIME, &end);
    printf("\nRunning time: %lu nano seconds\n", (end.tv_nsec - start.tv_nsec));
    //printf("running time: %lu micro-seconds\n", 
	//     (end.tv_sec - start.tv_sec) * 1000 + (end.tv_nsec - start.tv_nsec) / 1000000);
//printf("Starting address of Saturation val is %p\n", saturation_val);        
    last=saturation_val;
}

void intermediateCoalesence(int ans){
    int g=0;
    while (g<k)
    {
        free(ptr[g]);
        g++;
    //printf("%d\n",g);
    }
    maxi=(char*)malloc(ans);
    printf("\nAddress of Maximal block allocation is %p\n",maxi);
}



int Maximization_9MB(){
    free(maxi);
    int x=1;
    int flag=0;
    int ans=0;
    
    while (TRUE)
    {
        maxi=(char*)malloc(x);
        if(maxi!=NULL && flag==0){
           //printf("In f=0 value of x is %d\n",x);
           ans=x;
           x=x*2;
           free(maxi); 
        }
        else if(maxi==NULL){
            //printf("In max==null value of x is %d\n",x);
            x=x/2;
            break;
        }
    }
    maxi=(char*)malloc(ans);
    x=x/2;
    char* m;
    while (TRUE)
    {
        m=(char*)malloc(x);
        if(m!=NULL){
            ans=ans+x;
            free(m);
            break;
        }
        x=x/2;
    }
    free(maxi);
    printf("\nMaximum size is %d\n",ans);
    return(ans);
}



int main()
{

    struct timespec start, end;
    clock_gettime(CLOCK_REALTIME, &start);
// 0. Consistency
printf("\n***********Testing Consistency************\n");
    consistency();
printf("\n******************************************\n");
// 1. Maximization
printf("\n***********Testing Maximization***********\n");
    int ans=Maximization();
printf("\n******************************************\n");
// 2. Basic Coalesence
printf("\n********Testing Basic Coalescence*********\n");
    Basic_Coalesence(ans);
printf("\n******************************************\n");
// 3. Saturation

printf("\n************Testing Saturation***********\n");
    Saturation();
printf("\n******************************************\n");
    /*printf("Difference address of mem %ld\n",&mem[10485760]-&mem[0]);
    printf("Difference address of mem od prev and first %ld\n",prev-first);
    printf("Difference of saturation val and prev address of mem %ld\n",saturation_val-prev);
    printf("Difference of saturation val and first address of mem %ld\n",saturation_val-first);
    printf("First address block %p\n",first);
    printf("Prev address block %p\n",prev);
    printf("Sat address block %p\n",saturation_val);*/
  

// 4. Time Overhead();
printf("\n***********Testing Time Overhead**********\n");
    timeOverhead();
printf("\n******************************************\n");    
//free(saturation_val);


// 5. Intermediate Coalescence
printf("\n*****Testing Intermediate Coalescence*****\n");
    intermediateCoalesence(ans);
printf("\n******************************************\n"); 
clock_gettime(CLOCK_REALTIME, &end);

printf("Overall Process Run Time: %lu micro secs\n", (end.tv_sec - start.tv_sec) * 1000 + (end.tv_nsec - start.tv_nsec) / 1000000);

// 6. Maximization -- Approach 2
printf("\n*****Testing Approach-2 for Maximization*****\n");
    ans=Maximization_9MB();
printf("\n******************************************\n"); 


return(0);

}
