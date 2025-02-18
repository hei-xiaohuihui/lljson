/*
*	windows下的内存泄漏测试—— C Runtime Library(CRT)
*/
#ifdef _WINDOWS
#define CRTDBG_MAO_ALLOC
#include <crtdbg.h>
#endif // _WINDOWS

/*
	单元测试框架
*/
#include <stdio.h>
#include <stdlib.h>
#include "lljson.h"

static int main_ret = 0; // 程序运行结束后的返回值，0表示正常，1表示有错误
static int test_count = 0; // 进行单元测试的次数
static int test_pass = 0; // 单元测试通过的次数

// 反斜杠代表该行未结束
// 如果宏里有多于一个语句，就需要使用do{ /*...*/}while(0)包裹成单个语句
#define EXPECT_EQ_BASE(equality, expect, actual, format) \
	do { \
		test_count++; \
		if(equality)\
			test_pass++;\
		else{\
			fprintf(stderr, "%s:%d: expect: " format " actual: " format "\n", \
				__FILE__, __LINE__, expect, actual);  \
			main_ret = 1; \
		}\
	}while(0) 

#define EXPECT_EQ_INT(expect, actual) \
	EXPECT_EQ_BASE((expect)==(actual), expect, actual, "%d")

#define EXPECT_EQ_DOUBLE(expect, actual) \
	EXPECT_EQ_BASE((expect)==(actual), expect, actual, "%.17g") // g表示输出实数

#define EXPECT_EQ_STRING(expect, actual, length) \
	EXPECT_EQ_BASE(sizeof(expect) - 1 == length && memcmp(expect, actual, length) == 0, expect, actual, "%s")

#define EXPECT_TRUE(actual) \
	EXPECT_EQ_BASE(actual != 0, "true", "false", "%s")

#define EXPECT_FALSE(actual) \
	EXPECT_EQ_BASE(actual == 0, "false", "true", "%s")

#if defined(_MSC_VER) // 不同编译器输出size_t的格式有所不同
#define EXPECT_EQ_SIZE_T(expect, actual) EXPECT_EQ_BASE(expect == actual, (size_t)expect, (size_t)actual, "%Iu")
#else
#define EXPECT_EQ_SIZE_T(expect, actual) EXPECT_EQ_BASE(expect == actual, (size_t)expect, (size_t)actual, "%zu")
#endif

// null解析单元测试
static void test_parse_null() { // 声明为静态函数，使其只能在定义其的文件内访问
	lljson_value v;
	v.type = LLJSON_FALSE;
	// 使用解析函数解析null类型，解析成功会将v.type置为null类型
	EXPECT_EQ_INT(LLJSON_PARSE_OK, lljson_parse(&v, "null"));
	// 判断解析后的v.type是否为null类型
	EXPECT_EQ_INT(LLJSON_NULL, lljson_get_type(&v));
	lljson_free(&v);
}

// true解析单元测试
static void test_parse_true() { // 
	lljson_value v;
	v.type = LLJSON_FALSE;
	EXPECT_EQ_INT(LLJSON_PARSE_OK, lljson_parse(&v, "true"));
	EXPECT_EQ_INT(LLJSON_TRUE, lljson_get_type(&v));
	lljson_free(&v);
}

// false解析单元测试
static void test_parse_false() {
	lljson_value v;
	v.type = LLJSON_FALSE;
	EXPECT_EQ_INT(LLJSON_PARSE_OK, lljson_parse(&v, "false"));
	EXPECT_EQ_INT(LLJSON_FALSE, lljson_get_type(&v));
	lljson_free(&v);
}

//// root_not_singular解析单元测试
//static void test_parse_root_not_singular() {
//	lljson_value v;
//	v.type = LLJSON_FALSE;
//	// 测试能否正确解析root_not_singular这种情况
//	EXPECT_EQ_INT(LLJSON_PARSE_ROOT_NOT_SINGULAR, lljson_parse(&v, "null ab"));
//	// 对于root_not_singular这种情况v->type应该为null类型（错误默认置为null类型）
//	EXPECT_EQ_INT(LLJSON_NULL, lljson_get_type(&v));
//}

