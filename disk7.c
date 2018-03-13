#include "disk1.h"

//initializes superblocks and blocks
int initialize()
{
	int i;
	super.totalblocks=T_BLOCKS;
	super.totalfreeb=T_BLOCKS;
	for(i=0;i<super.totalblocks;i++)
	{
		super.freeblocknos[i]=i;
	}

	blocks=(struct block **)malloc(sizeof(struct block *)*(super.totalblocks));
	
	for(i=0;i<super.totalblocks;i++)
	{
		blocks[i]=(struct block *)malloc(sizeof(struct block));
		blocks[i]->valid=-1;
		blocks[i]->blockno=i;
		blocks[i]->data=(void *)malloc(sizeof(char)*BLOCK_SIZE);
		blocks[i]->current_size=0;
		blocks[i]->size=BLOCK_SIZE;
		
	}
	return 0;
}

//given a block number and offset, read from the block
int read_block(int blocknr, char *buf,off_t new_off,off_t off,size_t n)
{
	if(blocknr>T_BLOCKS)
		return -1;
	if(n>BLOCK_SIZE)
		return -1;	

	memcpy(buf+off,(char *) (blocks[blocknr]->data)+new_off,n);
	return 0;
}

//given a block number and offset, write in the block
int write_block(int blocknr,const char *buf,off_t new_off,off_t off,size_t n)
{
	printf("in writeb %d %ld\n",blocknr,n);
	if(blocknr>T_BLOCKS)
		return -1;
	if(n>BLOCK_SIZE)
		return -1;
	memcpy((blocks[blocknr]->data)+new_off, buf+off,n);
	return 0;
}

//given a path, search for the file.
//If file is not in the root (s = 1) it will search in the directory
//If s = 0, it returns the directory structure containing that file
int searchroot(const char *path,struct file **file_t,struct directry **dir,int s)
{

	if(strcmp(path,"/")==0)
		return 0;
	const char *name;
	if(path[0]=='/')
		name=path+1;
	else
		name=path;
	int i;
	for(i=0;i<fileroot->filecount;i++)
	{
		if(strcmp(name,fileroot->files[i]->fname)==0)
		{
			(*file_t)=fileroot->files[i];
			return 1;
		}
	}
	
	int namelen = 0;
 	const char *name_end = name;
 	while(*name_end != '\0' && *name_end != '/') {
  		  name_end++;
  		  namelen++;
 	 }

	struct directry * d =(struct directry *)malloc(sizeof(struct directry));
	d=fileroot->dir;
	for(i=0;i<fileroot->dircount;i++)
	{
		
		if(strncmp(d->dname,name,namelen)==0)
		{	
			if(s==1)
				return searchdir(d,name_end,file_t);
			else
			{   
				strcpy((*file_t)->fname,name_end);
				(*dir)=d;
				return 2;
			}
		}
		else
			d=d->next;
			
	}
	return -1;
	
}

//search for the file in the directory
int searchdir(struct directry * d,const char *name,struct file **file_t)
{

	int i;
	if(name[0]=='/')
		name=name+1;
	for(i=0;i<d->filecount;i++)
	{
		if(strcmp(name,d->f[i]->fname)==0)
		{
			(*file_t)=d->f[i];
			return 1;
		}
	}
	return -1;
}

