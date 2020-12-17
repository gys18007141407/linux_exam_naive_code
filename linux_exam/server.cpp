
#include <iostream>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <thread>
#include <vector>
#include <dlfcn.h>
#include <dirent.h>
#include <mutex>
#include <condition_variable>
#include "serialize.h"

std::vector<std::thread> threads;
std::vector<std::string> myPlugins;

std::string plugin_path = "./plugin/";  // plugins' dictionary

std::ofstream ofs;
std::ifstream ifs;

//use file orderly
std::mutex file_mutex;

std::condition_variable cond_file;   // unfamiliar to use this

std::string servercmd;
bool cmdIsRead;

const int LISTEN_LEN = 5;
const int MAX_THREAD = 3;
const int BUFFER_LEN = 1024;

void getPlugin(std::vector<std::string> &plugins){  // server's plugin
    DIR *dir = opendir("./plugin");
  
    if(NULL == dir) return;

    struct dirent * cur = NULL;
    while(NULL != (cur = readdir(dir))){
        if(0 == strcmp(cur->d_name, ".") || 0 == strcmp(cur->d_name, "..")) continue;

        plugins.push_back(cur->d_name);

    }

    closedir(dir);
    return;

}


void threadFUNC(int connectSocket){  // serve client

    char buffer[BUFFER_LEN];
    std::vector<std::string> parameters;


    // mutex to use server.data
    
    std::unique_lock<std::mutex> uni_mutex(file_mutex, std::defer_lock);


    while(1){

        int len = recv(connectSocket, buffer, BUFFER_LEN, 0); //after receiving, deserialize it

        if(0 == len){
            std::perror("client quit!!!");
            break;
        }

        uni_mutex.lock();

        // deserialize
        ofs.open("server.data");
        ofs.write(buffer, BUFFER_LEN);
        ofs.close();

        ifs.open("server.data");
        deserialize(ifs, parameters);
        ifs.close();

        uni_mutex.unlock();


        if(parameters.size() == 0) continue;

        //analyze client's command
        // a mount of if-else how can refine it

        if("getPlugin" == parameters[0]){
            parameters.clear();
            getPlugin(parameters);

        }else{
            //produce the result in parameters
            if(myPlugins.size() == 0){
                parameters.clear();
                parameters.push_back(std::string("server has no function!"));
            }
            else{

                //function index
                int index = -1;

                for(int i = 0; i<myPlugins.size(); ++i) {
                    if(myPlugins[i] == parameters[0]){
                        index = i;
                        break;
                    }
                }

                if(-1 == index){
                    parameters.clear();
                    parameters.push_back(std::string("wrongInput"));
                }
                else{
                    // assign parameters to function
                    
                    std::string function = plugin_path + myPlugins[index];
                    void *handle = dlopen(function.c_str(), RTLD_LAZY);

                    if(NULL == handle){
                        parameters.clear();
                        parameters.push_back(std::string("server call this function failed"));
                    }
                    else{

                        typedef void(*FUNC)(std::vector<std::string>&);

                        FUNC myfunc = (FUNC)dlsym(handle, myPlugins[index].c_str()); 

                        if(NULL == myfunc){
                            parameters.clear();
                            parameters.push_back(std::string("server call this function failed"));
                        }
                        else{
                            // call this function
                            (*myfunc)(parameters);
                        }  
                    }
                    dlclose(handle);
                }
            }

        }

        uni_mutex.lock();
        //serialize
        ofs.open("server.data");
        serialize(ofs, parameters);
        ofs.close();


        // store to buffer
        ifs.open("server.data");
        ifs.read(buffer, BUFFER_LEN);
        ifs.close();
        uni_mutex.unlock();

        send(connectSocket, buffer, BUFFER_LEN, 0);  // before send, serialize it


    }

    close(connectSocket);

}

void serverCommand(){
    while(cmdIsRead){
        std::cin >> servercmd;

        cmdIsRead = false;

        if("quit" == servercmd) break;
    }
}

int main(int argc, char** argv){

    if(argc != 3){
        std::cout << "\t usage:\t ./server <ip> <port>" << std::endl;

        return 0;
    }

    getPlugin(myPlugins);

    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    if(-1 == serverSocket){
        std::perror("server socket failed");
        return 0;
    }

    sockaddr_in serverADDR;
    serverADDR.sin_port = htons(atoi(argv[2]));
    if(-1 == inet_pton(AF_INET, argv[1], &serverADDR.sin_addr)){
        std::perror("server ip assign failed");
        close(serverSocket);
        return 0;
    }

    if(-1 == bind(serverSocket, (sockaddr*)&serverADDR, sizeof(sockaddr_in))){
        std::perror("server bind failed");
        close(serverSocket);
        return 0;
    }

    if(-1 == listen(serverSocket, LISTEN_LEN)){
        std::perror("server listen failed");
        close(serverSocket);
        return 0;
    }

    //listening
    std::cout << "listening" << std::endl;

    sockaddr_in clientADDR;
    socklen_t len = sizeof(sockaddr_in);

    //get server's command to close server immediately
    servercmd = "";
    cmdIsRead = true;
    std::thread cmd(serverCommand);

    while(threads.size() < MAX_THREAD){

        if(!cmdIsRead){
            if(servercmd == "quit") break;
            cmdIsRead = true;
        }

        int newSocket = accept(serverSocket, (sockaddr*)&clientADDR, (socklen_t*)&len);

        if(-1 == newSocket){
            std::perror("client connect server failed");
        }else{
            std::thread p(threadFUNC, newSocket);

            threads.push_back(std::move(p));
        }

    }

    for(auto &s : threads) s.join();

    close(serverSocket);


    return 0;
}
