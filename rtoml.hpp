/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Mario Vretenar
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
 
#ifndef RTOML_H
#define RTOML_H

#include <string>
#include <stack>
#include <stdexcept>
#include <map>
#include ".tsl/ordered_map.h"
#include ".toml/toml.hpp"

namespace rtoml{
    std::string format(toml::basic_value<toml::preserve_comments, tsl::ordered_map>& data){   // fixes indentation, removes empty comments (needed to prevent inline)
        std::string str = toml::format(data,80,10);                                           // tsl::ordered_map preserves insertion order
        if(str.size()==0) return str;
        
        std::size_t lineBegin=0;
        std::size_t lineEnd=str.find('\n');
        if(lineEnd==std::string::npos) lineEnd=str.size();
        std::size_t pos;
        std::string sstr;
        
        int level=0;
        const std::string indentation="    ";
        const size_t indSize=indentation.size();
        const bool addNewlineAfterEveryVar=true;
        
        std::stack<std::size_t> commentPos;
        for(;;){                                //goes over every line
            sstr=str.substr(lineBegin,lineEnd-lineBegin);
            
            if(sstr[0]=='#'){
                if(sstr.size()==1){             //empty comment... remove it
                    str.erase(lineBegin,2);
                    lineEnd=str.find('\n',lineBegin);
                    continue;
                }
                commentPos.push(lineBegin);
            }else{
                bool isSecOrVar=false;
                if(sstr[0]=='['){
                    pos=sstr.find(']');
                    if(pos!=std::string::npos){ // there is [...]...
                        level=0;
                        isSecOrVar=true;
                        for(std::size_t i=1;i!=pos-1;i++)
                            if(sstr[i]=='.') level++;
                    }
                }
                
                if(addNewlineAfterEveryVar){
                    size_t pos;
                    if(!isSecOrVar){
                        pos=sstr.find('#');
                        if(pos!=std::string::npos) sstr=str.substr(0,pos);
                        pos=sstr.find('=');
                        if(pos!=std::string::npos) isSecOrVar=true;
                    }
                    if(isSecOrVar) str.insert(lineEnd+1,"\n");
                }
                
                for(int i=0;i!=level;i++){      // apply indentation
                    str.insert(lineBegin,indentation);
                    if(lineEnd!=std::string::npos) lineEnd+=indSize;
                }
                while(!commentPos.empty()){     // apply indentation to comments
                    for(int i=0;i!=level;i++){
                        str.insert(commentPos.top(),indentation);
                        lineBegin+=indSize;
                        if(lineEnd!=std::string::npos) lineEnd+=indSize;
                    }
                    commentPos.pop();
                }
            }
            
            if(lineEnd==std::string::npos) break;
            lineBegin=lineEnd+1;
            lineEnd=str.find('\n',lineBegin);
        }
        return str;
    }

    class vsr{
        private:
            class _BVar{
                public:
                    void virtual save(toml::basic_value<toml::preserve_comments, tsl::ordered_map>& dst){}
                    void virtual load(toml::basic_value<toml::preserve_comments, tsl::ordered_map>& src){}
            };
            template <typename T> class _Var : public _BVar{
                public:
                    T* var;
                    _Var(T& ovar): var(&ovar){}
                    void save(toml::basic_value<toml::preserve_comments, tsl::ordered_map>& dst){ dst=*var; }
                    void load(toml::basic_value<toml::preserve_comments, tsl::ordered_map>& src){ *var=toml::get<T>(src); }
            };
            _BVar* var{nullptr};
            std::map<std::string, vsr>* map{nullptr};
            vsr* parent{nullptr};
            std::string key;        // save filename if parent==nullptr, else the key; redundant but reimplementing map for saving a bit of space is too much work
            std::string _debug_getFullKeyString(){
                if(parent!=nullptr) return parent->_debug_getFullKeyString()+"["+key+"]";
                return "";
            }
            toml::basic_value<toml::preserve_comments, tsl::ordered_map>& _getSubTomlTable(toml::basic_value<toml::preserve_comments, tsl::ordered_map>& data){
                if(parent!=nullptr) return parent->_getSubTomlTable(data)[key];
                return data;
            }
            void _saveToToml(toml::basic_value<toml::preserve_comments, tsl::ordered_map>& data, bool clear){
                data.comments()=comments;
                if(map!=nullptr){                           // initialized as a map - call save of all entries
                    if(clear) data=toml::basic_value<toml::preserve_comments, tsl::ordered_map>();
                    if(map->size()==1) for(auto& [key, val]:*map) if((*map)[key].comments.empty()) (*map)[key].comments.push_back("");  // prevent inline
                    for(auto& [key, val]:*map) val._saveToToml(data[key], clear);
                }else{                                      // initialized as a variable - save it
                    var->save(data);
                }
            }
            void _loadFromToml(toml::basic_value<toml::preserve_comments, tsl::ordered_map>& data, bool trip){
                if(map!=nullptr){                           // initialized as a map - call load of all entries
                    for(auto& [key, val]:*map){
                        try {val._loadFromToml(toml::find(data,key), trip);}   
                        catch(std::exception& e){if(trip)throw;}
                    } 
                    
                }else{                                      // initialized as a variable - load it
                    var->load(data);
                }
            }
        public:
            vsr(){}
            vsr(std::string confFilename):key(confFilename){}   // you can also initialize the top object with the load/save filename
            ~vsr(){
                if(map!=nullptr) delete map;
                if(var!=nullptr) delete var;
            }
            template <typename T> T& operator = (T& nvar){  // constructor, set it equal to a variable to initialize it to that variable
                if(map!=nullptr) throw std::invalid_argument("Error in vsr with key "+_debug_getFullKeyString()+": this entry is already initialized as a map.");
                if(var==nullptr) var=new _Var<T>(nvar);
                else ((_Var<T>*)(var))->var=&nvar;
                return *(((_Var<T>*)(var))->var); 
            }
            vsr& operator [](const char* _key) {            // find entry in map: use only if it has not been already intialized as a variable
                if(var!=nullptr) throw std::invalid_argument("Error in vsr with key "+_debug_getFullKeyString()+": this entry is already initialized as a variable.");
                if(map==nullptr) map=new std::map<std::string, vsr>;
                (*map)[_key];
                (*map)[_key].parent=this;
                (*map)[_key].key=_key;
                return (*map)[_key];
            }
            