//Creates a directory
static int file_mkdir(const char *path, mode_t mode)
{
	
	if(strcmp(path,"/")==0)
		return -1;
	const char *name;
	if(path[0]=='/')
		name=path+1;
	else
		name=path; //getting the directory name
	
	//Initializing the directory
	struct directry *temp=(struct directry *)malloc(sizeof(struct directry));
	memset(temp, 0, sizeof(struct directry));
	strcpy(temp->dname,name);
	temp->dirId=fileroot->dircount;
	temp->mode=S_IFDIR | mode;
	temp->filecount=0;
	temp->next=NULL;
	temp->gid=fileroot->gid;
	temp->uid=fileroot->uid;
	struct stat *stbuf =(struct stat*)malloc(sizeof(struct stat));
	
	//Link the directory to the data structure
	if(fileroot->dircount==0)
	{
		fileroot->dir=(struct directry *)malloc(sizeof(struct directry));
		fileroot->dir=temp;
		fileroot->dircount=(fileroot->dircount)+1;
		
		stbuf->st_uid=temp->uid;
		stbuf->st_gid=temp->gid;
		memset(stbuf, 0, sizeof(struct stat));
		stbuf->st_mode  = temp->mode;
		stbuf->st_nlink = 0;
		return 0;
	}
	
	else
	{
		struct directry *t=(struct directry *)malloc(sizeof(struct directry));
		t=fileroot->dir;
		while(t->next!=NULL)
			t=t->next;
		t->next=(struct directry *)malloc(sizeof(struct directry));
		t->next=temp;
		fileroot->dircount=(fileroot->dircount)+1;
		stbuf->st_uid=temp->uid;
		stbuf->st_gid=temp->gid;
		  memset(stbuf, 0, sizeof(struct stat));
		  stbuf->st_mode  = temp->mode;
		  stbuf->st_nlink = 0;

		return 0;
	}

	return -errno;
}

//Open a file
static int file_open(const char *path, struct fuse_file_info *fi) 
{
	struct file *ft=(struct file *)malloc(sizeof(struct file));
	struct directry *d=(struct directry*)malloc(sizeof(struct directry));
	
	int flag=searchroot(path,&ft,&d,1);

	if(flag==1)
	{
		ft->fd_count++;
		fi->fh = (uint64_t) ft;
		return 0;
	}
	
		return -ENOENT;
}

//Get file attributes
static int file_getattr(const char *path, struct stat *stbuf,struct fuse_file_info *fi)
{
	
	printf("Entering getattr....\n");
	struct file *ft=(struct file *)malloc(sizeof(struct file));
	struct directry *d=(struct directry*)malloc(sizeof(struct directry));

	int flag=searchroot(path,&ft,&d,1);
	printf("%d\n",flag);
	if(flag==0)//root
	{
		  printf("In Root\n");
		  stbuf->st_mode   = fileroot->mode;
		  stbuf->st_uid    = fileroot->uid;
		  stbuf->st_gid    = fileroot->gid;
		  return 0;
	}
	else if(flag==1)//root files in the root
	{
		printf("Files in Root\n");
		  stbuf->st_mode   = ft->fil->mode;
		  stbuf->st_size   = ft->fil->size;
		  stbuf->st_blocks = ft->fil->blockcount;
		  stbuf->st_uid    = ft->fil->uid;
		  stbuf->st_gid    = ft->fil->gid;
		  return 0;
	}
	
	else //files within a directory or a root directory
	{
		printf("File in a directory or a directory in the Root\n");
		struct directry *t=(struct directry*)malloc(sizeof(struct directry));
		const char * name=path;
		int count=0;

		while(*name!='\0')
		{
			if(*name=='/')
				count++;
			name++;
		}
		if(count==0 || count==1){
		int flag1=searchroot(path,&ft,&t,0);
		if(flag1==2)
		{
			stbuf->st_mode   = t->mode;
			stbuf->st_uid    = t->uid;
			stbuf->st_gid    = t->gid;
			return 0;
		}
	
		else
			 return -ENOENT;
		}
		else
		{
			int flag1=searchroot(path,&ft,&t,0);
			if(flag1==2)
			{
				struct file *f1=(struct file *)malloc(sizeof(struct file));
				int fl=searchdir(t,ft->fname,&f1);
				if(fl==-1)
					return -ENOENT;
				else
				{
					stbuf->st_mode   = f1->fil->mode;
					  stbuf->st_size   = f1->fil->size;
					  stbuf->st_blocks = f1->fil->blockcount;
					  stbuf->st_uid    = f1->fil->uid;
					  stbuf->st_gid    = f1->fil->gid;
						return 0;
				}	
			}
		}	
	}
	return 0;
}

