/*
 * Copyright 2016 The George Washington University
 * Written by Hang Liu 
 * Directed by Prof. Howie Huang
 *
 * https://www.seas.gwu.edu/~howie/
 * Contact: iheartgraph@gmail.com
 *
 * 
 * Please cite the following paper:
 * 
 * Hang Liu and H. Howie Huang. 2015. Enterprise: breadth-first graph traversal on GPUs. In Proceedings of the International Conference for High Performance Computing, Networking, Storage and Analysis (SC '15). ACM, New York, NY, USA, Article 68 , 12 pages. DOI: http://dx.doi.org/10.1145/2807591.2807594
 
 *
 * This file is part of Enterprise.
 *
 * Enterprise is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Enterprise is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Enterprise.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <iostream>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>

#define INFTY int(1<<30)
using namespace std;

typedef long int vertex_t;
typedef long int index_t;

inline off_t fsize(const char *filename) {
    struct stat st; 
    if (stat(filename, &st) == 0)
        return st.st_size;
    return -1; 
}

		
int main(int argc, char** argv){
	int fd,i;
	char* ss_head;
	char* ss;
	
	std::cout<<"Input: ./exe tuple_file(text) "
			<<"reverse_the_edge(1 reverse, 0 not reverse) lines_to_skip\n";
	if(argc<4){printf("Wrong input\n");exit(-1);}
	
	size_t file_size = fsize(argv[1]);
	bool is_reverse=(atol(argv[2])==1);	
	long skip_head=atol(argv[3]);
	

	fd=open(argv[1],O_CREAT|O_RDWR,00666 );
	if(fd == -1)
	{
		printf("%s open error\n", argv[1]);
		perror("open");
		exit(-1);
	}

	ss_head = (char*)mmap(NULL,file_size,PROT_READ|PROT_WRITE,MAP_PRIVATE,fd,0);
	assert(ss_head != MAP_FAILED);
	size_t head_offset=0;
	int skip_count = 0;
	while(true)
	{
		if(skip_count == skip_head) break;
		if(head_offset == file_size &&
				skip_count < skip_head)
		{
			std::cout<<"Eorr: skip more lines than the file has\n\n\n";
			exit(-1);
		}

		head_offset++;
		if(ss_head[head_offset]=='\n')
		{
			head_offset++;
			skip_count++;
			if(skip_count == skip_head) break;
		}
	}

	ss = &ss_head[head_offset];
	file_size -= head_offset;

	size_t curr=0;
	size_t next=0;

	//step 1. vert_count,edge_count,
	size_t edge_count=0;
	size_t vert_count;
	vertex_t v_max = 0;
	vertex_t v_min = INFTY;//as infinity
	vertex_t a;

	//int reg = 0;
	while(next<file_size){
		char* sss=ss+curr;
		a = atol(sss);
		//std::cout<<a<<"\n";
		//if(reg ++ > 10) break;

		if(v_max<a){
			v_max = a;
		}
		if(v_min>a){
			v_min = a;
		}

		while((ss[next]!=' ')&&(ss[next]!='\n')&&(ss[next]!='\t')){
			next++;
		}
		while((ss[next]==' ')||(ss[next]=='\n')||(ss[next]=='\t')){
			next++;
		}
		curr = next;
		
		//one vertex is counted once
		edge_count++;
	}
	
	const index_t line_count=edge_count>>1;
	if(!is_reverse) edge_count >>=1;
	
	vert_count = v_max - v_min + 1;
	assert(v_min<INFTY);
	cout<<"edge count: "<<edge_count<<endl;
	cout<<"max vertex id: "<<v_max<<endl;
	cout<<"min vertex id: "<<v_min<<endl;

	cout<<"edge count: "<<edge_count<<endl;
	cout<<"vert count: "<<vert_count<<endl;
	
	//step 2. each file size
	char filename[256];
	sprintf(filename,"%s_csr.bin",argv[1]);
	int fd1 = open(filename,O_CREAT|O_RDWR,00666 );
	ftruncate(fd1, edge_count*sizeof(vertex_t));
	vertex_t* csr_adj = (vertex_t*)mmap(NULL,edge_count*sizeof(vertex_t),PROT_READ|PROT_WRITE,MAP_SHARED,fd1,0);
	assert(csr_adj != MAP_FAILED);
    
	sprintf(filename,"%s_csc.bin",argv[1]);
	int fd2 = open(filename,O_CREAT|O_RDWR,00666 );
	ftruncate(fd2, edge_count*sizeof(vertex_t));
	vertex_t* csc_adj = (vertex_t*)mmap(NULL,edge_count*sizeof(vertex_t),PROT_READ|PROT_WRITE,MAP_SHARED,fd2,0);
	assert(csc_adj != MAP_FAILED);

	sprintf(filename,"%s_outbeg_pos.bin",argv[1]);
	int fd3 = open(filename,O_CREAT|O_RDWR,00666 );
	ftruncate(fd3, (vert_count+1)*sizeof(index_t));
	index_t* inbegin  = (index_t*)mmap(NULL,(vert_count+1)*sizeof(index_t),PROT_READ|PROT_WRITE,MAP_SHARED,fd3,0);
	assert(inbegin != MAP_FAILED);
	
	sprintf(filename,"%s_inbeg_pos.bin",argv[1]);
	int fd4 = open(filename,O_CREAT|O_RDWR,00666 );
	ftruncate(fd4, (vert_count+1)*sizeof(index_t));
	index_t* outbegin  = (index_t*)mmap(NULL,(vert_count+1)*sizeof(index_t),PROT_READ|PROT_WRITE,MAP_SHARED,fd4,0);
	assert(outbegin != MAP_FAILED);
    
	sprintf(filename,"%s_outdeg.bin",argv[1]);
	int fd5 = open(filename,O_CREAT|O_RDWR,00666 );
	ftruncate(fd5, vert_count*sizeof(index_t));
	index_t* indegree = (index_t*)mmap(NULL,vert_count*sizeof(index_t),PROT_READ|PROT_WRITE,MAP_SHARED,fd5,0);
	assert(indegree != MAP_FAILED);
	
    sprintf(filename,"%s_outdeg.bin",argv[1]);
	int fd6 = open(filename,O_CREAT|O_RDWR,00666 );
	ftruncate(fd6, vert_count*sizeof(index_t));
	index_t* outdegree = (index_t*)mmap(NULL,vert_count*sizeof(index_t),PROT_READ|PROT_WRITE,MAP_SHARED,fd6,0);
	assert(outdegree != MAP_FAILED);
    
    //added by Hang to generate a weight file
	sprintf(filename,"%s_weight.bin",argv[1]);
	int fd7 = open(filename,O_CREAT|O_RDWR,00666 );
	ftruncate(fd7, edge_count*sizeof(vertex_t));
	index_t* weight= (vertex_t*)mmap(NULL,edge_count*sizeof(vertex_t),PROT_READ|PROT_WRITE,MAP_SHARED,fd7,0);
	assert(weight != MAP_FAILED);
	//-End 

	sprintf(filename,"%s_head.bin",argv[1]);
	int fd8 = open(filename,O_CREAT|O_RDWR,00666 );
	ftruncate(fd8, edge_count*sizeof(vertex_t));
	vertex_t* head = (vertex_t*)mmap(NULL,edge_count*sizeof(vertex_t),PROT_READ|PROT_WRITE,MAP_SHARED,fd8,0);
	assert(head != MAP_FAILED);

	//step 3. write degree
	for(int i=0; i<vert_count;i++){
		indegree[i]=0;
        outdegree[i]=0;
	}

	vertex_t index, dest;
	size_t offset =0;
	curr=0;
	next=0;

	printf("Getting degree...\n");
	while(offset<line_count){
		char* sss=ss+curr;
		index = atol(sss)-v_min;
		while((ss[next]!=' ')&&(ss[next]!='\n')&&(ss[next]!='\t')){
			next++;
		}
		while((ss[next]==' ')||(ss[next]=='\n')||(ss[next]=='\t')){
			next++;
		}
		curr = next;

		char* sss1=ss+curr;
		dest=atol(sss1)-v_min;

		while((ss[next]!=' ')&&(ss[next]!='\n')&&(ss[next]!='\t')){
			next++;
		}
		while((ss[next]==' ')||(ss[next]=='\n')||(ss[next]=='\t')){
			next++;
		}
		curr = next;
		outdegree[index]++;
        indegree[dest]++;
		if(is_reverse) {
            outdegree[dest]++;
            indegree[index]++;
        }
//		cout<<index<<" "<<degree[index]<<endl;

		offset++;
	}
//	exit(-1);
	inbegin[0]=0;
	inbegin[vert_count]=edge_count;
	outbegin[0]=0;
	outbegin[vert_count]=edge_count;

	printf("Calculate beg_pos ...\n");
	for(size_t i=1; i<vert_count; i++){
		outbegin[i] = outbegin[i-1] + outdegree[i-1];
//		cout<<begin[i]<<" "<<degree[i]<<endl;
		outdegree [i-1] = 0;
        inbegin[i] = inbegin[i-1] + indegree[i-1];
        indegree [i-1] = 0;
	}
    indegree[vert_count-1] = 0;
	outdegree[vert_count-1] = 0;
	//step 4: write adjacent list 
	vertex_t v_id;
	offset =0;
	next = 0;
	curr = 0;
	
	printf("Constructing CSR...\n");
	while(offset<line_count){
		char* sss=ss+curr;
		index = atol(sss)-v_min;
		while((ss[next]!=' ')&&(ss[next]!='\n')&&(ss[next]!='\t')){
			next++;
		}
		while((ss[next]==' ')||(ss[next]=='\n')||(ss[next]=='\t')){
			next++;
		}
		curr = next;

		char* sss1=ss+curr;
		v_id = atol(sss1)-v_min;
		csr_adj[outbegin[index]+outdegree[index]] = v_id;
        csc_adj[inbegin[v_id]+indegree[v_id]] = index;
		if(is_reverse) {
            csr_adj[outbegin[v_id]+outdegree[v_id]] = index;
            csc_adj[inbegin[index]+inbegin[index]] = v_id;
        }
		//Added by Hang
		int rand_weight=(rand()%63+1);
		weight[outbegin[index]+outdegree[index]] = rand_weight;
		if(is_reverse){
			weight[outbegin[v_id]+outdegree[v_id]] = rand_weight;
		//-End
        }	
		head[outbegin[index]+outdegree[index]]= index;
		while((ss[next]!=' ')&&(ss[next]!='\n')&&(ss[next]!='\t')){
			next++;
		}
		while((ss[next]==' ')||(ss[next]=='\n')||(ss[next]=='\t')){
			next++;
		}
		curr = next;
		outdegree[index]++;
        indegree[v_id]++;
		if(is_reverse) {outdegree[v_id]++;indegree[index]++;}
		offset++;
	}
	
	//step 5
	//print output as a test
//	for(size_t i=0; i<vert_count; i++){
	for(size_t i=0; i<(vert_count<8?vert_count:8); i++){
		cout<<i<<" "<<outbegin[i+1]-outbegin[i]<<" ";
		for(index_t j=outbegin[i]; j<outbegin[i+1]; j++){
			cout<<csr_adj[j]<<" ";
		}
//		if(degree[i]>0){
        cout<<"\t\t";
		cout<<inbegin[i+1]-inbegin[i]<<" ";
		for(index_t j=inbegin[i]; j<inbegin[i+1]; j++){
			cout<<csc_adj[j]<<" ";
		}
			cout<<endl;
//		}
	}

//	for(int i=0; i<edge_count; i++){
//	for(int i=0; i<64; i++){
//		cout<<degree[i]<<endl;
//	}

	munmap( ss,sizeof(char)*file_size );
	
	//-Added by Hang
	munmap( weight,sizeof(vertex_t)*edge_count );
	//-End

	munmap( csr_adj,sizeof(vertex_t)*edge_count );
	munmap( csc_adj,sizeof(vertex_t)*edge_count );
	munmap( head,sizeof(vertex_t)*edge_count );
	munmap( inbegin,sizeof(index_t)*vert_count+1 );
	munmap( outbegin,sizeof(index_t)*vert_count+1 );
	munmap( indegree,sizeof(index_t)*vert_count );
	munmap( outdegree,sizeof(index_t)*vert_count );
	close(fd1);
	close(fd2);
	close(fd3);
	close(fd4);
	close(fd5);
	close(fd6);
	close(fd7);
	close(fd8);
	
	//-Added by Hang
	//-End
}
