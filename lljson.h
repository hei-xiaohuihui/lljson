﻿// #pragma once 是依赖于编译器的，可移植性不高
// #ifndef是C/C++标准中的一部分，用于防止头文件被多次包含，且可移植性高
#ifndef LLJSON_H__
#define LLJSON_H__

// c语言中枚举值用全大写，而类型和函数用小写

#include <stddef.h> /* size_t */

// JSON中有空值 boolean值（这里分为true和false） 数值 字符串 数组 对象6中数据类型
// 为json中的数据类型定义一个枚举类
typedef enum {	
	LLJSON_NULL = 0, 
	LLJSON_FALSE = 1, 
	LLJSON_TRUE = 2, 
	LLJSON_NUMBER = 3, 
	LLJSON_STRING = 4, 
	LLJSON_ARRAY = 5, 
	LLJSON_OBJECT = 6
} lljson_type;

// (size_t)-1将得到size_t类型的最大值
#define LLJSON_KEY_NOT_EXIST ((size_t)-1)

// JSON字符串是一个树形结构，使用结构体lljson_value定义该树的节点
// 因为lljson_value中使用了自身类型的指针，所以必须前向声明此类型
typedef struct lljson_value lljson_value;
typedef struct lljson_object_member lljson_object_member;
struct lljson_value{
	/* 一个值不可能同时为数字、字符串或数组，因此使用union来节省内存 */
	union {
		// size是对象中的成员个数，capacity是对象的容量
		struct { lljson_object_member* m; size_t size, capacity; }object; /* object */
		// size是数组中实际元素的个数，capacity是数组的最大容量
		struct { lljson_value* e; size_t size, capacity; }array; /* array */
		// len是字符串的长度
		struct { char* s; size_t len; }string; /* string */
		double number; /* number */
	}u;
	lljson_type type; // 数据类型
	//double number; // 数值
	//char* string; // 字符串
	//size_t length; // 字符串长度
};

/*
*	对象中的成员类型：
		key-value（键值对的形式）
*	（1）由于key一定是JSON字符串类型，采用lljson_value存储键key会浪费其中的type
*		字段，因此不采用lljson_value类型存储key，而采用指针+长度的形式；
*	（2）value可以为JSON中的任意合法类型，因此采用lljson_value存储。
*/
struct lljson_object_member { 
	// len为键key（字符串类型）的长度
	struct { char* k; size_t len; }key; /* member key */
	lljson_value value; /* member value */
};

// 函数返回值枚举
enum {
	LLJSON_PARSE_OK = 0, // 无错误
	LLJSON_PARSE_EXPECT_VALUE, // JSON为空
	LLJSON_PARSE_INVALID_VALUE, // 非JSON的6大类型中的类型
	LLJSON_PARSE_ROOT_NOT_SINGULAR, //  root must not follow by other values
	LLJSON_PARSE_NUMBER_TOO_BIG, // 数值溢出
	LLJSON_PARSE_MISS_QUOTATION_MARK, // 缺引号
	LLJSON_PARSE_INVALID_STRING_ESCAPE, // 无效的转义字符
	LLJSON_PARSE_INVALID_STRING_CHAR, // 无效的字符串字符
	LLJSON_PARSE_INVALID_UNICODE_HEX, // 无效的十六进制位数
	LLJSON_PARSE_INVALID_UNICODE_SURROGATE, // 无效的代理对
	LLJSON_PARSE_MISS_COMMA_OR_SQUARE_BRACKET, // 缺逗号或方括号（数组）
	LLJSON_PARSE_MISS_KEY, // 缺少键key
	LLJSON_PARSE_MISS_COLON, // 缺少冒号
	LLJSON_PARSE_MISS_COMMA_OR_CURLY_BRACKET // 缺少逗号或花括号（对象）
};

// JSON生成器 -> 将树形数据结构转换成JSON文本
// length参数是可选的，它会存储JSON的长度，传入NULL可忽略此参数
char* lljson_stringify(const lljson_value* v, size_t* length);

// 复制/移动/交换操作
// 深度复制：把value整个复制一份出来修改，原来的保持不变
void lljson_copy(lljson_value* dst, const lljson_value* src);
// 移动语义：把value的拥有权转移至新增的键值对，再把原来的value设置为null
void lljson_move(lljson_value* dst, lljson_value* src);
// 交换值
void lljson_swap(lljson_value* lhs, lljson_value* rhs);

// 释放lljson_value*类型指向的内存空间
void lljson_free(lljson_value* v);

// json解析函数/解析器
int lljson_parse(lljson_value* v, const char* json);
// 获取节点的json类型
lljson_type lljson_get_type(const lljson_value* v);
// 判断两个lljson_value是否相等
int lljson_is_equal(const lljson_value* lhs, const lljson_value* rhs);

// null
#define lljson_set_null(v) lljson_free(v) // set_null于free操作基本相同，因此使用宏定义

// 数字
void lljson_set_number(lljson_value* v, double n); // set数字
double lljson_get_number(const lljson_value* v); // 获取JSON数字的数值(double类型)

// 字符串
void lljson_set_string(lljson_value* v, const char* s, size_t len); // set字符串
const char* lljson_get_string(const lljson_value* v); // get字符串
size_t lljson_get_string_length(const lljson_value* v); // 获取字符串长度

// boolean类型
void lljson_set_boolean(lljson_value* v, int b); // set布尔值
int lljson_get_boolean(const lljson_value* v); // get布尔值

// array类型
void lljson_set_array(lljson_value* v, size_t capacity); // 将一个lljson_value对象初始化为数组类型，设置其初始容量
size_t lljson_get_array_size(const lljson_value* v); // get 数组中元素的个数
size_t lljson_get_array_capacity(const lljson_value* v);// get 数组的容量
void lljson_reserve_array(lljson_value* v, size_t capacity); // 扩容
void lljson_shrink_array(lljson_value* v); // 数组不需要在修改时，将容量缩小至size
void lljson_clear_array(lljson_value* v); // 清空数组中的所有元素（不改变容量）
lljson_value* lljson_get_array_element(const lljson_value* v, size_t index); // 获取数组下标index处的元素
lljson_value* lljson_pushback_array_element(lljson_value* v); // 在末尾插入一个元素
void lljson_popback_array_element(lljson_value* v); // 将末尾元素删除
lljson_value* lljson_insert_array_element(lljson_value* v, size_t index); // 在下标index处插入一个元素
void lljson_erase_array_element(lljson_value* v, size_t index, size_t count); // 删除从下标index开始的count个元素

// object类型 - > 一种无序的键值对集合，以{开始，}结束，键与值之间使用:分割
void lljson_set_object(lljson_value* v, size_t capacity);
size_t lljson_get_object_size(const lljson_value* v); // get对象中的成员个数
size_t lljson_get_object_capacity(const lljson_value* v); // get对象的容量
void lljson_reserve_object(lljson_value* v, size_t capacity); // object容量不足时，为其增加容量
// object类型的键key必须是字符串类型，而值可以是JSON中的任意类型
const char* lljson_get_object_key(const lljson_value* v, size_t index); // get key
size_t lljson_get_object_key_length(const lljson_value* v, size_t index); // get key's length
lljson_value* lljson_get_object_value(const lljson_value* v, size_t index); // get value
// 根据键key查询一个键值对是否存在，存在返回其在object中的下标
size_t lljson_find_object_index(const lljson_value* v, const char* k, size_t len);
// 根据key获取其对应的值value
lljson_value* lljson_find_object_value(const lljson_value* v, const char* k, size_t len);
// set value
lljson_value* lljson_set_object_value(lljson_value* v, const char* k, size_t len);

#endif

