
#include <iostream>
#include <vector>

extern "C" void addMOD997(std::vector<std::string>& parameters){
    //parameters[0] is the function's name
    //others are parameters

    int sum = 0;
    for(auto it = parameters.begin()+1; it != parameters.end(); ++it) sum += atoi((*it).c_str()) % 997,  sum %= 997;

    parameters.clear();

    parameters.push_back(std::string(std::to_string(sum)));
}