[[EN]](./README.md)

# Aliyun Linkkit tools
> 该工具可以批量为设备烧录 aliyun key,提高工作效率.

1. ** 批量配置`四元组信息`**：当前为了更加方便的烧录,现以使用脚本工具将`四元组`进行处理后一起烧录到模组内,用户与需要修改`config.csv`和`souce.csv`
    ```shell
    修改 linkkit_key 中的 config.csv
    ProductKey,a1HcqsNW4fC
	ProductSecret,cqtAPGYEKK420Gva
    RegistrationList,source.csv
    OutputList,config/output.csv
    ```
     > 1.其中`souce.csv` 格式为批量投产到时产生的秘钥文件将 xlsx格式转换为 csv格式得到.
     > 2.测试阶段只需要将`新增测试设备`得到的参数 按照 csv 格式填入
     > 3.为了保证参数的唯一性,会在 Bin 中根据模组的 mac 地址生成对应的文件,故一个设备只会占用一个参数

2. ** Aliyun Linkkit 配置编译**：

	>1.将 make menuconfig 配置 linkkit key 关闭
    ```shell
    make menuconfig --->Linkkit Configuration  --->  Enable configuration linkit key ---> n
    ```

	> 2.设备 使用脚本编译下载,'gen_misc.sh' 为 shell 脚本 集成了编译下载和根据设备 mac 地址下载配置文件,并且自动打开串口log
	```shell
    ./gen_misc.sh light 1 e iotkit
    ```
     > 2.gen_misc.sh 后面的 light  所需要编译的工程名称
     > 3.gen_misc.sh 后面的 1 为串口号 dev/ttyUSB1
     > 4.gen_misc.sh 后面的 e 为 erase_flash,即清空 flash
     > 5.gen_misc.sh 后面的 clean 为清除历史编译, 相同与 make clean 功能.

	> 3.设备 使用脚本编译下载,'multi_downloads.sh' 为 shell 脚本 集成了编译和批量下载和根据设备 mac 地址下载配置文件,并且自动打开串口log
	```shell
    ./multi_downloads.sh light 3 e
    ```
     > 2.multi_downloads.sh 后面的 light  所需要编译的工程名称
     > 3.multi_downloads.sh 后面的 3 为下载 串口号 dev/ttyUSB0 ~ dev/ttyUSB2 共三个设备
     > 4.multi_downloads.sh 后面的 e 为 erase_flash,即清空 flash
     > 5.multi_downloads.sh 后面的 clean 为清除历史编译, 相同与 make clean 功能.
