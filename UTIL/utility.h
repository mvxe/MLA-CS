#ifndef UTILITY_H
#define UTILITY_H

#include <string>

class util{
public:
    template<typename... Args>
    static std::string toString(Args... args);
private:
    template <typename T>
    static std::string _toString(std::stringstream* strm, T value);
    template<typename T, typename... Args>
    static std::string _toString(std::stringstream* strm, T value, Args... args);

public:
    template<typename... Args>
    static std::string toCmdString(Args... args);
private:
    template <typename T>
    static std::string _toCmdString(std::stringstream* strm, T value);
    template<typename T, typename... Args>
    static std::string _toCmdString(std::stringstream* strm, T value, Args... args);

public:
    class doTimes{      //doIt() returns false N times and then true
    public:
        doTimes(unsigned times): toGo(times), iniT(times), times(iniT){}
        bool doIt(){
            if (toGo==0) return true;
            toGo--;
            return false;
        }
        void reset(unsigned times=0){
            if (times!=0) iniT=times;
            toGo=iniT;
        }
        const unsigned& times;
    private:
        unsigned toGo;
        unsigned iniT;
    };
};


template<typename... Args>
std::string util::toString(Args... args){
    std::stringstream* nstrm=new std::stringstream();
    return _toString(nstrm, args...);
}
template <typename T>
std::string util::_toString(std::stringstream* strm, T value){
    *strm<<value;
    std::string ret=strm->str();
    strm->str("");
    strm->clear();
    delete strm;
    return ret;
}
template<typename T, typename... Args>
std::string util::_toString(std::stringstream* strm, T value, Args... args){
    *strm<<value;
    return _toString(strm, args...);
}


template<typename... Args>
std::string util::toCmdString(Args... args){
    std::stringstream* nstrm=new std::stringstream();
    return _toCmdString(nstrm, args...);
}
template <typename T>
std::string util::_toCmdString(std::stringstream* strm, T value){
    *strm<<value<<")";
    std::string ret=strm->str();
    strm->str("");
    strm->clear();
    delete strm;
    return ret;
}
template<typename T, typename... Args>
std::string util::_toCmdString(std::stringstream* strm, T value, Args... args){
    if(!strm->str().empty()) *strm<<value<<",";
    else *strm<<value<<"(";
    return _toCmdString(strm, args...);
}

#endif // UTILITY_H