/*
	因为number测试单元测试案例较多，因为使用宏来减少重复的代码
*/
#define TEST_NUMBER(expect, json) \
	do { \
		lljson_value v;\
		EXPECT_EQ_INT(LLJSON_PARSE_OK, lljson_parse(&v, json)); \
		EXPECT_EQ_INT(LLJSON_NUMBER, lljson_get_type(&v)); \
		EXPECT_EQ_DOUBLE(expect, lljson_get_number(&v)); \
		lljson_free(&v); \
	} while (0)

// 合法数值输入测试单元
static void test_parse_number() {
	TEST_NUMBER(0.0, "0");
	TEST_NUMBER(0.0, "-0");
	TEST_NUMBER(0.0, "-0.0");

	TEST_NUMBER(1.0, "1");
	TEST_NUMBER(-1.0, "-1");
	TEST_NUMBER(1.5, "1.5");
	TEST_NUMBER(-1.5, "-1.5");
	TEST_NUMBER(3.1416, "3.1416");
	TEST_NUMBER(1E10, "1E10");
	TEST_NUMBER(1e10, "1e10");
	TEST_NUMBER(1E+10, "1E+10");
	TEST_NUMBER(1E-10, "1E-10");
	TEST_NUMBER(-1E10, "-1E10");
	TEST_NUMBER(-1e10, "-1e10");
	TEST_NUMBER(-1E+10, "-1E+10");
	TEST_NUMBER(-1E-10, "-1E-10");
	TEST_NUMBER(1.234E+10, "1.234E+10");
	TEST_NUMBER(1.234E-10, "1.234E-10");
	//TEST_NUMBER(0.0, "1e-10000"); /* must underflow */
}

/*
	不合法用例测试
*/
// 使用宏定义减少重复代码
#define TEST_ERROR(error, json) \
	do { \
		lljson_value v; \
		v.type = LLJSON_FALSE; \
		EXPECT_EQ_INT(error, lljson_parse(&v, json)); \
		EXPECT_EQ_INT(LLJSON_NULL, lljson_get_type(&v)); \
		lljson_free(&v); \
	} while (0)

// 在一个值的空白之后还有其他字符（root_not_singualr） 测试单元
// 因为root_not_singular测试单元与TEST_ERROR中代码重复，因此重写
static void test_parse_root_not_singular() {
	TEST_ERROR(LLJSON_PARSE_ROOT_NOT_SINGULAR, "null ab");
	/* after 0 should be  '.' 'e' 'E' or nothing */
	TEST_ERROR(LLJSON_PARSE_INVALID_VALUE, "0123");
	/*TEST_ERROR(LLJSON_PARSE_ROOT_NOT_SINGULAR, "0x0");
	TEST_ERROR(LLJSON_PARSE_ROOT_NOT_SINGULAR, "0x123");*/
}

// 非法输入测试单元
// 以下均为非法输入
static void test_parse_invalid_value() {
	// 非法字符串
	TEST_ERROR(LLJSON_PARSE_INVALID_VALUE, "nul");
	TEST_ERROR(LLJSON_PARSE_INVALID_VALUE, "?");

	// 非法数字
	TEST_ERROR(LLJSON_PARSE_INVALID_VALUE, "+0");
	TEST_ERROR(LLJSON_PARSE_INVALID_VALUE, "+1");
	TEST_ERROR(LLJSON_PARSE_INVALID_VALUE, ".456");
	TEST_ERROR(LLJSON_PARSE_INVALID_VALUE, "1.");
	TEST_ERROR(LLJSON_PARSE_INVALID_VALUE, "INF");
	TEST_ERROR(LLJSON_PARSE_INVALID_VALUE, "inf");
	TEST_ERROR(LLJSON_PARSE_INVALID_VALUE, "NAN");
	TEST_ERROR(LLJSON_PARSE_INVALID_VALUE, "nan");
}

