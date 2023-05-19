#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include<iostream>
#include <map>
#include <vector>
#include<pthread.h>
#include<bits/stdc++.h>
#define SERVER_BUFFER (100)
#define SIZE 16*1024
// #define SIZE 100

std::map< std::string, std::string> user_pass;
std::map< std::string, std::string > user_port_map;
std::map< std::string, bool> user_logged;
//std::map< std::string, std::vector<std::string> > file_port;
std::map< std::string, std::map< std::string, std::vector<std::string> > > gid_files_user;

std::map< std::string, std::map< std::string, std::map< std::string, bool> > > file_is_sharable;
// gid_user_file

std::map< std::string, std::vector<std::string> > gid_users;
std::map< std::string, std::vector<std::string> > gid_req;
std::map< std::string, std::vector<std::string> > gid_files;


std::vector<std::string> str_split(std::string s, char delim)
{
    std::string ans;
    std::vector<std::string>res;
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

std::string array_to_string( std::vector<std::string> vec ){
  std::string res = "";
  int n = (int)vec.size();
  for( int i = 0; i < n; i++ ){
    if( i != n-1 )
      res += vec[i]+" ";
    else
      res += vec[i];
  }
  return res;
}

bool isMember( std::string user, std::vector<std::string> members ){
  for( auto member : members ){
    if( member == user ){
      return true;
    }
  }
  return false;
}

// bool isPresent( std::string user, std::vector<std::string> vec ){
//       int n = vec.size();
//       for( int i = 0; i < n; i++ ){
//         if( vec[i] == user )
//           return true;
//       }
//       return false;
//     }
bool isAnyFileSharable(std::string gid, std::string filename){
        auto users = gid_users[gid];
        for( auto user: users ){
          if( file_is_sharable[gid][user][filename] )
            return true;
          }
        return false;
      }

std::string parse_commands(std::vector<std::string> vec){
  int n = vec.size();
  std::string cmd = vec[0];
  std::cout << cmd << std::endl;
  if( cmd == "create_user" ){
    if( n != 3 ){
      return "Invalid command";
    }
    std::string user = vec[1];
    std::string pass = vec[2];
    if( user_logged[user] ){ 
      return "Invalid: user must logout to create new user";
    }
    else if( user_pass.count(user) > 0 ){ 
      return "user already exists";
    }
    else{
      user_pass[user] = pass;
      user_logged[user] = false;
      return "user created successfully";
    }
  }
  
  if( cmd == "login" ){
    if( n != 4 ){
      return "Invalid command";
    }
    std::string user = vec[1];
    std::string pass = vec[2];
    std::string cl_port = vec[3];

    if( user_logged[user] ){ 
      return "user already logged in";
    }
    if( user_pass.count(user) == 0 ){
      return "user does not exist";
    }
    if( user_pass[user] == pass ){
      user_logged[user] = true;
      user_port_map[user] = cl_port;
      std::cout << user << " " << pass << " " << cl_port << std::endl;
      return "user logged in successfully";
    }
    else{
      return "incorrect password";
    }
  }

  if( cmd == "create_group" ){
    if( n != 3 ){
      return "Invalid command";
    }
    std::string gid = vec[1];
    std::string user = vec[2];
    if( !user_logged[user] ){ 
      return "User must be logged in";
    }
    if( gid_users.count(gid) > 0 )
    {
      return "Group already exists";
    }
    std::vector<std::string> v;
    gid_req[gid] = v;
    v.push_back(user);
    gid_users[gid] = v;
    return "Group "+ gid +" created successfully";
  }

  if( cmd == "join_group" ){
    if( n != 3 ){
      return "Invalid command";
    }
    std::string gid = vec[1];
    std::string user = vec[2];
    if( !user_logged[user] ){ 
      return "User must be logged in";
    }
    if( gid_users.count(gid) == 0 )
    {
      return "Group does not exists";
    }
    if( isMember(user, gid_users[gid]) ){
      return "User is already member of the group";
    }
    if( isMember(user, gid_req[gid]) ){
      return "Request has already been sent to join the group " + gid + " by " + user;
    }
    gid_req[gid].push_back(user);
    return "request sent to join the group " + gid + " by " + user;
  }

  if( cmd == "list_groups" ){
    if( n != 2 ){
      return "Invalid command";
    }
    std::string user = vec[1];
    if( !user_logged[user] ){ 
      return "User must be logged in";
    }
    std::string str = "";
    int n = gid_users.size();
    for( auto gid: gid_users ){
      std::cout << gid.first << std::endl;
      if( n == 1 )               // if last element of list
        str += gid.first;
      else
        str += (gid.first+"\n");
      n--;
    }
    if( str == "" ){
      str = "0 groups yet";
    }
    return str;
  }

  if( cmd == "list_requests" ){
    if( n != 3 ){
      return "Invalid command";
    }
    std::string gid = vec[1];
    std::string user = vec[2];

    if( !user_logged[user] ){ 
      return "User must be logged in";
    }
    if( gid_users.count(gid) == 0 )
    {
      return "Group does not exists";
    }
    if( !isMember(user, gid_users[gid]) ){
      std::cout << user << std::endl;
      return "User is not a member of the given group";
    }
    if( std::find(gid_users[gid].begin(), gid_users[gid].end(), user) != gid_users[gid].begin() ){ 
      return "User must be an admin";
    }

    std::vector<std::string> requests = gid_req[gid];
    std::string str = "";
    int n = requests.size();
    for( auto req: requests ){
      std::cout << req << std::endl;
      if( n == 1 )               // if last element of list
        str += req;
      else
        str += (req+"\n");
      n--;
    }
    if( str == "" ){
      str = "No requests yet";
    }
    return str;
  }

  //accept_request <group_id> <user_id>
  if( cmd == "accept_request" ){
    if( n != 4 ){
      return "Invalid command";
    }
    std::string gid = vec[1];
    std::string user = vec[2];

    std::string myself_user = vec[3];

    if( !user_logged[myself_user] ){ 
      return "User must be logged in";
    }
    if( gid_users.count(gid) == 0 )
    {
      return "Group does not exists";
    }
    if( !isMember(myself_user, gid_users[gid]) ){
      return "User is not a member of the given group";
    }
    if( std::find(gid_users[gid].begin(), gid_users[gid].end(), myself_user) != gid_users[gid].begin() ){ 
      return "User must be an admin";
    }
    // auto position1 = std::find(gid_users[gid].begin(), gid_users[gid].end(), user);
    // if (position1 != gid_users[gid].end()) {
    // }
    //auto position = std::find(requests.begin(), requests.end(), user);


    auto requests = gid_req[gid];
    int position = -1;
    for( int i = 0; i < requests.size(); i++ ){
      if( user == requests[i] ){
        position = i;
      }
      
    }

    if ( position != -1 ) {
        requests.erase( requests.begin() + position );
        gid_req[gid] = requests;
        //gid_users[gid].erase(position);
        gid_users[gid].push_back(user);
        return "request to join group accepted for user: " + user;
    }
    else{
      return "no such request exist by user " + user;
    }
  }

  if( cmd == "leave_group" ){
    if( n != 3 ){
      return "Invalid command";
    }
    std::string gid = vec[1];
    std::string user = vec[2];
    if( !user_logged[user] ){ 
      return "User must be logged in";
    }
    if( gid_users.count(gid) == 0 )
    {
      return "Group does not exists";
    }
    if( !isMember(user, gid_users[gid]) ){
      return "User is not a member of the given group";
    }
    auto all_users = gid_users[gid];
    //auto position = std::find(all_users.begin(), all_users.end(), user);
    int position = -1;
    for( int i = 0; i < all_users.size(); i++ ){
      if( user == all_users[i] ){
        position = i;
      }
    }

    std::cout << "*****" << std::endl;
    for( int i = 0; i < all_users.size(); i++ ){
      std::cout << all_users[i] << std::endl;
    }
    std::cout << std::endl;

    if (position != -1 ) {// == myVector.end() means the element was not found
      //gid_users[gid].erase(position);
      all_users.erase( all_users.begin() + position );
      gid_users[gid] = all_users;
    }

    for( int i = 0; i < gid_users[gid].size(); i++ ){
      std::cout << gid_users[gid][i] << std::endl;
    }

    return "User " + user + " left the group "+ gid;
  }

  if( cmd == "list_files" ){
    if( n != 3 ){
      return "Invalid command";
    }
    std::string gid = vec[1];
    std::string user = vec[2];

    if( !user_logged[user] ){ 
      return "User must be logged in";
    }
    if( gid_users.count(gid) == 0 )
    {
      return "Group does not exists";
    }
    std::cout << gid << " | "<< gid_files.count(gid) << std::endl;

    std::string str = "";
    if( gid_files.count(gid) ){
      auto vec = gid_files[gid];
      int n = vec.size();

      for( auto it: vec ){
        n--;
        // if( ! isMember( user, gid_files_user[gid][it] ) ){
        //   continue;
        // }
        std::cout << it << std::endl;
        std::cout << gid <<  " " << user << " " << it << " " << file_is_sharable[gid][user][it] << std::endl;
        
        if( !isAnyFileSharable(gid,it)  ){
          continue;
        }

        std::cout << it << std::endl;
        // if( n == 1 )               // if last element of list
        //   str += it;
        // else
          str += (it+"\n");
      }
    }

    if( str == "" ){
      str = "No file present in group";
    }
    return str;
  }
  
  if( cmd == "logout" ){
    if( n != 2 ){
      return "Invalid command";
    }
    std::string user = vec[1];
    if( user_logged.count(user) > 0 )
    {
      user_logged[user] = false;
    }
    return "Client Logged Out..";
  }
  
  if( cmd == "stop_share" ){
    if( n != 4 ){
      return "Invalid command";
    }
    std::string gid = vec[1];
    std::string file_name = vec[2];
    std::string user = vec[3];
    file_is_sharable[gid][user][file_name] = false;
    return "stop_share "+ gid + " " + user + " " + file_name;
  }

  if( cmd == "upload_file" ){
    if( n != 4 ){
      return "Invalid command";
    }
    std::string file_path = vec[1];
    std::string gid = vec[2];
    std::string user = vec[3];
    auto file_vec = str_split(file_path, '/');
    int s = file_vec.size();
    std::string file_name = file_vec[s-1];

    std::string user_port = user_port_map[user];
    std::cout << file_path << " " << user << " " << user_port << "..." << std::endl;
    

    if( !user_logged[user] ){ 
      return "User must be logged in";
    }
    if( gid_users.count(gid) == 0 )
    {
      return "Group does not exists";
    }
    if( !isMember(user, gid_users[gid]) ){
      return "User is not a member of the given group";
    }

    file_is_sharable[gid][user][file_name] = true;
    std::cout << gid <<  " " << user << " " << file_name << " " << file_is_sharable[gid][user][file_name] << std::endl;

    if( gid_files.count(gid) == 0 ){
      std::vector<std::string> v;
      v.push_back(file_name);
      gid_files[gid] = v;
    }
    else{
      gid_files[gid].push_back(file_name);
    }
    std::cout << file_name << ": added in gid_files of group: "<<  gid << std::endl;
    

    if( gid_files_user[gid].count(file_name) == 0 ){
      std::vector<std::string> v;
      //v.push_back(user_port);
      v.push_back(user);
      std::cout << file_name << " " << user << std::endl;
      gid_files_user[gid][file_name] = v;
    }
    else{
      //file_port[file_name].push_back(user_port);
      //gid_files_port[gid][file_name].push_back(user_port);
      gid_files_user[gid][file_name].push_back(user);
    }

    return "file uploaded successfully";
  }

  if( cmd == "download_file" ){
    std::string gid = vec[1];
    std::string file_name = vec[2];
    std::string user = vec[4];

    if( !user_logged[user] ){ 
      return "User must be logged in";
    }
    if( gid_users.count(gid) == 0 )
    {
      return "Group does not exists";
    }
    if( !isMember(user, gid_users[gid]) ){
      return "User is not a member of the given group";
    }
    if( gid_files_user[gid].count(file_name) == 0 ){
      return "File does not exist in the group";
    }

    std::cout << "file ka naam: " << file_name << std::endl;
    // if( file_port.count(file_name) == 0 ){
    //   return "file does not exists";
    // }

    std::vector<std::string> logged_in_users;
    //auto ports = gid_files_port[gid][file_name];
    auto users = gid_files_user[gid][file_name];

    for( int i = 0; i < users.size(); i++ ){
      if( user_logged[users[i]] && file_is_sharable[gid][users[i]][file_name] )
        logged_in_users.push_back(users[i]);
    }

    std::vector<std::string> logged_in_ports;
    int n = logged_in_users.size();
    for( int i = 0; i < n; i++ ){
      std::cout << logged_in_users[i] << " ";
      logged_in_ports.push_back(user_port_map[logged_in_users[i]]);
    }
    std::cout << "\n";

    std::string str_port = array_to_string(logged_in_ports);
    std::cout << "ports ka naam: " << str_port << std::endl;
    return str_port;
  }

  if( cmd == "update_file_port_map" ){
    std::string gid = vec[1];
    std::string file_name = vec[2];
    std::string user = vec[3];
    std::string port = user_port_map[user];
    std::cout << "file ka naam: " << file_name << std::endl;
    if( gid_files.count(gid) == 0 ){
      return "group does not exists";
    }
    if( gid_files_user[gid].count(file_name) == 0 ){
      return "file does not exists";
    }
    gid_files[gid].push_back(file_name);
    gid_files_user[gid][file_name].push_back(user);
    return "tracker update: "+ user + " with port " + port + " has now "+ file_name + " file";
  }

  return "";
}

void handle_connection(int client_sock, sockaddr_in client_addr) {

  char send_buffer[SIZE];
  char recv_buffer[SIZE];
  std::string reply;
  //std::cout << "started " << client_sock << std::endl;
  while(1){
    
    bzero(recv_buffer, SIZE);
    int bytesread =  recv(client_sock, recv_buffer, sizeof(recv_buffer), 0);
    if(bytesread < 1)
    {
      close(client_sock);
      std::cout << "Peer Terminated.." << std::endl;
      return;
    }

    std::string str = recv_buffer;
    std::cout << str << std::endl;
    std::vector<std::string> vec = str_split(str,' ');
    int n = vec.size();

    for( int i = 0; i < n; i++ ){
      std::cout << vec[i] << " ";
    }
    std::cout << "\n";

    //std::cout << n << std::endl;
    if( n == 0 )
      continue;

    if(vec[0] == "close()")
    {         
      // if(user_logged.find(uname) != User_map.end())
      // {
      //     user_logged[uname].isloggedIn = false;
      // }
      std::cout<<"User " <<client_sock<<" logged out....\n";
      break;
    }
    else{
        reply = parse_commands(vec);
        if( reply == "" )
          continue;
    }

    strcpy(send_buffer,reply.c_str());
    std::cout << send_buffer << std::endl;
    send(client_sock,send_buffer,strlen(send_buffer),0);  
    //send(client_sock,send_buffer,SIZE,0);  
  }

  close(client_sock);
}

int main(){

  const char *ip = "127.0.0.1";
  int tracker_port = 5566;

  int server_sock, client_sock;
  struct sockaddr_in server_addr, client_addr;
  socklen_t addr_size;
  char buffer[SIZE];
  int n;

  server_sock = socket(AF_INET, SOCK_STREAM, 0);
  if (server_sock < 0){
    perror("[-]Socket error");
    exit(1);
  }
  std::cout << ("[+]TCP server socket created.\n") << std::endl;

  memset(&server_addr, '\0', sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = tracker_port;
  server_addr.sin_addr.s_addr = inet_addr(ip);

  n = bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr));
  if (n < 0){
    perror("[-]Bind error");
    exit(1);
  }
  std::cout << "[+]Bind to the port number: " << tracker_port << std::endl;

  listen(server_sock, SERVER_BUFFER);
  std::cout << "Listening..." << std::endl;


  std::vector<std::thread> t_vect;
  addr_size = sizeof(client_addr);

  while(1){
    
    client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &addr_size);
    if(client_sock < 0)
    {
      std::cout << ("Client accept error...") << std::endl;
      exit(0);
    }
    std::cout << "Client Connected " << client_sock << std::endl;
    //printf("ip:port of client: %s\n", buffer);

    t_vect.push_back(std::thread(handle_connection,client_sock,client_addr));  
  }

   for (auto &th:t_vect)
   {
       if(th.joinable())
       {
           th.join();
       }
   }
    
    
 close(server_sock);

  return 0;
}