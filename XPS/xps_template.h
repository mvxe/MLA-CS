#ifndef XPS_TEMPLATE_H
#define XPS_TEMPLATE_H

template <typename T>                   void XPS::execCommand(T value){
    std::stringstream* nstrm=new std::stringstream();
    eexecCommand(nstrm, value);
}
template<typename T, typename... Args>  void XPS::execCommand(T value, Args... args){
    std::stringstream* nstrm=new std::stringstream();
    eexecCommand(nstrm, value, args...);
}
template <typename T>                   void XPS::eexecCommand(std::stringstream* strm, T value){
    *strm<<value;
    mpq.lock();
    priority_queue.push({strm->str(),nullptr});
    mpq.unlock();
    strm->str("");
    strm->clear();
    delete strm;
}
template<typename T, typename... Args>  void XPS::eexecCommand(std::stringstream* strm, T value, Args... args){
    *strm<<value;
    eexecCommand(strm, args...);
}

template <typename T>                   std::string XPS::execCommandR(T value){
    std::stringstream* nstrm=new std::stringstream();
    return eexecCommandR(nstrm, value);
}
template<typename T, typename... Args>  std::string XPS::execCommandR(T value, Args... args){
    std::stringstream* nstrm=new std::stringstream();
    return eexecCommandR(nstrm, value, args...);
}
template <typename T>                   std::string XPS::eexecCommandR(std::stringstream* strm, T value){
    *strm<<value;
    std::string ret;
    mpq.lock();
    priority_queue.push({strm->str(),&ret});
    mpq.unlock();
    strm->str("");
    strm->clear();
    delete strm;
    for(bool done=false;done==false;){
        std::this_thread::sleep_for (std::chrono::milliseconds(1));
        mpq.lock();
        if(!ret.empty()) done=true;
        mpq.unlock();
    }
    return ret;
}
template<typename T, typename... Args>  std::string XPS::eexecCommandR(std::stringstream* strm, T value, Args... args){
    *strm<<value;
    return eexecCommandR(strm, args...);
}

#endif // XPS_TEMPLATE_H