//Remove directory
int file_rmdir(const char *path)
{
    if(path[0] == '/')
    {
        path = path+1;
    }
    struct directry *prev = (struct directry *)malloc(sizeof(struct directry));
    struct directry *tmp = (struct directry *)malloc(sizeof(struct directry));
	tmp=fileroot->dir;
	prev=NULL;
    while(tmp != NULL)
    {
	if((strcmp(tmp->dname,path) == 0)){
	break;
    }
	prev = tmp;
    tmp = tmp -> next;
    }
    
    if(tmp != NULL)
    {
    if(tmp->filecount == 0)
    {
	if(prev!=NULL){
        prev->next = tmp->next;
        fileroot->dircount = fileroot->dircount-1;
	}
	else
	{
		fileroot->dir=NULL;
		fileroot->dircount =fileroot->dircount- 1;
	}
		printf("Directory removed..\n");
        return 0;

    }
	}
printf("Cant remove directry. Its not empty\n");
	
		
    

	return -errno;

}

//Remove a file
int file_unlink(const char *path)
{
    const char * name=path;
	int count=0;

	while(*name!='\0')
	{
		if(*name=='/')
			count++;
		name++;
	}
	printf("count %d\n",count);
	if(count==0 || count==1) //file is in root
	{

		if(path[0] == '/')
	    {
		path = path+1;
	    }
	    int i;
	    for(i = 0; i < fileroot->filecount ; i++)
	    {
		if(strcmp(fileroot->files[i]->fname,path)==0)
		    break;
	    }
	    if(i >= fileroot->filecount)
		return -errno;
	    struct fileinfo *fd = fileroot->files[i]->fil;
	    int j;
	    for(j = 0 ; j < fd->blockcount; j++)
	    {
		super.freeblocknos[super.totalfreeb++]=fd->blockno[j]; //freeing the blocks allocated to the file
		blocks[fd->blockno[j]]->current_size = 0;
	    }
	    if(i == fileroot->filecount-1){
		fileroot->filecount = fileroot->filecount-1;
		printf("File Deleted..\n");
		return 0;
	}
	    else
		{
		    fileroot->files[i] = fileroot->files[fileroot->filecount-1];
		    fileroot->filecount = fileroot->filecount-1;
		    printf("File Deleted..\n");
		    return 0;
		}
		printf("File Deletion failed..\n");
	    return -errno;
	}

	else //file is in a directory
	{
		
		struct file *ft=(struct file *)malloc(sizeof(struct file));
		struct directry *d=(struct directry*)malloc(sizeof(struct directry));
		int flag=searchroot(path,&ft,&d,0);

		if(flag!=2) 
		{
			printf("File Deletion failed..\n");
			return -errno;
		}
		
		 int i;
		const char *p=ft->fname;
		if(p[0]=='/')
			p=p+1;
	    for(i = 0; i < d->filecount ; i++)
	    {
		if(strcmp(d->f[i]->fname,p)==0)
		    break;
	    }
	    if(i >= d->filecount)
		return -errno;
	    struct fileinfo *fd = d->f[i]->fil;
	    int j;
	    for(j = 0 ; j < fd->blockcount; j++)
	    {
		super.freeblocknos[super.totalfreeb++]=fd->blockno[j];
		blocks[fd->blockno[j]]->current_size = 0;
	    }
	    if(i == d->filecount-1){
		d->filecount = d->filecount-1;
		printf("File Deleted..\n");
		return 0;}
	    else
		{
		    d->f[i] = d->f[d->filecount-1];
		    d->filecount = d->filecount-1;
		    printf("File Deleted..\n");
		    return 0;
		}
		printf("File Deletion Failed..\n");
	    return -errno;
				
	}

    }

