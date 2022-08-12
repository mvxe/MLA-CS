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
    template<typename>   struct is_atomic                 : std::false_type {};
    template<typename T> struct is_atomic<std::atomic<T>> : std::true_type  {};
    template<typename>   struct is_vector                 : std::false_type {};
    template<typename T> struct is_vector<std::vector<T>> : std::true_type  {};
    template<typename>   struct is_unordered_map                                    : std::false_type {};
    template<typename T> struct is_unordered_map<std::unordered_map<toml::key, T>>  : std::true_type  {};
    class vsr{
        private:
            class _BVar{
                public:
                    void virtual save(toml::basic_value<toml::preserve_comments, tsl::ordered_map>& dst){}
                    void virtual load(toml::basic_value<toml::preserve_comments, tsl::ordered_map>& src){}
                    bool virtual changed(toml::basic_value<toml::preserve_comments, tsl::ordered_map>& src){return false;}
            };
            template <typename T> class _Var : public _BVar{
                public:
                    T* var;
                    _Var(T& ovar): var(&ovar){}
                    void save(toml::basic_value<toml::preserve_comments, tsl::ordered_map>& dst){
                        if constexpr(!std::is_pointer<T>::value){
                            if constexpr(std::is_arithmetic<T>::value || std::is_same<T, std::string>::value 
                                || rtoml::is_vector<T>::value || rtoml::is_unordered_map<T>::value) dst=*var;
                            else if constexpr(rtoml::is_atomic<T>::value) dst=var->load();
                            else dst=var->get();
                        }else{
                            if constexpr(std::is_arithmetic<T*>::value || std::is_same<T*, std::string>::value
                                || rtoml::is_vector<T*>::value || rtoml::is_unordered_map<T*>::value) dst=**var;
                            else if constexpr(rtoml::is_atomic<T*>::value) dst=(*var)->load();
                            else dst=(*var)->get();
                        }
                    }
                    void load(toml::basic_value<toml::preserve_comments, tsl::ordered_map>& src){
                        if constexpr(!std::is_pointer<T>::value){
                            if constexpr(std::is_arithmetic<T>::value || std::is_same<T, std::string>::value
                                || rtoml::is_vector<T>::value || rtoml::is_unordered_map<T>::value) *var=toml::get<T>(src);
                            else if constexpr(rtoml::is_atomic<T>::value) var->store(toml::get<decltype(var->load())>(src));
                            else var->set(toml::get<decltype(var->get())>(src));
                        }else{
                            if constexpr(std::is_arithmetic<T*>::value || std::is_same<T*, std::string>::value
                                || rtoml::is_vector<T*>::value || rtoml::is_unordered_map<T*>::value) **var=toml::get<T>(src);
                            else if constexpr(rtoml::is_atomic<T*>::value) (*var)->store(toml::get<decltype((*var)->load())>(src));
                            else (*var)->set(toml::get<decltype((*var)->get())>(src));
                        }
                    }
                    bool changed(toml::basic_value<toml::preserve_comments, tsl::ordered_map>& src){
                        if constexpr(!std::is_pointer<T>::value){
                            if constexpr(std::is_arithmetic<T>::value || std::is_same<T, std::string>::value
                                || rtoml::is_vector<T>::value || rtoml::is_unordered_map<T>::value) return *var!=toml::get<T>(src);
                            else if constexpr(rtoml::is_atomic<T>::value) return var->load()!=toml::get<decltype(var->load())>(src);
                            else return var->get()!=toml::get<decltype(var->get())>(src);
                        }else{
                            if constexpr(std::is_arithmetic<T*>::value || std::is_same<T*, std::string>::value
                                || rtoml::is_vector<T*>::value || rtoml::is_unordered_map<T*>::value) return **var!=toml::get<T>(src);
                            else if constexpr(rtoml::is_atomic<T*>::value) return (*var)->load()!=toml::get<decltype((*var)->load())>(src);
                            else return (*var)->get()!=toml::get<decltype((*var)->get())>(src);
                        }
                    }
            };
            _BVar* var{nullptr};
            tsl::ordered_map <std::string, vsr>* map{nullptr};
            bool exmap{false};      //if true, it is an external map (do not deallocate in destructor)
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
            void _saveToToml(toml::basic_value<toml::preserve_comments, tsl::ordered_map>& data, bool clear) const{
                if(map!=nullptr){                           // initialized as a map - call save of all entries
                    if(map->empty()) return;
                    if(clear) data=toml::basic_value<toml::preserve_comments, tsl::ordered_map>();
                    data.comments()=_comments;
                    for(auto& [key, val]:*map){
                        if(val.map!=nullptr) if(val.map->empty()) continue;
                        if(val.map!=nullptr || val.var!=nullptr)
                            val._saveToToml(data[key], clear);
                    }
                }else if(var!=nullptr){                                      // initialized as a variable - save it
                    data.comments()=_comments;
                    var->save(data);
                }
            }
            void _loadFromToml(toml::basic_value<toml::preserve_comments, tsl::ordered_map>& data, bool trip) const{
                if(map!=nullptr){                           // initialized as a map - call load of all entries
                    for(auto& [key, val]:*map){
                        try {val._loadFromToml(toml::find(data,key), trip);}
                        catch(std::exception& e){if(trip)throw;}
                    }
                }else{                                      // initialized as a variable - load it
                    var->load(data);
                }
            }
            bool _checkFromToml(toml::basic_value<toml::preserve_comments, tsl::ordered_map>& data) const{
                if(map!=nullptr){                           // initialized as a map - call check of all entries
                    for(auto& [key, val]:*map){
                        try {if(val._checkFromToml(toml::find(data,key))) return true;}
                        catch(std::exception& e){           // var does not exist, return true (we dont care about empty maps)
                            if(val.var!=nullptr) return true;
                            if(val.map!=nullptr) if(!val.map->empty())  return true;
                        }
                    }
                    return false;
                }else return var->changed(data);            // initialized as a variable - check it
            }
            std::string indent(std::string data){   // indent sections
                std::size_t lineBegin=0;
                std::size_t lineEnd=data.find('\n');
                std::string sstr;
                std::size_t pos;
                int level=0;
                int preceedingCommentNum=0;
                bool checkpreceedingComments=false;
                const std::string indentation="    ";
                bool printingString=false;
                while(lineEnd!=std::string::npos){
                    sstr=data.substr(lineBegin,lineEnd-lineBegin);
                    pos=sstr.find("\"\"\"");
                    if(pos!=std::string::npos) printingString^=1;
                    if(!printingString || pos!=std::string::npos){
                        if(sstr[0]=='[' && sstr[sstr.size()-1]==']'){   // section
                            level=std::count(sstr.begin(),sstr.end(),'.');
                            checkpreceedingComments=true;
                        }else if(sstr[0]=='#') preceedingCommentNum++;
                        else preceedingCommentNum=0;
                        
                        // indent
                        if(sstr.size()>1) for(int i=0;i!=level;i++){
                            data.insert(lineBegin,indentation);
                            lineBegin+=indentation.size();
                            lineEnd+=indentation.size();
                        }
                        // indent previous comments
                        pos=lineBegin;
                        if(level>0 && checkpreceedingComments){
                            checkpreceedingComments=false;
                            while(preceedingCommentNum>0){
                                pos=data.find_last_of('#',pos);
                                data.insert(pos,indentation);
                                lineEnd+=indentation.size();
                                preceedingCommentNum--;
                            }
                        }
                    }
                    lineBegin=lineEnd+1;
                    lineEnd=data.find('\n',lineBegin);
                }
                return data;
            }
            std::vector<std::string> _comments;
            std::vector<std::string>* _exComments;
        public:
            vsr(){}
            vsr(std::string confFilename):key(confFilename){}   // you can also initialize the top object with the load/save filename
            ~vsr(){
                if(map!=nullptr && !exmap) delete map;
                if(var!=nullptr) delete var;
            }
            template <typename T> T& operator = (T& nvar){  // constructor, set it equal to a variable to initialize it to that variable
                if(map!=nullptr) throw std::invalid_argument("Error in vsr with key "+_debug_getFullKeyString()+": this entry is already initialized as a map.");
                if(var==nullptr) var=new _Var<T>(nvar);
                else ((_Var<T>*)(var))->var=&nvar;
                return *(((_Var<T>*)(var))->var);
            }
            vsr& operator = (vsr& nvar){                    // constructor, set it equal to a another vsr (this makes it point to that vsr's map)
                 if(var!=nullptr) throw std::invalid_argument("Error in vsr with key "+_debug_getFullKeyString()+": this entry is already initialized as a variable.");
                 if(map!=nullptr && !exmap) delete map;
                 if(nvar.map==nullptr) nvar.map=new tsl::ordered_map<std::string, vsr>;     // in case the external vsr is empty
                 if(nvar.parent!=nullptr) throw std::invalid_argument("Error in vsr with key "+_debug_getFullKeyString()+": this map has already been assigned and has a parent");
                 map=nvar.map;
                 nvar.parent=parent;
                 nvar.key=key;
                 exmap=true;
                 _exComments=&nvar.comments();
                 return *this;
            }
            vsr& operator [](std::string _key) {            // find entry in map: use only if it has not been already intialized as a variable
                if(var!=nullptr) throw std::invalid_argument("Error in vsr with key "+_debug_getFullKeyString()+": this entry is already initialized as a variable.");
                if(map==nullptr) map=new tsl::ordered_map<std::string, vsr>;
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
            std::vector<std::string>& comments(){
                if(exmap) return *_exComments;
                else return _comments;
            }

            void setConfFilename(std::string confFilename){ // set the load/save (path+)filename; always modifies the top object's filename
                if(parent!=nullptr) parent->setConfFilename(confFilename);
                else key=confFilename;
            }
            std::string getConfFilename(){                  // get the top object's load/save (path+)filename
                if(parent!=nullptr) return parent->getConfFilename();
                else return key;
            }
            void save(bool clear=false, std::string confFilename="", int width=160, int precision=10){
                                                            // if clear is true and the called object is a map, extra entries in the file will be deleted,
                                                            //      otherwise the new file will still contain unused entries (reformatted though)
                                                            // if confFilename is specified, it overrides the filename
                if(map==nullptr && var==nullptr)            // not initialized
                    throw std::invalid_argument("Error in vsr.save() with key "+_debug_getFullKeyString()+": trying to save an entry that was not initialized.");
                if(map!=nullptr && var==nullptr) if(map->empty())
                    throw std::invalid_argument("Error in vsr.save() with key "+_debug_getFullKeyString()+": trying to save an empty map.");
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
                saveFile<<indent(toml::format(data,width,precision));
                saveFile.close();
            }
            void load(bool trip=false, std::string confFilename=""){
                                                            // if trip is true, and the file does not contain all of the initialized variables or does not exist,
                                                            //      an exception will be thrown; otherwise, missing file/entries will be ignored
                                                            //      extra entries are always ignored
                                                            // if confFilename is specified, it overrides the filename
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
            bool changed(std::string confFilename=""){
                                                            // returns true if at least one of the entries in the file differs from data in the map, or is missing
                                                            // useful for save prompts
                                                            // if confFilename is specified, it overrides the filename
                if(map==nullptr && var==nullptr)            // not initialized
                    throw std::invalid_argument("Error in vsr.hasChanged() with key "+_debug_getFullKeyString()+": trying to check an entry that was not initialized.");
                if(confFilename.empty())
                    confFilename=this->getConfFilename();
                if(confFilename.empty())
                    throw std::invalid_argument("Error in vsr.hasChanged() with key "+_debug_getFullKeyString()+": the confFilename was not provided.");

                toml::basic_value<toml::preserve_comments, tsl::ordered_map> data;
                try {data = toml::parse<toml::preserve_comments, tsl::ordered_map>(confFilename);}
                catch(std::exception& e){return true;}      // the file does not exist - return true

                return _checkFromToml(_getSubTomlTable(data));
            }
    };
}

#endif //RTOML_H