// 数值溢出测试单元
static void test_parse_number_too_big() {
	TEST_ERROR(LLJSON_PARSE_NUMBER_TOO_BIG, "1e309");
	TEST_ERROR(LLJSON_PARSE_NUMBER_TOO_BIG, "-1e309");
}

static void test_parse_expect_value() {
	TEST_ERROR(LLJSON_PARSE_EXPECT_VALUE, "");
	TEST_ERROR(LLJSON_PARSE_EXPECT_VALUE, " ");
}

/*
*	string类型解析测试单元
*/ 
#define TEST_STRING(expect, json) \
	do { \
		lljson_value v; \
		v.type = LLJSON_NULL; \
		EXPECT_EQ_INT(LLJSON_PARSE_OK, lljson_parse(&v, json)); \
		EXPECT_EQ_INT(LLJSON_STRING, lljson_get_type(&v));\
		EXPECT_EQ_STRING(expect, lljson_get_string(&v), lljson_get_string_length(&v));\
		lljson_free(&v); \
	} while(0)

// 合法string类型测试单元
static void test_parse_string() {
	TEST_STRING("", "\"\"");
	TEST_STRING("hello", "\"hello\"");
	TEST_STRING("Hello\nWorld", "\"Hello\\nWorld\""); // "Hello\nWorld"
	TEST_STRING("\" \\ \/ \b \r \f \n \t", "\"\\\" \\\\ \\/ \\b \\r \\f \\n \\t\"");
	TEST_STRING("\\", "\"\\\\\"");

	/* /uXXXX测试单元（Unicode转utf-8测试单元） */
	TEST_STRING("\x24", "\"\\u0024\"");
	TEST_STRING("\xC2\xA2", "\"\\u00A2\"");
	TEST_STRING("\xE2\x82\xAC", "\"\\u20AC\"");
}

// string类型缺少引号测试单元
static void test_parse_missing_quotation_mark() {
	TEST_ERROR(LLJSON_PARSE_MISS_QUOTATION_MARK, "\"");
	TEST_ERROR(LLJSON_PARSE_MISS_QUOTATION_MARK, "\"abc");
}

// 无效转义字符测试单元
static void test_parse_invalid_string_escape() {
	TEST_ERROR(LLJSON_PARSE_INVALID_STRING_ESCAPE, "\"\\v\""); 
	TEST_ERROR(LLJSON_PARSE_INVALID_STRING_ESCAPE, "\"\\a\"");
}

// 无效字符串字符测试单元
static void test_parse_invalid_string_char() {
	TEST_ERROR(LLJSON_PARSE_INVALID_STRING_CHAR, "\"hello\x01world\""); // 非法控制符 \x01
	TEST_ERROR(LLJSON_PARSE_INVALID_STRING_CHAR, "\"\x1F\"");
}

// 非法的Unicode(\uXXXX)十六进制数测试单元
static void test_parse_invalid_unicode_hex() {
	TEST_ERROR(LLJSON_PARSE_INVALID_UNICODE_HEX, "\"\\u\"");
	TEST_ERROR(LLJSON_PARSE_INVALID_UNICODE_HEX, "\"\\u0\"");
	TEST_ERROR(LLJSON_PARSE_INVALID_UNICODE_HEX, "\"\\u01\"");
	TEST_ERROR(LLJSON_PARSE_INVALID_UNICODE_HEX, "\"\\u012\"");
	TEST_ERROR(LLJSON_PARSE_INVALID_UNICODE_HEX, "\"\\uG012\"");
	TEST_ERROR(LLJSON_PARSE_INVALID_UNICODE_HEX, "\"\\u 123\"");
}

// 有效/合法的代理对测试单元
static void test_parse_unicode_surrogate() {
	TEST_STRING("\xF0\x9D\x84\x9E", "\"\\uD834\\uDD1E\"");
	TEST_STRING("\xF0\x9D\x84\x9E", "\"\\ud834\\udd1e\"");
}

// 无效的代理对测试单元
static void test_parse_invalid_unicode_surrogate() {
	TEST_ERROR(LLJSON_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\"");
	TEST_ERROR(LLJSON_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uDBFF\"");
	TEST_ERROR(LLJSON_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\\\\\"");
	TEST_ERROR(LLJSON_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\\uDBFF\"");
	TEST_ERROR(LLJSON_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\\uE000\"");
}