//Read Directory
static int file_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
			 off_t offset, struct fuse_file_info *fi,
			 enum fuse_readdir_flags flags)
{
	
	struct file *ft=(struct file *)malloc(sizeof(struct file));
	struct directry *d=(struct directry*)malloc(sizeof(struct directry));
	int i;
	int flag=searchroot(path,&ft,&d,0);
	filler(buf, ".", NULL, 0,0);
	filler(buf, "..", NULL, 0,0);
	if(flag==0)
	{
		for(i=0;i<fileroot->filecount;i++)
			filler(buf,fileroot->files[i]->fname,NULL,0,0);
		
		struct directry *temp=(struct directry*)malloc(sizeof(struct directry));
		temp=fileroot->dir;
		for(i=0;i<fileroot->dircount;i++)
		{
			filler(buf,temp->dname,NULL,0,0);
			temp=temp->next;
		}
	}
	
	else if(flag==2)
	{
		for(i=0;i<d->filecount;i++)
		{
			filler(buf,d->f[i]->fname,NULL,0,0);
		}
	}
	
	else
		return -ENOENT;

	return 0;
}

//Create File
int file_create(const char *path, mode_t mode,struct fuse_file_info * fi)
{	
	struct file *ft=(struct file *)malloc(sizeof(struct file));
	struct directry *d=(struct directry*)malloc(sizeof(struct directry));
	
	int flag=searchroot(path,&ft,&d,1);

	if(flag==1) 
	{	
		printf("File already exists\n");
		return -errno;
	}
	else if(flag==-1)
	{
		printf("Creating File..\n");
		ft->fil=(struct fileinfo *)malloc(sizeof(struct fileinfo));
		ft->fil->offset=0;
		ft->fil->size=0;
		ft->fil->mode=S_IFREG | mode;
		ft->fil->blockcount=0;
		ft->fil->block_size=BLOCK_SIZE;
		ft->fil->uid=fileroot->uid;
		ft->fil->gid=fileroot->gid;
		ft->fd_count=0;
		
		const char * name=path;
		int count=0;

		while(*name!='\0')
		{
			if(*name=='/')
				count++;
			name++;
		}
		if(count==1 || count==0)
		{

		strcpy(ft->fname,path+1);
		if(fileroot->filecount==0)
		{
			fileroot->files=(struct file **)malloc(sizeof(struct file *)*10);
		}
		
			fileroot->files[fileroot->filecount]=(struct file*)malloc(sizeof(struct file))	;	
			ft->fil->fileId=fileroot->filecount;
			fileroot->files[fileroot->filecount]=ft;
			(fileroot->filecount)++;
			
			printf("Creating File..\n");
			return 0;
		}

		else
		{
		searchroot(path,&ft,&d,0);
		const char *name=path;
		int c=0;
		while(*name != '\0')
		{
			if(*name=='/')
				c++;
			name++;
			if(c==2)
				break;
		}

		strcpy(ft->fname,name);
		if(d->filecount==0)
		{
			d->f=(struct file **)malloc(sizeof(struct file *)*10);
			
		}
		
		
			d->f[d->filecount]=(struct file*)malloc(sizeof(struct file))	;	
			ft->fil->fileId=d->filecount;
			d->f[d->filecount]=ft;
			(d->filecount)++;
			printf("Creating File..\n");
			return 0;
		
		}
		}		
	printf("File Creation Failed..\n");		
	return -errno;	
}
	

