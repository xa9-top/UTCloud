# UTCloud

UTCloud是一个基于C++（使用Boost库、libcurl和libzip）的客户端和PHP的服务端开发的Undertale游戏存档同步程序。它能够帮助玩家在本地和云端之间自动同步游戏存档，确保玩家在不同设备上可以无缝继续游戏。以下是UTCloud的详细介绍和使用指南。

## 功能特点

1. **云存档模式**：可以将本地的Undertale游戏存档上传至云端或下载最新的游戏存档到本地。
2. **本地模式**：可以将本地游戏存档复制到指定的文件夹，仅用一个存储设备实现无网络在不同设备之间同步游戏存档。
3. **自动化操作**：无需手动操作，启动UTCloud后程序会自动根据配置文件进行存档同步并启动Undertale游戏。
4. **错误处理**：在操作过程中遇到错误会自动提示用户，方便用户及时处理问题。

## 依赖库

- **Boost**：用于处理INI配置文件和字符串操作。
- **libcurl**：用于处理HTTP请求，进行文件的下载和上传。
- **libzip**：用于处理ZIP文件的压缩和解压。

## 使用说明（客户端）

### 快速开始

1. **下载程序** 直接下载编译好的程序：[Releases](https://github.com/xa9-top/UTCloud/releases/latest)

3. **配置文件说明** 将`zip.dll`、`zlib.dll`、`UTCloud.exe`放在Undertale游戏根目录，然后运行`UTCloud.exe`。首次启动会生成`UTCloud.ini`，编辑此配置文件：

   ```ini
   [UTCloud]
   ;云存档模式下载api
   downloadapi = http://
   ;云存档模式上传api
   uploadapi = http://
   ;本地模式存档路径
   path = ./UTCloud/UNDERTALE
   ;存档备份文件夹路径
   bak = ./UTCloud/bak
   ;模式，cloud表示云存档模式，local表示本地模式
   mode = cloud
   ;状态，on表示开启存档同步，off表示不开启，如果为off则直接跳过有关存档的操作直接启动UT
   state = on
   
   ```

4. **运行程序** 启动时不启动`Undertale.exe`而是`UTCloud.exe`即可(觉得steam正版启动太慢的可以将Undertale目录下的`steam_api.dll`删除或重命名)
   

### 自己编译

1. **安装依赖库** 通过CMake安装libcurl和libzip，确保Boost库版本为1.8.5，libcurl版本为8.8.0，libzip版本为1.10.1，zlib版本为1.3.1。

2. **编译** 使用Visual Studio打开项目，确保所有依赖库已正确安装，所有包含文件路径及库路径设置无误，然后构建项目。

   **注意：此项目使用ISO C++ 17标准**

3. **配置INI文件** 在首次运行程序时，会自动生成一个`UTCloud.ini`配置文件。用户需要根据自己的需求编辑该配置文件（参见上文配置文件说明）。

### 运行程序

直接双击`UTCloud.exe`，程序会根据`UTCloud.ini`中的配置进行操作。

### 参数说明

- `-V, -v, -Version, -version, -About, -about`：显示程序版本信息。

## 使用说明（服务端）

首先编辑提供的PHP文件开头的两个常量：

```php
define('TOKEN', '123456'); // 这里是定义 token 的位置，用来验证你的身份
define('FILE_PATH', './UNDERTALE.zip'); // 这里是定义 file_path 的位置，用来保存和读取你的存档文件的位置，注意要确保php对这个路径有权限
```

然后将此PHP文件放置到你的服务器或者虚拟主机的web目录下（名称随意，能访问到即可）。

接下来，配置文件中的`downloadapi`和`uploadapi`分别设置为：

```
http(s)://<你的服务器域名>/<你的php路径>.php?token=<你定义的token>
```

## 常见问题

1. 程序提示`ini配置文件格式有误`
   - 请检查`UTCloud.ini`配置文件的格式是否正确，确保所有字段均已正确填写。
2. 无法连接至服务器
   - 请检查`downloadapi`和`uploadapi`是否正确配置，确保服务器可用。
3. 无法写入文件或创建文件夹
   - 请检查程序是否具有足够的文件读写权限。
4. 无法压缩或解压文件
   - 请确保libzip库正确安装，并检查ZIP文件的完整性。

## 贡献与反馈

如果你在使用过程中遇到问题，或者有新的功能建议，欢迎通过以下方式与我们联系：

- 提交Issue至[Github](https://github.com/xa9-top/UTCloud/issues)
- 通过Email联系：[xa9_top@qq.com](mailto:xa9_top@qq.com)
- 添加QQ群(不是交流群是闲聊群这小项目不值得建一个交流群)：[837177830](https://qm.qq.com/q/R5UGH4P4uy)

感谢你对UTCloud的支持与贡献！

## 版权声明

UTCloud Version 1.0 版权所有 (C) 2024 Xa9

尝试新的Undertale存档方式(doge)
