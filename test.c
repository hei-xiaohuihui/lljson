/*
*	windows�µ��ڴ�й©���ԡ��� C Runtime Library(CRT)
*/
#ifdef _WINDOWS
#define CRTDBG_MAO_ALLOC
#include <crtdbg.h>
#endif // _WINDOWS

/*
	��Ԫ���Կ��
*/
#include <stdio.h>
#include <stdlib.h>
#include "lljson.h"

static int main_ret = 0; // �������н�����ķ���ֵ��0��ʾ������1��ʾ�д���
static int test_count = 0; // ���е�Ԫ���ԵĴ���
static int test_pass = 0; // ��Ԫ����ͨ���Ĵ���

// ��б�ܴ������δ����
// ��������ж���һ����䣬����Ҫʹ��do{ /*...*/}while(0)�����ɵ������
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
	EXPECT_EQ_BASE((expect)==(actual), expect, actual, "%.17g") // g��ʾ���ʵ��

#define EXPECT_EQ_STRING(expect, actual, length) \
	EXPECT_EQ_BASE(sizeof(expect) - 1 == length && memcmp(expect, actual, length) == 0, expect, actual, "%s")

#define EXPECT_TRUE(actual) \
	EXPECT_EQ_BASE(actual != 0, "true", "false", "%s")

#define EXPECT_FALSE(actual) \
	EXPECT_EQ_BASE(actual == 0, "false", "true", "%s")

#if defined(_MSC_VER) // ��ͬ���������size_t�ĸ�ʽ������ͬ
#define EXPECT_EQ_SIZE_T(expect, actual) EXPECT_EQ_BASE(expect == actual, (size_t)expect, (size_t)actual, "%Iu")
#else
#define EXPECT_EQ_SIZE_T(expect, actual) EXPECT_EQ_BASE(expect == actual, (size_t)expect, (size_t)actual, "%zu")
#endif

// null������Ԫ����
static void test_parse_null() { // ����Ϊ��̬������ʹ��ֻ���ڶ�������ļ��ڷ���
	lljson_value v;
	v.type = LLJSON_FALSE;
	// ʹ�ý�����������null���ͣ������ɹ��Ὣv.type��Ϊnull����
	EXPECT_EQ_INT(LLJSON_PARSE_OK, lljson_parse(&v, "null"));
	// �жϽ������v.type�Ƿ�Ϊnull����
	EXPECT_EQ_INT(LLJSON_NULL, lljson_get_type(&v));
	lljson_free(&v);
}

// true������Ԫ����
static void test_parse_true() { // 
	lljson_value v;
	v.type = LLJSON_FALSE;
	EXPECT_EQ_INT(LLJSON_PARSE_OK, lljson_parse(&v, "true"));
	EXPECT_EQ_INT(LLJSON_TRUE, lljson_get_type(&v));
	lljson_free(&v);
}

// false������Ԫ����
static void test_parse_false() {
	lljson_value v;
	v.type = LLJSON_FALSE;
	EXPECT_EQ_INT(LLJSON_PARSE_OK, lljson_parse(&v, "false"));
	EXPECT_EQ_INT(LLJSON_FALSE, lljson_get_type(&v));
	lljson_free(&v);
}

//// root_not_singular������Ԫ����
//static void test_parse_root_not_singular() {
//	lljson_value v;
//	v.type = LLJSON_FALSE;
//	// �����ܷ���ȷ����root_not_singular�������
//	EXPECT_EQ_INT(LLJSON_PARSE_ROOT_NOT_SINGULAR, lljson_parse(&v, "null ab"));
//	// ����root_not_singular�������v->typeӦ��Ϊnull���ͣ�����Ĭ����Ϊnull���ͣ�
//	EXPECT_EQ_INT(LLJSON_NULL, lljson_get_type(&v));
//}