// array类型测试单元
static void test_parse_array() {
	lljson_value v;
	v.type = LLJSON_NULL;
	EXPECT_EQ_INT(LLJSON_PARSE_OK, lljson_parse(&v, "[ ]"));
	EXPECT_EQ_INT(LLJSON_ARRAY, lljson_get_type(&v));
	EXPECT_EQ_SIZE_T(0, lljson_get_array_size(&v));
	lljson_free(&v);

	v.type = LLJSON_NULL;
	EXPECT_EQ_INT(LLJSON_PARSE_OK, lljson_parse(&v, "[ null , false , true , 123 , \"abc\" ]")); 
	EXPECT_EQ_INT(LLJSON_ARRAY, lljson_get_type(&v));
	EXPECT_EQ_SIZE_T(5, lljson_get_array_size(&v));

	EXPECT_EQ_INT(LLJSON_NULL, lljson_get_type(lljson_get_array_element(&v, 0)));
	EXPECT_EQ_INT(LLJSON_FALSE, lljson_get_type(lljson_get_array_element(&v, 1)));
	EXPECT_EQ_INT(LLJSON_TRUE, lljson_get_type(lljson_get_array_element(&v, 2)));
	EXPECT_EQ_INT(LLJSON_NUMBER, lljson_get_type(lljson_get_array_element(&v, 3)));
	EXPECT_EQ_INT(LLJSON_STRING, lljson_get_type(lljson_get_array_element(&v, 4)));
	EXPECT_EQ_DOUBLE(123.0, lljson_get_number(lljson_get_array_element(&v, 3)));
	EXPECT_EQ_STRING("abc", lljson_get_string(lljson_get_array_element(&v, 4)), lljson_get_string_length(lljson_get_array_element(&v, 4))); // 之前报错，突然正常了
	lljson_free(&v);

	v.type = LLJSON_NULL;
	EXPECT_EQ_INT(LLJSON_PARSE_OK, lljson_parse(&v, "[ [ ] , [ 0 ] , [ 0 , 1 ] , [ 0 , 1 , 2 ] ]"));
	EXPECT_EQ_INT(LLJSON_ARRAY, lljson_get_type(&v));
	EXPECT_EQ_SIZE_T(4, lljson_get_array_size(&v));
	for (size_t i = 0; i < 4; i++) {
		lljson_value* a = lljson_get_array_element(&v, i);
		EXPECT_EQ_INT(LLJSON_ARRAY, lljson_get_type(a));
		EXPECT_EQ_SIZE_T(i, lljson_get_array_size(a));
		for (size_t j = 0; j < i; j++) {
			lljson_value* e = lljson_get_array_element(a, j);
			EXPECT_EQ_INT(LLJSON_NUMBER, lljson_get_type(e));
			EXPECT_EQ_DOUBLE((double)j, lljson_get_number(e));
		}
	}
	lljson_free(&v);
}

// 缺少逗号或方括号（数组）测试单元
static void test_parse_miss_comma_or_square_bracket() {
	TEST_ERROR(LLJSON_PARSE_MISS_COMMA_OR_SQUARE_BRACKET, "[1");
	TEST_ERROR(LLJSON_PARSE_MISS_COMMA_OR_SQUARE_BRACKET, "[1}");
	TEST_ERROR(LLJSON_PARSE_MISS_COMMA_OR_SQUARE_BRACKET, "[1 2");
	TEST_ERROR(LLJSON_PARSE_MISS_COMMA_OR_SQUARE_BRACKET, "[[]");
}