            template <typename T> T* get(){                 // get the pointer to the underlying variable; you have to know and specify the variable's type in template
                if(map!=nullptr)        
                    throw std::invalid_argument("Error in vsr.<>get() with key "+_debug_getFullKeyString()+": trying to get a value of a vsr initialized as a map.");
                else if(var==nullptr)   
                    throw std::invalid_argument("Error in vsr.<>get() with key "+_debug_getFullKeyString()+": trying to get a value of a uninitialized vsr.");
                return ((_Var<T>*)(var))->var;
            }
            std::vector<std::string> comments;              // free access to comments which are saved/loaded along with the variable
            
            void setConfFilename(std::string confFilename){ // set the load/save (path+)filename; always modifies the top object's filename
                if(parent!=nullptr) parent->setConfFilename(confFilename);
                else key=confFilename;
            }
            std::string getConfFilename(){                  // get the top object's load/save (path+)filename
                if(parent!=nullptr) return parent->getConfFilename();
                else return key;
            }
            void save(bool clear=false, std::string confFilename=""){   
                                                            // if clear is true and the called object is a map, extra entries in the file will be deleted,
                                                            //      otherwise the new file will still contain unused entries (reformatted though)
                                                            // if confFilename is specified, it overrides the top object's filename
                if(map==nullptr && var==nullptr)            // not initialized
                    throw std::invalid_argument("Error in vsr.save() with key "+_debug_getFullKeyString()+": trying to save an entry that was not initialized.");
                if(confFilename.empty())
                    confFilename=this->getConfFilename();
                if(confFilename.empty())
                    throw std::invalid_argument("Error in vsr.save() with key "+_debug_getFullKeyString()+": the confFilename was not provided.");
                
                toml::basic_value<toml::preserve_comments, tsl::ordered_map> data;
                try{ data = toml::parse<toml::preserve_comments, tsl::ordered_map>(confFilename); }  // read configuration
                catch(std::runtime_error e){}               // if it doesn't exist, just ignore
                
                _saveToToml(_getSubTomlTable(data), clear);
                
                std::ofstream saveFile;
                saveFile.open(confFilename);
                saveFile<<format(data);
                saveFile.close();
            }
            void load(bool trip=false, std::string confFilename=""){     
                                                            // if trip is true, and the file does not contain all of the initialized variables or does not exist, 
                                                            //      an exception will be thrown; otherwise, missing file/entries will be ignored
                                                            //      extra entries are always ignored
                                                            // if confFilename is specified, it overrides the top object's filename
                if(map==nullptr && var==nullptr)            // not initialized
                    throw std::invalid_argument("Error in vsr.load() with key "+_debug_getFullKeyString()+": trying to load an entry that was not initialized.");
                if(confFilename.empty())
                    confFilename=this->getConfFilename();
                if(confFilename.empty())
                    throw std::invalid_argument("Error in vsr.load() with key "+_debug_getFullKeyString()+": the confFilename was not provided.");
                    
                toml::basic_value<toml::preserve_comments, tsl::ordered_map> data;
                try {data = toml::parse<toml::preserve_comments, tsl::ordered_map>(confFilename);}   
                catch(std::exception& e){if(trip)throw;}
                
                _loadFromToml(_getSubTomlTable(data), trip);
            }
    };
}

#endif //RTOML_H

