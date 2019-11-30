
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <iostream>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <fstream>
#define TRUE 1
#define FALSE 0
#define PORT 8888
using namespace std;

void trim(char *str)
{
  int i;
  int begin = 0;
  int end = strlen(str) - 1;

  while (isspace((unsigned char) str[begin]))
  begin++;

  while ((end >= begin) && isspace((unsigned char) str[end]))
  end--;

  // Shift all characters back to the start of the string array.
  for (i = begin; i <= end; i++)
  str[i - begin] = str[i];

  str[i - begin] = '\0'; // Null terminate string.
}
int main(int argc , char *argv[])
{
  int opt = TRUE;
  unordered_map <string, vector<string> > seeder_list;
  string ip = argv[1];
  int index_of_port = ip.find(":");
  string port = ip.substr(index_of_port + 1, ip.size() - index_of_port);
  ip = ip.substr(0, index_of_port);
  cout<<"Tracker is started and is running on ";
  cout<<ip<< " "<<port<<endl;
  //struct sockaddr_in address;
  ifstream infile(argv[2]);
  string str,line;


  while (getline(infile, line))
  {
    //cout<<line;
    str = line;
    //cout<<str<<endl;
    int key = str.find(":");

    string val = str.substr(key + 1, str.size() - key);
    if(seeder_list.find(str.substr(0, key)) != seeder_list.end()){
      seeder_list[str.substr(0, key)].push_back(val);
    }
    else{
      vector <string> v;
      v.push_back(val);
      seeder_list[str.substr(0, key)] = v;
    }

  }

  infile.close();
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
  if( setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt,
  sizeof(opt)) < 0 )
  {
    perror("setsockopt");
    exit(EXIT_FAILURE);
  }
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons( atoi(port.c_str()) );

  if (bind(master_socket, (struct sockaddr *)&address, sizeof(address))<0)
  {
    perror("bind failed");
    exit(EXIT_FAILURE);
  }
  if (listen(master_socket, 3) < 0)
  {
    perror("listen");
    exit(EXIT_FAILURE);
  }


  addrlen = sizeof(address);

  while(TRUE)
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
      printf("select error");
    }

    if (FD_ISSET(master_socket, &readfds))
    {
      if ((new_socket = accept(master_socket,
        (struct sockaddr *)&address, (socklen_t*)&addrlen))<0)
        {
          perror("accept");
          exit(EXIT_FAILURE);
        }


        for (i = 0; i < max_clients; i++)
        {
          //if position is empty
          if( client_socket[i] == 0 )
          {
            client_socket[i] = new_socket;

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

            getpeername(sd , (struct sockaddr*)&address , (socklen_t*)&addrlen);
            //printf("Host disconnected , ip %s , port %d \n" ,inet_ntoa(address.sin_addr) , ntohs(address.sin_port));
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
            if(strcmp(token, "Share") == 0){
              //cout<<"Inside share"<<endl;
              //cout<<"CLient:"<<buffer<<endl;
              string t = token;
              token = strtok(NULL, ";");
              token = strtok(NULL, ";");
              token = strtok(NULL, ";");
              trim(token);
              //cout<<"File:"<<token<<endl;
              //token = strtok(NULL, ";");
              //cout<<received_buffer<<endl;
              FILE *seeder_list_file = fopen(argv[2], "a");
              fprintf(seeder_list_file, "%s:%s\n",token, received_buffer.c_str());
              fclose(seeder_list_file);
              if(seeder_list.find(token) == seeder_list.end()){
                vector <string > s;
                s.push_back(received_buffer);
                seeder_list[token] = s;
                cout<<"Adding "<<token<<" to seederlist"<<endl;
              }
              else{
                seeder_list[token].push_back(received_buffer);
                cout<<"Adding "<<token<< "  to seederlist"<<endl;
              }
              //cout<<"List:"<<seeder_list[token].size()<<endl;
              char buf[100];
              strcpy(buf, "Seed info received\0");
              //cout<<"Sending "<<buf<<endl;
              send(sd, buf, strlen(buf), 0);
              //cout<<"Sent "<<buf<<endl;


            }
            else if(strcmp(token, "Remove") == 0){
              //cout<<"Inside remove:"<<buffer<<endl;
              //cout<<"CLient:"<<buffer<<endl;
              string t = buffer + 7;
              //cout<<"To remove:"<<t<<endl;
              token = strtok(NULL, ";");
              token = strtok(NULL, ";");
              token = strtok(NULL, ";");
              trim(token);
              //cout<<"File:"<<token<<endl;
              //token = strtok(NULL, ";");
              //cout<<received_buffer<<endl;
              string message = "";
              string command = "sed -i '/"+ t  +"/d' ./"+argv[2];
              system(command.c_str());
              if(seeder_list.find(token) == seeder_list.end()){
                message = "Seeder doesn't exist.";
              }
              else{
                //seeder_list[token].push_back(received_buffer);
                vector <string> v = seeder_list[token];
                for(int i = 0; i < v.size() ;i++){
                  if(t.compare(v[i]) == 0){
                    v.erase(v.begin() + i);
                    //cout<<"Found and removed..."<<endl;
                  }

                }
                vector<string>::iterator position = find(v.begin(), v.end(), t);
                if (position != v.end()){
                  v.erase(position);
                  //cout<<"Found, removing !"<<endl;
                }
                else{
                  //cout<<"Not found"<<endl;
                  for(int i = 0; i < v.size(); i++){
                    //cout<<v[i]<<endl;
                  }
                }
                if(v.size() != 0)
                seeder_list[token] = v;
                else
                seeder_list.erase(seeder_list.find(token));
                message = "Seeder has been removed from the list.";
              }


              char buf[100];
              strcpy(buf, message.c_str());
              //snprintf(buf, sizeof buf, "Seed info received\0");
              //printf("Tracker: Host  ip %s , port %d \n" ,inet_ntoa(address.sin_addr) , ntohs(address.sin_port));
              //cout<<"Sending "<<buf<<endl;
              send(sd, buf, strlen(buf), 0);
              //cout<<"Sent "<<buf<<endl;
            }
            else if(strcmp(token, "Get") == 0){
              //cout<<"Inside Get"<<token<<endl;
              token = strtok((char *)received_buffer.c_str(), ";");
              trim(token);
              //cout<<"Search:"<<token<<endl;
              //cout<<"Len:"<<strlen(token);
              //cout<<received_buffer<<endl;
              string buf = "";
              int i;
              if(seeder_list.find(token) != seeder_list.end()){
                //cout<<"Found"<<endl;
                //cout<<seeder_list[token].size()<<endl;
                for(i = 0; i < seeder_list[token].size() - 1; i++){
                  buf += seeder_list[token][i] + "-";
                }
                buf += seeder_list[token][i] ;
              }
              else{
                buf = "No seeders available";
              }

              /*
              unordered_map <string , vector<string>> :: iterator itr;
              for(itr = seeder_list.begin(); itr != seeder_list.end(); itr++){
              //cout<<"Map:"<<itr->first<<endl;
              //for(int i = 0; i < itr->second.size(); i++)
              //cout<<itr->second[i]<<endl;
            }*/
            //strcpy(buf, "Seed info received\0");
            //snprintf(buf, sizeof buf, "Seed info received\0");
            //printf("Tracker: Host  ip %s , port %d \n" ,inet_ntoa(address.sin_addr) , ntohs(address.sin_port));
            //cout<<"Sending "<<buf<<endl;
            send(sd, buf.c_str(), strlen(buf.c_str()), 0);
            //cout<<"Sent "<<buf<<endl;
          }
          //send(sd , buffer , strlen(buffer) , 0 );
        }
      }
    }
  }

  return 0;
}
