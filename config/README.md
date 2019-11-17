# 1.更新U-boot
- **超级终端**
	- 波特率：57600
	- 数据流控制：无
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
- 烧写系统结束后，执行一次清理操作
```c
root@widora:/# firstboot -y
```

# 3.替换、添加配置文件

## 3.1 添加网关服务程序
- 添加gateway文件夹至 `/root/`目录下  
	- 不同网关编号修改 conf.js中的 GW000X号

## 3.2 配置串口
- 替换`/etc/6lbr/6lbr.conf`   
	- 配置串口设备： /dev/ttyS1（与STM32对应）
	- 配置波特率： 230400  （与STM32对应）
## 3.3 配置网络
- 替换`/etc/config/network`

## 3.4 添加自启动脚本
- 添加`/etc/rc.d/S94gateway` 
	- 自启动node main.js 
	
- 输入`chmod +x /etc/rc.d/S94gateway`  使脚本生效


# 4.开启wifi
- `/etc/config/wireless` 将disable注释
	