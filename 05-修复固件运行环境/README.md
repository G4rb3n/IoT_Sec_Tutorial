# 修复固件运行环境

之前我们学习了如何模拟运行IoT固件，但是，现实中并不是所有固件都能模拟运行成功，比如固件运行可能依赖于硬件，qemu无法完全模拟，所以，本节我们就来学习如何修复固件的运行环境，从而成功模拟固件运行。

我们本次使用的固件是D-Link的DIR-605L FW_113固件，下载地址：

```
ftp://ftp2.dlink.com/PRODUCTS/DIR-605L/REVA/
```

![1](https://github.com/G4rb3n/IoT_Sec_Tutorial/blob/master/05-修复固件运行环境/pic/1.png)

拿到固件后，我们惯用的使用fat工具进行模拟，会发现它报错了，Interfaces那里也为空，说明模拟运行失败了。

![2](https://github.com/G4rb3n/IoT_Sec_Tutorial/blob/master/05-修复固件运行环境/pic/2.png)

上github上查询该错误，作者无奈地回复说其实不是所有固件都能用firmadyne模拟的，尤其是那些需要与硬件设备交互的固件。

![3](https://github.com/G4rb3n/IoT_Sec_Tutorial/blob/master/05-修复固件运行环境/pic/3.png)


好，那就换种思路，模拟固件运行的实质其实就是把固件的Web程序跑起来，而模拟失败则说明Web程序运行出错了，我们接下来就要针对看看Web程序报错的原因以及如何修复运行环境。首先用binwalk提取固件的文件系统，提取出来的路径为squashfs-root-0。

![4](https://github.com/G4rb3n/IoT_Sec_Tutorial/blob/master/05-修复固件运行环境/pic/4.png)


我们可以使用find ./ -name boa命令来定位该固件的Web程序。Boa程序是一个轻量级的web服务器程序，常见于嵌入式系统中。dlink就是在boa开源代码的基础上新增了很多功能接口以实现路由器上的不同功能。boa程序的路径为/bin/boa，同时我们发现在/etc/boa路径下还有个boa的密码配置文件，我们可以直接获取到boa加密后的密码。

![5](https://github.com/G4rb3n/IoT_Sec_Tutorial/blob/master/05-修复固件运行环境/pic/5.png)

我们直接使用命令模拟运行程序，会发现报错了，提示初始化MIB错误。

```
sudo chroot . ./qemu-mips-static bin/boa
```

![6](https://github.com/G4rb3n/IoT_Sec_Tutorial/blob/master/05-修复固件运行环境/pic/6.png)


使用IDA对boa程序进行逆向分析（需要先添加mip插件：https://github.com/devttys0/ida），定位到字符串“Initialize AP MIB failed”，接下来就在这里下断点，调试看下什么情况下会报这个错。

![7](https://github.com/G4rb3n/IoT_Sec_Tutorial/blob/master/05-修复固件运行环境/pic/7.png)

首先使用qemu调试命令运行boa程序。

![8](https://github.com/G4rb3n/IoT_Sec_Tutorial/blob/master/05-修复固件运行环境/pic/8.png)

然后在用ida打开boa程序，选择Debugger为Remote GDB（我这里是远程调试129虚拟机里的boa程序），点击Debugger -> Process Options进行配置IP和端口。

![9](https://github.com/G4rb3n/IoT_Sec_Tutorial/blob/master/05-修复固件运行环境/pic/9.png)

然后点击Debugger -> Attach to process附加调试，直接运行到断点处，这里的jalr命令即为调用apmib_init函数。

![10](https://github.com/G4rb3n/IoT_Sec_Tutorial/blob/master/05-修复固件运行环境/pic/10.png)

google下这个这个函数是干嘛的，原来是从flash中读取mib值到RAM中，模拟环境没有flash硬件，所以应该会读取失败。

![11](https://github.com/G4rb3n/IoT_Sec_Tutorial/blob/master/05-修复固件运行环境/pic/11.png)

经过调试，我们知道由于没有flash硬件，apmib_init读取数据失败返回0，赋值给%v0，然后bnez命令对$v0进行检测，若为0，则回显初始化失败，报错退出。

![12](https://github.com/G4rb3n/IoT_Sec_Tutorial/blob/master/05-修复固件运行环境/pic/12.png)

我直接在IDA中对字节码进行修改，将bnez(0x14)命令改成beqz(0x10)，就可以进入正常的逻辑顺利运行下去了。

![13](https://github.com/G4rb3n/IoT_Sec_Tutorial/blob/master/05-修复固件运行环境/pic/13.png)

patch完程序后，再次运行boa，发现它不再报“Initialize AP MIB failed”错误了，但又来了2个“Create chklist file error!”错误以及一个内存崩溃错误。

![14](https://github.com/G4rb3n/IoT_Sec_Tutorial/blob/master/05-修复固件运行环境/pic/14.png)

前2个“Create chklist file error!”来自于create_chklist_file和create_devInfo_file函数，后面那个内存崩溃是apmib_get函数导致的。

![15](https://github.com/G4rb3n/IoT_Sec_Tutorial/blob/master/05-修复固件运行环境/pic/15.png)

定位到“Create chklist file error!”字符串，经分析确认这个错误是open函数的返回值导致，但由于不影响执行我们就细看了。

![16](https://github.com/G4rb3n/IoT_Sec_Tutorial/blob/master/05-修复固件运行环境/pic/16.png)

重点放到apmib_get函数，从上面我们可以知道该函数的功能是从RAM读取MIB值，所以有可能又是硬件依赖导致的错误。该函数正常情况下会返回4种值。

![17](https://github.com/G4rb3n/IoT_Sec_Tutorial/blob/master/05-修复固件运行环境/pic/17.png)

由于代码复杂，我们就不细致分析了，直接参考作者的代码编写劫持代码，目的是伪造apmib_init和apmib_get函数，让其返回正确的值，由于为了方便IDA调试，作者把fork也一并劫持了（IDA遇到fork异常）。

![18](https://github.com/G4rb3n/IoT_Sec_Tutorial/blob/master/05-修复固件运行环境/pic/18.png)

编写好apmib.c后，使用mips-linux-gnu-gcc命令将该代码编译成so，在这之前得先下载mips的gcc。

```
sudo apt install gcc-mips-linux-gnu
```

然后使用命令进行编译。

```
mips-linux-gnu-gcc -Wall fPIC -shared apmib.c -o apmib-ld.so
```

![19](https://github.com/G4rb3n/IoT_Sec_Tutorial/blob/master/05-修复固件运行环境/pic/19.png)

使用LD_PRELOAD参数指定劫持so，这样当boa执行到apmib_init和apmib_get时，就会调用到apmib-ld.so里面的函数，从而顺利运行，运行成功的效果如下。

![20](https://github.com/G4rb3n/IoT_Sec_Tutorial/blob/master/05-修复固件运行环境/pic/20.png)

我们可以看到Web服务以跑起来了，80端口。

![21](https://github.com/G4rb3n/IoT_Sec_Tutorial/blob/master/05-修复固件运行环境/pic/21.png)

然而当我们打开页面时，页面自动跳转到了Wizard_Easy_LangSelect.asp，程序又双叒崩溃了。。

![22](https://github.com/G4rb3n/IoT_Sec_Tutorial/blob/master/05-修复固件运行环境/pic/22.png)

看下该ASP文件的代码，通过文件名可以猜测功能是选择页面语言，而它尝试从硬件设备读取语言，显然又会出错了。

![23](https://github.com/G4rb3n/IoT_Sec_Tutorial/blob/master/05-修复固件运行环境/pic/23.png)

那我们就想办法不让它进入这个页面，查看入口网页，发现逻辑是先判断系统语言，成功则直接进入Webcome界面。

![24](https://github.com/G4rb3n/IoT_Sec_Tutorial/blob/master/05-修复固件运行环境/pic/24.png)

所以...我们直接魔改，两种情况都进入Wizard_Easy_Welcome.asp界面。

![25](https://github.com/G4rb3n/IoT_Sec_Tutorial/blob/master/05-修复固件运行环境/pic/25.png)

改完后再次访问，终于成功了，不容易，至此Web服务已经成功跑起来了，就可以开始挖洞了。

![26](https://github.com/G4rb3n/IoT_Sec_Tutorial/blob/master/05-修复固件运行环境/pic/26.png)