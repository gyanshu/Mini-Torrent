#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <cstring>
#include <openssl/sha.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <vector>
#include <sstream>
#include <cmath>
#include <unordered_map>
using namespace std;
int chunk_size = 512 * 1024;
unordered_map <string, char > downloads;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
struct TorrentInfo get_torrent_info(string mtorrent_path);
struct ChunkData{
  int chunk_number;
  string filename;
  string ip;
  int port;
};
struct Client{
  string client_ip;
  string client_port;
};
struct DownloadFile{
  string mtorrent_file;
  string destination;
};
struct TorrentInfo{
  string tracker_ip;
  int tracker_port;
  string filename;
  long long size;
  string hash;
  friend ostream & operator << (ostream &out, const struct TorrentInfo &info)
  {

    out << info.tracker_ip;
    out << "\n" << info.tracker_port << endl<<info.filename<<endl<<info.size<<endl<<info.hash<<endl;;
    return out;
  }
};
struct SeedInfo{
  string seeder_ip;
  int seeder_port;
};
string create_mtorrent_file(string filename, string mtorrent_file, string tracker_ip, string tracker_port){
  FILE *in = fopen(filename.c_str(), "r");
  //cout<<filename<<" file opened, creating mtorrent"<<endl;
  FILE *out = fopen(mtorrent_file.c_str(), "w");
  char data[chunk_size];
  //cout<<tracker_ip<< " "<<tracker_port<<endl;
  fprintf(out,"Tracker1:%s %s\n", tracker_ip.c_str(), tracker_port.c_str());
  /* Read in 256 8-bit numbers into the buffer */
  size_t bytes_read = 0;
  string hash = "";
  fseek(in, 0, SEEK_END);
  unsigned long len = (unsigned long)ftell(in);
  fseek( in, 0, SEEK_SET );
  //printf("%ld\n", len);
  fprintf(out, "Filename:%s\n", filename.c_str());
  fprintf(out,"Size:%ld \n", len);
  fprintf(out,"Hash:");


  //char *temp = hash_concat_pieces;
  string op = "";
  int i = 0;
  while((bytes_read = (fread(data, sizeof(unsigned char), chunk_size, in)) ) > 0){
    //cout<<data<<endl;
    //cout<<++i<<endl;
    unsigned char obuf[20];
    //char *buf = (char *)malloc(sizeof(char) * strlen(data));
    char buf[bytes_read+1];
    strcpy(buf, data);
    strcat(buf, "\0");
    //cout<<data;
    SHA1((unsigned char *)&data, strlen(buf),(unsigned char *)obuf);
    int i;

    char data[41];
    //strcat(hash_concat_pieces, (char *)obuf);
    for (i = 0; i < 20; i++) {
      //cout<<hex<<obuf[i]<<" ";

      fprintf(out,"%02x", obuf[i]);
      snprintf(data+i*2,4,  "%02x", obuf[i]);
    }
    op += data;

    memset(data, 0, sizeof data);
  }
  unsigned char final_hash[20];
  char hash_concat_pieces[op.size()+1] = {0};
  strcpy(hash_concat_pieces, op.c_str());
  strcat(hash_concat_pieces, "\0");
  //cout<<data;
  SHA1((unsigned char *)&hash_concat_pieces, strlen(op.c_str()),(unsigned char *)final_hash);
  char ha[41];
  for (int i = 0; i < 20; i++) {
    //printf("%02x", final_hash[i]);
    snprintf(ha+i*2,4,  "%02x", final_hash[i]);
  }

  string str_hash = string(ha);
  fclose(in);
  fclose(out);
  cout<<"Mtorrent created"<<endl;
  return str_hash;
}