// object类型测试单元
static void test_parse_object() {
	lljson_value v;

	v.type = LLJSON_NULL;
	EXPECT_EQ_INT(LLJSON_PARSE_OK, lljson_parse(&v, " { } "));
	EXPECT_EQ_INT(LLJSON_OBJECT, lljson_get_type(&v));
	EXPECT_EQ_SIZE_T(0, lljson_get_object_size(&v));
	lljson_free(&v);

	v.type = LLJSON_NULL;
	EXPECT_EQ_INT(LLJSON_PARSE_OK, lljson_parse(&v, 
		" { " 
			"\"n\" : null "								
		" } "
	));
	EXPECT_EQ_INT(LLJSON_OBJECT, lljson_get_type(&v));
	EXPECT_EQ_SIZE_T(1, lljson_get_object_size(&v));
	lljson_free(&v);

	/*
		{ "n" : null , "f" : false , "t" : true , "i" : 123 , "s" : "abc", "a" : [ 1, 2, 3 ],"o" : { "1" : 1, "2" : 2, "3" : 3 } }
	*/
	v.type = LLJSON_NULL;
	EXPECT_EQ_INT(LLJSON_PARSE_OK, lljson_parse(&v, 
		" { "
			"\"n\" : null , "
			"\"f\" : false , "
			"\"t\" : true , "
			"\"i\" : 123 , "
			"\"s\" : \"abc\", "
			"\"a\" : [ 1, 2, 3 ],"
			"\"o\" : { \"1\" : 1, \"2\" : 2, \"3\" : 3 }"
		" } "
	));
	EXPECT_EQ_INT(LLJSON_OBJECT, lljson_get_type(&v));
	EXPECT_EQ_SIZE_T(7, lljson_get_object_size(&v));
	// 第0个键值对 -> "n" : null
	EXPECT_EQ_STRING("n", lljson_get_object_key(&v, 0), lljson_get_object_key_length(&v, 0));
	EXPECT_EQ_INT(LLJSON_NULL,  lljson_get_type(lljson_get_object_value(&v, 0)));
	// 第1个键值对 -> "f" : false
	EXPECT_EQ_STRING("f", lljson_get_object_key(&v, 1), lljson_get_object_key_length(&v, 1));
	EXPECT_EQ_INT(LLJSON_FALSE, lljson_get_type(lljson_get_object_value(&v, 1)));
	// 第2个键值对 -> "t" : true
	EXPECT_EQ_STRING("t", lljson_get_object_key(&v, 2), lljson_get_object_key_length(&v, 2));
	EXPECT_EQ_INT(LLJSON_TRUE, lljson_get_type(lljson_get_object_value(&v, 2)));
	// 第3个键值对 -> "i" : 123
	EXPECT_EQ_STRING("i", lljson_get_object_key(&v, 3), lljson_get_object_key_length(&v, 3));
	EXPECT_EQ_INT(LLJSON_NUMBER, lljson_get_type(lljson_get_object_value(&v, 3)));
	EXPECT_EQ_DOUBLE(123.0, lljson_get_number(lljson_get_object_value(&v, 3)));
	// 第4个键值对 ->  "s" : "abc"
	EXPECT_EQ_STRING("s", lljson_get_object_key(&v, 4), lljson_get_object_key_length(&v, 4));
	EXPECT_EQ_INT(LLJSON_STRING, lljson_get_type(lljson_get_object_value(&v, 4)));
	EXPECT_EQ_STRING("abc", lljson_get_string(lljson_get_object_value(&v, 4)), lljson_get_string_length(lljson_get_object_value(&v, 4)));
	// 第5个键值对 -> "a" : [ 1, 2, 3 ]
	EXPECT_EQ_STRING("a", lljson_get_object_key(&v, 5), lljson_get_object_key_length(&v, 5));
	EXPECT_EQ_INT(LLJSON_ARRAY, lljson_get_type(lljson_get_object_value(&v, 5)));
	EXPECT_EQ_SIZE_T(3, lljson_get_array_size(lljson_get_object_value(&v, 5)));
	for (size_t i = 0; i < 3; i++) {
		lljson_value* e = lljson_get_array_element(lljson_get_object_value(&v, 5), i);
		EXPECT_EQ_INT(LLJSON_NUMBER, lljson_get_type(e));
		EXPECT_EQ_DOUBLE(i + 1.0, lljson_get_number(e));
	}
	// 第6个键值对 -> "o" : { "1" : 1, "2" : 2, "3" : 3 }
	EXPECT_EQ_STRING("o", lljson_get_object_key(&v, 6), lljson_get_object_key_length(&v, 6)); // key
	{
		lljson_value* o = lljson_get_object_value(&v, 6); // 又是一个object类型
		EXPECT_EQ_INT(LLJSON_OBJECT, lljson_get_type(o));
		for (size_t i = 0; i < 3; i++) { // 循环3个键值对
			// i + '1'得到字符类型，lljson_get_object_key(o, i)[0]加上[0]是因为其返回值为字符串类型
			EXPECT_TRUE(i + '1' == lljson_get_object_key(o, i)[0]);
			EXPECT_EQ_SIZE_T(1, lljson_get_object_key_length(o, i));
			lljson_value* ov = lljson_get_object_value(o, i); // value
			EXPECT_EQ_INT(LLJSON_NUMBER, lljson_get_type(ov));
			EXPECT_EQ_DOUBLE(i + 1.0, lljson_get_number(ov));
		}
	}
	lljson_free(&v);
}

