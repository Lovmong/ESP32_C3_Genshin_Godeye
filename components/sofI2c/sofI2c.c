#include "sofI2c.h"
// #include "esp_rom.h"
bool ack;
uint8_t I2CADR = 0x2b;

static void delay_us(uint32_t u_sec)
{
	uint16_t cnt = 0, j = 0, i = 0;
	volatile uint32_t ew = 0xFFFF;
	volatile uint32_t se = 0;
	while (u_sec--)
	{
		for (cnt = 5; cnt > 0; cnt--)
		{
			ew--;
			se = ew;
		}
	}
}

void SW_I2C1_Init(void)
{
	I2C_SDA_OUT(); // PB7输出模式
}

/*
 * @brief IIC起始信号
 */
void I2C_Start(void)
{
	I2C_SDA_OUT();
	SDA_H; // 先拉高SDA再拉高SCL，防止出现错误信号
	SCL_H;
	delay_us(10); // 延时保证SCL高电平时，SDA下降沿
	SDA_L;
	delay_us(8);
	SCL_L; // 钳住I2C总线，准备发送或接收数据
	delay_us(3);
}

/*
 * @brief IIC停止信号
 */
void I2C_Stop(void)
{
	// SCL_L; //phph2045
	I2C_SDA_OUT(); // phph2045
	SDA_L;		   // 先拉低SDA再拉高SCL，防止出现错误信号
	delay_us(5);   // 延时保证SCL高电平时，SDA上升沿 //phph2045
	SCL_H;
	delay_us(10); // 延时保证SCL高电平时，SDA上升沿
	SDA_H;
	//	delay_us(10);
	// 这里就不用拉低SCL了，因为IIC通讯已经结束
	// SCL_L; //钳住I2C总线，准备发送或接收数据
}

/*
 * @brief IIC主机等待应答
 * 返回值：0，接收应答成功
 *         -1，接收应答失败
 */
int8_t I2C_Wait_Ack(void)
{
	uint8_t ucErrTime = 0;

	SDA_H; // 一定要先拉高SDA，再拉高SCL，否则就可能变成起始信号和结束信号了
	delay_us(1);
	I2C_SDA_IN(); // SDA设置为输入
	delay_us(1);  // phph2045
	SCL_H;
	delay_us(2);
	while (I2C_Read_SDA)
	{
		ucErrTime++;
		if (ucErrTime > 250)
		{
			I2C_Stop();
			I2C_SDA_OUT(); // SDA恢复为输出
			return 0;
		}
	}
	SCL_L;		   // IIC通讯没结束，拉低SCL线
	I2C_SDA_OUT(); // SDA恢复为输出

	return 1;
}

// 产生ACK应答
void I2C_Ack(void)
{
	// SCL_L; //phph2047
	I2C_SDA_OUT(); // SDA设置为输出
	// SCL_L; //phph2046
	SDA_L;
	delay_us(2);
	SCL_H;
	delay_us(2);
	SCL_L; // IIC通讯没结束，拉低SCL线
}

// 产生NACK应答
void I2C_NAck(void)
{
	SCL_L;		   // phph2045
	I2C_SDA_OUT(); // SDA设置为输出
	// SCL_L; //phph2046
	SDA_H;
	delay_us(2);
	SCL_H;
	delay_us(2);
	SCL_L; // IIC通讯没结束，拉低SCL线
}

// 读1个字节，ack=1时，发送ACk；ack=0，发送NACK
uint8_t I2C_Read_Byte(uint8_t ack)
{
	uint8_t i = 0;
	uint8_t recv = 0;

	I2C_SDA_IN(); // SDA设置为输入

	for (i = 0; i < 8; i++)
	{
		SCL_L; // 拉低时钟线
		delay_us(2);
		SCL_H; // 拉高时钟线，接收1位数据
		recv <<= 1;
		// phph2045 delay_us(2);
		if (I2C_Read_SDA)
			recv |= 0x01;
		delay_us(2);
	}
	SCL_L; // phph2047

	if (!ack)
		I2C_NAck(); // 发送NACK
	else
		I2C_Ack(); // 发送ACK

	I2C_SDA_OUT(); // SDA恢复为输出

	return recv;
}

/*
 * @brief IIC发送一个字节
 * @para  待写入的字节数据
 * 返回从机有无应答
 *    1，有应答
 *    0，无应答
 */
