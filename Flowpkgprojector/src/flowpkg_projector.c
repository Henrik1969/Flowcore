#define _POSIX_C_SOURCE 200809L

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

typedef struct { char *relative; char *source; int directory; int create; } Entry;
typedef struct { Entry *items; size_t used, size; } Entries;

static void fail(const char *format, ...) {
    va_list arguments; va_start(arguments, format);
    fputs("FLOWPKG_PROJECTOR_ERROR ", stderr); vfprintf(stderr, format, arguments);
    fputc('\n', stderr); va_end(arguments); exit(2);
}
static char *copy_string(const char *value) { char *p = strdup(value); if (!p) fail("out of memory"); return p; }
static char *join(const char *a, const char *b) {
    size_t al = strlen(a), bl = strlen(b); int slash = al && a[al-1] != '/';
    char *p = malloc(al + slash + bl + 1); if (!p) fail("out of memory");
    memcpy(p,a,al); if(slash)p[al++]='/'; memcpy(p+al,b,bl+1); return p;
}
static void append(Entries *entries, const char *relative, const char *source, int directory) {
    if (entries->used == entries->size) { entries->size = entries->size ? entries->size*2 : 256; entries->items = realloc(entries->items, entries->size*sizeof *entries->items); if(!entries->items)fail("out of memory"); }
    entries->items[entries->used++] = (Entry){copy_string(relative), source?copy_string(source):NULL, directory, 0};
}
static int names(const struct dirent **a, const struct dirent **b) { return strcmp((*a)->d_name,(*b)->d_name); }
static void scan_tree(const char *root, const char *relative, Entries *entries) {
    char *directory = relative[0] ? join(root, relative) : copy_string(root);
    struct dirent **list = NULL; int count = scandir(directory, &list, NULL, names);
    if (count < 0) fail("scan %s: %s", directory, strerror(errno));
    for (int i=0;i<count;i++) {
        const char *name=list[i]->d_name; if(!strcmp(name,".")||!strcmp(name,"..")){free(list[i]);continue;}
        char *child = relative[0] ? join(relative,name) : copy_string(name);
        char *source = join(root,child); struct stat status;
        if (lstat(source,&status)!=0) fail("stat %s: %s",source,strerror(errno));
        if (S_ISDIR(status.st_mode)) { append(entries,child,NULL,1); scan_tree(root,child,entries); }
        else append(entries,child,source,0);
        free(source); free(child); free(list[i]);
    }
    free(list); free(directory);
}
static char *destination(const char *target, const char *relative) { return join(target,relative); }
static int path_exists(const char *path, struct stat *status) { if(lstat(path,status)==0)return 1; if(errno==ENOENT)return 0; fail("stat %s: %s",path,strerror(errno)); return 0; }
static void require_normalized(const char *label,const char *path){size_t length=strlen(path);if(path[0]!='/'||strstr(path,"/../")||strstr(path,"/./")||(length>1&&path[length-1]=='/'))fail("%s must be a normalized absolute path",label);}
static void write_line(FILE *file,const char *format,...){va_list a;va_start(a,format);if(vfprintf(file,format,a)<0)fail("write plan");va_end(a);}
static void sync_file(FILE *file){if(fflush(file)!=0||fsync(fileno(file))!=0)fail("sync transaction: %s",strerror(errno));}
static void write_text(const char *path,const char *text){FILE*f=fopen(path,"w");if(!f)fail("open %s: %s",path,strerror(errno));write_line(f,"%s\n",text);sync_file(f);if(fclose(f)!=0)fail("close %s",path);}
static void progress(const char *phase,size_t done,size_t total){if(done==total||done==1||done%1000==0){fprintf(stderr,"FLOWPKG_PROJECTOR_PROGRESS phase=%s done=%zu total=%zu\n",phase,done,total);}}

