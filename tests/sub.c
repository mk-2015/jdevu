#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <devu/ioctl.h>

int main(void) {
    int fd;
    struct ipchub_reg reg;
    const char *topic_name = "telemetry";
    struct pollfd pfd;

    fd = open("/dev/ipchub", O_RDONLY);
    if (fd < 0) {
        perror("Failed to open /dev/ipchub");
        return 1;
    }

    /* Register as a subscriber */
    reg.type = IPCHUB_TYPE_SUB;
    strncpy(reg.topic, topic_name, MAX_TOPIC_LEN);
    reg.topic[MAX_TOPIC_LEN - 1] = '\0';

    if (ioctl(fd, IPCHUB_IOC_REGISTER, &reg) < 0) {
        perror("ioctl IPCHUB_IOC_REGISTER failed for subscriber");
        close(fd);
        return 1;
    }

    printf("Subscriber listening on topic '%s'...\n", topic_name);

    pfd.fd = fd;
    pfd.events = POLLIN;

    /* Listen for events using poll */
    while (1) {
        int ret = poll(&pfd, 1, 5000); // 5-second timeout
        if (ret < 0) {
            perror("Poll failed");
            break;
        } else if (ret == 0) {
            printf("Timeout waiting for messages...\n");
            continue;
        }

        if (pfd.revents & POLLIN) {
            ipchub_event_t event;
            ssize_t bytes = read(fd, &event, sizeof(event));
            
            if (bytes > 0) {
                printf("[EVENT RECEIVED] PID: %d | UID: %u | GID: %u | Msg: %s\n",
                       (int)event.pid, event.uid, event.gid, event.payload);
            }
        }
    }

    close(fd);
    return 0;
}
