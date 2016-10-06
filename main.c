#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <sched.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/mount.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/syscall.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>

#define BRIDGE "br0"
const char *hostname = "container01";
const char *domainname = "container";

int network_setup(pid_t pid);
int create_peer();

//wrapper for pivot root syscall
int pivot_root(char *a,char *b)
{
	if (mount(a,a,"bind",MS_BIND | MS_REC,"")<0){
		printf("error mount: %s\n",strerror(errno));
	}
	if (mkdir(b,0755) <0){
		printf("error mkdir %s\n",strerror(errno));
	}
	printf("pivot setup ok\n");

	return syscall(SYS_pivot_root,a,b);
}

int child_exec(void *arg)
{
	int err =0;
	char **commands = (char **)arg;
	char **cmd_arg = commands + 1;	
		
	printf("child...%s\n",cmd_arg[0]);
	if(unshare(CLONE_NEWNS) <0)
		printf("unshare issue?\n");
	if (umount2("/proc",MNT_DETACH) <0)
		printf("error unmount: %s\n",strerror(errno));
	if (pivot_root("./busy","./busy/.old")<0){
		printf("error pivot: %s\n",strerror(errno));
	}
	mount("tmpfs","/dev","tmpfs",MS_NOSUID | MS_STRICTATIME,NULL);
	if (mount("proc", "/proc", "proc",0, NULL) <0)
		printf("error proc mount: %s\n",strerror(errno));
	mount("t", "/sys", "sysfs", 0, NULL);

	chdir("/"); //change to root dir, man page for pivot_root suggests this
	if( umount2("/.old",MNT_DETACH)<0)
		printf("error unmount old: %s\n",strerror(errno));

	//set new system info
	setdomainname(domainname, strlen(domainname));	
	sethostname(hostname,strlen(hostname));	
	
	system("ip link set veth1 up");
        
	char *ip_cmd;
	asprintf(&ip_cmd,"ip addr add %s/24 dev veth1",commands[0]);
	system(ip_cmd);
	system("route add default gw 172.16.0.100 veth1");	
	execvp(cmd_arg[0],cmd_arg);	
	return 0;
}


int main(int argc, char *argv[])
{
	int err =0;
	char c_stack[1024];	
	char **args = &argv[1];
	srand(time(0));	


	unsigned int flags =  SIGCHLD | CLONE_NEWNS | CLONE_NEWPID | CLONE_NEWUTS | CLONE_NEWNET;
	system("mount --make-rprivate  /");
	printf("starting...\n");
	create_peer();
	pid_t pid = clone(child_exec,c_stack, flags ,args);
	if(pid<0)
		fprintf(stderr, "clone failed %s\n", strerror(errno));
	network_setup(pid);
	

	waitpid(pid,NULL,0);
	return err;
}

void rand_char(char *str,int size)
{
	char new[size];
	for(int i=0;i<size;i++){
		new[i] = 'A' + (rand() % 26);
	}
	new[size] = '\0';
	strncpy(str,new,size);
	return;
}

int create_peer()
{
 	char *id = (char*) malloc(4);
        char *set_int;
	char *set_int_up;
	char *add_to_bridge;

        rand_char(id,4);            
        printf("id is %s\n",id);
        asprintf(&set_int,"ip link add veth%s type veth peer name veth1",id);
        system(set_int);
	asprintf(&set_int_up,"ip link set veth%s up",id);
	system(set_int_up);
	asprintf(&add_to_bridge,"brctl addif %s veth%s",BRIDGE,id);
	system(add_to_bridge);

	free(id);
	return 0;
}

int network_setup(pid_t pid)
{
	char *set_pid_ns;
	asprintf(&set_pid_ns,"ip link set veth1 netns %d",pid);
	system(set_pid_ns);
	return 0;
}
