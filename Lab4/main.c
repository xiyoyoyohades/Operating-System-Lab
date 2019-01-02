#define _GNU_SOURCE

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<time.h>
#include<dirent.h>
#include<grp.h>
#include<pwd.h>
#include<libgen.h>
#include<locale.h>

#include<sys/stat.h>
#include<sys/types.h>
#include<sys/cdefs.h>
#include<bsd/string.h>

time_t current_time;

int dir_entry_compare(const void *a, const void *b) {
    struct dirent *dir_a = (struct dirent *)a;
    struct dirent *dir_b = (struct dirent *)b;

    // sort according to current locale
    return strcoll(dir_a->d_name, dir_b->d_name);
}

int print_entry_in_dir(struct dirent *entry, int LinkWidth, int SizeWidth) {
    struct stat status;

    // read and print status
    if (stat(entry->d_name, &status) == 0) {
        // get permission (strmode will append a space after the string)
        char mode_str[13];
        strmode(status.st_mode, mode_str);       
        //bsd util,converts a file mode (the type and permission information associated with an inode) into a symbolic string

        // get username and groupname
        struct passwd *entry_passwd=getpwuid(status.st_uid);
        struct group *entry_group=getgrgid(status.st_gid);

        struct tm *timeTemp = localtime(&(status.st_mtime));
        char time_str[20];

        if (current_time - status.st_mtime > (365 * 24 * 60 * 60 / 2) || current_time < status.st_mtime) {
            strftime(time_str, 20, "%b %e  %Y", timeTemp);      //format time:month,day,year
        } else {												//%e works for "day", but without the leading zero
            strftime(time_str, 20, "%b %e %H:%M", timeTemp);    //format time:month,day,hour,minute
        }

        // print permission, hardlink-number, username, group name, size, modify-date, name
        printf("%s%*ld %s %s %*ld %s %s\n",
                mode_str,
                LinkWidth, status.st_nlink,
                entry_passwd->pw_name,
                entry_group->gr_name,
                SizeWidth, status.st_size,
                time_str,
                entry->d_name);
        return 0;
    }
    return 1;
}

int print_dir(char* dir, int depth){
    current_time=time(NULL);
    unsigned long total_block_num = 0;
    DIR *current_dir = NULL;

    char *dir_backup = NULL;
    long path_size = pathconf(".", _PC_PATH_MAX);	//get dir size
    dir_backup = getcwd(dir_backup, path_size);		//get working dir

    //char *dircopy = malloc(strlen(dir) + 1);
    //strcpy(dircopy, dir);
    //char *dirBase = basename(dircopy);
    //struct stat statusTemp;
    //stat(dirBase, &statusTemp);
    //printf("%s:\n", dir);
    if ((current_dir = opendir(dir)) == NULL) {
        perror("Failed to open directory");
        return 1;
    }
    chdir(dir);
    printf("%s:\n", dir);

    struct dirent *entries=NULL;
    struct dirent *tmp_entry=NULL;
    size_t entry_num = 0;
    while ((tmp_entry = readdir(current_dir))){		//count entry number
        entry_num++;
    }
        
    // read all entries, add to entries
    entries = (struct dirent *)malloc(entry_num * sizeof(struct dirent));
    rewinddir(current_dir);
    for (int i = 0; (tmp_entry = readdir(current_dir)); i++){
        memcpy(entries + i, tmp_entry, sizeof(struct dirent));
    }
    qsort(entries, entry_num, sizeof(struct dirent), dir_entry_compare);		//sort according to name

    __nlink_t max_link_num = 0;						//aka unsigned short
    __off_t max_size = 0;							//aka unsigned long
   
    struct stat status;
    for (int i = 0; i < entry_num; i++) {
        tmp_entry = entries + i;
        stat(tmp_entry->d_name, &status);
        // update max size
        if (status.st_size > max_size){
            max_size = status.st_size;
        }
            
        // update total block size

        if (tmp_entry->d_name[0] != '.'             	 // skip hidden file  || strcmp(tmp_entry->d_name, ".") || strcmp(tmp_entry->d_name, ".." )
            && strcmp(tmp_entry->d_name, ".") &&         // skip current file
            strcmp(tmp_entry->d_name, "..")              // skip upper file
            ) {       

            total_block_num += status.st_blocks;
            // update max link-num
            if (status.st_nlink > max_link_num){
                max_link_num = status.st_nlink;
            }
                
        }

    }

    int sizeWidth = 0, linkWidth = 0;
    while(max_link_num){
        linkWidth++;
        max_link_num /= 10;
    }
    while(max_size){
        sizeWidth++;
        max_size /= 10;
    }
    printf("total %lu\n", total_block_num * 512 / 1024 );	//unit: kbytes, block size is 512bytes

//print entry info
    rewinddir(current_dir);
    for (int i = 0; i < entry_num; i++) {
        tmp_entry = entries + i;

        if (strcmp(tmp_entry->d_name, ".") &&           // skip current file
            strcmp(tmp_entry->d_name, "..") &&          // skip upper file
            tmp_entry->d_name[0] != '.'                 // skip hidden file
            //tmp_entry->d_name[0] != '.' || strcmp(tmp_entry->d_name, ".") || strcmp(tmp_entry->d_name, ".." ) 
            ){               
                print_entry_in_dir(tmp_entry, linkWidth, sizeWidth);
        }                
            
    }
    
    chdir(dir_backup);		//back to original dir in case using relative path
    

//recurse into sub-dir
    rewinddir(current_dir);
    for (int i = 0; i < entry_num; i++) {
        tmp_entry = entries + i;
        if (tmp_entry->d_type == DT_DIR &&			   //should be a dir
            tmp_entry->d_name[0] != '.' &&           // skip hidden dir
            strcmp(tmp_entry->d_name, ".") &&          // skip current dir
            strcmp(tmp_entry->d_name, "..") ) {        // skip upper dir

            // construct path
            char *dir_name = (char *)malloc(strlen(dir) + strlen(tmp_entry->d_name) + 2);
            strcpy(dir_name, dir);
            if (dir[strlen(dir) - 1] != '/')    strcat(dir_name, "/");
            strcat(dir_name, tmp_entry->d_name);

            // recurse into next directory
            putchar('\n');
            //printf("%s\n",dir_name);
            print_dir(dir_name, depth + 1);
            free(dir_name);
        }
    }

    closedir(current_dir);
    chdir(dir_backup);      //back to original dir
	free(entries);
    free(dir_backup);
    
    return 0;
        

    
}

int main(int argc, char *argv[]){
    if (argc != 2) {
        printf("Usage %s <directory or file>\n", argv[0]);
        return 1;
    }
    setlocale(LC_COLLATE, "");
    char *path = malloc(strlen(argv[1]) + 1);
    strcpy(path, argv[1]);
    print_dir(path, 0);
    free(path);
	return 0;
}
