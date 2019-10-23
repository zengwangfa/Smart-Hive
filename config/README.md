# 1.更新U-boot
- 超级终端 
	- 波特率57600
	- 数据流控制无
uboot按7进入烧写，以`kermit`协议发送文件，如下图
[uboot](../docs/pictures/uboot.png)

- 设置root密码为 `123456`

# 2.烧写系统
- 使用SCP将固件传输至/tmp
- 执行更新命令
```c
root@widora:/# cd /tmp
root@widora:/# sysupgrade openwrt.bin
```
- 执行一次清理操作
```c
root@widora:/# firstboot -y
```

# 3.替换、添加配置文件

- 3.1 添加gateway文件夹至 `/root/`中  
- 3.2 替换`/etc/6lbr/6lbr.conf`   
	- 配置串口	
- 3.3 替换`/etc/config/network`
	- 配置网络
- 3.4 添加`/etc/rc.d/S94gateway` 
	- 自启动node main.js 
- 3.5 输入`chmod +x /etc/rc.d/S94gateway`  使生效


# 4.开启wifi
- `/etc/config/wireless` 将disable注释
	