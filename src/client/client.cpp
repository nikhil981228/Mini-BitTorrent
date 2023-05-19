#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include<iostream>
#include<bits/stdc++.h>
#include<pthread.h>
#include <sys/stat.h>
#include <gcrypt.h>
#include <fcntl.h>
#include <algorithm>

#include "sha1.h"
#include "sha1.cpp"

// g++ client.cpp -lstdc++ -lgcrypt -o client
using namespace std;
#define SERVER_BUFFER (100)
#define SIZE 16*1024
#define CHUNKSIZE 512*1024
// #define SIZE 100
// #define CHUNKSIZE 200

//#define _XOPEN_SOURCE 500
#include <unistd.h>

#include <mutex>          // std::mutex

//std::mutex mtx;

#include <iostream>
#include <string>
#include <algorithm>
 
const std::string WHITESPACE = " \n\r\t\f\v";
 
std::string ltrim(const std::string &s)
{
    size_t start = s.find_first_not_of(WHITESPACE);
    return (start == std::string::npos) ? "" : s.substr(start);
}
 
std::string rtrim(const std::string &s)
{
    size_t end = s.find_last_not_of(WHITESPACE);
    return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string client_ip = "";
int client_port = 0;
bool isloggedin = false;
string uname_loggedin = "";
bool chunkDownloaded = true;
char IP_ADDR[50];
short unsigned int tracker_port;

// map< int,map<string, string > > port_file_path_map;

// map<string, string > file_path_map;
// map<string,long long> file_size_map;
// map<string,vector<int>> file_piece_map;

map<string, map<string, string> > gid_file_path_map;
map<string, map<string,long long> > gid_file_size_map;
map<string, map<string,vector<int>> > gid_file_piece_map;
map<string, map<string,string> > gid_file_status;


//pthread_mutex_t lock1;

string do_sha1_file(char* buf){
    char hash[20];
    gcry_md_hash_buffer( GCRY_MD_SHA1, hash, buf, sizeof(buf));
    return string(hash);
}


string calculate_file_sha(string file_path){
  FILE *f;
  f = fopen(file_path.c_str(), "rb");
  if (f == NULL) {
    perror("Could not read file");
  }
  char buf[524288];               // 524288
  memset(buf , '\0', SIZE);
  string sha1="";
  while (fread(buf, 1, sizeof buf, f)>0) {
    sha1 = sha1+do_sha1_file(buf);
  }  
  int err = ferror(f);
  fclose(f);
  return sha1;
}

//  string sha1 = calculate_file_sha("client.cpp");


std::vector<std::string> str_split(std::string s, char delim)
{
    std::string ans;
    std::vector<std::string>res;
    if( s == "" ){
      return res;
    }
    int n = s.size();
    int start = 0;
    for( int i = 0; i <= n; i++ ){
        if( s[i] == delim || s[i] == '\0' ){
            if( i == 0 ){
                res.push_back("");
                start = 1;
            }
            else{
                ans = s.substr(start, i-start);
                start = i+1;
                res.push_back(ans);
            }
        }
    }
    return res;
}

string array_to_string( vector<int> vec ){
  string res = "";
  int n = (int)vec.size();
  for( int i = 0; i < n; i++ ){
    if( i != n-1 )
      res += to_string(vec[i])+" ";
    else
      res += to_string(vec[i]);
  }
  return res;
}


// void send_file(int sockfd, string file_name){
//   std::ifstream fin(file_name, std::ifstream::binary);
//   //std::ifstream fin(file_name, ios::binary);
//   char data[SIZE] = {0};//reads only the first SIZE bytes
// 	//fin.seekg(6, std::ios::beg);
//   while(!fin.eof()) {
// 			fin.read(data, SIZE);
// 			std::cout << data << std::endl;
// 			if (send(sockfd, data , strlen(data), 0) == -1) {
// 				perror("[-]Error in sending file.");
// 				exit(1);
// 			  }
//      bzero(data,SIZE);
// 	}
// }

// void write_file(int sockfd, string file_name ){
//   int n;
//   char buffer[SIZE];
//   std::ofstream outfile (file_name, std::ofstream::binary);
//   //std::ofstream outfile (file_name, ios::binary);
//   while (1) {
//     n = read(sockfd, buffer, SIZE);
// 	  std::cout << buffer << std::endl;
//     if (n <= 0){
//       break;
//     }
//     // fprintf(fp, "%s", buffer);
// 	  outfile.write(buffer,strlen(buffer));
//     bzero(buffer, SIZE);
//   }
//   return;
// }


long long GetFileSize(std::string filename)
{
  struct stat stat_buf;
  int rc = stat(filename.c_str(), &stat_buf);
  return rc == 0 ? stat_buf.st_size : -1;
}

void receiveFile(int new_socket, long file_size, string file_name="recv.mp3" ){
   //long long file_size=GetFileSize(file_name);
  char buffer[SIZE];
  bzero(buffer, SIZE);
  recv(new_socket, buffer, sizeof(buffer), 0);
  string sha1_orig = buffer;

   cout<<"transferring File "<< file_size <<endl;
   //string fname="output.png";
    FILE *fp = fopen ( file_name.c_str() , "wb" );
    //size_t chunk=512;
    char Buffer [SIZE] = {0};
    //long long file_size;
    long long n;
    //memset(Buffer , '\0', SIZE);
    while (( n = recv( new_socket , Buffer ,   SIZE, 0) ) > 0  &&  file_size > 0)
    {
        fwrite (Buffer , sizeof (char), n, fp);
        memset ( Buffer , 0, SIZE);
        file_size = file_size - n;
        if(n<=0 || file_size<=0)
        {
            break;
        }
    }

    fclose (fp);

    string sha1_new = calculate_file_sha(file_name);
    if( sha1_new == sha1_orig ){
      cout<<"File transfer complete "<<endl;
    }
    else{
      cout << "File Corrupted " << endl;
      cout << sha1_new.size() << " sha1_new: " << sha1_new << endl;
      cout << endl;
      cout << sha1_orig.size() << "sha1_orig: " << sha1_orig << endl;
    }
}

void transferFile(int socketID, long long filesize, string file_name = "send.mp3"){
  char buffer1[SIZE];

  string sha1 = calculate_file_sha(file_name);
  strcpy(buffer1, sha1.c_str());
  send(socketID,buffer1, strlen(buffer1), 0 );
  memset( buffer1 , 0, SIZE);
  
  //size_t chunk=512;
  string fname=file_name;
  // send(socketID,&filesize,sizeof(filesize),0);
  FILE *fp= fopen(fname.c_str(),"rb");
  
  long long n;
  //cout << filesize << " done1" << endl;
  while ( ( n = fread( buffer1 , sizeof(char) , SIZE , fp ) ) > 0  && filesize > 0 )
  {
    send (socketID,buffer1, n, 0 );
    memset ( buffer1 , 0, SIZE);
    filesize = filesize - n ;
    if(n<=0 || filesize <=0)
    {
      break;
    }
  }
  //cout << "done2" << endl;
}

/*
void transfer_chunk(  int client_socket, int chunk_size, int chunk_no, bool lastChunk, string file_path ){
  
  FILE *fp= fopen(file_path.c_str(),"rb");
  //fd = open(fl_nm, O_RDWR|O_CREAT, 0777);

  char buffer1[SIZE];
  memset( buffer1 , 0, SIZE);

  if( lastChunk ){
    string msg = to_string(chunk_size);
    //bzero(buffer1, SIZE);
    strcpy(buffer1, msg.c_str());
    //printf("%s", buffer);
    send(client_socket, buffer1, SIZE, 0 );
    cout<<"Transfer last chunk no "<< chunk_size << " " <<  chunk_no << " " << buffer1 << endl;
  }
  else
  cout<<"Transfer chunk no "<< chunk_size << " " << chunk_no << endl;

  memset( buffer1 , 0, SIZE);
  fseek(fp,chunk_no*CHUNKSIZE,SEEK_SET);

  //fseek(fp, 0, chunk_no*512*1024);

  long long n;

  //nw = pread(0, buffer1, strlen(buffer1), chunk_no*CHUNKSIZE);

  while ( ( n = fread( buffer1 , sizeof(char) , SIZE , fp ) ) > 0  && chunk_size > 0 )
  {
    cout << buffer1 << endl;
    send (client_socket, buffer1, n, 0 );
    memset ( buffer1 , 0, SIZE);
    chunk_size = chunk_size - n ;
    if(n<=0 || chunk_size <=0)
    {
      break;
    }
  }
  cout << "done2" << endl;
}

void receiveChunk(int new_socket, int chunk_no, bool lastChunk, string file_path ){
  //long long file_size=GetFileSize(file_name);
  char Buffer [SIZE] ={0};
  memset(Buffer , 0, SIZE);
  int chunk_size = CHUNKSIZE;
  if( lastChunk ){
    bzero(Buffer, SIZE);
    recv( new_socket , Buffer ,   SIZE, 0);
    string s = Buffer;
    chunk_size = stoi(s);
    cout << "panga1 " << s << chunk_size << endl;
    // s = rtrim(s);
    // cout << "panga2 " << s << endl;
    
    cout<<"Recieving last chunk no "<< chunk_size << " " << chunk_no << endl;
  }
  else
    cout<<"Recieving chunk no "<< chunk_size << " " << chunk_no << endl;
  
  FILE *fp = fopen( file_path.c_str() , "wb" );
  fseek(fp,chunk_no*CHUNKSIZE,SEEK_SET);

  // int fd = open(file_path.c_str(), O_RDWR|O_CREAT, 0777);
  // if(fd == -1){
  //   perror("[error in open]\n");
  // }
        
  long long n;
  bzero(Buffer, SIZE);
  // memset(Buffer , '\0', SIZE);
  // cout << "panga1 " << Buffer << endl;
  while (( n = recv( new_socket , Buffer ,   SIZE, 0) ) > 0  &&  chunk_size > 0)
  {
    cout << Buffer  << endl;
    cout << " **|** " << n << endl;
    //int BITES = fwrite (Buffer , sizeof(char), n, fp);
    int BITES = fputs(Buffer, fp);

    cout<<"Data Read is "<<BITES << " | " << ftell(fp) <<endl;
    // int nw = pwrite(fd, Buffer, strlen(Buffer), chunk_no*CHUNKSIZE);
    // if(nw == -1){
    //   perror("[error in write]\n");
    // }
    //printf("%ld\n", ftell(fp));
    memset(Buffer , 0, SIZE);
    bzero(Buffer, SIZE);
    //memset ( Buffer , '\0', SIZE);
    chunk_size = chunk_size - n;
    cout<<"CHUNK SIZE IS......"<<chunk_size<<endl;
    if(n<=0 || chunk_size<=0)
    {
      break;
    }
    
  }
  cout << "done " << endl;
  // if(close(fd) == -1){
  //   perror("[error in close]\n");
  // }else{
  //   printf("[succeeded in close]\n");
  // }
  fclose (fp);
}
*/



void transfer_chunk(  int client_socket, int chunk_size, int chunk_no, bool lastChunk, string file_path ){
  
  ifstream in(file_path.c_str(), std::ios::in | std::ios::binary);
  char buffer1[SIZE] = {0};
  memset( buffer1 , 0, SIZE);

  if( lastChunk ){
    string msg = to_string(chunk_size);
    //bzero(buffer1, SIZE);
    strcpy(buffer1, msg.c_str());
    //printf("%s", buffer);
    send(client_socket, buffer1, SIZE, 0 );
    cout<<"Transfer last chunk no "<< chunk_size << " " <<  chunk_no << " " << buffer1 << endl;
  }
  else
  cout<<"Transfer chunk no "<< chunk_size << " " << chunk_no << endl;

  memset( buffer1 , 0, SIZE);
  in.seekg( chunk_no*CHUNKSIZE, in.beg );

  long long n;
  //nw = pread(0, buffer1, strlen(buffer1), chunk_no*CHUNKSIZE);
  do
  {
    in.read(buffer1, SIZE);
    n = in.gcount();
    // cout << buffer1 << " **|** " << n << endl;
    send( client_socket, buffer1, n, 0 );
    memset ( buffer1 , 0, SIZE);
    chunk_size = chunk_size - n;
    if(n<=0 || chunk_size <=0)
    {
      break;
    }
  }
  while (  n  >  0  && chunk_size > 0 );

  cout << "Transferred chunk no: " << chunk_no << endl;

  in.close();
}



void receiveChunk(int new_socket, int chunk_no, bool lastChunk, string file_path ){

  char Buffer [SIZE] ={0};
  memset(Buffer , 0, SIZE);
  int chunk_size = CHUNKSIZE;
  if( lastChunk ){
    bzero(Buffer, SIZE);
    recv( new_socket , Buffer ,   SIZE, 0);
    string s = Buffer;
    cout << "panga1 " << s << " " << chunk_size << endl;
    cout<<"Recieving last chunk no "<< chunk_size << " " << chunk_no << endl;
    chunk_size = stoi(s);
  }
  else
    cout<<"Recieving chunk no "<< chunk_size << " " << chunk_no << endl;

  //ofstream filename(file_path.c_str(), std::ios::out);
  fstream op(file_path.c_str(), std::fstream::in | std::fstream::out | std::fstream::binary);
  op.seekp(chunk_no * CHUNKSIZE, ios::beg);
 
  long long n;
  memset(Buffer , 0, SIZE);

  long long cnt = 0;
  while (( n = recv( new_socket , Buffer ,   SIZE, 0) ) > 0  &&  chunk_size > 0)
  {
    //cout << Buffer  << endl;
    //cout << " **|** " << n << endl;
    op.write(Buffer, n);
    cnt += n;
    memset(Buffer , 0, SIZE);
    chunk_size = chunk_size - n;
    //cout<<"CHUNK SIZE IS......"<<chunk_size<<endl;
    if(n<=0 || chunk_size<=0)
    {
      break;
    }
  }
  cout << "Chunk no Received: " << chunk_no << " | " << cnt << endl;
  op.close();
}




void handle_connection(int client_socket){
    // char buffer[SIZE];
    // bzero(buffer, SIZE);
    // int bytesread = recv(client_socket, buffer, sizeof(buffer), 0);

    // bzero(buffer, SIZE);
    // strcpy(buffer, "abhi file nhi dunga");
    // // printf("Server: %s\n", buffer);
    // send(client_socket,buffer,strlen(buffer),0);         

///////////////////////////////
  
  char buffer[SIZE];
  bzero(buffer, SIZE);
  //recv(client_socket, buffer, sizeof(buffer), 0);
  recv(client_socket, buffer, SIZE, 0);
  printf("other clent: %s\n", buffer);
  string msg = buffer;
  auto vec = str_split(msg, ' ');

  string file_name = vec[1];
  if( vec[0] == "get_chunk_vector"){
    // string file_path = file_path_map[file_name];
    // string chunks = array_to_string(file_piece_map[file_name]);
    // long long filesize = file_size_map[file_name];
    string gid = vec[2];

    string file_path = gid_file_path_map[gid][file_name];
    string chunks = array_to_string(gid_file_piece_map[gid][file_name]);
    long long filesize = gid_file_size_map[gid][file_name];

    string filesize_str = to_string(filesize);
    bzero(buffer, SIZE);
    msg = chunks+"/"+filesize_str;
    strcpy(buffer, msg.c_str());
    printf("%s", buffer);
    send(client_socket, buffer, strlen(buffer), 0);
    //send(client_socket, buffer, SIZE, 0);
    cout << file_name << " " << file_path << " $$$$$$$$$$"<< endl;
  }
  //string msg = "download_chunk "+file_name+ " " + to_string(chunk_no) + " \n";
  
  else if( vec[0] == "download_chunk" ) {

    string gid = vec[3];
    string file_path = gid_file_path_map[gid][file_name];
    long long filesize = gid_file_size_map[gid][file_name];
    int total_chunks = ceil(filesize/(CHUNKSIZE*1.0));
    int last_chunk_no = total_chunks-1;

    int chunk_no = stoi(vec[2]);
    int chunk_size = chunk_no == last_chunk_no ? filesize - (total_chunks-1)*CHUNKSIZE : CHUNKSIZE;
    
    bool lastChunk = (chunk_no == last_chunk_no);


    string filesize_str = to_string(filesize);
    bzero(buffer, SIZE);
    msg = filesize_str;
    strcpy(buffer, msg.c_str());
    printf("%s", buffer);
    send(client_socket, buffer, sizeof(buffer), 0);


    cout << "file_path: " << file_path << endl;
    cout << "chunk_size: " << chunk_size << endl;
    cout << "chunk_no: " << chunk_no << endl;

    transfer_chunk( client_socket, chunk_size, chunk_no, lastChunk, file_path);
    cout << "data sent to other clent" << endl;
    

  }

  sleep(1);
  close(client_socket);

  //long long filesize=GetFileSize(file_path);
  // send_file(client_socket, file_path );
  // transferFile(client_socket, filesize, file_path);
  // cout << "data sent to other clent" << endl;
  // close(client_socket);
}



//void server_func()
void* server_func(void *)
{
    int peer_socket=-1;
    struct sockaddr_in  peer_socket_address;
    
    if ((peer_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
    { 
      cout << "tracker socket failed...." << endl;
    } 
    
    bzero(&peer_socket_address,sizeof(peer_socket_address));
    peer_socket_address.sin_family=AF_INET;
    peer_socket_address.sin_addr.s_addr=inet_addr(client_ip.c_str());
    peer_socket_address.sin_port = htons(client_port);

    int optval =0;
    int optlen = sizeof(optval);
    setsockopt(peer_socket,SOL_SOCKET,SO_REUSEPORT,(char *)&optval,optlen);

    if(bind(peer_socket,(struct sockaddr *)(&peer_socket_address),sizeof(peer_socket_address)) < 0)
    {
        cout << ("Tracker... Unable to Bind") << endl;
        exit(0);
    }
    if(listen(peer_socket,SERVER_BUFFER) < 0)
    {
        //onend("Tracker .... Listening Failed..");
        cout << ("Tracker .... Listening Failed..") << endl;
        exit(0);
    }
    cout<<"Listening on IP :- "<<client_ip<<" and port no.:- "<<client_port<<endl; 
    vector<thread> t_vect;
       
    while(true)
    {
              

        int client_socket= accept(peer_socket,(struct sockaddr *)NULL,NULL);
        if(client_socket < 0)
        {
            cout << ("Client accept error...") << endl;
            exit(0);
        }

        //cout<<"connected..:"<<client_socket<<endl;
    
        
        t_vect.push_back(thread(handle_connection,client_socket));   
        
        
    }

   for (auto &th:t_vect)
   {
       if(th.joinable())
       {
           th.join();
       }
   }

  close(peer_socket);
  return NULL;
}



string get_tracker_info_path(string argv){
  return "../"+argv;
}


string send_util( int &client_socket,string input )
{   
  int bytesread; 
  char recv_buffer[SIZE];
  char send_buffer[SIZE];
  strcpy(send_buffer,input.c_str());   
  send(client_socket,send_buffer,strlen(send_buffer),0); 
  //send(client_socket, send_buffer, SIZE, 0);

  bytesread = read(client_socket,recv_buffer,SIZE);
  // if(bytesread < 1)
  // {
  //   close(client_socket);
  //   onend("Peer Terminated..");
  // }
  recv_buffer[bytesread]=0;      // OR bzero(recv_buffer, SIZE);
  cout << recv_buffer << endl;
  string rec_str = recv_buffer;
  return rec_str;
}

vector<string> connect_client(int final_port, string file_name, string gid ){
  struct sockaddr_in addr;
  int sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0){
    perror("[-]Socket error");
    exit(1);
  }
    //printf("[+]TCP server socket created.\n");
  addr.sin_family = AF_INET;
  addr.sin_port = htons(final_port);
  addr.sin_addr.s_addr = inet_addr(client_ip.c_str());

  int e = connect(sock, (struct sockaddr*)&addr, sizeof(addr));
  if(e == -1) {
    perror("[-]Error in socket");
    exit(1);
  }

  char buffer[SIZE];
  bzero(buffer, SIZE);
  string msg = "get_chunk_vector "+file_name+ " " + gid + " \n";
  strcpy(buffer, msg.c_str());
  printf("%s", buffer);
  send(sock, buffer, strlen(buffer), 0);
  //send(sock, buffer, SIZE, 0);

  bzero(buffer, SIZE);
  //recv(sock, buffer, sizeof(buffer), 0);
  recv(sock, buffer, SIZE, 0);
  // printf("file size recv: %s\n", buffer);
  msg = buffer;
  auto vec1 = str_split(msg, '/');
  long long filesize = stoi(vec1[1]);

  cout << "Port "<< final_port << "has chunks: " << vec1[0] << endl;
  vector<string> chunk_vec = str_split(vec1[0], ' ');
  return chunk_vec;
}

vector<int> piece_selection(map<int, vector<int> > mpp){
  int n = mpp.size();
  vector<int> ports;
  for( int i = 0; i < n; i++ ){
    int m = mpp[i].size();
    int r = ( std::rand() % ( m ) );
    ports.push_back(mpp[i][r]);
  }
  return ports;
}


void handleDownload( int cl_port, int chunk_no, int total_chunks, string file_name, string full_path, string gid ){

  struct sockaddr_in addr;
  int sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0){
    perror("[-]Socket error");
    exit(1);
  }

  addr.sin_family = AF_INET;
  addr.sin_port = htons(cl_port);
  addr.sin_addr.s_addr = inet_addr(client_ip.c_str());

  int e = connect(sock, (struct sockaddr*)&addr, sizeof(addr));
  if(e == -1) {
    perror("[-]Error in socket");
    exit(1);
  }

  char buffer[SIZE];
  bzero(buffer, SIZE);
  string msg = "download_chunk "+file_name+ " " + to_string(chunk_no)+ " " + gid + " \n";
  strcpy(buffer, msg.c_str());
  printf("%s", buffer);
  send(sock, buffer, strlen(buffer), 0);


  bzero(buffer, SIZE);
  recv( sock , buffer ,   SIZE, 0);
  string s = buffer;
  long long filesize = stoll(s);
  cout << "***filesize***:" << filesize << " | " << full_path << endl;


  //send(sock, buffer, SIZE, 0);
  bool lastChunk = (chunk_no == total_chunks-1);

  //pthread_mutex_lock(&lock1);
  //mtx.lock();
  receiveChunk( sock, chunk_no, lastChunk, full_path);
  //pthread_mutex_unlock(&lock1);
  //mtx.unlock();
  close(sock);

  
  if( gid_file_piece_map.count(gid) && gid_file_piece_map[gid].count(file_name) ){
    gid_file_piece_map[gid][file_name].push_back(chunk_no);
    cout << "chunk info updated 2 :" << chunk_no << endl;
  }
  else{
    vector<int> v;
    v.push_back(chunk_no);
    gid_file_piece_map[gid][file_name]=v;
    cout << "chunk info updated 1 :" << chunk_no << endl;
  }

  sort(gid_file_piece_map[gid][file_name].begin(), gid_file_piece_map[gid][file_name].end());
  cout << gid << " " << file_name << " " << chunk_no << endl;



  if( chunkDownloaded ){
    
    chunkDownloaded = false;
    struct sockaddr_in tracker_addr;
    char buffer[SIZE];
    gid_file_size_map[gid][file_name] = filesize;
    gid_file_path_map[gid][file_name] = full_path;

    int tracker_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (tracker_sock < 0){
      perror("[-]Socket error");
      exit(1);
    }
    printf("[+]TCP server socket created.\n");

    memset(&tracker_addr, '\0', sizeof(tracker_addr));
    tracker_addr.sin_family = AF_INET;
    tracker_addr.sin_port = tracker_port;
    tracker_addr.sin_addr.s_addr = inet_addr(IP_ADDR);

    int e = connect(tracker_sock, (struct sockaddr*)&tracker_addr, sizeof(tracker_addr));
    if(e == -1) {
      perror("[-]Error in socket");
      exit(1);
    }

    //char buffer[SIZE] = {0};
    bzero(buffer, SIZE);
    string msg = "update_file_port_map "+ gid + " " + file_name + " " + uname_loggedin + " \n";
    strcpy(buffer, msg.c_str());
    printf("%s", buffer);
    send(sock, buffer, strlen(buffer), 0);
    //send(sock, buffer, SIZE, 0);

    bzero(buffer, SIZE);
  }

}

void multi_thread_func( map<int, vector<int> > piece_cl_mpp, string file_name, string full_path, string gid ){
    vector<int> selection_vec = piece_selection(piece_cl_mpp);
    int total_chunks = selection_vec.size();

    vector<thread> t_vect;

    for( int i = 0; i < total_chunks; i++ )
    {
      int cl_port = selection_vec[i];
      int chunk_no = i;

      //printf("[+]TCP server socket created.\n");
      handleDownload(cl_port,chunk_no,total_chunks,file_name,full_path, gid);
      //t_vect.push_back( thread(handleDownload,cl_port,chunk_no,total_chunks,file_name,full_path,gid) );
      //t_vect.push_back(thread(receiveChunk,sock, chunk_no, lastChunk, full_path));     
    }

    for (auto &th:t_vect)
    {
      if(th.joinable())
      {
        th.join();
      }
    }

    chunkDownloaded = true;
    gid_file_status[gid][file_name] = "C";
    cout << "File has been Downloaded" << endl;
}


void parsecommand(vector<string>&vec,string &input,int client_socket)
{


  //  string Errstr = "INVALID COMMAND OR ARGUMENTS...";
  //  if(args.size()==0)
  //  cout<< Errstr <<endl;

  string cmd=vec[0];
  if(cmd=="create_user")
  { 
   send_util(client_socket,input);
  }
  else if(cmd=="login")
  {
    send_util(client_socket,input+" "+to_string(client_port));
    uname_loggedin = vec[1];
  }
  else if(cmd=="create_group")
  {
    send_util(client_socket,input+" "+uname_loggedin);
  }
  // else if(cmd=="create_group")
  // {
  //   send_util(client_socket,input+" "+uname_loggedin);
  // }
  else if(cmd=="join_group")
  {
    send_util(client_socket,input+" "+uname_loggedin);
  }
  else if(cmd=="list_groups")
  {
    send_util(client_socket,input+" "+uname_loggedin);
  }
  else if(cmd=="list_requests")
  {
    send_util(client_socket,input+" "+uname_loggedin);
  }
  else if(cmd=="accept_request")
  {
    send_util(client_socket,input+" "+uname_loggedin);
  }
  else if(cmd=="leave_group")
  {
    send_util(client_socket,input+" "+uname_loggedin);
  }
  else if(cmd=="list_files")
  {
    send_util(client_socket,input+" "+uname_loggedin);
  }
  else if(cmd=="stop_share")
  {
    send_util(client_socket,input+" "+uname_loggedin);
  }
  else if(cmd == "logout")
  {
    send_util(client_socket,input+" "+uname_loggedin);
    isloggedin = false;
    uname_loggedin = "";
  }
  else if(cmd == "upload_file")
  {
    send_util(client_socket,input+" "+uname_loggedin);
    
    auto vec2 = str_split(input, ' ');
    int n2 = vec2.size();
    string gid = vec2[n2-1];

    auto vec1 = str_split(vec2[n2-2], '/');
    int n1 = vec1.size();
    string file_name = vec1[n1-1];

    cout << file_name << " " << input << endl;
    string full_path = vec2[n2-2];

    //file_path_map[file_name] = full_path;
    gid_file_path_map[gid][file_name] = full_path;

    long long file_size = GetFileSize(full_path);
    //file_size_map[file_name] = file_size;
    gid_file_size_map[gid][file_name] = file_size;


    int chunks = (int)ceil(file_size/(CHUNKSIZE*1.0));
    cout << "no of chunks: " << chunks << " file_size: " << file_size << endl;
    
    //vector<int> v = {0,2,4,6,8,10,12};
    //vector<int> v = {1,3,5,7,9,11};
    vector<int> v;
    for( int i = 0; i < chunks; i++ ){
      //if( i % 2 == 1 ){
        v.push_back(i);
      //}
    }

    // if( v.size() == 0 ){
    //   for( int i = 0; i < chunks; i++ ){
    //       v.push_back(i);
    //   }
    // }

    //file_piece_map[file_name] = v;
    gid_file_piece_map[gid][file_name] = v;
  }

  else if(cmd == "show_downloads")
  {
    //cout << "hello" << endl;
    if( (int)gid_file_status.size() == 0 ){
      cout << "No info yet" << endl;
      return;
    }
    for( auto file_status : gid_file_status ){
      string gid = file_status.first;
      auto file_status_map = file_status.second;
      for( auto status : file_status_map ){
        string file_name = status.first;
        string stat = status.second;
        cout << stat << " " << gid << " " << file_name << endl;
      }
    }
  }

  else if(cmd == "download_file")
  {
    string cl_ports = send_util(client_socket,input+" "+uname_loggedin);
    cout << "ports: "<< cl_ports << endl;

    string gid = vec[1];
    string full_path = vec[3] + vec[2];
    string file_name = str_split(input, ' ')[2];

    cout << file_name << endl;

    vector<string> cl_ports_arr = str_split(cl_ports,' ');
    //cout << final_port << endl;
    map<string, vector<string> > cl_piece_mpp;
    map<int, vector<int> > piece_cl_mpp;

    for( int i = 0; i < cl_ports_arr.size(); i++ ){
      int final_port = stoi(cl_ports_arr[i]);
      vector<string> chunk_vec = connect_client(final_port, file_name, gid);
      cl_piece_mpp[cl_ports_arr[i]] = chunk_vec;

      for( int i = 0; i < chunk_vec.size(); i++ ){
        if( piece_cl_mpp.count(stoi(chunk_vec[i])) == 0 ){
          vector<int> v;
          v.push_back(final_port);
          piece_cl_mpp[stoi(chunk_vec[i])] = v;
        }
        else{
          piece_cl_mpp[stoi(chunk_vec[i])].push_back(final_port);
        }
      }
    }

    gid_file_status[gid][file_name] = "D";

    //thread(multi_thread_func, piece_cl_mpp,file_name,full_path,gid);

    vector<int> selection_vec = piece_selection(piece_cl_mpp);
    int total_chunks = selection_vec.size();

    ofstream MyFile(full_path.c_str());
		MyFile.close();

    vector<thread> t_vect;


    set<int> st;
    int cnt = 0;
    while( cnt < total_chunks ){
      int i = ( std::rand() % ( total_chunks ) );
      if( st.count(i) > 0 ){
        continue;
      }
      int cl_port = selection_vec[i];
      int chunk_no = i;
      handleDownload(cl_port,chunk_no,total_chunks,file_name,full_path, gid);
      st.insert(i);
      cnt++;
    }



    // for( int i = 0; i < total_chunks; i++ )
    // {
    //   int cl_port = selection_vec[i];
    //   int chunk_no = i;
    //   //printf("[+]TCP server socket created.\n");
    //   handleDownload(cl_port,chunk_no,total_chunks,file_name,full_path, gid);
    //   // t_vect.push_back( thread(handleDownload,cl_port,chunk_no,total_chunks,file_name,full_path,gid) );
    //   //t_vect.push_back(thread(receiveChunk,sock, chunk_no, lastChunk, full_path));     
    // }

    // for (auto &th:t_vect)
    // {
    //   if(th.joinable())
    //   {
    //     th.join();
    //   }
    // }

    chunkDownloaded = true;
    gid_file_status[gid][file_name] = "C";
    cout << "File has been Downloaded" << endl;


    // close(sock);
  }

}

int main(int argc,char * argv[]){
  
  FILE * tracker_info;
  string portip;
  // if (pthread_mutex_init(&lock1, NULL) != 0) {
  //   printf("\n mutex init has failed\n");
  //   return 1;
  // }
  
  tracker_info = fopen(get_tracker_info_path(argv[2]).c_str(),"r");
  if(tracker_info == NULL)
  {
        cout << ("No such tracker info file..") << endl;
        exit(0);
  }
  portip = argv[1];

  client_ip = portip.substr(0,portip.find(":")); 
  client_port = stoi(portip.substr(portip.find(":")+1));

  cout <<  client_ip << ":" << client_port << endl;

  // char IP_ADDR[50];
  // short unsigned int tracker_port;

  fscanf(tracker_info,"%s",IP_ADDR);
  fscanf(tracker_info,"%hu",&tracker_port);
  fclose(tracker_info);

  cout <<  "tracker: " << IP_ADDR << ":" << tracker_port << endl;
  string tracker_ip = IP_ADDR;
  

  pthread_t server_thread;
  if(pthread_create(&server_thread,NULL,server_func,NULL) < 0)
  {
    cout << ("Error in Running as Server..") << endl;
    exit(0);
  }
  //thread(server_func);

  struct sockaddr_in tracker_addr;
  char buffer[SIZE];

  int tracker_sock = socket(AF_INET, SOCK_STREAM, 0);
  if (tracker_sock < 0){
    perror("[-]Socket error");
    exit(1);
  }
  printf("[+]TCP server socket created.\n");

  memset(&tracker_addr, '\0', sizeof(tracker_addr));
  tracker_addr.sin_family = AF_INET;
  tracker_addr.sin_port = tracker_port;
  tracker_addr.sin_addr.s_addr = inet_addr(IP_ADDR);

  connect(tracker_sock, (struct sockaddr*)&tracker_addr, sizeof(tracker_addr));
  printf("Connected to the server.\n");


  // bzero(buffer, SIZE);
  // strcpy(buffer, portip.c_str());
  // send(sock, buffer, strlen(buffer), 0);

  while(true)
  {
    int bytesread;
    string input;
    getline(cin,input);
    if(input.size()==0)
      continue;

    // bzero(buffer, SIZE);
    // strcpy(buffer, input.c_str());
    // send(tracker_sock, buffer, strlen(buffer), 0);

    vector<string> args = str_split(input,' ');
    parsecommand(args, input, tracker_sock);
   }
   close(tracker_sock);
  
   //pthread_mutex_destroy(&lock1);
}




// int main(){

//   const char *ip = "127.0.0.1";
//   int tracker_port = 5566;

//   int sock;
//   struct sockaddr_in tracker_addr;
//   socklen_t addr_size;
//   char buffer[SIZE];
//   int n;

//   sock = socket(AF_INET, SOCK_STREAM, 0);
//   if (sock < 0){
//     perror("[-]Socket error");
//     exit(1);
//   }
//   printf("[+]TCP server socket created.\n");

//   memset(&tracker_addr, '\0', sizeof(tracker_addr));
//   tracker_addr.sin_family = AF_INET;
//   tracker_addr.sin_port = tracker_port;
//   tracker_addr.sin_addr.s_addr = inet_addr(ip);

//   connect(sock, (struct sockaddr*)&tracker_addr, sizeof(tracker_addr));
//   printf("Connected to the server.\n");


//   //bzero(buffer, SIZE);
//   //strcpy(buffer, "HELLO, THIS IS CLIENT.");
//   //printf("Client: %s\n", buffer);

//   string s;
//   cin >> s;
//   bzero(buffer, SIZE);
//   strcpy(buffer, s.c_str());
//   send(sock, buffer, strlen(buffer), 0);

//   sleep(2);

//   bzero(buffer, SIZE);
//   recv(sock, buffer, sizeof(buffer), 0);
//   s = buffer;
//   printf("Server: %s\n", buffer);

//   if( s == "success, user has been created" ){
//       string s;
//       cin >> s;
//       bzero(buffer, SIZE);
//       strcpy(buffer, s.c_str());
//       send(sock, buffer, strlen(buffer), 0);
//   }

//   // send(sock, "DOBARA", strlen(buffer), 0);

//   // close(sock);
//   // printf("Disconnected from the server.\n");

//   return 0;

// }