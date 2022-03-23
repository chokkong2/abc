﻿#include "Header.h"

unsigned char check;                          // Check Sum
unsigned char binary[16];                     // 2진 data 배열
unsigned char lowByte;                        // 2진 data의 하위 byte
unsigned char highByte;                       // 2진 data의 상위 byte
unsigned char firstLowByte;                   // 첫 번째 모터에 전달할 하위 byte
unsigned char firstHighByte;                  // 첫 번째 모터에 전달할 상위 byte
unsigned char secondLowByte;                  // 두 번째 모터에 전달할 하위 byte
unsigned char secondHighByte;                 // 두 번째 모터에 전달할 상위 byte
int Coordinate_X, Coordinate_Y;               // 목표 좌표 (X,Y)   
int temp;                                     // Transform 함수의 변수
extern unsigned char instr[20];


int Square (int i, int j)             // i의 j제곱
{
	unsigned int result = 1;
	int k=0;
	for (k=0; k<j; k++)
	{
		result *= i;
	}
	return result;
}

void Transform (unsigned char *InstrPacket)             // PC에서 송신한 ASCII 코드 data를 정수형으로 변환
{
	int i = 0;
	
	if (InstrPacket[4] != 32 || InstrPacket[1] > 51 || InstrPacket[6] > 51)          // 300 이상의 좌표는 제한
	{
		return ;
	}

	for (i=1; i<4; i++)
	{
		temp += (Square(10,3-i)*(InstrPacket[i]-48));
	}
	if (InstrPacket[0] == '-')
	{
		Coordinate_X = -temp;
	}
	else
	{
		Coordinate_X = temp;
	}
	temp = 0;
	
	for (i=6; i<9; i++)
	{
		temp += (Square(10,8-i)*(InstrPacket[i]-48));   
	}
	if (InstrPacket[5] == '-')
	{
		Coordinate_Y = -temp;
	}
	else
	{
		Coordinate_Y = temp;
	}
	temp = 0;
}

unsigned char CheckSum (int check)      // checksum의 하위 바이트 구하기
{
	int i=0, j=0;
	unsigned char checkLowByte = 0;
	while (check != 0)
	{
		binary[i] = check % 2;
		check /= 2;
		i++;
	}
	
	for (j=i; j<16; j++)
	{
		binary[j] = 0;
		i++;
	}

	for (j=i-9; j>=0; j--)
	{
		checkLowByte += (binary[j] * Square(2,j));
	}
	return checkLowByte;
}

void DecimalToBinary (int decimal)      // 10진수를 LOW, HIGH의 2byte로 분할
{
	int i= 0, j=0;
	lowByte = 0;
	highByte = 0;
	while (decimal != 0)
	{
		binary[i] = decimal % 2;
		decimal /= 2;
		i++;
	}
	
	for (j=i; j<16; j++)
	{
		binary[j] = 0;
		i++;
	}

	for (j=i-1; j>=i-8; j--)
	{
		highByte += (binary[j] * Square(2,j-8));
	}
	
	for (j=i-9; j>=0; j--)
	{
		lowByte += (binary[j] * Square(2,j));
	}
}

void Direction_Tx (void)                             // Direction_PORT Tx 설정
{
	PORTA &=0b11111101;
	PORTA |=0b00000010;
}

void Direction_Rx (void)                             // Direction_PORT Rx 설정
{
	PORTA &=0b11111101;
}

void Ping (unsigned char ID)
{
	check = 0;
	Direction_Tx();                              // Dynamixel 통신방향 설정
	cli();
	Tx_MCUtoDyn(header);
	Tx_MCUtoDyn(header);
	Tx_MCUtoDyn(ID);
	Tx_MCUtoDyn(0x02);
	Tx_MCUtoDyn(pING);
	check = ~(ID + 0x02 + pING);
	Tx_MCUtoDyn(check);
	while (!(UCSR1A & 0x40));                    // 송신 시프트 레지스터가 비어있는지 확인
	_delay_us(100);
	Direction_Rx();                              // Dynamixel 통신방향 설정
	sei();
}

void GoalPosition (unsigned char ID, int angle)      // 180도 위치
{
	angle = (int)(((float)(angle / 0.088))+0.5);       // Goal Position(L,H) 레지스터 값 (반올림)
	DecimalToBinary(angle);                          // 2Byte data로 분할
	check = 0;
	
	Direction_Tx();                                  // Dynamixel 통신방향 설정
	cli();
	Tx_MCUtoDyn(header);
	Tx_MCUtoDyn(header);
	Tx_MCUtoDyn(ID);
	Tx_MCUtoDyn(0x07);                        // Length
	Tx_MCUtoDyn(WRITE_DATA);                  // write data
	Tx_MCUtoDyn(GoalPosition_Address);        // 목표 위치 값의 주소 하위 바이트
	Tx_MCUtoDyn(lowByte);                     // 목표 위치값의 하위 바이트
	Tx_MCUtoDyn(highByte);                    // 목표 위치값의 상위 바이트
	Tx_MCUtoDyn(0x00);
	Tx_MCUtoDyn(0x02);
	check = ~CheckSum(ID + 0x07 + WRITE_DATA + GoalPosition_Address + lowByte + highByte + 0x00 + 0x02);
	Tx_MCUtoDyn(check);
	while (!(UCSR1A & 0x40));                 // 송신 시프트 레지스터가 비어있는지 확인
	_delay_us(100);
	Direction_Rx();                           // Dynamixel 통신방향 설정
	sei();
}