//Reading File
static int file_read(const char *path, char *buf, size_t size,off_t offset, struct fuse_file_info *fi)
{
	struct file *ft=(struct file *)malloc(sizeof(struct file));
	struct directry *d=(struct directry*)malloc(sizeof(struct directry));
	
	int flag=searchroot(path,&ft,&d,1);
	
	if(flag==1)
	{
		if(offset>ft->fil->size)
			{
				printf("Can't read. Offset is large\n");
				return 0;
			}
		else
		{
			size_t avail = ft->fil->size - offset; //available size
			
  			 size = (size < avail) ? size : avail; //Takes the smaller size
			if(ft->fil->blockcount>0)
			{
				int i;
				for(i=0;i<ft->fil->blockcount;i++)
				{
					if(offset <= ((BLOCK_SIZE-1)*(i+1))) //gives the block index
						break;
				}
				//Calculating offset within the block with the index returned
				off_t new_off=offset-(BLOCK_SIZE * i);	
			size_t new=size;
			off_t off=0;
			avail=BLOCK_SIZE;
			size_t n = (size < avail) ? size : avail; 	

			while(size>0)
			{
				read_block(ft->fil->blockno[i],buf,new_off,off, n);
				i=i+1;
				off=off+n;
				size=size-n;
				
				avail=BLOCK_SIZE;
				
				
  				n = (size > avail) ? avail : size;
				new_off=0;	
 			}
				ft->fil->offset=offset+new;
				printf("Read complete..\n");
				return new;
			}
			else 
			{	
				printf("Read Failed\n");
				return 0;
			}
		}
	}
		
	else
	{
		printf("Read Failed\n");
		return -errno;
	}
}

//write in the file
static int file_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) 
{
	
	struct file *ft=(struct file *)malloc(sizeof(struct file));
	struct directry *d=(struct directry*)malloc(sizeof(struct directry));

	int flag=searchroot(path,&ft,&d,1);
	if(flag==1)
	{
		//Calculate the required number of blocks to write
		blkcnt_t req_blocks =ceil( (offset + size + BLOCK_SIZE - 1) / BLOCK_SIZE);
		if(req_blocks<=ft->fil->blockcount)
		{
			int i;
			for(i=0;i<ft->fil->blockcount;i++)
			{
				if(offset <= ((BLOCK_SIZE-1)*(i+1)))
					break;
			}
			
			off_t new_off=offset-(BLOCK_SIZE * i);	
			size_t avail = BLOCK_SIZE-new_off;
  			size_t n = (size > avail) ? avail : size;
			size_t new=size;
			off_t off=0;
			int z;
			while(size>0)
			{
				z=write_block(ft->fil->blockno[i],buf,new_off,off, n);
				if(z==-1)
				{	
					printf("No Space");
					return -errno;
				}	
		
				i=i+1;
				off=off+n;
				size=size-n;
				avail = BLOCK_SIZE;
				if(size>0){
				int x=blocks[ft->fil->blockno[i]]->current_size;
				blocks[ft->fil->blockno[i]]->current_size=x+n;
				}
  				n = (size > avail) ? avail : size;
				
					
				new_off=0;	
 			}
			
			//Updating the file size and offset
			if(ft->fil->size<offset+new)
				ft->fil->size=offset+new;
			ft->fil->offset=offset+size;
			return new;
				
		}

		else
		{
			//No. of extra blocks required
			blkcnt_t remain_b=req_blocks-ft->fil->blockcount;
			int i,x,j;
			for(i=0;i<remain_b;i++)
			{
				x=super.freeblocknos[0];
				j=(super.totalfreeb)-1;
				//Take the extra blocks from the freeblocks in superblock
				super.freeblocknos[0]= super.freeblocknos[j];
			
				(super.totalfreeb)--;
				ft->fil->blockno[ft->fil->blockcount]=x;
				(ft->fil->blockcount)++;
					
			}

			for(i=0;i<ft->fil->blockcount;i++)
			{
				if(offset <= ((BLOCK_SIZE)*(i+1)))
					break;
			}
			
			off_t new_off=offset-(BLOCK_SIZE * i);	
			size_t avail = BLOCK_SIZE-new_off;
  			size_t n = (size > avail) ? avail : size;
			size_t new=size;
			off_t off=0;
			int z=0;
			
			
			while(size>0)
			{
				z=write_block(ft->fil->blockno[i],buf,new_off,off, n);
				
				if(z==-1)
				{	
					printf("no space");
					return -errno;
				}	
				
				
				i=i+1;
				off=off+n;
			
				size=size-n;
				avail = BLOCK_SIZE;
				
				if(size>0)
				{
				int x=blocks[ft->fil->blockno[i]]->current_size;
				blocks[ft->fil->blockno[i]]->current_size=x+n;
				}
  				n = (size > avail) ? avail : size;
					
				new_off=0;	
 			}
			
			if(ft->fil->size<offset+new)
				ft->fil->size=offset+new;
			ft->fil->offset=offset+new;
			printf("Write Completed..\n");
			return new;
		}
	
	}		
	else
	{
		printf("Write Failed..\n");
		return -errno;
	}
}