int remove_file(string mtorrent_path, string client_ip , string client_port){
    struct TorrentInfo info = get_torrent_info(mtorrent_path);
    unsigned char final_hash[20];
    char buf[info.hash.size() + 1];
    strcpy(buf, info.hash.c_str());
    strcat(buf, "\0");
    /*cout<<endl;
    for(int i = 0; i < strlen(buf); i++)
      printf("%c", buf[i]);
    cout<<endl;*/
    SHA1((unsigned char *)(buf), strlen(info.hash.c_str()),(unsigned char *)final_hash);
    //cout<<endl;
    char ha[41];
    for (int i = 0; i < 20; i++) {
      //cout<<hex<<obuf[i]<<" ";
      //printf("%02x", final_hash[i]);
      snprintf(ha+i*2,4,  "%02x", final_hash[i]);
    }

    string str_hash = string(ha);


    //string str_hash = (char *)final_hash;

    struct sockaddr_in address;
    int sock = 0, valread;
    struct sockaddr_in serv_addr;
    char buffer[1024] = {0};

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
      printf("\n Socket creation error \n");
      return -1;
    }
    int option = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

    //cout<<"Socket created"<<endl;

    memset(&serv_addr, '0', sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons( info.tracker_port );
    //cout<<"Ip:"<<info.tracker_ip.c_str()<<endl;
    // Convert IPv4 and IPv6 addresses from text to binary form
    if(inet_pton(AF_INET, info.tracker_ip.c_str(), &serv_addr.sin_addr)<=0)
    {
      printf("\nInvalid address/ Address not supported \n");
      return -1;
    }
    //cout<<"Ipv4 Converted from text to binary"<<endl;

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
      cout<<"Connecting ..."<<endl;
      printf("\nConnection Failed \n");
      return -1;
    }
    //cout<<"Connected "<<endl;
    string send_data = "Remove";
    send_data += ";" + client_ip + ";" + client_port + ";" + info.filename + ";" + str_hash;
    //cout<<"Sending:"<<send_data<<endl;
    send(sock, send_data.c_str(), strlen(send_data.c_str()), 0);
    valread = read( sock , buffer, 1024);
    remove(mtorrent_path.c_str());
    //printf("Client: Message received from server - %s\n",buffer );
    //cout<<buffer<<endl;

}
int share_file(string filename, string mtorrent_file, string tracker_ip, string tracker_port, string client_ip,string upload_port){

  string hash = create_mtorrent_file(filename, mtorrent_file, tracker_ip, tracker_port);
  struct sockaddr_in address;
  int sock = 0, valread;
  struct sockaddr_in serv_addr;
  char buffer[1024] = {0};

  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    printf("\n Socket creation error \n");
    return -1;
  }
  int option = 1;
  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

  //cout<<"Socket created"<<endl;

  memset(&serv_addr, '0', sizeof(serv_addr));

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(atoi(tracker_port.c_str()));
  //cout<<"Ip:"<<tracker_ip.c_str()<<endl;
  // Convert IPv4 and IPv6 addresses from text to binary form
  if(inet_pton(AF_INET, tracker_ip.c_str(), &serv_addr.sin_addr)<=0)
  {
    printf("\nInvalid address/ Address not supported \n");
    return -1;
  }
  //cout<<"Ipv4 Converted from text to binary"<<endl;

  if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
  {
    //cout<<"Connecting ..."<<endl;
    printf("\nConnection Failed \n");
    return -1;
  }
  //cout<<"Connected "<<endl;
  string send_data = "Share";
  send_data += ";" + client_ip + ";" + upload_port + ";" + filename + ";" + hash;
  //cout<<"Sending:"<<send_data<<endl;
  send(sock, send_data.c_str(), strlen(send_data.c_str()), 0);

  //send(sock , hash.c_str()  , strlen(hash.c_str()) , 0 );

  //printf("Client: Hello message sent\n");
  valread = read( sock , buffer, 1024);
  printf("Message received from server - %s\n",buffer );
  //cout<<buffer<<endl;



}

struct TorrentInfo get_torrent_info(string mtorrent_path){
  //cout<<"Inside get torrentInfo:"<<mtorrent_path<<endl;
  FILE *in = fopen(mtorrent_path.c_str(),"r");
  struct TorrentInfo info;
  if(in == NULL)
    cout<<"Not able to open file";
  string ip;
  char ipc[chunk_size];
  //cout<<"Memory allocated"<<endl;
  fscanf(in, "%s %d", ipc, &info.tracker_port);
  //cout<<"Read"<<endl;
  strcat(ipc, "\0");
//  cout<<ipc;
  info.tracker_ip = ipc;
  //cout<<info.tracker_ip<<endl;;