/*
	��Ϊnumber���Ե�Ԫ���԰����϶࣬��Ϊʹ�ú��������ظ��Ĵ���
*/
#define TEST_NUMBER(expect, json) \
	do { \
		lljson_value v;\
		EXPECT_EQ_INT(LLJSON_PARSE_OK, lljson_parse(&v, json)); \
		EXPECT_EQ_INT(LLJSON_NUMBER, lljson_get_type(&v)); \
		EXPECT_EQ_DOUBLE(expect, lljson_get_number(&v)); \
		lljson_free(&v); \
	} while (0)

// �Ϸ���ֵ������Ե�Ԫ
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
	���Ϸ���������
*/
// ʹ�ú궨������ظ�����
#define TEST_ERROR(error, json) \
	do { \
		lljson_value v; \
		v.type = LLJSON_FALSE; \
		EXPECT_EQ_INT(error, lljson_parse(&v, json)); \
		EXPECT_EQ_INT(LLJSON_NULL, lljson_get_type(&v)); \
		lljson_free(&v); \
	} while (0)

// ��һ��ֵ�Ŀհ�֮���������ַ���root_not_singualr�� ���Ե�Ԫ
// ��Ϊroot_not_singular���Ե�Ԫ��TEST_ERROR�д����ظ��������д
static void test_parse_root_not_singular() {
	TEST_ERROR(LLJSON_PARSE_ROOT_NOT_SINGULAR, "null ab");
	/* after 0 should be  '.' 'e' 'E' or nothing */
	TEST_ERROR(LLJSON_PARSE_INVALID_VALUE, "0123");
	/*TEST_ERROR(LLJSON_PARSE_ROOT_NOT_SINGULAR, "0x0");
	TEST_ERROR(LLJSON_PARSE_ROOT_NOT_SINGULAR, "0x123");*/
}

// �Ƿ�������Ե�Ԫ
// ���¾�Ϊ�Ƿ�����
static void test_parse_invalid_value() {
	// �Ƿ��ַ���
	TEST_ERROR(LLJSON_PARSE_INVALID_VALUE, "nul");
	TEST_ERROR(LLJSON_PARSE_INVALID_VALUE, "?");

	// �Ƿ�����
	TEST_ERROR(LLJSON_PARSE_INVALID_VALUE, "+0");
	TEST_ERROR(LLJSON_PARSE_INVALID_VALUE, "+1");
	TEST_ERROR(LLJSON_PARSE_INVALID_VALUE, ".456");
	TEST_ERROR(LLJSON_PARSE_INVALID_VALUE, "1.");
	TEST_ERROR(LLJSON_PARSE_INVALID_VALUE, "INF");
	TEST_ERROR(LLJSON_PARSE_INVALID_VALUE, "inf");
	TEST_ERROR(LLJSON_PARSE_INVALID_VALUE, "NAN");
	TEST_ERROR(LLJSON_PARSE_INVALID_VALUE, "nan");
}

// ��ֵ������Ե�Ԫ
static void test_parse_number_too_big() {
	TEST_ERROR(LLJSON_PARSE_NUMBER_TOO_BIG, "1e309");
	TEST_ERROR(LLJSON_PARSE_NUMBER_TOO_BIG, "-1e309");
}

static void test_parse_expect_value() {
	TEST_ERROR(LLJSON_PARSE_EXPECT_VALUE, "");
	TEST_ERROR(LLJSON_PARSE_EXPECT_VALUE, " ");
}

/*
*	string���ͽ������Ե�Ԫ
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

// �Ϸ�string���Ͳ��Ե�Ԫ
static void test_parse_string() {
	TEST_STRING("", "\"\"");
	TEST_STRING("hello", "\"hello\"");
	TEST_STRING("Hello\nWorld", "\"Hello\\nWorld\""); // "Hello\nWorld"
	TEST_STRING("\" \\ \/ \b \r \f \n \t", "\"\\\" \\\\ \\/ \\b \\r \\f \\n \\t\"");
	TEST_STRING("\\", "\"\\\\\"");

	/* /uXXXX���Ե�Ԫ��Unicodeתutf-8���Ե�Ԫ�� */
	TEST_STRING("\x24", "\"\\u0024\"");
	TEST_STRING("\xC2\xA2", "\"\\u00A2\"");
	TEST_STRING("\xE2\x82\xAC", "\"\\u20AC\"");
}

