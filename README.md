# Welcome to EVE LogLite!
-------------------

LogLite is a client log viewer developed by [CCP Games] (https://www.ccpgames.com/) for their award winning MMO [EVE Online] (http://www.eveonline.com/).


## Build Instructions
-------------------

* Requires Qt 6.4 or newer (http://www.qt.io/)
* Get source code and build with:
```
> cmake -S . -B .build-folder -DCMAKE_OSX_ARCHITECTURES=x86_64;arm64 -DCMAKE_OSX_DEPLOYMENT_TARGET=10.14
> cmake --build .build-folder --target LogLite --parallel
```



## License
-------------------

All files in this project are under the [LICENSE](LICENSE) license unless otherwise stated in the file or by a dependency's license file.


## Author
-------------------

Filipp Pavlov - Team TriLambda