void I2C_Send_Byte(uint8_t ucByte)
{
	uint8_t bit_Cnt = 0;

	I2C_SDA_OUT();
	// phph2045 SCL_L; //拉低时钟开始数据传输

	for (bit_Cnt = 0; bit_Cnt < 8; bit_Cnt++)
	{
		if ((ucByte << bit_Cnt) & 0x80)
		{
			SDA_H;
		}
		else
		{
			SDA_L;
		}
		delay_us(5);
		SCL_H;
		delay_us(6);
		SCL_L;
	}
	delay_us(5);
	SDA_H;
	delay_us(8);
	I2C_SDA_IN();
	SCL_H;
	delay_us(5);

	if (I2C_Read_SDA)
	{
		ack = 0;
	}
	else
	{
		ack = 1;
	}
	SCL_L;
	I2C_SDA_OUT();
	//	delay_us(8);
}

bool I2C_write_byte(uint8_t sla, uint8_t suba, uint8_t *s, uint8_t no)
{
	uint8_t i;
	I2C_Start();

	I2C_Send_Byte(sla);
	if (ack == false)
	{
		I2C_Stop();
		return false;
	}

	I2C_Send_Byte(suba);
	if (ack == false)
	{
		I2C_Stop();
		return false;
	}

	for (i = 0; i < no; i++)
	{
		I2C_Send_Byte(*s);
		if (ack == false)
		{
			I2C_Stop();
			return false;
		}
		s++;
	}
	I2C_Stop();
	return true;
}

uint8_t RcvByte(void)
{
	uint8_t retc = 0;
	uint8_t bit_Cnt;
	SDA_H;
	I2C_SDA_IN(); // SDA设置为输入
	for (bit_Cnt = 0; bit_Cnt < 8; bit_Cnt++)
	{
		delay_us(3);
		SCL_L;
		delay_us(5);
		SCL_H;
		delay_us(3);
		retc = retc << 1;
		if (I2C_Read_SDA)
		{
			retc = retc + 1;
		}
		delay_us(5);
	}
	SCL_L;
	delay_us(5);
	return retc;
}

void Ack_I2C(bool a)
{
	I2C_SDA_OUT();
	if (a == 0)
	{
		SDA_L;
	}
	else
	{
		SDA_H;
	}

	delay_us(8);
	SCL_H;
	delay_us(15);
	SCL_L;
	delay_us(10);
}

bool I2C_read_byte(uint8_t sla, uint8_t suba, uint8_t *s, uint8_t no)
{
	uint8_t i;
	I2C_Start();

	I2C_Send_Byte(sla);
	// if (ack == 0)
	// {
	// 	I2C_Stop();
	// 	*s = 0x00;
	// 	return 0;
	// }

	I2C_Send_Byte(suba);
	// if (ack == 0)
	// {
	// 	I2C_Stop();
	// 	*s = 0x00;
	// 	return 0;
	// }

	I2C_Start();
	I2C_Send_Byte(sla + 1);
	// if (ack == 0)
	// {
	// 	I2C_Stop();
	// 	*s = 0x00;
	// 	return 0;
	// }
	for (i = 0; i < no - 1; i++)
	{
		*s = RcvByte();
		Ack_I2C(0);
		s++;
	}
	*s = RcvByte();
	Ack_I2C(1);
	I2C_Stop();
	return 1;
}

void write_addr(uint8_t addr)
{
	I2CADR = addr;
}

uint8_t Read_I2C_Byte(uint8_t RegAddr)
{
	uint8_t p_data = 0;
	if (I2C_read_byte(I2CADR, RegAddr, &p_data, 1))
	{
		return p_data;
	}
	return p_data;
}

bool Read_I2C_ByteN(uint8_t RegAddr, uint8_t *p_data, uint8_t N)
{
	bool flag;
	flag = I2C_read_byte(I2CADR, RegAddr, p_data, N);
	return flag;
}

bool Write_I2C_Byte(uint8_t RegAddr, uint8_t d)
{
	bool flag;
	flag = I2C_write_byte(I2CADR, RegAddr, &d, 1);
	return flag;
}

bool Write_I2C_ByteN(uint8_t RegAddr, uint8_t *d, uint8_t N)
{
	bool flag;
	flag = I2C_write_byte(I2CADR, RegAddr, d, N);
	return flag;
}