//Destroy the file 
//making it Persistent
void file_destroy(void *private_data)
{
	FILE *d;
	int i,j;
	//Binary file
	d=fopen("/home/tanya/fuse-3.2.1/example/dir1.txt","wb+");
	//Initializing the data structure
	fseek(d,0,SEEK_SET);
	fwrite(fileroot,sizeof(*fileroot),1,d);
		
	struct directry *temp_d;
	temp_d=fileroot->dir;
	
	struct file *temp_f;
	struct fileinfo *temp_fi;
	struct block *temp_b;
	//write root directories and files within them
	for(i=0;i<fileroot->dircount;i++)
	{
		fwrite(temp_d,sizeof(*temp_d),1,d);
		
		for(i=0;i<temp_d->filecount;i++)
		{
			
			temp_f=temp_d->f[i];
			fwrite(temp_f,sizeof(*temp_f),1,d);
			temp_fi=temp_f->fil;
			fwrite(temp_fi,sizeof(*temp_fi),1,d);
			for(j=0;j<temp_fi->blockcount;j++)
			{
				void *temp_da;
				temp_b=blocks[temp_fi->blockno[j]];
				fwrite(temp_b,sizeof(*temp_b),1,d);
				temp_da=temp_b->data;
				fwrite(temp_da,sizeof(*temp_da),BLOCK_SIZE,d);
				free(temp_da);
				
			
			}
		}


		temp_d=temp_d->next;
	}		
	
	//write the superblocks
	fwrite(&super,sizeof(super),1,d);
	
	
	//write only root files
	for(i=0;i<fileroot->filecount;i++)
	{
		
		temp_f=fileroot->files[i];
		fwrite(temp_f,sizeof(*temp_f),1,d);
		temp_fi=temp_f->fil;
		fwrite(temp_fi,sizeof(*temp_fi),1,d);
		for(j=0;j<temp_fi->blockcount;j++)
		{
			void *temp_da;
			temp_b=blocks[temp_fi->blockno[j]];
			fwrite(temp_b,sizeof(*temp_b),1,d);
			temp_da=temp_b->data;
			fwrite(temp_da,sizeof(*temp_da),BLOCK_SIZE,d);
			free(temp_da);
			
		}
	}

	
	fclose(d);
	printf("Write to Binary File Complete..\n");
}
	

