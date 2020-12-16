#include <iostream>
#include <fstream>
#include <string>
#include <vector>


// 容器和嵌套容器的序列化 普通类的序列化

//SERIALIZE
template<typename T, typename std::enable_if<std::is_trivially_copyable<T>::value, int>::type N = 0>    // T本身可平凡复制
void serialize(std::ofstream& os, const T &t){
    std::cout << "copyable" << std::endl;
    os.write((const char*)&t, sizeof(T));

}



template<typename T, typename std::enable_if<std::is_trivially_copyable<typename T::value_type>::value &&     //容器:T内元素可平凡复制
                                                std::is_same<typename T::iterator, decltype(std::declval<T>().begin())>::value && 
                                                std::is_same<typename T::iterator, decltype(std::declval<T>().end())>::value , int>::type N = 0>
void serialize(std::ofstream& os, const T &t){
    

    unsigned int size = t.size();  // 容器内元素的个数
    os.write((const char*)&size, sizeof(size));

    os.write((const char*)t.data(), size*sizeof(typename T::value_type));
}


template<typename T, typename std::enable_if<!std::is_trivially_copyable<typename T::value_type>::value &&   //T内元素不可平凡复制：嵌套容器
                                                std::is_same<typename T::iterator, decltype(std::declval<T>().begin())>::value &&
                                                std::is_same<typename T::iterator, decltype(std::declval<T>().end())>::value, int>::type N = 0>
void serialize(std::ofstream& ofs, const T& t){
    
    unsigned int size = t.size();

    ofs.write((const char*)&size, sizeof(size));

    for(auto &v : t) serialize(ofs, v);
}






// DESERIALIZE
template<typename T, typename std::enable_if<std::is_trivially_copyable<T>::value, int>::type N = 0>     // 可平凡复制
void deserialize(std::ifstream& ifs, const T &t){
    ifs.read((char*)&t, sizeof(T));
}


template<typename T, typename std::enable_if<std::is_trivially_copyable<typename T::value_type>::value &&     //容器内可平凡复制：vector<int>
                                                std::is_same<typename T::iterator, decltype(std::declval<T>().begin())>::value &&
                                                std::is_same<typename T::iterator, decltype(std::declval<T>().end()) >::value, int>::type N = 0>
void deserialize(std::ifstream& ifs, T &t){

 
    unsigned int size = 0;

    ifs.read((char*)&size, sizeof(size));
    t.resize(size);
    ifs.read((char*)t.data(), size*sizeof(typename T::value_type));
}


template<typename T, typename std::enable_if<!std::is_trivially_copyable<typename T::value_type>::value &&  //容器内不可平凡复制:vector<vector<>>
                                                std::is_same<typename T::iterator, decltype(std::declval<T>().begin())>::value &&
                                                std::is_same<typename T::iterator, decltype(std::declval<T>().end())>::value, int>::type N = 0>
void deserialize(std::ifstream& ifs, T& t){

 
    unsigned int size = 0;

    ifs.read((char*)&size, sizeof(size));
    t.resize(size);

    for(auto &v: t) deserialize(ifs, v);
}




// int main(){
//     std::cout << std::is_trivially_copyable<std::vector<std::string>::value_type>::value << std::endl;
//     std::vector<std::string> a,b;
//     a.push_back("hello");
//     a.push_back("world");

//     for(auto &s : a) std::cout << s << std:: endl;

//     std::ofstream ofs("output.data");
//     serialize(ofs, a);
//     ofs.close();  // 关闭文件使其写入磁盘，否则ifs读取的是空文件
//     std::ifstream ifs("output.data");
//     deserialize(ifs,b);
//     ifs.close();

//     std::cout << b.size() << std:: endl;
//     for(auto &s : b) std::cout << b.size() << s << std:: endl;

//     return 0;
// }