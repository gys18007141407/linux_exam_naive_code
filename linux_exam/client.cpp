#include <iostream>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <thread>
#include <vector>
#include <condition_variable>
#include <mutex>
#include "serialize.h"

const int BUFFER_LEN = 1024;
char buffer[BUFFER_LEN];

std::string clientcmd;
bool cmdIsRead;

std::ofstream ofs;
std::ifstream ifs;
std::string filename;

std::mutex cm;
std::condition_variable cond;

void showInfo(){
    std::cout << "input command to get service, input getPlugin to see server's function\n\tusage:\t<function-name> <param1> <param2> <param3> ... \n\tinput quit will QUIT" << std::endl;
    std::cout << "+++++++++++++++++++++++++++++++++++++++++++++++++++++++" << std::endl;
}

void clientCommand(){  // get client's command

    std::unique_lock<std::mutex> cmr(cm);
    showInfo();
    

    while(1){
        cond.wait(cmr);
        if(cmdIsRead){
            
            std::getline(std::cin, clientcmd);
            std::cout << "servers respond : " << std::endl;

            cmdIsRead = false;

            if("quit" == clientcmd) break;
        }
    }
}

void seperate(std::string &str, std::vector<std::string> &v){

    v.clear();
    for(auto &c : str) if(c == '\t') c = ' ';
    
    int from = 0, to;

    while(1){
        to = str.find(' ', from);

        if(-1 == to){
            v.push_back(str.substr(from));
            break;
        }

        if(to > from) v.push_back(str.substr(from, to-from));
        from = to + 1;
    }
}

int main(int argc, char** argv){

    if(argc != 3){
        std::cout << "\t usage:\t ./client <serverIP> <port>" << std::endl;

        return 0;
    }

    std::cout << "input the filename where you want to store swaping information : ";
    std::cin >> filename;
    getchar();

    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);

    if(-1 == clientSocket){
        std::perror("client socket failed");
        return 0;
    }

    sockaddr_in serverADDR;
    serverADDR.sin_port = htons(atoi(argv[2]));
    serverADDR.sin_family = AF_INET;
    if(-1 == inet_pton(AF_INET, argv[1], &serverADDR.sin_addr)){
        std::perror("server ip assign failed");
        close(clientSocket);
        return 0;
    }

    if(-1 == connect(clientSocket, (sockaddr*)&serverADDR, sizeof(sockaddr_in))){
        std::perror("connect failed");
        close(clientSocket);
        return 0;
    }

    //connect successful

    cmdIsRead = true;
    std::thread p(clientCommand);


    //function and argvss
    std::vector<std::string> parameters;

    while(1){

        if(!cmdIsRead){
            if("quit" == clientcmd) break;

            //seperate string
            seperate(clientcmd, parameters);


            //serialize
            ofs.open(filename.c_str());
            serialize(ofs, parameters);
            ofs.close();


            //send file
            ifs.open(filename.c_str());
            ifs.read(buffer, BUFFER_LEN);
            ifs.close();

            // before sending, serialize it
            send(clientSocket, buffer, BUFFER_LEN, 0);

            int len = recv(clientSocket, buffer, BUFFER_LEN, 0);  // after receiving, deserialize it

            ofs.open(filename.c_str());
            ofs.write(buffer, BUFFER_LEN);
            ofs.close();

            if(0 == len){
                std::perror("server quit!");
                break;
            }

            //deserialize
            ifs.open(filename.c_str());
            deserialize(ifs, parameters);
            ifs.close();

            //output server messege
            if(parameters[0] == "wrongInput") {
                std::cout << "invalid input !!! please input with rules below:" << std::endl;
                showInfo();
            }
            else{
                for(auto &m : parameters) std::cout << m << std::endl;
                std::cout << "+++++++++++++++++++++++++++++++++++++++++++++++++++++++" << std::endl;
            }

            cmdIsRead = true;

            parameters.clear();

            cond.notify_one();

        }else cond.notify_one();
    }

    close(clientSocket);

    return 0;
}