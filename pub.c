#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <devu/ioctl.h>

int main(void) {
    int fd;
    struct ipchub_reg reg;
    const char *topic_name = "telemetry";
    int count = 0;

    fd = open("/dev/ipchub", O_RDWR);
    if (fd < 0) {
        perror("Failed to open /dev/ipchub");
        return 1;
    }

    /* Register as a publisher */
    reg.type = IPCHUB_TYPE_PUB;
    strncpy(reg.topic, topic_name, MAX_TOPIC_LEN);
    reg.topic[MAX_TOPIC_LEN - 1] = '\0';

    if (ioctl(fd, IPCHUB_IOC_REGISTER, &reg) < 0) {
        perror("ioctl IPCHUB_IOC_REGISTER failed for publisher");
        close(fd);
        return 1;
    }

    printf("Publisher registered on topic '%s'. Sending messages...\n", topic_name);

    while (count < 5) {
        char msg[128];
        int len = snprintf(msg, sizeof(msg), "Hello #%d from PID %d", count, getpid());

        if (write(fd, msg, len + 1) < 0) {
            perror("Write failed");
            break;
        }

        printf("Published: '%s'\n", msg);
        sleep(1);
        count++;
    }

    close(fd);
    printf("Publisher closed.\n");
    return 0;
}
