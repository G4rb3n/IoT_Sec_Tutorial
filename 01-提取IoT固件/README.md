# 提取 IoT 固件

首先我们需要安装固件提取工具 binwalk，由于该工具的安装流程比较繁琐，建议直接使用Kali Linux，该系统默认安装有 binwalk。
```
https://github.com/ReFirmLabs/binwalk
```
![1](https://github.com/G4rb3n/IoT_Sec_Tutorial/blob/master/01-%E6%8F%90%E5%8F%96IoT%E5%9B%BA%E4%BB%B6/1.png)

提取固件系统的参数是-e，我们可以加上-t -vv参数查看详细的提取过程。通过输出信息，可以得知该固件系统没有加密压缩，且系统为Squashfs。
```
binwalk -t -vv -e RT-N300_3.0.0.4_378_9317-g2f672ff.trx
```
![2](https://github.com/G4rb3n/IoT_Sec_Tutorial/blob/master/01-%E6%8F%90%E5%8F%96IoT%E5%9B%BA%E4%BB%B6/2.png)

提取出来的文件夹为_RT-N300_3.0.0.4_378_9317-g2f672ff.trx.extracted，其中的squashfs-root就是我们想要的该固件的文件系统。
![3](https://github.com/G4rb3n/IoT_Sec_Tutorial/blob/master/01-%E6%8F%90%E5%8F%96IoT%E5%9B%BA%E4%BB%B6/3.png)

那么binwalk的是怎么实现提取的呢？原理就是，通过自带的强大的magic特征集，扫描固件中文件系统初始地址的特征码，若匹配成功，则将该段数据dump下来，这个magic特征集已公开。
```
https://github.com/ReFirmLabs/binwalk/blob/62e9caa164305a18d7d1f037ab27d14ac933d3cf/src/binwalk/magic/filesystems
```
![4](https://github.com/G4rb3n/IoT_Sec_Tutorial/blob/master/01-%E6%8F%90%E5%8F%96IoT%E5%9B%BA%E4%BB%B6/4.png)

以这个固件为例，是Squashfs文件系统，对应的扫描特征码为hsqs。
![5](https://github.com/G4rb3n/IoT_Sec_Tutorial/blob/master/01-%E6%8F%90%E5%8F%96IoT%E5%9B%BA%E4%BB%B6/5.png)

我们可以做个实验，使用hexdump搜索hsqs的地址，为0xe20c0，这个就是文件系统的初始地址。
```
hexdump -C RT-N300_3.0.0.4_378_9317-g2f672ff.trx | grep -i 'hsqs'
```
![6](https://github.com/G4rb3n/IoT_Sec_Tutorial/blob/master/01-%E6%8F%90%E5%8F%96IoT%E5%9B%BA%E4%BB%B6/6.png)

使用dd命令截取地址925888（0xe20c0）之后的数据，保存到rt-n300-fs。
```
dd if=RT-N300_3.0.0.4_378_9317-g2f672ff.trx bs=1 skip=925888 of=rt-n300-fs
```
![7](https://github.com/G4rb3n/IoT_Sec_Tutorial/blob/master/01-%E6%8F%90%E5%8F%96IoT%E5%9B%BA%E4%BB%B6/7.png)

最后，使用unsquashfs rt-n300-fs命令解析rt-n300-fs文件，得到的squashfs-root就是固件系统，这个跟上述binwalk提取的那个是一样的。
```
unsquashfs rt-n300-fs
```
![8](https://github.com/G4rb3n/IoT_Sec_Tutorial/blob/master/01-%E6%8F%90%E5%8F%96IoT%E5%9B%BA%E4%BB%B6/8.png)
