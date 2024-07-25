#ifdef _WINDOWS		// windows平台下的内存泄漏检测
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif // _WINDOWS

#include "lljson.h"
#include <assert.h> // 使用c语言中的防御性编程方式断言assert
#include <stdlib.h> // strtod() NULL
#include <errno.h> // 使用errno.h中定义的宏来检测和处理数据溢出情况（ERANGE）
#include <math.h> // HUGE_VAL
#include <stdio.h>
#include <string.h> // memcpy

// 传入的JSON对象是一个c语言字符串（以\0结尾）

// 确保当前JSON字符串中的字符与预期字符匹配，然后将指针移动到下一个字符
#define EXPECT(c, ch) do { assert(*c->json == ch); c->json++; } while(0)
// 判断一个字符是否是数字
#define ISDIGIT(ch) (ch >= '0' && ch <= '9')
// 判断一个字符是否是数字1-9
#define ISDIGIT1TO9(ch) (ch >= '1' && ch <= '9')
// 堆栈的初始化大小
#define STACK_INIT_SIZE 256
// 入栈操作宏定义
#define PUTC(c, ch) do { *(char*)lljson_context_push(c, sizeof(char)) = ch; } while(0)

// 为了减少解析函数之间传递多个参数，将数据都放入结构体lljson_context中
typedef struct {
	const char* json;
	// 每次解析JSON时创建一个动态堆栈数据结构
	char* stack; // 指向堆栈起始地址，堆栈以字节为单位存储
	size_t size; // 当前堆栈的大小
	size_t top; // 栈顶位置，由于会扩展stack，因此top声明为非指针形式
}lljson_context;

/*
*	堆栈的入栈操作：
*		实现的是以字节存储的堆栈，每次可压入任意大小的数据;
*		入栈函数的返回值是指向栈顶数据的指针.
*/
// void*称为 通用型指针/无类型指针
// void*指针可以指向任意的数据类型
// 使用void*指针要进行强制类型转换，即显式说明该指针指向的内存中存放的是什么类型的数据
static void* lljson_context_push(lljson_context* c,size_t size) {
	void* ret;
	assert(size > 0); // 插入的数据大小需大于1
	if (c->top + size > c->size) { // 堆栈内存不足时
		if (c->size == 0) // 堆栈大小为0时将其大小初始化为宏定义的大小
			c->size = STACK_INIT_SIZE;
		while (c->top + size > c->size) // 再次判断若空间不足
			c->size += c->size >> 1; // 将堆栈大小增加50%
		c->stack = (char*)realloc(c->stack, c->size); // 使用realloc重新分配内存
	}
	ret = c->stack + c->top; // ret指向栈顶数据
	c->top += size;
	return ret;
}

/*
*	堆栈出栈操作：
*		返回值为指向栈顶数据的指针.
*/
static void* lljson_context_pop(lljson_context* c, size_t size) {
	assert(c->top >= size); 
	return c->stack + (c->top -= size);
}

// 处理JSON字符串中的空白符 ' '  '\t'  '\n'  '\r'
// 声明为静态函数，使其只能在定义其的文件内部使用
static void lljson_parse_whitespace(lljson_context* c) { 
	const char* p = c->json;
	//printf("%s\n", p);
	// 跳过JSON字符串中的空格符
	while (*p == ' ' || *p == '\n' || *p == '\t' || *p == '\r')
		p++;
	c->json = p; // 将指针移至非空格符位置
	//printf("%s\n", c->json);
}

//// 解析空值类型null
//static int lljson_parse_null(lljson_context* c, lljson_value* v) {
//	EXPECT(c, 'n'); // 判断首字符是否为n
//	if (c->json[0] != 'u' || c->json[1] != 'l' || c->json[2] != 'l')
//		return LLJSON_PARSE_INVALID_VALUE;
//	c->json += 3; // 指向JSON字符串的指针后移，跳过"ull"
//	v->type = LLJSON_NULL; // 置当前节点类型为null类型
//	return LLJSON_PARSE_OK; // 返回解析成功
//}
//
//// 解析true类型
//static int lljson_parse_true(lljson_context* c, lljson_value* v) {
//	EXPECT(c, 't');
//	if (c->json[0] != 'r' || c->json[1] != 'u' || c->json[2] != 'e')
//		return LLJSON_PARSE_INVALID_VALUE;
//	c->json += 3; // 指针后移，跳过"rue"
//	v->type = LLJSON_TRUE;
//	return LLJSON_PARSE_OK;
//}
//
//// 解析false类型
//static int lljson_parse_false(lljson_context* c, lljson_value* v) {
//	EXPECT(c, 'f');
//	if (c->json[0] != 'a' || c->json[1] != 'l' || c->json[2] != 's' || c->json[3] != 'e')
//		return LLJSON_PARSE_INVALID_VALUE;
//	c->json += 4;
//	v->type = LLJSON_FALSE;
//	return LLJSON_PARSE_OK;
//}

