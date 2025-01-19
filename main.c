#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <unistd.h>

typedef struct {
  int rsock;
  int wsock;
}Socket_setting;

typedef struct {
  char *content;
  long file_size;
}File_content;

#define PORT 8080

Socket_setting socket_set(void) {
  Socket_setting socket_setting;

  struct sockaddr_in addr;
  int ret;

  // 通信口の作成
  // AF_INET = IPv4 の通信
  // SOCK_STREAM = バイトストリーム形式の通信
  socket_setting.rsock = socket(AF_INET, SOCK_STREAM, 0);

  printf("%d\n", socket_setting.rsock);

  if (socket_setting.rsock < 0) {
    fprintf(stderr, "Error. Cannot make socket\n");
    return socket_setting;
  }

  // ソケットの設定をする
  addr.sin_family = AF_INET;
  addr.sin_port = htons(PORT);
  addr.sin_addr.s_addr = INADDR_ANY;

  // ソケットに設定を割り当てる
  ret = bind(socket_setting.rsock, (struct sockaddr *)&addr, sizeof(addr));

  if (ret < 0) {
    fprintf(stderr, "Error. Cannot bind socket\n");
    close(socket_setting.rsock);
    return socket_setting;
  }

  return socket_setting;
}

File_content read_file_content(FILE *file) {
  File_content file_content;

  char *content;
  long file_size;

  fseek(file, 0, SEEK_END);
  file_size = ftell(file);
  rewind(file);

  content = malloc(file_size + 1);
  if (content == NULL) {
      perror("Error: Memory allocation failed");
      return file_content;
  }

  fread(content, 1, file_size, file);
  content[file_size] = '\0';

  file_content.content = content;
  file_content.file_size = file_size;

  return file_content;
}

int main(void){
  Socket_setting socket_setting = socket_set();

  socklen_t len;
  struct sockaddr_in client;

  FILE *file;
  file = fopen("./hoge.html", "r");
  if(file == NULL) {
    perror("Error open file.");
    close(socket_setting.rsock);
    return 1;
  }

  File_content file_content;
  file_content = read_file_content(file);

  // bind したソケットに対して接続を待つ。
  if (listen(socket_setting.rsock, 5) < 0) {
      perror("Error: Listen failed");
      close(socket_setting.rsock);
      return 1;
  }

  while (1) {
    // 接続要求に対して accept で受ける
    len = sizeof(client);
    socket_setting.wsock = accept(socket_setting.rsock, (struct sockaddr *)&client, &len);

    if (socket_setting.wsock < 0) {
      perror("Error accepting connection.");
      continue;
    }

    // HTTPレスポンスの作成
    char *response;
    if (asprintf(&response,
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: text/html; charset=utf-8\r\n"
             "Content-Length: %ld\r\n"
             "Connection: close\r\n"
             "\r\n",
             file_content.file_size) < 0){
      perror("Error: Failed to allocate memory for response");
      close(socket_setting.wsock);
      continue;
    }


    write(socket_setting.wsock, response, strlen(response));
    write(socket_setting.wsock, file_content.content, file_content.file_size);

    free(response);

    close(socket_setting.wsock);

    // sleep(5);
  }

  fclose(file);
  free(file_content.content);
  close(socket_setting.rsock);

  return 0;
}
