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
    // sleep 2 seconds so that prints do not get messed up
    sleep(2);
    printf("Clone internal pid: %ld\n", (long) getpid());
    pid_t child_pid = fork();

    if (child_pid) {
        printf("Clone fork child pid: %ld\n\n", (long) child_pid);
        // Mount namespace
        system("mount --make-rprivate -o remount /");
        system("mount -t proc proc /proc --make-private");
        printf("Child's pstree:\n");
        system("pstree");
        printf("\n");
       
        // Adjust networkinterface 
        system("ifconfig veth1 10.1.1.2/24 up");
        printf("New 'net' namespace:\n");
        system("ip link");
        printf("\n\n");
        
        // Create loop device and mount it
        system("dd if=/dev/zero of=/fs bs=1k count=100"); 
        system("losetup /dev/loop1 /fs");
        system("mkfs /dev/loop1 100");
        printf("Mount loop device at directory /mnt.\n");
        system("mount /dev/loop1 /mnt --make-private");
    
        printf("Create an exmaple_file at /mnt directory.\n\n");
        system("touch /mnt/example_file");
        printf("Contents of directory /mnt:\n");
        system("ls /mnt"); 
        printf("\n");
        
        sleep(2);
        system("bash");
 
        // Unmount loop device 
        system("umount /dev/loop1");
        system("losetup -d /dev/loop1");
        system("umount /proc");
    }

    return 0;
}

int main() {
    // Create directory for cgroup and prepare it for restriction of cpu usage
    system("sudo mkdir /sys/fs/cgroup/cpu/demo");
    // I do not reduce the performance here, since I need to benchmark my container
    system("echo 100000 > /sys/fs/cgroup/cpu/demo/cpu.cfs_quota_us");
    system("echo 100000 > /sys/fs/cgroup/cpu/demo/cpu.cfs_period_us");

    pid_t child_pid = clone(child_fn, child_stack + 1048576, CLONE_NEWPID | CLONE_NEWNET | CLONE_NEWNS | SIGCHLD, NULL);
    printf("Child's pid from opinion of parent is %ld\n\n", (long) child_pid);
    char child_pid_str[100];
    snprintf(child_pid_str, 100, "%ld", (long) child_pid);

    char net_command[100];
    char cgroup_command[100];
    strcpy(net_command, "ip link add name veth0 type veth peer name veth1 netns "); 
    strcat(net_command, child_pid_str);
    system(net_command);
    system("ifconfig veth0 10.1.1.1/24 up");
    printf("Original 'net' namespace:\n");
    system("ip link");
    printf("\n");

    strcpy(cgroup_command, "echo ");
    strcat(cgroup_command, child_pid_str);
    strcat(cgroup_command, " > /sys/fs/cgroup/cpu/demo/tasks");
    // Add process to cgroup
    system(cgroup_command);
    
    sleep(5);
    printf("\nContents of /mnt directory from a point of view of host:\n");
    system("ls /mnt");
 
    waitpid(child_pid, NULL, 0);
    // Remove cgroup 
    system("rmdir /sys/fs/cgroup/cpu/demo");
    _exit(EXIT_SUCCESS);
}