// 重构解析null false和true类型为lljson_parse_literal
static int lljson_parse_literal(lljson_context* c, lljson_value* v, const char* str, lljson_type type) {
	size_t i; // c语言中，数组长度、索引值最好使用size_t类型，而不是int类型
	EXPECT(c, str[0]);
	for (i = 0; str[i + 1]; i++) {
		if (c->json[i] != str[i + 1])
			return LLJSON_PARSE_INVALID_VALUE;
	}
	c->json += i;
	v->type = type;
	return LLJSON_PARSE_OK;
}

// 解析4位十六进制数来获取码点
// 解析成功返回指向解析后文本的指针，失败返回空指针NULL
static const char* lljson_parse_hex4(const char* p, unsigned int* u) {
	/* TODO */
	for (int i = 0; i < 4; i++) { // 解析4位十六进制数
		char ch = *p++;
		*u <<= 4; // u先左移4位（即移动一位十六进制数）
		if (ISDIGIT(ch)) *u |= ch - '0';
		else if (ch >= 'A' && ch <= 'F') *u |= ch - ('A' - 10);
		else if (ch >= 'a' && ch <= 'f') *u |= ch - ('a' - 10);
		else return NULL; // 其他情况为非表示十六进制数的字符
	}
	return p; // 解析成功返回指向解析后文本的指针
}

// 将码点转换为uft-8编码
static void lljson_encode_utf8(lljson_context* c, unsigned int u) {
	if (u <= 0x7F) { // 码点范围 U+0000 - U+007F 
		// 单字节
		PUTC(c, u & 0xFF);
	}else if (u >= 0x80 && u <= 0x07FF) { // 码点范围 U+0080 - U+07FF
		// 双字节
		PUTC(c, (u >> 6) & 0xFF | 0xC0);
		PUTC(c, u & 0x3F | 0x80);
	}else if (u >= 0x800 && u <= 0xFFFF) { // 码点范围 U+0800 - U+FFFF
		//三字节
		PUTC(c, (u >> 12) & 0xFF | 0xE0);
		PUTC(c, (u >> 6) & 0x3F | 0x80);
		PUTC(c, u & 0x3F | 0x80);
	}else { // 码点范围 U+10000 - U+10FFFF
		// 四字节
		assert(u <= 0x10FFFF); // 对于非范围内的码点采用断言
		PUTC(c, (u >> 18) & 0xFF | 0xF0);
		PUTC(c, (u >> 12) & 0x3F | 0x80);
		PUTC(c, (u >> 6) & 0x3F | 0x80);
		PUTC(c, u & 0x3F | 0x80);
	}
}