int persistent()
{
	FILE *d;
	int i,j;
	
	d=fopen("/home/tanya/fuse-3.2.1/example/dir1.txt","rb+");
	

	fseek(d,0,SEEK_END);
	long int p=ftell(d);

	if(p==0)
	{
		
		fclose(d);
		return 0;
	}
	printf("Restoring original File System..\n");
	fseek(d,0,SEEK_SET);
	fileroot=(struct root *)malloc(sizeof(struct root));
	memset(fileroot, 0, sizeof(struct root));
	
	fread(fileroot,sizeof(*fileroot),1,d);
	
	struct directry *prev;
	
	//read root directories and files within them
	for(i=0;i<fileroot->dircount;i++)
	{
		struct directry *temp_d=(struct directry*)malloc(sizeof(struct directry));
		fread(temp_d,sizeof(*temp_d),1,d);
		if(i==0)
		{
			temp_d->next=NULL;
			fileroot->dir=temp_d;
			prev=temp_d;
			
		}
		else
		{
			
			temp_d->next=NULL;
			prev->next=temp_d;
			prev=temp_d;
		
		}

		temp_d->f=(struct file **)malloc(sizeof(struct file *)*10);
		for(i=0;i<temp_d->filecount;i++)
		{
		
			struct file *temp_f=(struct file*)malloc(sizeof(struct file));
			struct fileinfo *temp_fi=(struct fileinfo*)malloc(sizeof(struct fileinfo));
				
			fread(temp_f,sizeof(*temp_f),1,d);
			fread(temp_fi,sizeof(*temp_fi),1,d);
		
			for(j=0;j<temp_fi->blockcount;j++)
			{
			
				struct block *temp_b=(struct block*)malloc(sizeof(struct block));
				void *temp_da=(void *)malloc(sizeof(char)*BLOCK_SIZE);
				//printf("in b %d\n",temp_fi->blockno[j]);
			
				fread(temp_b,sizeof(*temp_b),1,d);
				fread(temp_da,sizeof(*temp_da),BLOCK_SIZE,d);
			
				temp_b->data=(char *)temp_da;
				blocks[temp_fi->blockno[j]]=temp_b;
			}
		
			temp_f->fil=temp_fi;
			temp_d->f[i]=temp_f;
		
		}
	
		
	}		
	
	//reading superblocks
	fread(&super,sizeof(super),1,d);
	
	//reading root files
	fileroot->files=(struct file **)malloc(sizeof(struct file *)*10);
	for(i=0;i<fileroot->filecount;i++)
	{
		
		struct file *temp_f=(struct file*)malloc(sizeof(struct file));
		struct fileinfo *temp_fi=(struct fileinfo*)malloc(sizeof(struct fileinfo));
				
		fread(temp_f,sizeof(*temp_f),1,d);
		fread(temp_fi,sizeof(*temp_fi),1,d);
		
		for(j=0;j<temp_fi->blockcount;j++)
		{
			
			struct block *temp_b=(struct block*)malloc(sizeof(struct block));
			void *temp_da=(void *)malloc(sizeof(char)*BLOCK_SIZE);
			//printf("in b %d\n",temp_fi->blockno[j]);
			
			fread(temp_b,sizeof(*temp_b),1,d);
			fread(temp_da,sizeof(*temp_da),BLOCK_SIZE,d);
			
			temp_b->data=(char *)temp_da;
			blocks[temp_fi->blockno[j]]=temp_b;
		}
		
		temp_f->fil=temp_fi;
		fileroot->files[i]=temp_f;
		
	}
	
	
	fclose(d);
	printf("File System Restored..\n");
	return 1;
	

}	
	
static void *file_init(struct fuse_conn_info *conn,struct fuse_config *cfg)
{
	
	initialize();
	if(persistent()==0)
	{
	fileroot=(struct root *)malloc(sizeof(struct root));
	memset(fileroot, 0, sizeof(struct root));
	fileroot->uid=getuid();
	fileroot->gid=getgid();
	fileroot->mode=S_IFDIR | 0777;
	fileroot->filecount=0;
	fileroot->dircount=0;
	fileroot->dir=NULL;
	}
	return NULL;
	
}
static struct fuse_operations file_oper = {
  .getattr      = file_getattr,
  .readdir      = file_readdir,
  .mkdir        = file_mkdir,
  .open         = file_open,
  .init		=file_init,
  .read         = file_read,
  .write        = file_write,
  .create    	=file_create,
  .destroy	=file_destroy,
  .rmdir 	=file_rmdir,
  .unlink	=file_unlink
};



int main(int argc, char *argv[]) 
{
 	
	
	umask(0);
	return fuse_main(argc, argv, &file_oper, NULL);
}
		