// 缺少逗号或花括号（对象）测试单元
static void test_parse_miss_comma_or_curly_bracket() {
	TEST_ERROR(LLJSON_PARSE_MISS_COMMA_OR_CURLY_BRACKET, "{\"a\":1");
	TEST_ERROR(LLJSON_PARSE_MISS_COMMA_OR_CURLY_BRACKET, "{\"a\":1]");
	TEST_ERROR(LLJSON_PARSE_MISS_COMMA_OR_CURLY_BRACKET, "{\"a\":1 \"b\"");
	TEST_ERROR(LLJSON_PARSE_MISS_COMMA_OR_CURLY_BRACKET, "{\"a\":{}");
	TEST_ERROR(LLJSON_PARSE_MISS_COMMA_OR_CURLY_BRACKET, "{\"a\":{} \"b\":{}");
}

// 缺少键key
static void test_parse_miss_key() {
	TEST_ERROR(LLJSON_PARSE_MISS_KEY, "{:1,");
	TEST_ERROR(LLJSON_PARSE_MISS_KEY, "{1:1,");
	TEST_ERROR(LLJSON_PARSE_MISS_KEY, "{true:1,");
	TEST_ERROR(LLJSON_PARSE_MISS_KEY, "{false:1,");
	TEST_ERROR(LLJSON_PARSE_MISS_KEY, "{null:1,");
	TEST_ERROR(LLJSON_PARSE_MISS_KEY, "{[]:1,");
	TEST_ERROR(LLJSON_PARSE_MISS_KEY, "{{}:1,");
	TEST_ERROR(LLJSON_PARSE_MISS_KEY, "{\"a\":1,");
}

// 缺少冒号
static void test_parse_miss_colon() {
	TEST_ERROR(LLJSON_PARSE_MISS_COLON, "{\"a\"}");
	TEST_ERROR(LLJSON_PARSE_MISS_COLON, "{\"a\",\"b\"}");
}

/*
*	访问测试单元
*/
static void test_access_null() {
	lljson_value v;
	v.type = LLJSON_NULL;
	// 先将v设置为其他类型以测试再次设置v时是否调用lljson_free释放其原先指向的内存
	lljson_set_string(&v, "a", 1);
	lljson_set_null(&v);
	EXPECT_EQ_INT(LLJSON_NULL, lljson_get_type(&v));
	lljson_free(&v);
}

/*
*	JSON生成器测试单元/字符串化（stringify）测试单元
*/
#define TEST_ROUNDTRIP(json) \
	do { \
		lljson_value v; \
		v.type = LLJSON_NULL; \
		size_t length; \
		char* json2; \
		EXPECT_EQ_INT(LLJSON_PARSE_OK, lljson_parse(&v, json)); \
		json2 = lljson_stringify(&v, &length); \
		EXPECT_EQ_STRING(json, json2, length); \
		lljson_free(&v); \
		free(json2); \
	} while(0)

