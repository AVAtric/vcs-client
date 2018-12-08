#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <simple_message_client_commandline_handling.h>
#include <unistd.h>

void usage(FILE *stream, const char *cmnd, int exitcode);

int connectToServer(const char *server, const char *port);

int send_req(FILE *write_fd, int sfd, const char *user, const char *message, const char *img_url);

int read_resp(FILE *read_fd);

int main(const int argc, const char *const argv[]) {

    const char *server;
    const char *port;
    const char *user;
    const char *message;
    const char *img_url;
    int verbose;
    int sfd;
    FILE *write_fd = NULL;
    FILE *read_fd = NULL;

    smc_parsecommandline(argc, argv, usage, &server, &port, &user, &message, &img_url, &verbose);

    printf("Server:  %s\n", server);
    printf("Port:    %s\n", port);
    printf("User:    %s\n", user);
    printf("Message: %s\n", message);
    printf("Img_url: %s\n", img_url);
    printf("Verbose: %i\n", verbose);

    sfd = connectToServer(server, port);

    write_fd = fdopen(sfd, "w");
    if (write_fd == NULL) {
        fprintf(stderr, "Could not open write fd\n");
        fclose(write_fd);
        exit(EXIT_FAILURE);
    }

    send_req(write_fd, sfd, user, message, img_url);
    shutdown(sfd, SHUT_WR);

    read_fd = fdopen(sfd, "r");
    if (read_fd == NULL) {
        fprintf(stderr, "Could not open read fd\n");
        fclose(write_fd);
        exit(EXIT_FAILURE);
    }

    read_resp(read_fd);

    fclose(write_fd);
    fclose(read_fd);
    //close(sfd);

    printf("SUCCCESSS\n");

    return 0;
}

int connectToServer(const char *server, const char *port) {

    int sfd, s;
    struct addrinfo hints;
    struct addrinfo *result, *rp;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC; /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM; /* Stream socket */

    s = getaddrinfo(server, port, &hints, &result);
    if (s != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        exit(EXIT_FAILURE);
    }

    for (rp = result; rp != NULL; rp = rp->ai_next) {
        sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sfd == -1)
            continue;
        if (connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1)
            break; /* Success */

    }

    if (rp == NULL) { /* No address succeeded */
        fprintf(stderr, "Could not connect\n");
        exit(EXIT_FAILURE);
    }

    freeaddrinfo(result); /* No longer needed */
    return sfd;
}

void usage(FILE *stream, const char *cmnd, int exitcode) {
    fprintf(stream, "usage: %s options\n", cmnd);
    fprintf(stream, "options:\n");
    fprintf(stream, "        -s, --server <server>   full qualified domain name or IP address of the server\n");
    fprintf(stream, "        -p, --port <port>       well-known port of the server [0..65535]\n");
    fprintf(stream, "        -u, --user <name>       name of the posting user\n");
    fprintf(stream, "        -i, --image <URL>       URL pointing to an image of the posting user\n");
    fprintf(stream, "        -m, --message <message> message to be added to the bulletin board\n");
    fprintf(stream, "        -v, --verbose           verbose output\n");
    fprintf(stream, "        -h, --help\n");
    exit(exitcode);
}

int send_req(FILE *write_fd, int sfd, const char *user, const char *message, const char *img_url) {

    const char *pre_user = "user=";
    const char *pre_message = "\n";
    const char *pre_img_url;


    if (img_url != NULL) {
        pre_img_url = "\nimg=";
    } else {
        pre_img_url = "";
        img_url = "";
    }

    //size_t msg_length = strlen(pre_user) + strlen(user) + strlen(pre_img_url) + strlen(img_url) + strlen(pre_message) + strlen(message) + 1;
    //char *formatedString =  calloc(msg_length, sizeof(char));
    //snprintf(formatedString,msg_length, "%s%s%s%s%s%s",pre_user,user,pre_img_url,img_url,pre_message,message);
    //fprintf(write_fd, "%s",formatedString);
    //free(formatedString);

    fprintf(write_fd, "%s%s%s%s%s%s", pre_user, user, pre_img_url, img_url, pre_message, message);

    fflush(write_fd);

    return 0;

}

int read_resp(FILE *read_fd) {
    const int buffersize = 200;
    char *line = NULL;
    char buffer[buffersize];
    char *file_name = NULL;
    long status = -1;
    long file_len = 0;
    long counter = 0;
    long toprocess = 0;
    size_t len = 0;
    FILE *fp = NULL;

    if ((getline(&line, &len, read_fd)) != -1) {
        printf("line: %s", line);
        strtok(line, "=");
        status = strtol(strtok(NULL, "\n"), NULL, 10);

        printf("status: %ld", status);
    } else {
        printf("Cannot read Status");
    }

    while ((getline(&line, &len, read_fd)) != -1) {
        //get file
        printf("line: %s", line);
        strtok(line, "=");
        file_name = strtok(NULL, "\n");

        printf("file_name: %s", file_name);

        //create/open file
        if ((fp = fopen(file_name, "w+")) == NULL) {
            break;
        }

        //get len
        if ((getline(&line, &len, read_fd)) != -1) {
            printf("line: %s", line);
            strtok(line, "=");
            file_len = strtol(strtok(NULL, "\n"), NULL, 10);

            printf("len: %ld", file_len);
        } else {
            printf("Cannot read len");
        }

        counter = file_len;

        while (counter != 0) {
            //get data
            // memset(buffer, '\0', buffersize);
            toprocess = counter;
            if (toprocess > buffersize){
                toprocess = buffersize;
            }

            fread(buffer, 1, (size_t)toprocess, read_fd);

            printf("\n\n%ld - %zd\n\n",toprocess,(size_t)toprocess);
            counter -= toprocess;

            printf("%s", buffer);

            fwrite(buffer, sizeof(char), (size_t)toprocess, fp);


            }

        if (fclose(fp) == EOF) {
            break;

        }

    }

    return 0;

}
