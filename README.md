# 智能蜂箱系统

## 一、项目概述


| 硬件 | 主控制器(STM32) | 网关(MT7688) | 
| :--: | :--: | :--: |
|芯片型号| STM32F405RGT6 | MT7688AN |
|CPU| 32位 Cortex-M4(ARMv7) | MIPS24KEc |
|主频| 168MHz | 580MHz |
|RAM| 192KB | 256MB |
|外存| 1MB(FLASH) | 32MB(FLASH) |
|单元| FPU、DSP | Ethernet、WiFi |

> 针对户外大范围、大批量的蜂箱养殖，以及目前人工养蜂的人力成本过高等诸多问题，本设计提出了一套智能蜂箱终端控制方案，实现的内容为：用户可通过web端远程实时监控蜂箱状态（例如实时了解各个蜂箱的编号、重量、温度等）以及蜂箱防盗并且可以发送一系列指令进行远程操控（例如清洗蜂箱、喂食或气温下降给蜂箱加热、），极大便利了蜜蜂的养殖过程，降低了养蜂的人工成本。



## 二、通讯示意结构

![Communicatiom Structure](/docs/pictures/Communication_Structure.jpg)

## 三、软件设计说明

- 编译环境：IAR 8.30
- 运行平台：STM32F405RGT6
- 所使用的的操作系统：RT-Thread


- 该软件使用的平台为RT-Thread 3.0物联网操作系统，采用6LoWPAN物联网协议**6LoWPAN** (IPv6 over Low-Power Wireless Personal Area Networks) 作为物联网的一种协议方式。使用这项技术，所有节在点网络层通过 **IPv6**联系起来。

### 3.1 整体结构图

![System Structure](/docs/pictures/System_Structure.png)

- 主要功能为：采集外围设备数据（温度、重量、编号），将数据按照规定协议打包成**CJSO**数据格式，通过**433M**无线模块向**网关**上传数据，接收来自433M无线模块的控制数据包，进行外设的控制（加热控制、喂食控制、清洗控制）。
---
### 3.2 程序简易流程图
![Flow Chart](/docs/pictures/Flow_Chart.png)
---
### 3.3 实物样机测试
![Dome1](/docs/pictures/node1.jpg)

![Dome1](/docs/pictures/node2.jpg)
---
### 3.4 Web端显示
![Flow Chart](/docs/pictures/web1.jpg)

![Flow Chart](/docs/pictures/web2.png)