// 解析string类型
static int lljson_parse_string(lljson_context* c, lljson_value* v) {
	unsigned int u = 0, u2 = 0; // 记录解析4位十六进制数所得到的码点
	size_t head = c->top; //记录当前栈顶的位置
	size_t len;
	//printf("%s\n", c->json);
	const char* p;
	EXPECT(c, '\"');
	//printf("%s\n", c->json);
	p = c->json;
	while(1) {
		//printf("%s\n", p);
		char ch = *p++;
		switch (ch) {
			case '\"': // 解析正常结束（格式正确的字符串）
				// size_t len; “报错声明不能包含标签”，因为不能在case中声明变量
				// set字符串
				len = c->top - head;
				//printf("parse_string len: %Iu\n", len);
				//printf("%d\n", len);
				lljson_set_string(v, (const char*)lljson_context_pop(c, len), len);
				c->json = p; // 指针后移，因为后续可能还有其他类型的字符需处理
				return LLJSON_PARSE_OK;
			case '\\': // 转义字符的解析(处理转移字符）
				//printf("%s\n", p);
				switch (*p++) {
					case '\"': PUTC(c, '\"'); break; /* 单引号 \" -> " */
					case '\\': PUTC(c, '\\'); break; /* 反斜杠 \\ -> \ */
					case '/': PUTC(c, '/'); break; /* \/ -> / */
					case 'n': PUTC(c, '\n'); break; /* 换行符 \\n -> \n */
					case 't': PUTC(c, '\t'); break; /* 制表符 \\t -> \t */
					case 'r': PUTC(c, '\r'); break; /* 回车符 \\r -> \r */
					case 'b': PUTC(c, '\b'); break; /* 退格符 \\b -> \b */
					case 'f': PUTC(c, '\f'); break; /* 换页符 \\f -> \f */
					case 'u': // 处理转移序列 \uXXXX，实现将Unicode码点转换为utf-8编码
						if (!(p = lljson_parse_hex4(p, &u))) { // 解析4位十六进制数表示的Unicode码点
							// 4位十六进制数解析失败，返回INVALID_UNICODE_HEX
							c->top = head;
							return LLJSON_PARSE_INVALID_UNICODE_HEX;
						}
						/* 代理对surrogate pair */
						if (u >= 0xD800 && u <= 0xDBFF) { // 高代理项必须在前
							// 缺少低代理项（高代理项后不是低代理项）
							if (*p++ != '\\') {
								c->top = head;
								return LLJSON_PARSE_INVALID_UNICODE_SURROGATE;
							} 
							if (*p++ != 'u') {
								c->top = head;
								return LLJSON_PARSE_INVALID_UNICODE_SURROGATE;
							}
							// 低代理项十六进制位表达有误
							if (!(p = lljson_parse_hex4(p, &u2))) {
								c->top = head;
								return LLJSON_PARSE_INVALID_UNICODE_SURROGATE;
							}
							// 低代理项不在合法范围内
							if (u2 < 0xDC00 || u2 > 0xDFFF) { 
								c->top = head;
								return LLJSON_PARSE_INVALID_UNICODE_SURROGATE;
							}
							// 使用合法的高低代理项计算得到码点
							u = (((u - 0xD800) << 10) | (u2 - 0xDC00)) + 0x10000;
						}
						// 将码点转换为utf-8编码
						lljson_encode_utf8(c, u); 
						break;
					default: // 其他情况为无效的转义字符
						c->top = head; // 抛弃本次解析过程中存储的字符串
						return LLJSON_PARSE_INVALID_STRING_ESCAPE;
				}
				break;
			case '\0':  // 缺少右引号
				c->top = head; // 抛弃这次解析过程中存储的字符串，因为是无效的字符串
				return LLJSON_PARSE_MISS_QUOTATION_MARK;
			default: // 其他情况
				// 任何低于0x20的字符都是非法的，因为他们是不可见的控制字符
				if ((unsigned char)ch < 0x20) {
					c->top = head; // 抛弃此次解析过程中存储的字符串
					return LLJSON_PARSE_INVALID_STRING_CHAR;
				}
				//*(char*)lljson_context_push(c, sizeof(char)) = ch;
				PUTC(c, ch); // 除上述情况外，其余情况将字符入栈
		}
	}
}

// 解析JSON中的number类型
static int lljson_parse_number(lljson_context* c, lljson_value* v) {
	// 确认数值是否有效
	const char* p = c->json;
	if (*p == '-') p++;
	if (*p == '0') {
		p++;
		if (ISDIGIT(*p)) return LLJSON_PARSE_INVALID_VALUE; // 诸如0123这种情况
	}
	else {
		if(!ISDIGIT1TO9(*p)) return LLJSON_PARSE_INVALID_VALUE; // 开头非 -/ 0/ 数字
		for (p++; ISDIGIT(*p); p++); // 是数字，循环跳过
	}
	if (*p == '.') {
		p++;
		if (!ISDIGIT(*p)) return LLJSON_PARSE_INVALID_VALUE;
		for (p++; ISDIGIT(*p); p++);
	}
	if (*p == 'e' || *p == 'E') {
		p++;
		if (*p == '-' || *p == '+') p++;
		if (!ISDIGIT(*p)) return LLJSON_PARSE_INVALID_VALUE;
		for (p++; ISDIGIT(*p); p++);
	}
	// 检查数据是否溢出
	errno = 0;
	// 解析数字，若数字过大会设置错误码errno
	v->u.number = strtod(c->json, NULL);
	//printf("%f\n", v->number);
	if (errno == ERANGE || errno == HUGE_VAL || errno == -HUGE_VAL)
		return LLJSON_PARSE_NUMBER_TOO_BIG;
	c->json = p;
	v->type = LLJSON_NUMBER;
	// 使用c函数库中的strtod将字符串类型（const char*）转为double类型
	return LLJSON_PARSE_OK;
}