static void preflight(const char *target, Entries *entries) {
    for(size_t i=0;i<entries->used;i++){
        Entry *entry=&entries->items[i]; char *dest=destination(target,entry->relative); struct stat status;
        int exists=path_exists(dest,&status);
        if(entry->directory){if(exists&&!S_ISDIR(status.st_mode))fail("directory collision %s",dest);entry->create=!exists;}
        else if(exists)fail("path collision %s",dest);
        free(dest);
    }
}
static void remove_prepared(const char *object,const char *target,const char *incoming,Entries *entries,int strict){
    for(size_t n=entries->used;n>0;n--){Entry*e=&entries->items[n-1];if(e->directory)continue;char*d=destination(target,e->relative);struct stat s;
        if(path_exists(d,&s)){char linkbuf[PATH_MAX+1];ssize_t length=S_ISLNK(s.st_mode)?readlink(d,linkbuf,PATH_MAX):-1;if(length<0){if(strict)fail("recovery ownership changed %s",d);}else{linkbuf[length]=0;if(strcmp(linkbuf,e->source)){if(strict)fail("recovery target changed %s",d);}else if(unlink(d)!=0)fail("unlink %s: %s",d,strerror(errno));}}free(d);}
    for(size_t n=entries->used;n>0;n--){Entry*e=&entries->items[n-1];if(!e->directory||!e->create)continue;char*d=destination(target,e->relative);if(rmdir(d)!=0&&errno!=ENOENT&&errno!=ENOTEMPTY)fail("rmdir %s: %s",d,strerror(errno));free(d);}
    (void)object; if(rmdir(incoming)!=0&&errno!=ENOENT) { /* plan files remain and are evidence */ }
}
static void write_plans(const char *incoming,const char *object,Entries *entries){
    char *links=join(incoming,"links.list"),*dirs=join(incoming,"dirs.list"),*obj=join(incoming,"object"),*state=join(incoming,"state");
    FILE*lf=fopen(links,"w"),*df=fopen(dirs,"w");if(!lf||!df)fail("open transaction plans");
    for(size_t i=0;i<entries->used;i++){Entry*e=&entries->items[i];if(e->directory&&e->create)write_line(df,"%s\n",e->relative);else if(!e->directory)write_line(lf,"%s\t%s\n",e->relative,e->source);}
    sync_file(lf);sync_file(df);fclose(lf);fclose(df);write_text(obj,object);write_text(state,"prepared");free(links);free(dirs);free(obj);free(state);
}
static void project(const char*object,const char*target,const char*projection){
    char *root=join(object,"root"),*incoming=malloc(strlen(projection)+10);if(!incoming)fail("out of memory");sprintf(incoming,"%s.incoming",projection);
    struct stat s;if(!path_exists(root,&s)||!S_ISDIR(s.st_mode))fail("object root absent %s",root);if(path_exists(projection,&s)||path_exists(incoming,&s))fail("projection transaction already exists");
    Entries entries={0};scan_tree(root,"",&entries);preflight(target,&entries);
    if(mkdir(incoming,0755)!=0)fail("mkdir %s: %s",incoming,strerror(errno));
    write_plans(incoming,object,&entries);
    size_t dirs=0,links=0;for(size_t i=0;i<entries.used;i++)if(entries.items[i].directory&&entries.items[i].create)dirs++;else if(!entries.items[i].directory)links++;
    size_t done=0;for(size_t i=0;i<entries.used;i++){Entry*e=&entries.items[i];if(!e->directory||!e->create)continue;char*d=destination(target,e->relative);if(mkdir(d,0755)!=0)fail("mkdir %s: %s",d,strerror(errno));free(d);progress("directories",++done,dirs);}
    done=0;for(size_t i=0;i<entries.used;i++){Entry*e=&entries.items[i];if(e->directory)continue;char*d=destination(target,e->relative);if(symlink(e->source,d)!=0)fail("symlink %s: %s",d,strerror(errno));free(d);progress("links",++done,links);
#ifdef FLOWPKG_PROJECTOR_TESTING
        const char*stop=getenv("FLOWPKG_PROJECTOR_TEST_STOP_AFTER");if(stop&&(size_t)strtoull(stop,NULL,10)==done)_exit(99);
#endif
    }
    char*state=join(incoming,"state");write_text(state,"active");free(state);if(rename(incoming,projection)!=0)fail("promote transaction: %s",strerror(errno));
    printf("FLOWPKG_PROJECTOR_PROJECT_PASS directories=%zu links=%zu\n",dirs,links);free(root);free(incoming);
}
static void load_projection(const char*projection,const char*object,Entries*entries){
    char*links=join(projection,"links.list"),*dirs=join(projection,"dirs.list"),*object_file=join(projection,"object");FILE*df=fopen(dirs,"r");char*line=NULL;size_t cap=0;ssize_t n;
    FILE*of=fopen(object_file,"r");if(!of||getline(&line,&cap,of)<1)fail("projection object identity absent");fclose(of);line[strcspn(line,"\r\n")]=0;if(strcmp(line,object))fail("projection object identity mismatch");
    if(!df)fail("open %s",dirs);
    while((n=getline(&line,&cap,df))>0){if(line[n-1]=='\n')line[n-1]=0;append(entries,line,NULL,1);entries->items[entries->used-1].create=1;}fclose(df);
    FILE*lf=fopen(links,"r");if(!lf)fail("open %s",links);while((n=getline(&line,&cap,lf))>0){if(line[n-1]=='\n')line[n-1]=0;char*tab=strchr(line,'\t');if(!tab)fail("malformed links plan");*tab++=0;append(entries,line,tab,0);}fclose(lf);free(line);free(links);free(dirs);free(object_file);
}
static void validate_links(const char*target,Entries*entries){for(size_t i=0;i<entries->used;i++){Entry*e=&entries->items[i];if(e->directory)continue;char*d=destination(target,e->relative),buf[PATH_MAX+1];ssize_t n=readlink(d,buf,PATH_MAX);if(n<0)fail("owned link absent or changed %s",d);buf[n]=0;if(strcmp(buf,e->source))fail("owned link target changed %s",d);free(d);}}
static void rollback(const char*object,const char*target,const char*projection,const char*archive){struct stat s;if(!path_exists(projection,&s)||!S_ISDIR(s.st_mode))fail("projection inactive");if(path_exists(archive,&s))fail("archive exists");Entries entries={0};load_projection(projection,object,&entries);validate_links(target,&entries);
    size_t links=0,dirs=0;for(size_t i=0;i<entries.used;i++)if(entries.items[i].directory)dirs++;else links++;size_t done=0;
    for(size_t i=0;i<entries.used;i++){Entry*e=&entries.items[i];if(e->directory)continue;char*d=destination(target,e->relative);if(unlink(d)!=0)fail("unlink %s",d);free(d);progress("rollback-links",++done,links);}done=0;
    for(size_t n=entries.used;n>0;n--){Entry*e=&entries.items[n-1];if(!e->directory)continue;char*d=destination(target,e->relative);if(rmdir(d)!=0&&errno!=ENOTEMPTY)fail("rmdir %s: %s",d,strerror(errno));free(d);progress("rollback-directories",++done,dirs);}
    if(rename(projection,archive)!=0)fail("archive projection: %s",strerror(errno));
    printf("FLOWPKG_PROJECTOR_ROLLBACK_PASS directories=%zu links=%zu\n",dirs,links);
}
static void recover(const char*object,const char*target,const char*projection){char*incoming=malloc(strlen(projection)+10);sprintf(incoming,"%s.incoming",projection);struct stat s;if(!path_exists(incoming,&s))fail("no interrupted transaction");Entries entries={0};load_projection(incoming,object,&entries);remove_prepared(object,target,incoming,&entries,1);
    char archive[PATH_MAX];snprintf(archive,sizeof archive,"%s.interrupted.%ld",projection,(long)time(NULL));if(rename(incoming,archive)!=0)fail("archive interruption: %s",strerror(errno));printf("FLOWPKG_PROJECTOR_RECOVER_PASS\n");free(incoming);}
int main(int argc,char**argv){if(argc<2)fail("usage");
    if(argc>=5){require_normalized("object",argv[2]);require_normalized("target root",argv[3]);require_normalized("projection",argv[4]);if(!strcmp(argv[3],"/")&&geteuid()!=0)fail("root projection requires root");}
    if(!strcmp(argv[1],"project")&&argc==5){project(argv[2],argv[3],argv[4]);return 0;}
    if(!strcmp(argv[1],"rollback")&&argc==6){require_normalized("archive",argv[5]);rollback(argv[2],argv[3],argv[4],argv[5]);return 0;}
    if(!strcmp(argv[1],"recover")&&argc==5){recover(argv[2],argv[3],argv[4]);return 0;}fail("usage");return 2;}