// string����ȱ�����Ų��Ե�Ԫ
static void test_parse_missing_quotation_mark() {
	TEST_ERROR(LLJSON_PARSE_MISS_QUOTATION_MARK, "\"");
	TEST_ERROR(LLJSON_PARSE_MISS_QUOTATION_MARK, "\"abc");
}

// ��Чת���ַ����Ե�Ԫ
static void test_parse_invalid_string_escape() {
	TEST_ERROR(LLJSON_PARSE_INVALID_STRING_ESCAPE, "\"\\v\""); 
	TEST_ERROR(LLJSON_PARSE_INVALID_STRING_ESCAPE, "\"\\a\"");
}

// ��Ч�ַ����ַ����Ե�Ԫ
static void test_parse_invalid_string_char() {
	TEST_ERROR(LLJSON_PARSE_INVALID_STRING_CHAR, "\"hello\x01world\""); // �Ƿ����Ʒ� \x01
	TEST_ERROR(LLJSON_PARSE_INVALID_STRING_CHAR, "\"\x1F\"");
}

// �Ƿ���Unicode(\uXXXX)ʮ�����������Ե�Ԫ
static void test_parse_invalid_unicode_hex() {
	TEST_ERROR(LLJSON_PARSE_INVALID_UNICODE_HEX, "\"\\u\"");
	TEST_ERROR(LLJSON_PARSE_INVALID_UNICODE_HEX, "\"\\u0\"");
	TEST_ERROR(LLJSON_PARSE_INVALID_UNICODE_HEX, "\"\\u01\"");
	TEST_ERROR(LLJSON_PARSE_INVALID_UNICODE_HEX, "\"\\u012\"");
	TEST_ERROR(LLJSON_PARSE_INVALID_UNICODE_HEX, "\"\\uG012\"");
	TEST_ERROR(LLJSON_PARSE_INVALID_UNICODE_HEX, "\"\\u 123\"");
}

// ��Ч/�Ϸ��Ĵ���Բ��Ե�Ԫ
static void test_parse_unicode_surrogate() {
	TEST_STRING("\xF0\x9D\x84\x9E", "\"\\uD834\\uDD1E\"");
	TEST_STRING("\xF0\x9D\x84\x9E", "\"\\ud834\\udd1e\"");
}

// ��Ч�Ĵ���Բ��Ե�Ԫ
static void test_parse_invalid_unicode_surrogate() {
	TEST_ERROR(LLJSON_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\"");
	TEST_ERROR(LLJSON_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uDBFF\"");
	TEST_ERROR(LLJSON_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\\\\\"");
	TEST_ERROR(LLJSON_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\\uDBFF\"");
	TEST_ERROR(LLJSON_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\\uE000\"");
}

// array���Ͳ��Ե�Ԫ
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
	EXPECT_EQ_STRING("abc", lljson_get_string(lljson_get_array_element(&v, 4)), lljson_get_string_length(lljson_get_array_element(&v, 4))); // ֮ǰ����ͻȻ������
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

// ȱ�ٶ��Ż����ţ����飩���Ե�Ԫ
static void test_parse_miss_comma_or_square_bracket() {
	TEST_ERROR(LLJSON_PARSE_MISS_COMMA_OR_SQUARE_BRACKET, "[1");
	TEST_ERROR(LLJSON_PARSE_MISS_COMMA_OR_SQUARE_BRACKET, "[1}");
	TEST_ERROR(LLJSON_PARSE_MISS_COMMA_OR_SQUARE_BRACKET, "[1 2");
	TEST_ERROR(LLJSON_PARSE_MISS_COMMA_OR_SQUARE_BRACKET, "[[]");
}