// 解析数组类型（JSON中的一种复合类型）
/*
*	注意：lljson_parse_value中调用了lljson_parse_array,
*		lljson_parse_array中也调用了lljson_parse_value（互相调用）
*/
static int lljson_parse_array(lljson_context* c, lljson_value* v) {
	int ret;
	size_t size = 0; // 记录被解析数组中元素的个数
	//printf("%s\n", c->json);
	EXPECT(c, '['); // 左括号
	lljson_parse_whitespace(c); // /* [   "abc",[1,2],3] */
	if (*c->json == ']') { // 右括号，空数组的情况 -> 也属于解析成功
		c->json++; // 指针后移，因为此次可能解析的是内层数组，外层数组可能还有其他元素要解析
		//lljson_parse_whitespace(c); /* [[]  ,"abc",[1,2],3] */
		v->type = LLJSON_ARRAY;
		v->u.array.size = 0;
		v->u.array.e = NULL;
		return LLJSON_PARSE_OK;
	}
	while (1) {
		/* TODO */
		lljson_value e; // 临时变量存储解析出来的元素（array/string/number类型）
		e.type = LLJSON_NULL;
		if ((ret = lljson_parse_value(c, &e)) != LLJSON_PARSE_OK) { // 解析错误
			//lljson_free(v); // 解析有误时，先释放堆栈中已存储的临时值，再返回错误码
			//return ret;
			break; // 不能直接返回，要free后return，防止内存泄漏
		}
		// 解析成功一个元素就入栈，并将size++
		memcpy(lljson_context_push(c, sizeof(lljson_value)), &e, sizeof(lljson_value));
		size++; // 数组元素个数统计/栈内元素个数+1
		//printf("%Iu\n", size);
		lljson_parse_whitespace(c); /* ["abc"  ,[1,2],3] */
		if (*c->json == ',') { 
			c->json++; // 跳过逗号
			lljson_parse_whitespace(c); /* ["abc",  [1,2],3] */
		}
		else if (*c->json == ']') { // 右括号，解析正常结束
			// +1的原因是本次解析的可能是数组中的数组，后面若还有元素则仍需继续解析
			c->json++;
			v->type = LLJSON_ARRAY;
			v->u.array.size = size; // 数组中的元素个数
			// 下列中的size均表示size个字节
			size *= sizeof(lljson_value); // 数组中所有元素占用的字节大小
			//printf("%Iu\n", size);
			v->u.array.e = (lljson_value*)malloc(size); // 为数组分配大小为size字节的内存
			memcpy(v->u.array.e, lljson_context_pop(c, size), size); // 将解析的结果复制给v保存
			return LLJSON_PARSE_OK;
			/*
			* 将return LLJSON_PARSE_OK;替换为下列两行代码会引发错误！
			*	ret = LLJSON_PARSE_OK; // 数组解析成功
			*	break;
			* 原因是：
				替换之后，break跳出while循环后，由于在循环结束后有内存释放语句，
				这将导致之前分配的内存被释放，从而破坏解析结果。
			* 
			*/
		}else {
			ret = LLJSON_PARSE_MISS_COMMA_OR_SQUARE_BRACKET;
			break;
		}
	}
	/* 解析错误时的内存处理 */
	// 注意下列的size仍为栈中元素的个数
	for (size_t i = 0; i < size; i++) {
		printf("%Iu\n", size);
		// 释放从堆栈弹出的值
		// 因为栈中的每个元素都是lljson_value类型，所以得逐个释放（类似于二维指针数组的释放）
		lljson_free((lljson_value*)lljson_context_pop(c, sizeof(lljson_value)));
	}
	return ret;
}