void SyncGoalPosition (unsigned char ID1, unsigned char ID2, double firstangle, double secondangle)
{
	firstangle = (firstangle/M_PI)*180;      // rad to degree
	firstangle = (int)((firstangle/0.088)*(3.1)+0.5);     // Goal Position(L,H) 레지스터 값 (반올림)
	DecimalToBinary(firstangle);
	firstHighByte = highByte;
	firstLowByte = lowByte;
	
	secondangle = ((secondangle)/M_PI)*180;
    secondangle = (int)(((secondangle / 0.088)*3.1)+0.5);     // Goal Position(L,H) 레지스터 값 (반올림)
	DecimalToBinary(secondangle-firstangle);                  // 왜 두번째 각도에서 첫번째 각도를 뺐는지는 메모장에 설명해뒀음
	secondHighByte = highByte;
	secondLowByte = lowByte;
	
	check = 0;
	Direction_Tx();                        // Dynamixel 통신방향 설정
	cli();
	
	Tx_MCUtoDyn(header);
	Tx_MCUtoDyn(header);
	Tx_MCUtoDyn(Broadcasting_ID);          // 한번에 여러 개의 다이나믹셀을 동시에 제어하는 Instruction
	                                       // (data를 쓰고자하는 곳의 control table의 Address와 Length가 동일해야함)
	Tx_MCUtoDyn(0x0E);				       // Instruction Packet의 length
	Tx_MCUtoDyn(SYNC_WRITE);			   // Instruction
	Tx_MCUtoDyn(GoalPosition_Address);	   // Data를 쓰고자 하는 곳의 시작 Address
	Tx_MCUtoDyn(0x04);                     // 쓰고자 하는 Data의 length
	Tx_MCUtoDyn(ID1);                      // 첫 번째 모터의 ID
	Tx_MCUtoDyn(firstLowByte);             // 첫 번째 모터의 DATA 1
	Tx_MCUtoDyn(firstHighByte);            // 첫 번째 모터의 DATA 2
	Tx_MCUtoDyn(0x0E);					   // 첫 번째 모터의 DATA 3
	Tx_MCUtoDyn(0x01);					   // 첫 번째 모터의 DATA 4
	Tx_MCUtoDyn(ID2);					   // 두 번째 모터의 ID
	Tx_MCUtoDyn(secondLowByte);            // 두 번째 모터의 DATA 1
	Tx_MCUtoDyn(secondHighByte);           // 두 번째 모터의 DATA 2
	Tx_MCUtoDyn(0x0E);					   // 두 번째 모터의 DATA 3
	Tx_MCUtoDyn(0x01);					   // 두 번째 모터의 DATA 4
	check = ~CheckSum(Broadcasting_ID + 0X0E + SYNC_WRITE + GoalPosition_Address + 0x04 + ID1 + firstLowByte + firstHighByte + 0x0E + 0x01 + ID2 + secondLowByte + secondHighByte + 0x0E + 0x01);     // checksum 계산
	Tx_MCUtoDyn(check);					   // checksum
	while (!(UCSR1A & 0x40));
	_delay_us(100);
	Direction_Rx();                        // Dynamixel 통신방향 설정
	sei();
}

void SynGoalAcceleration (unsigned char ID1)
{
	check = 0;
	
	Direction_Tx();                                  // Dynamixel 통신방향 설정
	cli();
	Tx_MCUtoDyn(header);
	Tx_MCUtoDyn(header);
	Tx_MCUtoDyn(ID1);
	Tx_MCUtoDyn(0x04);                            // Length
	Tx_MCUtoDyn(WRITE_DATA);                      // write data
	Tx_MCUtoDyn(GoalAcceleration_Address);        // 목표 가속도값의 주소 하위 바이트
	Tx_MCUtoDyn(0x14);                            // 목표 가속도값의 하위 바이트
	check = ~CheckSum(ID1 + 0x04 + WRITE_DATA + GoalAcceleration_Address + 0x14);
	Tx_MCUtoDyn(check);
	while (!(UCSR1A & 0x40));                     // 송신 시프트 레지스터가 비어있는지 확인
	_delay_us(100);
	Direction_Rx();                               // Dynamixel 통신방향 설정
	sei();
}

void Read_Acceleration (unsigned char ID)
{
	check = 0;
	
	Direction_Tx();                                  // Dynamixel 통신방향 설정
	cli();
	Tx_MCUtoDyn(header);
	Tx_MCUtoDyn(header);
	Tx_MCUtoDyn(ID);
	Tx_MCUtoDyn(0x04);                            // Length
	Tx_MCUtoDyn(READ_DATA);                      // write data
	Tx_MCUtoDyn(GoalAcceleration_Address);        // 목표 가속도값의 주소 하위 바이트
	Tx_MCUtoDyn(0x01);                            // 쓰고자 하는 Data의 길이
	check = ~CheckSum(ID + 0x04 + READ_DATA + GoalAcceleration_Address + 0x01);
	Tx_MCUtoDyn(check);
	while (!(UCSR1A & 0x40));                     // 송신 시프트 레지스터가 비어있는지 확인
	_delay_us(100);
	Direction_Rx();                               // Dynamixel 통신방향 설정
	sei();
}