// object���Ͳ��Ե�Ԫ
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
	// ��0����ֵ�� -> "n" : null
	EXPECT_EQ_STRING("n", lljson_get_object_key(&v, 0), lljson_get_object_key_length(&v, 0));
	EXPECT_EQ_INT(LLJSON_NULL,  lljson_get_type(lljson_get_object_value(&v, 0)));
	// ��1����ֵ�� -> "f" : false
	EXPECT_EQ_STRING("f", lljson_get_object_key(&v, 1), lljson_get_object_key_length(&v, 1));
	EXPECT_EQ_INT(LLJSON_FALSE, lljson_get_type(lljson_get_object_value(&v, 1)));
	// ��2����ֵ�� -> "t" : true
	EXPECT_EQ_STRING("t", lljson_get_object_key(&v, 2), lljson_get_object_key_length(&v, 2));
	EXPECT_EQ_INT(LLJSON_TRUE, lljson_get_type(lljson_get_object_value(&v, 2)));
	// ��3����ֵ�� -> "i" : 123
	EXPECT_EQ_STRING("i", lljson_get_object_key(&v, 3), lljson_get_object_key_length(&v, 3));
	EXPECT_EQ_INT(LLJSON_NUMBER, lljson_get_type(lljson_get_object_value(&v, 3)));
	EXPECT_EQ_DOUBLE(123.0, lljson_get_number(lljson_get_object_value(&v, 3)));
	// ��4����ֵ�� ->  "s" : "abc"
	EXPECT_EQ_STRING("s", lljson_get_object_key(&v, 4), lljson_get_object_key_length(&v, 4));
	EXPECT_EQ_INT(LLJSON_STRING, lljson_get_type(lljson_get_object_value(&v, 4)));
	EXPECT_EQ_STRING("abc", lljson_get_string(lljson_get_object_value(&v, 4)), lljson_get_string_length(lljson_get_object_value(&v, 4)));
	// ��5����ֵ�� -> "a" : [ 1, 2, 3 ]
	EXPECT_EQ_STRING("a", lljson_get_object_key(&v, 5), lljson_get_object_key_length(&v, 5));
	EXPECT_EQ_INT(LLJSON_ARRAY, lljson_get_type(lljson_get_object_value(&v, 5)));
	EXPECT_EQ_SIZE_T(3, lljson_get_array_size(lljson_get_object_value(&v, 5)));
	for (size_t i = 0; i < 3; i++) {
		lljson_value* e = lljson_get_array_element(lljson_get_object_value(&v, 5), i);
		EXPECT_EQ_INT(LLJSON_NUMBER, lljson_get_type(e));
		EXPECT_EQ_DOUBLE(i + 1.0, lljson_get_number(e));
	}
	// ��6����ֵ�� -> "o" : { "1" : 1, "2" : 2, "3" : 3 }
	EXPECT_EQ_STRING("o", lljson_get_object_key(&v, 6), lljson_get_object_key_length(&v, 6)); // key
	{
		lljson_value* o = lljson_get_object_value(&v, 6); // ����һ��object����
		EXPECT_EQ_INT(LLJSON_OBJECT, lljson_get_type(o));
		for (size_t i = 0; i < 3; i++) { // ѭ��3����ֵ��
			// i + '1'�õ��ַ����ͣ�lljson_get_object_key(o, i)[0]����[0]����Ϊ�䷵��ֵΪ�ַ�������
			EXPECT_TRUE(i + '1' == lljson_get_object_key(o, i)[0]);
			EXPECT_EQ_SIZE_T(1, lljson_get_object_key_length(o, i));
			lljson_value* ov = lljson_get_object_value(o, i); // value
			EXPECT_EQ_INT(LLJSON_NUMBER, lljson_get_type(ov));
			EXPECT_EQ_DOUBLE(i + 1.0, lljson_get_number(ov));
		}
	}
	lljson_free(&v);
}

