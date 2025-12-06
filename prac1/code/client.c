#include <netdb.h> // Thư viện chứa struct hostent và gethostbyname
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

void error(const char *msg) {
  perror(msg);
  exit(0);
}

int main(int argc, char *argv[]) {
  int sockfd, portno, n;
  struct sockaddr_in serv_addr;
  struct hostent *server; // Struct quan trọng cho gethostbyname
  char buffer[1024];

  if (argc < 3) {
    fprintf(stderr, "Usage: %s hostname filename\n", argv[0]);
    exit(0);
  }

  portno = 8080;

  // 1. socket(): Tạo socket
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0)
    error("ERROR opening socket");

  // 2. gethostbyname(): Lấy địa chỉ IP từ tên máy (VD: "localhost")
  server = gethostbyname(argv[1]);
  if (server == NULL) {
    fprintf(stderr, "ERROR, no such host\n");
    exit(0);
  }

  // Thiết lập địa chỉ Server
  bzero((char *)&serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  // Copy địa chỉ từ struct hostent vào struct sockaddr_in
  bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr,
        server->h_length);
  serv_addr.sin_port = htons(portno);

  // 3. connect(): Kết nối đến server
  if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    error("ERROR connecting");

  // --- Bắt đầu gửi file ---
  char *filename = argv[2];
  FILE *fp = fopen(filename, "rb");
  if (fp == NULL)
    error("ERROR opening file");

  // BƯỚC A: Gửi tên file
  printf("Sending filename: %s\n", filename);
  send(sockfd, filename, strlen(filename), 0);

  // BƯỚC B: Chờ Server phản hồi "OK"
  bzero(buffer, 1024);
  recv(sockfd, buffer, 1023, 0);
  if (strcmp(buffer, "OK") != 0) {
    printf("Server did not acknowledge. Aborting.\n");
    close(sockfd);
    exit(1);
  }

  // BƯỚC C: Gửi nội dung file
  printf("Sending file data...\n");
  while (!feof(fp)) {
    // Đọc từ file vào buffer
    int n_read = fread(buffer, 1, 1024,
                       fp); // fread(<buffer>, <size>, <count>, <stream>)
    if (n_read > 0) {
      // Gửi buffer qua socket
      n = send(sockfd, buffer, n_read, 0);
      if (n < 0)
        error("ERROR writing to socket");
    }
  }

  printf("File sent successfully.\n");
  fclose(fp);
  close(sockfd);
  return 0;
}
