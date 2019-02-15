# Age Prediction

## Prerequisites

What things you need to install the software and how to install them

**download boost**

download boost from follow link

```
https://dl.bintray.com/boostorg/release/1.67.0/source/
```

**2. download MYSQL c++ connector**

```  
https://dev.mysql.com/downloads/connector/cpp/8.0.html
```  
save into C:\Program Files\MySQL\mysql-connector-c++-8.0.12-winx64\
    
note: if you install a different mysql-connector with a different path you can link it in the project properties (additional library directories)
    
**3.  add opencv dll**
    
copy *opencv_world341.dll* file from opencv\build\x64\vc14\bin into `x64/release` of your project folder    
copy *opencv_world341d.dll* file from opencv\build\x64\vc14\bin into `x64/debug`  of your project folder