// 解析对象类型（JSON中的另一种复合类型）
static int lljson_parse_object(lljson_context* c, lljson_value* v) {
	size_t size = 0; // 记录对象中成员的个数
	int ret; // 记录解析结果
	EXPECT(c, '{');
	lljson_parse_whitespace(c);
	if (*c->json == '}') { // 正常结束
		c->json++;
		v->type = LLJSON_OBJECT;
		v->u.object.size = 0;
		v->u.object.m = NULL;
		return LLJSON_PARSE_OK;
	}
	while (1) {
		lljson_object_member m; // 临时存储解析出的成员
		m.key.k = NULL;
		m.value.type = LLJSON_NULL;
		/* TODO */


		if ((ret = (lljson_parse_value(c, &m.value))) != LLJSON_PARSE_OK) { // 解析第一个成员的value
			// 解析失败，先break，释放内存后再返回
			break;
		}
		// 解析成功一个就存入堆栈中
		memcpy(lljson_context_push(c, sizeof(lljson_object_member)), &m, sizeof(lljson_object_member));
		size++; // 成员个数+1
	}
	// 解析失败时释放内存
	/* TODO */

	return ret;
}

// 继续解析处理完空白符后的JSON字符串，并返回解析结果
static int lljson_parse_value(lljson_context* c, lljson_value* v) {
	//return lljson_parse_true(c, v);
	//printf("test:%s\n", c->json);
	switch (*c->json) {
		case 'n': return lljson_parse_literal(c, v, "null", LLJSON_NULL);
		case 't': return lljson_parse_literal(c, v, "true", LLJSON_TRUE);
		case 'f': return lljson_parse_literal(c, v, "false", LLJSON_FALSE);
		case '\"': return lljson_parse_string(c, v); // 解析字符串类型
		case '[': return lljson_parse_array(c, v); // 解析数组类型
		case '{': return lljson_parse_object(c, v); // 解析对象类型
		case '\0': return LLJSON_PARSE_EXPECT_VALUE;
		// 数值的情况（且lljson_parse_number函数负责处理所有非法输入）
		default: return lljson_parse_number(c, v); 
	}
}

// JSON解析函数
int lljson_parse(lljson_value* v, const char* json) {
	assert(v != NULL); // 节点不能为空
	int ret; 
	lljson_context c;
	c.json = json;
	//printf("%s\n", c.json);
	// 在创建lljson_context时初始化stack，并最终释放内存
	c.stack = NULL;
	c.size = c.top = 0;
	v->type = LLJSON_NULL; // 若解析失败，会将v->type设为null类型，所以将它先设为null
	lljson_parse_whitespace(&c); // 处理空白符
	// 多加一层判断处理root_not_singular情况
	if ((ret = lljson_parse_value(&c, v)) == LLJSON_PARSE_OK) {
		lljson_parse_whitespace(&c);
		//printf("yes\n");
		if (*c.json != '\0') { // 注意此处为*c.json，因为c.json指向的是一个地址
			//v->type = LLJSON_NULL; 
			if(v->type != LLJSON_STRING) 
				ret = LLJSON_PARSE_ROOT_NOT_SINGULAR;
		}
	}
	// 加入断言确保堆栈中的所有数据都被弹出
	assert(c.top == 0);
	//printf("running\n");
	// 注意记得释放堆栈指向的内存空间
	free(c.stack);
	return ret;
}

void lljson_free(lljson_value* v) {
	assert(v != NULL);
	/*if (v->type == LLJSON_STRING)
		free(v->u.string.s);*/
	switch (v->type) {
		case LLJSON_STRING:
			free(v->u.string.s);
			break;
		case LLJSON_ARRAY:
			for (size_t i = 0; i < v->u.array.size; i++) { // array相当于一个多维数组（二维及以上）
				lljson_free(&v->u.array.e[i]); // 因为数组中元素的类型可能不同，因此使用lljson_free释放
			}
			free(v->u.array.e);
			break;
		default:
			break;
	}
	// 将v->type设置为null类型，避免重复释放引起未定义行为
	v->type = LLJSON_NULL;
}

