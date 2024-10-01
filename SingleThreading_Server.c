#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <arpa/inet.h>
#include <unistd.h>

#define PROC_DIR "/proc"
#define STAT_FILE "/stat"
#define MAX_PROCESSES 1024
#define PORT 8081
#define MAX_BUFFER_SIZE 1024

typedef struct {
    int pid;
    char comm[256];
    unsigned long utime;
    unsigned long stime;
    unsigned long total_time;
} ProcessInfo;

int read_process_stat(const char *pid, ProcessInfo *pinfo) {
    char stat_path[256];
    snprintf(stat_path, sizeof(stat_path), PROC_DIR "/%s" STAT_FILE, pid);
    
    FILE *stat_file = fopen(stat_path, "r");
    if (stat_file == NULL) {
        return -1; 
    }
    
    fscanf(stat_file, "%d %s %*c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %lu %lu", 
           &pinfo->pid, pinfo->comm, &pinfo->utime, &pinfo->stime);
    
    pinfo->total_time = pinfo->utime + pinfo->stime;
    
    fclose(stat_file);
    return 0;
}

int compare_process(const void *a, const void *b) {
    ProcessInfo *p1 = (ProcessInfo *)a;
    ProcessInfo *p2 = (ProcessInfo *)b;
    return (p2->total_time - p1->total_time);
}

void get_top_cpu_processes(char *result) {
    DIR *dir = opendir(PROC_DIR);
    if (dir == NULL) {
        perror("opendir");
        snprintf(result, MAX_BUFFER_SIZE, "Error opening /proc directory");
        return;
    }

    struct dirent *entry;
    ProcessInfo processes[MAX_PROCESSES];
    int process_count = 0;

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR && atoi(entry->d_name) > 0) {
            if (read_process_stat(entry->d_name, &processes[process_count]) == 0) {
                process_count++;
            }
        }
    }

    closedir(dir);
    qsort(processes, process_count, sizeof(ProcessInfo), compare_process);
    
    snprintf(result, MAX_BUFFER_SIZE, "Top CPU-consuming processes:\n");
    for (int i = 0; i < 2 && i < process_count; i++) {
        char temp[1024];
        snprintf(temp, sizeof(temp), "PID: %d, Name: %.255s, User Time: %lu, Kernel Time: %lu, Total CPU Time: %lu\n", 
                 processes[i].pid, processes[i].comm, processes[i].utime, processes[i].stime, processes[i].total_time);
        strncat(result, temp, MAX_BUFFER_SIZE - strlen(result) - 1);  
    }
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, MAX_PROCESSES) < 0) {  
        perror("listen");
        exit(EXIT_FAILURE);
    }
    
    printf("Server is listening on port %d\n", PORT);
    
    while (1) {
        new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
        if (new_socket < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        printf("Connection accepted from client IP: %s, Port: %d\n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));

        char buffer[1024] = {0};
        int valread = read(new_socket, buffer, 1024);
        printf("Received: %s\n", buffer + valread);
        
        char process_info[MAX_BUFFER_SIZE] = {0};
        get_top_cpu_processes(process_info);
        send(new_socket, process_info, strlen(process_info), 0);

        close(new_socket);
    }

    close(server_fd); 
    return 0;
}
