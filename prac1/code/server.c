#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

void error(const char *msg) {
  perror(msg);
  exit(1);
}

int main(int argc, char *argv[]) {
  int sockfd, newsockfd, portno;
  socklen_t clilen;
  char buffer[1024];
  struct sockaddr_in serv_addr, cli_addr;
  int n;
  int opt = 1; // Dùng cho setsockopt

  // Mặc định port 8080 nếu không nhập
  portno = 8080;

  // 1. socket(): Tạo socket
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0)
    error("ERROR opening socket");

  // 2. setsockopt(): Cho phép dùng lại địa chỉ IP/Port ngay lập tức
  // Giúp tránh lỗi "Address already in use" khi chạy lại code nhiều lần
  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
    error("ERROR setsockopt");
  }

  // Thiết lập cấu trúc địa chỉ
  bzero((char *)&serv_addr,
        sizeof(serv_addr)); // xoá cấu trúc địa chỉ (khi khai báo biến mà không
                            // gán giá trị sẽ có vùng đệm rác sinzero)
  serv_addr.sin_family = AF_INET;         // dùng ipv4
  serv_addr.sin_addr.s_addr = INADDR_ANY; // chấp nhận kết nối từ bất kỳ IP nào
  serv_addr.sin_port = htons(portno); // chuyển port thành network byte order
                                      // little endian -> big endian

  // 3. bind(): Gán socket với địa chỉ
  if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    error("ERROR on binding");

  // 4. listen(): Lắng nghe kết nối
  listen(sockfd, 5);
  printf("Server listening on port %d...\n", portno);

  clilen = sizeof(cli_addr);

  // 5. accept(): Chấp nhận kết nối
  newsockfd =
      accept(sockfd, (struct sockaddr *)&cli_addr,
             &clilen); // hàm accept trả về socket descriptor mới để socket cũ
                       // vẫn còn hoạt động và nhận được kết nối mới
  if (newsockfd < 0)
    error("ERROR on accept");
  // --- Bắt đầu nhận file ---

  // BƯỚC A: Nhận tên file
  bzero(buffer, 1024); // xoá vùng nhớ
  n = recv(
      newsockfd, buffer, 1023,
      0); // 1023 bytes để dành bit cuối cho ký tự \0 để đánh dấu cuối chuỗi
  if (n < 0)
    error("ERROR reading from socket");

  char original_filename[1024];
  strcpy(original_filename, buffer); // Lưu tên file gốc

  char new_filename[1024 +
                    sizeof("received_")]; // tạo vùng nhớ cho tên file mới Đủ
                                          // lớn cho "received_" + tên gốc
  snprintf(new_filename, sizeof(new_filename), "received_%s",
           original_filename); // tạo tên file mới
  printf("Client sending file: %s -> Saving as: %s\n", original_filename,
         new_filename);

  // BƯỚC B: Gửi xác nhận OK để Client biết mà gửi tiếp
  send(newsockfd, "OK", 2, 0);

  // BƯỚC C: Nhận nội dung file
  FILE *fp = fopen(new_filename, "wb");
  if (fp == NULL)
    error("ERROR opening file to write");

  int total = 0;
  while (1) {
    bzero(buffer, 1024);
    n = recv(newsockfd, buffer, 1024, 0);
    if (n <= 0)
      break; // Hết dữ liệu hoặc lỗi thì thoát
    fwrite(buffer, 1, n, fp);
    total += n;
  }

  printf("Received %d bytes. File saved.\n", total);
  fclose(fp);
  close(newsockfd);
  close(sockfd);
  return 0;
}