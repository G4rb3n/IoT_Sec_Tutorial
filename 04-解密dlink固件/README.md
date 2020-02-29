# 解密 dlink 固件

本次我们要研究的是路由器Dlink-882的固件，下载地址在：

```
ftp://ftp2.dlink.com/PRODUCTS/DIR-882/REVA/，其中的zip文件就是各版本的固件。
```

![1](https://github.com/G4rb3n/IoT_Sec_Tutorial/blob/master/04-解密dlink固件/1.png)

解压出来的固件版本从旧到新依次为：
```
FW100B07 --> FW101B02 --> FW104B02 --> FW110B02 --> FW111B01 --> FW120B06 --> FW130B10
```
其中下图标记的两个固件是打包在同一个zip固件包中的，这里大家应该可以察觉到，FW104B02 有点特别，而且通过名字我们也猜测到它是未加密的中间版本。

![2](https://github.com/G4rb3n/IoT_Sec_Tutorial/blob/master/04-解密dlink固件/2.png)

目前最新的版本是FW130B10，通过binwalk进行解析，会发现该固件被加密了，binwalk无法解析。

![3](https://github.com/G4rb3n/IoT_Sec_Tutorial/blob/master/04-解密dlink固件/3.png)

再来看下最早版本的固件FW100B07，使用binwalk解析是能直接提取的，没有被加密过。

![4](https://github.com/G4rb3n/IoT_Sec_Tutorial/blob/master/04-解密dlink固件/4.png)

综上，我们得知了，固件版本迭代的过程经历了固件未加密到固件加密，说明中间肯定有个中间版本（类似下图V 2.0），下图是固件从未加密到加密的升级原理图。

![5](https://github.com/G4rb3n/IoT_Sec_Tutorial/blob/master/04-解密dlink固件/5.png)

从binwalk解析结果来看，能确定FW100B07、FW101B02、FW104B02是未加密的，而FW104B02之后的版本，FW110B02、FW111B01、FW120B06、FW130B10均是加密的，那么FW104B02就可以确定是中间版本了。

![6](https://github.com/G4rb3n/IoT_Sec_Tutorial/blob/master/04-解密dlink固件/6.png)

确定了FW104B02是中间版本后，我们大概率可以从FW104B02的固件系统中找到解密程序了，方法是在/bin和/usr/bin目录下寻找decrypt字样的程序，很幸运，我们在bin目录下找到了imgdecrypt，这个程序猜测就是解密程序了。

![7](https://github.com/G4rb3n/IoT_Sec_Tutorial/blob/master/04-解密dlink固件/7.png)

对比下最早版本的FW100B07版本，/bin目录下就没有imgdecrypt程序。

![8](https://github.com/G4rb3n/IoT_Sec_Tutorial/blob/master/04-解密dlink固件/8.png)

接下来就可以分析下imgdecrypt的加密逻辑了，看下如何破解加密的固件，通过file命令，我们得知该程序是MIPS架构的可执行文件。

![9](https://github.com/G4rb3n/IoT_Sec_Tutorial/blob/master/04-解密dlink固件/9.png)

使用IDA分析，好吧，完全看不懂，放弃了。。这个是mips的汇编指令，跟x86汇编的语法不一样，感兴趣的同学可以学一下。

![10](https://github.com/G4rb3n/IoT_Sec_Tutorial/blob/master/04-解密dlink固件/10.png)

简单地从导入表中看下函数，发现有RSA、AES，看来是较复杂的加密方式，不是简单的异或运算，头疼。

![11](https://github.com/G4rb3n/IoT_Sec_Tutorial/blob/master/04-解密dlink固件/11.png)

既然逆向不出加密算法，是不是就没办法解加密固件了呢？并不是，我们可以直接本地运行解密程序imgdecrypt，来解被加密的FW120806固件。由于该程序是mips架构的，还得借助qemu-mipsel-static模拟器来运行。

![12](https://github.com/G4rb3n/IoT_Sec_Tutorial/blob/master/04-解密dlink固件/12.png)

使用qemu-mipsel-static之前要用chroot将当前固件系统路径设置为root路径，不然会提示缺少一些系统库（找不到库路径）。下图，我们就模拟运行imgdecrypt成功了，程序用法很简单，参数提供待解密的固件路径就行了。加密成功后，用binwalk就能解析出固件的系统结构了，进而提取固件系统。

![13](https://github.com/G4rb3n/IoT_Sec_Tutorial/blob/master/04-解密dlink固件/13.png)

同样的，最新的FW130B10版本也能通过该程序进行解密，按照这个逻辑，可能Dlink的其他系列路由器也能通过imgdecrypt解密，大家可以去试一下。

![14](https://github.com/G4rb3n/IoT_Sec_Tutorial/blob/master/04-解密dlink固件/14.png)