// ȱ�ٶ��Ż����ţ����󣩲��Ե�Ԫ
static void test_parse_miss_comma_or_curly_bracket() {
	TEST_ERROR(LLJSON_PARSE_MISS_COMMA_OR_CURLY_BRACKET, "{\"a\":1");
	TEST_ERROR(LLJSON_PARSE_MISS_COMMA_OR_CURLY_BRACKET, "{\"a\":1]");
	TEST_ERROR(LLJSON_PARSE_MISS_COMMA_OR_CURLY_BRACKET, "{\"a\":1 \"b\"");
	TEST_ERROR(LLJSON_PARSE_MISS_COMMA_OR_CURLY_BRACKET, "{\"a\":{}");
	TEST_ERROR(LLJSON_PARSE_MISS_COMMA_OR_CURLY_BRACKET, "{\"a\":{} \"b\":{}");
}

// ȱ�ټ�key
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

// ȱ��ð��
static void test_parse_miss_colon() {
	TEST_ERROR(LLJSON_PARSE_MISS_COLON, "{\"a\"}");
	TEST_ERROR(LLJSON_PARSE_MISS_COLON, "{\"a\",\"b\"}");
}

/*
*	���ʲ��Ե�Ԫ
*/
static void test_access_null() {
	lljson_value v;
	v.type = LLJSON_NULL;
	// �Ƚ�v����Ϊ���������Բ����ٴ�����vʱ�Ƿ����lljson_free�ͷ���ԭ��ָ����ڴ�
	lljson_set_string(&v, "a", 1);
	lljson_set_null(&v);
	EXPECT_EQ_INT(LLJSON_NULL, lljson_get_type(&v));
	lljson_free(&v);
}

/*
*	JSON���������Ե�Ԫ/�ַ�������stringify�����Ե�Ԫ
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

/* ���������Ե�Ԫ */ 
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
*	�ж�����lljson_value�Ƿ����
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

// set null���ʲ���
static void test_access_boolean() {
	lljson_value v;
	v.type = LLJSON_NULL;
	// �Ƚ�v����Ϊ���������Բ����ٴ�����vʱ�Ƿ����lljson_free�ͷ���ԭ��ָ����ڴ�
	lljson_set_string(&v, "a", 1);
	lljson_set_boolean(&v, 1);
	EXPECT_TRUE(lljson_get_boolean(&v));
	lljson_set_boolean(&v, 0);
	EXPECT_FALSE(lljson_get_boolean(&v));
	lljson_free(&v);
}

// set number���ʲ���
static void test_access_number() {
	lljson_value v;
	v.type = LLJSON_NULL;
	// �Ƚ�v����Ϊ���������Բ����Ƿ����lljson_free�ͷ�ԭ��ָ����ڴ�
	lljson_set_boolean(&v, 0);
	lljson_set_number(&v, 123456.1);
	EXPECT_EQ_DOUBLE(123456.1, lljson_get_number(&v));
	lljson_free(&v);
}

// set string���ʲ���
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

// �Ϸ�/�Ƿ����Ե�Ԫ
static void test_parse() {
	test_parse_null();
	test_parse_true();
	test_parse_false();
	test_parse_root_not_singular();
	test_parse_number(); // �Ϸ����ֲ���
	test_parse_invalid_value(); // ���Ϸ���������
	test_parse_number_too_big(); // ��������
	test_parse_expect_value(); // ȱ����Чֵ���Ե�Ԫ

	test_parse_string(); // �Ϸ�string���Ͳ���
	test_parse_missing_quotation_mark(); // ȱ���ŵ�Ԫ����
	test_parse_invalid_string_escape();
	test_parse_invalid_string_char();

	test_parse_invalid_unicode_hex(); // �Ƿ�ʮ�����Ʊ�����
	test_parse_unicode_surrogate(); // �Ϸ�����Բ���
	test_parse_invalid_unicode_surrogate(); // ��Ч����Բ���

	test_parse_array(); // ��������
	test_parse_miss_comma_or_square_bracket();
	
	test_parse_object(); // ��������
	test_parse_miss_comma_or_curly_bracket();
	test_parse_miss_key();
	test_parse_miss_colon();

	test_stringify();
	test_equal();
}

// ���ʲ���
static void test_access() {
	// ���ʲ���
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