// 获取节点的JSON类型
lljson_type lljson_get_type(const lljson_value* v) {
	assert(v != NULL);
	//printf("%d\n", v->type);
	return v->type;
}

// set null
//void lljson_set_null(lljson_value* v) {
//	lljson_free(v);
//}

// set数字
void lljson_set_number(lljson_value* v, double n) {
	lljson_free(v); // v之前可能指向其他内存空间，因此先释放一下
	v->u.number = n;
	v->type = LLJSON_NUMBER;
}

// 获取JSON数组的数值
double lljson_get_number(const lljson_value* v) {
	assert(v != NULL && v->type == LLJSON_NUMBER);
	//printf("%f", v->number);
	return v->u.number;
}

// set字符串
void lljson_set_string(lljson_value* v, const char* s, size_t len) {
	// 非空字符串（s != NULL）或是长度为0的字符串（len = 0）都是合法的
	assert(v != NULL && (s != NULL || len == 0));
	// 设置v之前先调用lljson_free释放其可能指向的内存，如其原来可能已指向一个字符串
	lljson_free(v);
	// 先使用malloc为v的string的指针分配内存
	// 再使用memcpy将s指向的内容复制给v->u.string.s
	v->u.string.s = (char*)malloc(len + 1); // +1的原因是结尾的空字符 \0
	memcpy(v->u.string.s, s, len);

	v->u.string.s[len] = '\0'; // 将字符串的结尾置为空字符
	v->u.string.len = len;
	//printf("set len: %Iu\n", len);
	v->type = LLJSON_STRING;
	//printf("Running\n");

	/*printf("%s\n", v->u.string.s[len]);
	printf("%s\n", '\0');
	printf("");*/
}

// get字符串
const char* lljson_get_string(lljson_value* v) {
	assert(v != NULL && v->type == LLJSON_STRING);
	/*printf("get string: %s\n", v->u.string.s);
	printf("get length: %Iu\n", lljson_get_string_length(v));
	printf("get size: %Iu\n", sizeof(v->u.string.s));*/
	return v->u.string.s;
}

// 获取字符串长度
size_t lljson_get_string_length(lljson_value* v) {
	//printf("%d\n", v->type);
	assert(v != NULL && v->type == LLJSON_STRING);
	assert(v != NULL);
	//printf("get len :%Iu\n", v->u.string.len);
	return v->u.string.len;
}

// set布尔值
void lljson_set_boolean(lljson_value* v, int b) {
	lljson_free(v); // 先释放v可能指向的内存空间
	v->type = b ? LLJSON_TRUE : LLJSON_FALSE;
}

// get布尔值
int lljson_get_boolean(const lljson_value* v) {
	assert(v != NULL && (v->type == LLJSON_TRUE || v->type == LLJSON_FALSE));
	return v->type == LLJSON_TRUE; // 将boolean类型的返回值转换位0或1
}

// get数组大小
size_t lljson_get_array_size(const lljson_value* v) {
	assert(v != NULL && v->type == LLJSON_ARRAY);
	return v->u.array.size;
}

// get数组中下标index处的元素
lljson_value* lljson_get_array_element(const lljson_value* v, size_t index) {
	assert(v != NULL && v->type == LLJSON_ARRAY && index < v->u.array.size);
	return &v->u.array.e[index];
}

// get对象中的成员个数
size_t lljson_get_object_size(const lljson_value* v)
{
	assert(v != NULL && v->type == LLJSON_OBJECT);
	return v->u.object.size;
}

// get对象中下标index处成员的键key -> 字符串类型
const char* lljson_get_object_key(const lljson_value* v, size_t index) {
	assert(v != NULL && v->type == LLJSON_OBJECT && index < v->u.object.size);
	return v->u.object.m[index].key.k;
}

// get对象中下标index处成员的键key长度
size_t lljson_get_object_key_length(const lljson_value* v, size_t index) {
	assert(v != NULL && v->type == LLJSON_OBJECT && index < v->u.object.size);
	return v->u.object.m[index].key.len;
}

// get对象中下标index处成员的值value
lljson_value* lljson_get_object_value(const lljson_value* v, size_t index) {
	assert(v != NULL && v->type == LLJSON_OBJECT && index < v->u.object.size);
	return &v->u.object.m[index].value;
}
