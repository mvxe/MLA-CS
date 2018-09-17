#ifndef UTILITY_H
#define UTILITY_H

#include <string>

class util{
public:
    template <typename T>
    static std::string toString(T value);
    template<typename T, typename... Args>
    static std::string toString(T value, Args... args);
private:
    template <typename T>
    static std::string ttoString(std::stringstream* strm, T value);
    template<typename T, typename... Args>
    static std::string ttoString(std::stringstream* strm, T value, Args... args);
};

template <typename T>
std::string util::toString(T value){
    std::stringstream* nstrm=new std::stringstream();
    return ttoString(nstrm, value);
}
template<typename T, typename... Args>
std::string util::toString(T value, Args... args){
    std::stringstream* nstrm=new std::stringstream();
    return ttoString(nstrm, value, args...);
}
template <typename T>
std::string util::ttoString(std::stringstream* strm, T value){
    *strm<<value;
    std::string ret=strm->str();
    strm->str("");
    strm->clear();
    delete strm;
    return ret;
}
template<typename T, typename... Args>
std::string util::ttoString(std::stringstream* strm, T value, Args... args){
    *strm<<value;
    return ttoString(strm, args...);
}
#endif // UTILITY_H