// number
static void test_stringify_number() {
	TEST_ROUNDTRIP("0");
	TEST_ROUNDTRIP("-0");
	TEST_ROUNDTRIP("1");
	TEST_ROUNDTRIP("-1");
	TEST_ROUNDTRIP("1.5");
	TEST_ROUNDTRIP("-1.5");
	TEST_ROUNDTRIP("3.25");
	TEST_ROUNDTRIP("1e+20");
	TEST_ROUNDTRIP("1.234e+20");
	TEST_ROUNDTRIP("1.234e-20");

	TEST_ROUNDTRIP("1.0000000000000002"); /* the smallest number > 1 */
	TEST_ROUNDTRIP("4.9406564584124654e-324"); /* minimum denormal */
	TEST_ROUNDTRIP("-4.9406564584124654e-324");
	TEST_ROUNDTRIP("2.2250738585072009e-308");  /* Max subnormal double */
	TEST_ROUNDTRIP("-2.2250738585072009e-308");
	TEST_ROUNDTRIP("2.2250738585072014e-308");  /* Min normal positive double */
	TEST_ROUNDTRIP("-2.2250738585072014e-308");
	TEST_ROUNDTRIP("1.7976931348623157e+308");  /* Max double */
	TEST_ROUNDTRIP("-1.7976931348623157e+308");
}

// string
static void test_stringify_string() {
	TEST_ROUNDTRIP("\"\"");
	TEST_ROUNDTRIP("\"Hello\"");
	TEST_ROUNDTRIP("\"Hello\\nWorld\"");
	TEST_ROUNDTRIP("\"\\\" \\\\ / \\b \\f \\n \\r \\t\"");
	TEST_ROUNDTRIP("\"Hello\\u0000World\"");
}

// array
static void test_stringify_array() {
	TEST_ROUNDTRIP("[]");
	TEST_ROUNDTRIP("[null,false,true,123,\"abc\",[1,2,3]]");
}

// object
static void test_stringify_object() {
	TEST_ROUNDTRIP("{}");
	TEST_ROUNDTRIP("{\"n\":null,\"f\":false,\"t\":true,\"i\":123,\"s\":\"abc\",\"a\":[1,2,3],\"o\":{\"1\":1,\"2\":2,\"3\":3}}");
}

/* 生成器测试单元 */ 
static void test_stringify() {
	TEST_ROUNDTRIP("null");
	TEST_ROUNDTRIP("true");
	TEST_ROUNDTRIP("false");

	test_stringify_number();
    test_stringify_string();
    test_stringify_array();
    test_stringify_object();
}

/*
*	判断两个lljson_value是否相等
*/
#define TEST_EQUAL(json1, json2, equality) \
	do { \
		lljson_value v1, v2; \
		v1.type = LLJSON_NULL; \
		v2.type = LLJSON_NULL; \
		EXPECT_EQ_INT(LLJSON_PARSE_OK, lljson_parse(&v1, json1)); \
		EXPECT_EQ_INT(LLJSON_PARSE_OK, lljson_parse(&v2, json2)); \
		EXPECT_EQ_INT(equality, lljson_is_equal(&v1, &v2)); \
		lljson_free(&v1); \
		lljson_free(&v2); \
	} while(0)

