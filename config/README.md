# 更新U-boot
- 超级终端 
	- kermit协议
	- 波特率57600
	- 数据流控制无

# 烧写系统

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
- 设置root密码为 `123456`
# 替换、添加配置文件

- 替换`/etc/6lbr/6lbr.conf`   
	- 配置串口	
- 替换`/etc/config/network`
	- 配置网络
- 添加`/etc/rc.d/S94gateway` 
	- 自启动node main.js 
- 输入`chmod +x /etc/rc.d/S94gateway`  使生效
	