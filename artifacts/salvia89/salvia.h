/**
 * @file salvia.h
 * @brief Salvia 格式化字符串库头文件
 * 
 * 提供类似 printf 的格式化功能，但更精简和可定制。
 *
 * @author  锻月楼主
 * @date    2025-08-19
 * @version 0.1
 */
#ifndef _SALVIA_H_
#define _SALVIA_H_

/**
 * @brief 格式化字符串写入缓冲区
 * 
 * @param szBuf 目标缓冲区，将接收格式化后的字符串
 * @param szFormat 格式化字符串，支持:
 *                %d - 整数
 *                %s - 字符串 
 *                %c - 字符
 *                %% - 百分号
 * @param ... 可变参数，与格式化字符串中的占位符对应
 * @return int 写入字符的数量(不包括结尾的空字符)
 * 
 * @example
 *  char buf[100];
 *  Salvia_Format(buf, "Integer:%d，String:%s", 42, "hello");
 */
int Salvia_Format(char* szBuf, const char* szFormat, ...);

#endif /* _SALVIA_H_ */