static test_equal() {
	TEST_EQUAL("true", "true", 1);
	TEST_EQUAL("true", "false", 0);
	TEST_EQUAL("false", "false", 1);
	TEST_EQUAL("null", "null", 1);
	TEST_EQUAL("null", "0", 0);
	TEST_EQUAL("123", "123", 1);
	TEST_EQUAL("123", "456", 0);
	TEST_EQUAL("\"abc\"", "\"abc\"", 1);
	TEST_EQUAL("\"abc\"", "\"abcd\"", 0);
	TEST_EQUAL("[]", "[]", 1);
	TEST_EQUAL("[]", "null", 0);
	TEST_EQUAL("[1,2,3]", "[1,2,3]", 1);
	TEST_EQUAL("[1,2,3]", "[1,2,3,4]", 0);
	TEST_EQUAL("[[]]", "[[]]", 1);
	TEST_EQUAL("{}", "{}", 1);
	TEST_EQUAL("{}", "null", 0);
	TEST_EQUAL("{}", "[]", 0);
	TEST_EQUAL("{\"a\":1,\"b\":2}", "{\"a\":1,\"b\":2}", 1);
	TEST_EQUAL("{\"a\":1,\"b\":2}", "{\"b\":2,\"a\":1}", 1);
	TEST_EQUAL("{\"a\":1,\"b\":2}", "{\"a\":1,\"b\":3}", 0);
	TEST_EQUAL("{\"a\":1,\"b\":2}", "{\"a\":1,\"b\":2,\"c\":3}", 0);
	TEST_EQUAL("{\"a\":{\"b\":{\"c\":{}}}}", "{\"a\":{\"b\":{\"c\":{}}}}", 1);
	TEST_EQUAL("{\"a\":{\"b\":{\"c\":{}}}}", "{\"a\":{\"b\":{\"c\":[]}}}", 0);
}

// set null访问测试
static void test_access_boolean() {
	lljson_value v;
	v.type = LLJSON_NULL;
	// 先将v设置为其他类型以测试再次设置v时是否调用lljson_free释放其原先指向的内存
	lljson_set_string(&v, "a", 1);
	lljson_set_boolean(&v, 1);
	EXPECT_TRUE(lljson_get_boolean(&v));
	lljson_set_boolean(&v, 0);
	EXPECT_FALSE(lljson_get_boolean(&v));
	lljson_free(&v);
}

// set number访问测试
static void test_access_number() {
	lljson_value v;
	v.type = LLJSON_NULL;
	// 先将v设置为其他类型以测试是否调用lljson_free释放原先指向的内存
	lljson_set_boolean(&v, 0);
	lljson_set_number(&v, 123456.1);
	EXPECT_EQ_DOUBLE(123456.1, lljson_get_number(&v));
	lljson_free(&v);
}

// set string访问测试
static void test_access_string() {
	lljson_value v;
	v.type = LLJSON_NULL;
	lljson_set_string(&v, "", 0);
	EXPECT_EQ_STRING("", lljson_get_string(&v), lljson_get_string_length(&v));
	lljson_set_string(&v, "hello", 5);
	EXPECT_EQ_STRING("hello", lljson_get_string(&v), lljson_get_string_length(&v));
	lljson_set_string(&v, "abc", 3);
	EXPECT_EQ_STRING("abc", lljson_get_string(&v), lljson_get_string_length(&v));
	lljson_free(&v);
}

// 合法/非法测试单元
static void test_parse() {
	test_parse_null();
	test_parse_true();
	test_parse_false();
	test_parse_root_not_singular();
	test_parse_number(); // 合法数字测试
	test_parse_invalid_value(); // 不合法测试用例
	test_parse_number_too_big(); // 大数测试
	test_parse_expect_value(); // 缺少有效值测试单元

	test_parse_string(); // 合法string类型测试
	test_parse_missing_quotation_mark(); // 缺引号单元测试
	test_parse_invalid_string_escape();
	test_parse_invalid_string_char();

	test_parse_invalid_unicode_hex(); // 非法十六进制表达测试
	test_parse_unicode_surrogate(); // 合法代理对测试
	test_parse_invalid_unicode_surrogate(); // 无效代理对测试

	test_parse_array(); // 解析数组
	test_parse_miss_comma_or_square_bracket();
	
	test_parse_object(); // 解析对象
	test_parse_miss_comma_or_curly_bracket();
	test_parse_miss_key();
	test_parse_miss_colon();

	test_stringify();
	test_equal();
}

// 访问测试
static void test_access() {
	// 访问测试
	test_access_null();
	test_access_boolean();
	test_access_number();
	test_access_string();
}

int main()
{
#ifdef _WINDOWS
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	//_CrtSetBreakAlloc(87);
#endif 
	test_parse();
	//test_access();
	printf("%d/%d (%3.2f%%) passed\n", test_pass, test_count, 
		test_pass * 100.0 / test_count);
	return main_ret;
}