  info.tracker_ip = info.tracker_ip.substr(info.tracker_ip.find(":")+1, info.tracker_ip.size());
  //cout<<info.tracker_ip<< " " <<info.tracker_port <<endl;

  memset(ipc, 0, chunk_size);

  fscanf(in, "%s", ipc );
  strcat(ipc, "\0");
  info.filename = ipc;
  info.filename = info.filename.substr(info.filename.find(":")+1, info.filename.size());
  //cout<<info.filename<<endl;

  memset(ipc, 0, chunk_size);

  fscanf(in, "%s", ipc );
  strcat(ipc, "\0");
  string size = ipc;
  size = size.substr(size.find(":")+1, size.size());

  info.size = atoll(size.c_str() );
  //cout<<"String to long convert."<<endl;
  memset(ipc, 0, chunk_size);
  //cout<<"Get hash"<<endl;
  fscanf(in, "%s", ipc );
  strcat(ipc, "\0");
  info.hash = ipc;
  info.filename = info.filename.substr(info.filename.find(":")+1, info.filename.size());
  info.hash = info.hash.substr(info.hash.find(":")+1, info.hash.size());
  //cout<<"Torrent info read complete"<<endl;
  fclose(in);
  return info;
}
struct Client get_client_info(char *token){
  //cout<<"Inside get client info from:"<<token<<endl;
  struct Client client;
  char *t = (char *)malloc(sizeof(char) * strlen(token));
  strcpy(t, token);
  //cout<<"Ip:"<<t<<endl;
  char *toks = strtok(t, ";");
  //cout<<toks<<endl;;
  client.client_ip = toks;
  toks = strtok(NULL, ";");
  //cout<<toks<<endl;
  client.client_port = toks;
  return client;
}
void * get_file_from_server(void *chunkdata ){
  struct ChunkData *cd = (struct ChunkData *)chunkdata;
  struct ChunkData chunk_data = *cd;

  //cout<<"Inside get file from server:"<<chunk_data.chunk_number<<endl;

  struct sockaddr_in address;
  int sock = 0, valread;
  struct sockaddr_in serv_addr;


  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    printf("\n Socket creation error \n");
    //return -1;
  }
  int option = 1;
  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

  //cout<<"Socket created"<<endl;

  memset(&serv_addr, '0', sizeof(serv_addr));

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(chunk_data.port);
  //cout<<"Ip:"<<chunk_data.ip.c_str()<<endl;
  // Convert IPv4 and IPv6 addresses from text to binary form
  if(inet_pton(AF_INET, chunk_data.ip.c_str() , &serv_addr.sin_addr)<=0)
  {
    printf("\nInvalid address/ Address not supported \n");
    //return -1;
  }
  //cout<<"Ipv4 Converted from text to binary"<<endl;
  //cout<<info.tracker_port<<endl;
  if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
  {
    //cout<<"Connecting ..."<<endl;
    printf("\nConnection Failed \n");
    //return -1;
  }
  //cout<<"Connected "<<endl;
  char share[] = "Download";
  string send_data = share;
  send_data += ";" + chunk_data.filename + ";" + to_string(chunk_data.chunk_number);
  //cout<<send_data<<endl;
  send(sock, send_data.c_str(), strlen(send_data.c_str()), 0);

  pthread_mutex_lock(&mutex);
  //cout<<"Reading from socket: "<<sock<<endl;
  string name = "chunk_" +  chunk_data.filename + "_" + to_string(chunk_data.chunk_number) ;

  FILE *in = fopen(name.c_str() , "w");
  char buffer[chunk_size] = {0};
  int temp = chunk_size;
  while(temp > 0){
    valread = recv( sock , buffer, chunk_size, 0) ;
    if(valread <= 0)
      break;
    //cout<<"Bytes received for chunk "<<chunk_data.chunk_number<<" : "<<valread<<endl;
    for(int i = valread; i < chunk_size; i++)
      buffer[i] = 0;
    //cout<<"Creating file:"<<name<<endl;
    fwrite(buffer, sizeof(char), valread, in);
    memset(buffer, 0, sizeof buffer);
    temp -= valread;

  }
  fclose(in);
  sleep(0.1);
  close(sock);
  pthread_mutex_unlock(&mutex);

  //cout<<"Downloaded chunk"<<endl;//<<buffer<<endl;


}
void * download_file(void *d){
  struct DownloadFile *download = (struct DownloadFile *)d;
  string mtorrent_path = download->mtorrent_file;
  string destination = download->destination;
  cout<<"File download started"<<endl;
  struct TorrentInfo info = get_torrent_info(mtorrent_path);
  int size = info.size;
  downloads[destination + "/" + info.filename] = 'D';

  struct sockaddr_in address;
  int sock = 0, valread;
  struct sockaddr_in serv_addr;
  char buffer[4096] = {0};

  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    printf("\n Socket creation error \n");
    //return;
  }
  int option = 1;
  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

  //cout<<"Socket created"<<endl;

  memset(&serv_addr, '0', sizeof(serv_addr));

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(info.tracker_port);
  //cout<<"Ip:"<<info.tracker_ip.c_str()<<endl;
  // Convert IPv4 and IPv6 addresses from text to binary form
  if(inet_pton(AF_INET, info.tracker_ip.c_str() , &serv_addr.sin_addr)<=0)
  {
    printf("\nInvalid address/ Address not supported \n");
    //return;
  }
  //cout<<"Ipv4 Converted from text to binary"<<endl;
  //cout<<info.tracker_port<<endl;
  if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
  {
  //  cout<<"Connecting ..."<<endl;
    printf("\nConnection Failed \n");
    //return;
  }
  //cout<<"Connected "<<endl;
  char share[] = "Get";
  string send_data = share;
  send_data += ";" + info.filename;
  //cout<<send_data<<endl;
  send(sock, send_data.c_str(), strlen(send_data.c_str()), 0);
  //cout<<"Sent"<<endl;
  valread = read( sock , buffer, 4096);
  //if(valread   > 0)
  //  cout<<"Read"<<valread<<endl;
  //else
  //  cout<<"Read failed"<<endl;
  for(int i = valread; i < 4096; i++){
      buffer[i] = 0;

  }

  //cout<<"Buffer:"<<buffer<<endl;
  vector <struct Client> seeder_list;
  char *token;
  char *buf = buffer;
  //cout<<buf<<endl;

  while ((token = strtok_r(buf, "-", &buf ))){
    //cout<<"Token"<<token<<endl;
    seeder_list.push_back(get_client_info(token));
  }
  //cout<<"Info received"<<endl;
  for(int i = 0; i < seeder_list.size(); i++){
    //cout<<seeder_list[i].client_ip << "  - " <<seeder_list[i].client_port<<endl;
  }
  //cout<<"Total size:"<<info.size<<endl;
  int chunks_to_fetch = ceil(info.size/(double)chunk_size);
  //cout<<"Chunks to fetch: "<<chunks_to_fetch<<endl;
  int server = 0;
  vector <pthread_t> threads_list;
  struct ChunkData chunkdata[chunks_to_fetch];
  for(int i = 0; i < chunks_to_fetch; i++){
    server = server % seeder_list.size();

    //struct ChunkData chunkdata;
    chunkdata[i].chunk_number = i;
    chunkdata[i].ip = seeder_list[server].client_ip;
    chunkdata[i].port = atoi(seeder_list[server].client_port.c_str());
    chunkdata[i].filename = info.filename;
    server++;
    pthread_t tid;
    //cout<<"Requesting chunk number:"<<i<<endl;
    pthread_create(&tid, NULL, get_file_from_server, (void *)(&chunkdata[i]) );
    pthread_join(tid,NULL);
    threads_list.push_back(tid);
    sleep(0.2);

  }
  //for (pthread_t th : threads_list)

  cout<<"File downloaded"<<endl;
  string command = "cat chunk_" + info.filename + "* > " + info.filename;
  system(command.c_str() );
  //sleep(5);
  command = "rm chunk_" + info.filename + "*";
  system(command.c_str() );
  command = "mv " + info.filename + " " + destination ;
  if(destination.compare(".") != 0)
    system(command.c_str() );
  downloads[destination + "/" + info.filename] = 'S';




}
void trim(char *str)
{
  int i;
  int begin = 0;
  int end = strlen(str) - 1;
  while (isspace((unsigned char) str[begin]))
  begin++;
  while ((end >= begin) && isspace((unsigned char) str[end]))
  end--;
  for (i = begin; i <= end; i++)
  str[i - begin] = str[i];
  str[i - begin] = '\0';
}
void * seed_file(void *c){
  struct Client *client = (struct Client *)c;
  string client_ip = client->client_ip;
  string client_port = client->client_port;
  int master_socket , addrlen , new_socket , client_socket[30] ,
  max_clients = 30 , activity, i , valread , sd;
  int max_sd;
  struct sockaddr_in address;

  char buffer[1025];
  fd_set readfds;
  for (i = 0; i < max_clients; i++)
  {
    client_socket[i] = 0;
  }
  if( (master_socket = socket(AF_INET , SOCK_STREAM , 0)) == 0)
  {
    perror("socket failed");
    exit(EXIT_FAILURE);
  }
  int opt = 1;
  if( setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt,
  sizeof(opt)) < 0 )
  {
    perror("setsockopt");
    exit(EXIT_FAILURE);
  }
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = inet_addr(client_ip.c_str());
  address.sin_port = htons( atoi(client_port.c_str() ) );
  if (bind(master_socket, (struct sockaddr *)&address, sizeof(address))<0)
  {
    perror("bind failed");
    exit(EXIT_FAILURE);
  }
  //printf("Listener on port %d \n", atoi(client_port.c_str()));

  //try to specify maximum of 3 pending connections for the master socket
  if (listen(master_socket, 3) < 0)
  {
    perror("listen");
    exit(EXIT_FAILURE);
  }

  //accept the incoming connection
  addrlen = sizeof(address);
  //puts("Waiting for connections ...");

  for(;;)
  {
    FD_ZERO(&readfds);
    FD_SET(master_socket, &readfds);
    max_sd = master_socket;
    for ( i = 0 ; i < max_clients ; i++)
    {
      sd = client_socket[i];
      if(sd > 0)
      FD_SET( sd , &readfds);
      if(sd > max_sd)
      max_sd = sd;
    }
    activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL);

    if ((activity < 0) && (errno!=EINTR))
    {
      //printf("select error");
    }
    if (FD_ISSET(master_socket, &readfds))
    {
      if ((new_socket = accept(master_socket,
        (struct sockaddr *)&address, (socklen_t*)&addrlen))<0)
        {
          perror("accept");
          exit(EXIT_FAILURE);
        }

        //inform user of socket number - used in send and receive commands
        //printf("New connection , socket fd is %d , ip is : %s , port : %d\n" , new_socket , inet_ntoa(address.sin_addr) , ntohs(address.sin_port));

        for (i = 0; i < max_clients; i++)
        {
          //if position is empty
          if( client_socket[i] == 0 )
          {
            client_socket[i] = new_socket;
            //printf("Adding to list of sockets as %d\n" , i);

            break;
          }
        }
      }

      for (i = 0; i < max_clients; i++)
      {
        sd = client_socket[i];
        char buffer[1024];
        if (FD_ISSET( sd , &readfds))
        {
          if ((valread = read( sd , buffer, 1024)) == 0)
          {
            //Somebody disconnected , get his details and print
            getpeername(sd , (struct sockaddr*)&address , (socklen_t*)&addrlen);
            //printf("Host disconnected , ip %s , port %d \n" ,inet_ntoa(address.sin_addr) , ntohs(address.sin_port));

            //Close the socket and mark as 0 in list for reuse
            close( sd );
            client_socket[i] = 0;
          }
          else
          {
            buffer[valread] = '\0';
            //cout<<"Read:"<<valread<<endl;
            if(valread < 0)
            continue;
            for(int i = valread; i < 1024 ;i++){
              buffer[i] = 0;
            }
            //cout<<"Received:"<<buffer<<endl;
            string received_buffer = buffer;
            received_buffer = received_buffer.substr(received_buffer.find(";")+1, received_buffer.size());
            char *token = strtok(buffer, ";");


            if(strcmp(token, "Download") == 0){
              //cout<<"Inside Download"<<token<<endl;
              token = strtok((char *)received_buffer.c_str(), ";");
              trim(token);
              //cout<<"Search:"<<token<<endl;
              int piece_number = atoi(strtok(NULL, ";"));
              //cout<<"Search:"<<token<<" piece number:"<<piece_number<<endl;
              trim(token);
              //cout<<"Search:"<<token<<endl;
              pthread_mutex_lock(&mutex);

              FILE *in;
              if((in = fopen(token, "r")) > 0){
                //cout<<"Opening file"<<endl;
                fseek(in ,  piece_number * chunk_size , 0);
                //cout<<"Seeking:"<<piece_number * chunk_size <<endl;
                char data[512 * 1024];
                int bytes_read = fread(data, sizeof(char), chunk_size, in);
                //cout<<"Sending bytes:"<<bytes_read<<endl;
                int bytes_sent = send(sd, data, bytes_read, 0);
                //cout<<"Sent bytes:"<<bytes_sent<<endl;
                fclose(in);
                //cout<<"CLosing file"<<endl;
                close( sd );
              }
              else{
                char data[] = "No data";
                //cout<<"No data"<<endl;
                send(sd, data, strlen(data), 0);
                close( sd );
              }
              pthread_mutex_unlock(&mutex);

            }
          }
        }
      }
    }

  }
  int main(int argc, char *argv[]){
    //struct TorrentInfo info = get_torrent_info("f.mtorrent");
    //cout<<info<<endl;
    cout<<"Torrent client is up. You can start sharing and downloading files"<<endl;
    struct DownloadFile d;
    //d.mtorrent_file = "f.mtorrent";
    //d.destination = ".";
    //download_file((void *)&d);
    //return 0;
    string client_ip = argv[1];
    int index_of_port = client_ip.find(":");
    string client_port = client_ip.substr(index_of_port + 1, client_ip.size() - index_of_port);
    client_ip = client_ip.substr(0, index_of_port);


    string tracker_ip = argv[2];
    index_of_port = tracker_ip.find(":");
    string tracker_port = tracker_ip.substr(index_of_port + 1, tracker_ip.size() - index_of_port);
    tracker_ip = tracker_ip.substr(0, index_of_port);
    struct Client client;
    client.client_ip = client_ip;
    client.client_port = client_port;
    pthread_t tid;
    pthread_create(&tid, NULL, seed_file, (void *)(&client) );
    //pthread_exit(NULL);
    sleep(5);
    while(true){
      //cout<<"Enter a command: ";
      string command;
      getline(cin, command);
      stringstream ss(command);
      string op;
      ss>>op;
      if(strcmp(op.c_str(), "share") == 0){
        string file_path, torrent_name;
        ss>>file_path>>torrent_name;
        share_file(file_path, torrent_name, tracker_ip, tracker_port, client_ip, client_port);
        cout<<"Seeding "<<file_path<< " \n";

      }
      else if(strcmp(op.c_str(), "remove") == 0){
        string torrent_name;
        ss>>torrent_name;

        remove_file(torrent_name, client_ip, client_port);
        cout<<"Removed file: " <<torrent_name<<endl;
      }
      else if(strcmp(op.c_str(), "get") == 0){
        string path_to_mtorrent, destination;
        ss>>path_to_mtorrent>>destination;
        //cout<<path_to_mtorrent<< " " <<destination;
        pthread_t tid;
        DownloadFile dfile;
        dfile.mtorrent_file = path_to_mtorrent;
        dfile.destination = destination;

        pthread_create(&tid, NULL, download_file, (void *)(&dfile) );
      }
      else if(strcmp(op.c_str(), "show") == 0){
        unordered_map <string, char> ::iterator itr;
        cout<<"List of downloads:"<<endl;
        for(itr = downloads.begin(); itr != downloads.end(); itr++){
          cout<<itr->second <<" "<<itr->first<<endl;
        }
      }
      //cout<<endl;
    }

    return 0;
  }
