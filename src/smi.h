//
// Created by AA on 2024/2/4.
//

#ifndef SQL_SMI_H
#define SQL_SMI_H

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define SIZE_OF_STR 1000

#define NUM_OF_FIELD 257

#define NUM_OF_RECORD 1024

#define NUM_OF_TOKEN 100

#define SIZE_OF_NAME 35

#define SIZE_OF_DATA 200

#define SIZE_FOR_CON 50

#define SYNTAX_ERROR {printf("SYNTAX ERROR\n");free(arg);return NULL;}

#define DELIMITER '\n'

//数据类型

typedef struct record {
    char **data; // 数据指针
    struct record *next; // 下一条记录
    struct record *prev; //上一条记录
} Record;

typedef struct table {
    char *name; // 表名
    int field_num; // 字段数
    char **field_name; // 字段名数组
    char **field_type; // 字段类型数组
    int primary_key_index; // 主键索引
    int *is_unique; // 唯一性约束
    int *is_not_null; // 非空约束
    Record *head; // 指向第一条记录
    Record *tail; // 指向最后一条记录
    struct table *next; // 下一张表
} Table;

typedef struct database {
    Table *head;
    Table *tail;
} DataBase;

typedef struct {
    char *operator;
    char *value[3];
} atomicClause;

typedef struct conditionClause {
    atomicClause *atomic_clause;
    int num_of_operator;
    int num_of_clause;
    int not[SIZE_FOR_CON];
    char **logic_operator;
    struct conditionClause **inner_clause;
} conditionClause;

typedef struct {
    int type;
    char *table_name;
    int field_num;
    char *field_name[NUM_OF_FIELD];
    char *field_type[NUM_OF_FIELD];
    int value_num;
    char *field_value[NUM_OF_FIELD];
    int is_primary_key[NUM_OF_FIELD];
    int is_unique[NUM_OF_FIELD];
    int is_not_null[NUM_OF_FIELD];
    int order_num;
    char *order_by[NUM_OF_FIELD];
    int order[NUM_OF_FIELD];
    conditionClause *con;
} argument;

//全局变量声明

extern DataBase Tables;//存储所有表

extern int idx;//记录当前解析到的token索引

extern char *key_word[];//关键字数组

enum {
    INT = 1, CHAR, CREATE, TABLE, PRIMARY, KEY, NOT, UNIQUE, INSERT, INTO, VALUES, DELETE, FROM, WHERE, UPDATE, ORDER,
    BY, ASC, DESC, AND, OR, BETWEEN, IS, SELECT
};//关键字索引

//读取函数

char *scan(void);//读取一条语句并添加分隔符

char **split(char *str, const char *delim);//分割字符串

argument *parse(char **token);//解析语句

argument *parse_create(char **token);//create语句解析

argument *parse_insert(char **token);//insert语句解析

argument *parse_update(char **token);//update语句解析

argument *parse_delete(char **token);//delete语句解析

argument *parse_select(char **token);//select语句解析

int get_key_word_index(const char *str);//获取关键字索引

void get_limit(argument *arg, char **token);//获取字段限制

bool condition_check(conditionClause *con, char **token);//检查条件子句

bool condition_parse(conditionClause *con, char **token);//条件子句解析

bool atomic_clause_parse(conditionClause *clause, char **token);//原子子句解析

bool name_check(const char *name);//检查字段名合法性

int type_name_check(const char *type);//检查类型名合法性

int value_check(const char *value);//检查值合法性

//执行函数

void execute(argument *arg);//执行函数

void execute_create(argument *arg);//执行create语句

void execute_insert(argument *arg);//执行insert语句

void execute_update(argument *arg);//执行update语句

void execute_delete(argument *arg);//执行delete语句

void execute_select(argument *arg);//执行select语句

int execute_con(conditionClause *con, Record *record, Table *table);//执行条件子句

int execute_atomic(atomicClause *atomic_clause, Record *record, Table *table);//执行原子子句

int get_value(atomicClause *atomic_clause, Record *record, Table *table, char **value);//获取可比较值，检查并返回类型

bool record_check(Record *record, Table *table, int index_of_field);//检查当前记录某字段的合法性

bool field_check(char *value, Table *table, int index_of_field);//检查插入值对于某字段的合法性

bool type_check(const char *value, const char *type);//检查值类型合法性

void record_cpy(Record *des, Record *src, int field_num);//复制记录

Table *find_table(argument *arg);//查找表

Record *find_record(argument *arg, Table *table, int *error);//查找记录

int find_field(char *field_name, Table *table);//查找字段索引

void sort(Record *arr[], int left, int right, argument *arg, Table *table,
          int (*comp)(Record *, Record *, argument *, Table *, int));//对于一组记录进行排序

int comp(Record *a, Record *b, argument *arg, Table *table, int index_of_order);//比较函数

void swap(Record **a, Record **b);//交换函数

//其他

void init_arg(argument *arg);//初始化argument

void init_con(conditionClause *con);//初始化conditionClause

void free_con(conditionClause *clause);//释放conditionClause

void free_record(Record *record, int num_of_data);//释放record

void free_table(Table *table);//释放table

int str_cmp(const char *a, const char *b);//字符串比较


#endif //SQL_SMI_H
