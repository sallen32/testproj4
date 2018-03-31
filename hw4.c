#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
int test = 0;
int testa = 0;
int testb = 0;
#define TEST printf("TESTa # %d\n", test++);
#define TESTa printf("TESTb # %d\n", testa++);
#define TESTb printf("TESTc # %d\n", testb++);

struct memory_region{
  size_t * start;
  size_t * end;
};

struct memory_region global_mem;
struct memory_region heap_mem;
struct memory_region stack_mem;

void walk_region_and_mark(void* start, void* end);

//how many ptrs into the heap we have
#define INDEX_SIZE 1000
void* heapindex[INDEX_SIZE];


//grabbing the address and size of the global memory region from proc 
void init_global_range(){
  char file[100];
  char * line=NULL;
  size_t n=0;
  size_t read_bytes=0;
  size_t start, end;

  sprintf(file, "/proc/%d/maps", getpid());
  FILE * mapfile  = fopen(file, "r");
  if (mapfile==NULL){
    perror("opening maps file failed\n");
    exit(-1);
  }

  int counter=0;
  while ((read_bytes = getline(&line, &n, mapfile)) != -1) {
    if (strstr(line, "hw4")!=NULL){
      ++counter;
      if (counter==3){
        sscanf(line, "%lx-%lx", &start, &end);
        global_mem.start=(size_t*)start;
        // with a regular address space, our globals spill over into the heap
        global_mem.end=malloc(256);
        free(global_mem.end);
      }
    }
    else if (read_bytes > 0 && counter==3) {
      if(strstr(line,"heap")==NULL) {
        // with a randomized address space, our globals spill over into an unnamed segment directly following the globals
        sscanf(line, "%lx-%lx", &start, &end);
        printf("found an extra segment, ending at %zx\n",end);						
        global_mem.end=(size_t*)end;
      }
      break;
    }
  }
  fclose(mapfile);
}


//marking related operations

int is_marked(size_t* chunk) {
  return ((*chunk) & 0x2) > 0;
}

void mark(size_t* chunk) {
  (*chunk)|=0x2;
}

void clear_mark(size_t* chunk) {
  (*chunk)&=(~0x2);
}

// chunk related operations

#define chunk_size(c)  ((*((size_t*)c))& ~(size_t)3 ) 
void* next_chunk(void* c) { 
  if(chunk_size(c) == 0) {
    printf("Panic, chunk is of zero size.\n");
  }

  if((c+chunk_size(c)) < sbrk(0))
    return ((void*)c+chunk_size(c));
  else 
    return 0;
}
int in_use(void *c) { 
  return (next_chunk(c) && ((*(size_t*)next_chunk(c)) & 1));
}


// index related operations

#define IND_INTERVAL ((sbrk(0) - (void*)(heap_mem.start - 1)) / INDEX_SIZE)

void build_heap_index() {
  // TODO
/*
int num = 0;
int i = 0;
  for(num = &heap_mem.start; num < &heap_mem.end; num++)
  {
    if(chunk_size(num) == 0)
    {
       break;
    }
    else
    {
       heapindex[i] = num;
       i++;
    }

  }*/
}



// the actual collection code
void sweep() {
// TODO

size_t * i = 0;
 //printf("start sweep\n");
for( i = (heap_mem.start - 1); i < (size_t *)sbrk(0); )
{
 //printf("%p %d %p %p\n",i,chunk_size(i),next_chunk(i), heap_mem.end);
 //      printf("%d\n", (int)i);

         size_t * temp = next_chunk(i);

         if(is_marked(i))
         {
           clear_mark(i);
         }

         else if (in_use(i))
         {
            free(i+1);
          }

         if( temp == sbrk(0) )
         {
            break;
         }

         if( temp == 0 || temp == NULL )
         {
           break;
         }

         i = temp;

  }
 }

//determine if what "looks" like a pointer actually points to a block in the heap
size_t * is_pointer(size_t * ptr) {
  // TODO

//printf("pointer %p \n", ptr);
//printf("start of heap %p end of heap %p \n", (heap_mem.start-1), heap_mem.end);
//printf("sbrk is %d \n", (size_t *)sbrk(0));

  size_t * i = 0;

  if(ptr < (heap_mem.start-1) || ptr > (size_t*)sbrk(0) )
  {
        return NULL;
  }

 if( !in_use(ptr) )
  {
         return NULL;
  }

//TEST

  for(i = (heap_mem.start-1); i < (size_t*)sbrk(0); )
  {
        size_t * temp = next_chunk(i);

     if( i < ptr && ptr < temp && in_use(i) )
     {
        //printf("what is I %p \n", i);
           return ((i - chunk_size(i)));
     }
        i = temp;

  }

}

void walk_region_and_mark(void* start, void* end) {
  // TODO

//size_t * begin = start;
size_t * finish = end+1;

size_t * i = start-1;
//size_t * c = 0;



//printf("we in this walk \n");
while(i < finish)
{
//for( i = (size_t*)start; i < (size_t*)end;  )
//{
  size_t* temp = next_chunk(i);

  size_t * c = is_pointer( i );

//printf("what is i %p \n",(i) );
//printf("what is size i %p \n", chunk_size(i));


// size_t* temp = next_chunk(c);

 // printf("start %p i %p end %p c %p \n",start,i,end,c);

  if (c == NULL)
  {
         break;
  }

  if( c && (!is_marked(c)) )
  {
     mark(c);
     walk_region_and_mark(c+1,next_chunk(c));
  }

  if( temp == end )
  {
        break;
  }

  if( temp == 0 || temp == NULL)
  {
        break;
  }

        i = temp;
 }

}

// standard initialization 

void init_gc() {
  size_t stack_var;
  init_global_range();
  heap_mem.start=malloc(512);
  //since the heap grows down, the end is found first
  stack_mem.end=(size_t *)&stack_var;
}

void gc() {
  size_t stack_var;
  heap_mem.end=sbrk(0);
  //grows down, so start is a lower address
  stack_mem.start=(size_t *)&stack_var;

  // build the index that makes determining valid ptrs easier
  // implementing this smart function for collecting garbage can get bonus;
  // if you can't figure it out, just comment out this function.
  // walk_region_and_mark and sweep are enough for this project.
  //build_heap_index();

  //walk memory regions
//printf("%s\n", "this is probably where it crashes");

  walk_region_and_mark(global_mem.start,global_mem.end);
  walk_region_and_mark(stack_mem.start,stack_mem.end);
  sweep();
}
