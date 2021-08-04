# rtoml 

Provides a convenient way to load/save variables to configuration files using TOML.  
Uses submodules ToruNiina/toml11 and Tessil/ordered-map .

Requires -std=c++17.  
Is a header only library.  

# Usage:

Declare a configuration map:  
~~~cpp
    rtoml::vsr mapa("filename.toml");
~~~
The argument is optional, and can be defined/overridden when saving.

First we define a variable, for example a double. We should give it a default value by initializing it.  
We can then associate a map entry to it. It then contains a reference to that variable.  
This means that calling `vsr::load` overwrites all the variables with entries found in the configuration file, and `vsr::save` saves all the entries into a file.  
The variable can be used normally once initialized and a load is performed.
~~~cpp
    double a=5;
    mapa["entry1"]=a;
~~~
It is possible to have entry hierarchy:
~~~cpp
    std::string b=5;
    mapa["Map1"]["Map2"]["entry2"]=b;
~~~
Note that if `mapa["entry1"]` is associated to a variable, trying `mapa["entry1"]["entry2"]` will obviously throw an exception.  
It is also possible to retrieve an entry directly by name (it must exist and you must know the type):
~~~cpp
    double var=*mapa["MapX"]["entryY"].get<int>();
~~~
Furthermore, you can add comments to both entries and maps that are then visible in the .toml configuration file.
~~~cpp
    mapa["x"]["xx"].comments.push_back("comment 1");
    mapa["x"]["xx"].comments.push_back("comment 2");
~~~

Once all entries have been initialized we can now load a file. This searches the file for entries matching those that have been initialized, and if found replaces the variable values with those from the file. Other entries are ignored.
~~~cpp
    mapa.load();
~~~
load takes two optional arguments:  
&nbsp;&nbsp;&nbsp;&nbsp; `bool trip (false)`  
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;if true, and the file does not contain all of the initialized variables or does not exist,  
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;an exception will be thrown; otherwise, missing file/entries will be ignored  
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;extra entries are always ignored  
&nbsp;&nbsp;&nbsp;&nbsp; `std::string confFilename=""`  
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;if specified, this overrides the top object's filename  

NOTE: you cannot use `.get<type>` for uninitialized entries even thought they were present in the loaded file.  
NOTE: you cannot load a file unless at least one entry has been added to the map object (would be pointless anyway without .get).

At this point you use and modify your variables as you see fit without the need to call any objects in the library.
Once you wish to save them just do:
~~~cpp
    mapa.save();
~~~
save takes two optional arguments:  
&nbsp;&nbsp;&nbsp;&nbsp; `bool clear (false)`  
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;if true and the called object is a map, extra entries in the file will be deleted,  
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;otherwise the new file will still contain unused entries (reformatted though)  
&nbsp;&nbsp;&nbsp;&nbsp; `std::string confFilename=""`  
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;if specified, this overrides the top object's filename  

Note that each non-variable member of the map is a map itself, so you can call `vsr::save` and `vsr::load` on it. For example:
~~~cpp
    mapa["Map1"]["Map2"]["entry1"]=a;
    mapa["Map1"]["Map2"].save(false, "filename.toml");
~~~
This saves only the entries in Map2. Note that in this case, you should specify the filename, otherwise the top object's (mapa) filename will be used.


An example is given in example.cpp.  

