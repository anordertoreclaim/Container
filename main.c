#define _GNU_SOURCE
#include <string.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <syscall.h>

static char child_stack[1048576];

static int child_fn(void *arg) {
    printf("Child process's pid by child process's opinion: %d\n", getpid());
    system("mount -t proc proc /proc --make-private");
    printf("Child's pstree:\n");
    system("pstree");
    printf("\n"); 
    system("ifconfig veth1 10.1.1.2/24 up");
    printf("New 'net' namespace:\n");
    system("ip link");
    printf("\n\n");

    system("dd if=/dev/zero of=/fs bs=1k count=100"); 
    system("losetup /dev/loop1 /fs");
    system("mkfs /dev/loop1 100");
    printf("Mount loop device at directory /mnt.\n");
    system("mount /dev/loop1 /mnt --make-private");
    
    printf("Create an exmaple_file at /mnt directory.\n\n");
    system("touch /mnt/example_file");
    printf("Contents of directory /mnt:\n");
    system("ls /mnt"); 
    printf("\n\n");

    printf("Perform benchmarks.\n\n");
    system("sysbench --test=cpu --cpu-max-prime=20000 run");
    system("sysbench --test=fileio --file-total-size=3G --file-test-mode=rndrw --max-time=60 prepare");
    system("sysbench --test=fileio --file-total-size=3G --file-test-mode=rndrw --max-time=60 run");
    system("sysbench --test=fileio --file-total-size=3G --file-test-mode=rndrw --max-time=60 cleanup");
    system("sysbench --test=memory run");
    system("sysbench --num-threads=64 --test=threads run");
    
    system("umount /dev/loop1");
    system("losetup -d /dev/loop1");
    return 0;
}

int main() {
    // create directory for cgroup and prepare it for restriction of cpu usage
    system("sudo mkdir /sys/fs/cgroup/cpu/demo");
    // I do not reduce the performance here, since I need to benchmark my container
    system("echo 100000 > /sys/fs/cgroup/cpu/demo/cpu.cfs_quota_us");
    system("echo 100000 > /sys/fs/cgroup/cpu/demo/cpu.cfs_period_us");

    printf("Original 'net' Namespace:\n");
    system("ip link");
    printf("\n\n");
    int child_pid = clone(child_fn, child_stack + 1048576, CLONE_NEWPID | CLONE_NEWNET | CLONE_NEWNS | SIGCHLD, NULL);
    char child_pid_str[100];
    snprintf(child_pid_str, 100, "%d", child_pid);

    char net_command[100];
    char cgroup_command[100];
    strcpy(net_command, "ip link add name veth0 type veth peer name veth1 netns "); 
    strcpy(cgroup_command, "echo ");
    strcat(cgroup_command, child_pid_str);
    strcat(cgroup_command, " > /sys/fs/cgroup/cpu/demo/tasks");
    // add process to cgroup
    system(cgroup_command);
 
    strcat(net_command, child_pid_str);
    system(net_command);
    system("ifconfig veth0 10.1.1.1/24 up"); 
    
    waitpid(child_pid, NULL, 0);
    // remove cgroup 
    system("rmdir /sys/fs/cgroup/cpu/demo");
    _exit(EXIT_SUCCESS);
}
