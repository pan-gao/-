# STM32G474 双通道示波器+多功能信号发生器
基于STM32G474VET6 + HAL库实现，ADC采集示波器、DAC输出正弦/方波/三角波。

## 项目功能
1. ADC1双通道DMA采样，串口上传波形至PC上位机
2. DAC1输出3种波形，上位机串口指令修改频率、幅值
3. 按键启停ADC示波器采样
4. Python上位机实时绘图显示波形

## 硬件接线
- PA0/PA1：ADC模拟输入
- PA4：DAC波形输出
- PA9(TX)/PA10(RX)：串口1接USB转TTL
- PB0：功能按键

## 开发环境
- STM32CubeMX 6.x
- KEIL MDK5
- Python3(上位机)

## 使用教程
1. CubeMX按文档配置时钟170MHz、ADC/DAC/TIM/USART
2. 编译下载固件到STM32
3. PC串口助手发送指令：`$0,200,1500;` →200Hz正弦波
4. 运行Python脚本接收波形绘图

## 开源协议
本项目采用 [MIT LICENSE](./LICENSE)
