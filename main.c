#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define SYNTAX_ERROR {printf("SYNTAX ERROR\n");free(arg);return NULL;}

#define SIZE_OF_STR 1000

#define NUM_OF_FIELD 257

#define NUM_OF_RECORD 1024

#define NUM_OF_TOKEN 100

#define SIZE_OF_NAME 35

#define SIZE_OF_DATA 200

#define SIZE_FOR_CON 50

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

struct {
    Table *head; // 第一张表
    Table *tail; // 最后一张表
} Tables;

#define DELIMITER '\n'

enum key_word_index {
    INT = 1, CHAR, CREATE, TABLE, PRIMARY, KEY, NOT, UNIQUE, INSERT, INTO, VALUES, DELETE, FROM, WHERE, UPDATE, ORDER,
    BY, ASC, DESC, AND, OR, BETWEEN, IS, SELECT
};

char *key_word[] = {"NULL", "INT", "CHAR", "CREATE", "TABLE", "PRIMARY", "KEY", "NOT", "UNIQUE", "INSERT", "INTO",
                    "VALUES", "DELETE", "FROM", "WHERE", "UPDATE", "ORDER", "BY", "ASC", "DESC", "AND", "OR", "BETWEEN",
                    "IS", "SELECT"};

int idx = 0;//当前解析位置

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


//读取函数

char *scan(void);

char **split(char *str, const char *delim);

argument *parse(char **token);

argument *parse_create(char **token);

argument *parse_insert(char **token);

argument *parse_update(char **token);

argument *parse_delete(char **token);

argument *parse_select(char **token);

int get_key_word_index(const char *str);

void get_limit(argument *arg, char **token);

bool condition_check(conditionClause *con, char **token);

bool condition_parse(conditionClause *con, char **token);

bool atomic_clause_parse(conditionClause *clause, char **token);

bool name_check(const char *name);

int type_name_check(const char *type);

int value_check(const char *value);

//执行函数

void execute(argument *arg);

void execute_create(argument *arg);

void execute_insert(argument *arg);

void execute_update(argument *arg);

void execute_delete(argument *arg);

void execute_select(argument *arg);

int execute_con(conditionClause *con, Record *record, Table *table);

int execute_atomic(atomicClause *atomic_clause, Record *record, Table *table);

bool field_value_check(char *value, Table *table, int index_of_field);

bool type_check(const char *value, const char *type);

Table *find_table(argument *arg);

Record *find_record(argument *arg, Table *table, int *error);

int find_field(char *field_name, Table *table);

void sort(Record *arr[], int left, int right, argument *arg, Table *table,
          int (*comp)(Record *, Record *, argument *, Table *, int));

int comp(Record *a, Record *b, argument *arg, Table *table, int index_of_order);

//其他

void init_arg(argument *arg);

void init_con(conditionClause *con);

void free_con(conditionClause *clause);

void free_record(Record *record, int num_of_data);

void free_table(Table *table);

int str_cmp(const char *a, const char *b);

int main(void) {
    int num = 0;
    Tables.head = NULL;
    Tables.tail = NULL;
    char *str = scan();
    while (str != NULL) {
        char **token = split(str, "\n");
        idx = 0;
        argument *arg = parse(token);
        if (arg != NULL) {
            execute(arg);
            free(arg);
        }
        free(token);
        free(str);
        num++;
        str = scan();
    }
    Table *curr = Tables.head;
    while (curr != NULL) {
        Table *temp = curr;
        curr = curr->next;
        free_table(temp);
    }
    return 0;
}

//读取一条语句并添加分隔符
char *scan(void) {
    char *str = (char *) malloc(SIZE_OF_STR * sizeof(char));
    char *p = str;
    char c = getchar();
    if (c == '$') {
        free(str);
        return NULL;
    }
    bool is_quote = false;//是否在引号内
    bool is_escape = false;//是否转义
    bool is_empty = true;//读入是否为空
    while ((c != ';' || is_quote) && c != EOF) {
        if (is_quote && !is_escape && c == '\n') is_quote = false;
        if (!is_quote && (c == '\n' || c == '\t' || c == ' ')) c = DELIMITER;
        if ((c == ',' || c == ')' || c == '(') && !is_quote) *p++ = DELIMITER;
        *p++ = c;
        if (c != '\n' && c != '\t' && c != ' ') is_empty = false;
        if ((c == '(') && !is_quote) *p++ = DELIMITER;
        c = getchar();
        if (c == '$') {
            free(str);
            return NULL;
        }
        if (c == '\'' && !is_escape) {
            is_quote = !is_quote;
        }
        if (c == '\\' && !is_escape) {
            is_escape = true;
        } else {
            is_escape = false;
        }
    }
    if (is_empty) {
        free(str);
        return NULL;
    }
    *p = '\0';
    return str;
}

//解析语句
argument *parse(char **token) {
    int index_of_key = get_key_word_index(token[0]);
    switch (index_of_key) {
        case CREATE:
            return parse_create(token);
        case INSERT:
            return parse_insert(token);
        case DELETE:
            return parse_delete(token);
        case UPDATE:
            return parse_update(token);
        case SELECT:
            return parse_select(token);
        default:
            printf("SYNTAX ERROR\n");
            return NULL;//未知语句
    }
}

//分割字符串
char **split(char *str, const char *delim) {
    char **token = (char **) malloc(sizeof(char *) * NUM_OF_TOKEN);
    char *p;
    int i = 0;
    p = strtok(str, delim);
    while (p != NULL) {
        token[i++] = p;
        p = strtok(NULL, delim);
    }
    token[i] = NULL;
    return token;
}

//获取关键字索引
int get_key_word_index(const char *str) {
    if (str == NULL) return -1;
    for (int i = 0; i < 25; ++i) {
        if (str_cmp(str, key_word[i]) == 0) {
            return i;
        }
    }
    return 0;
}

//create语句解析
argument *parse_create(char **token) {
    argument *arg = (argument *) malloc(sizeof(argument));
    init_arg(arg);
    arg->type = CREATE;
    if (str_cmp(token[1], "TABLE") != 0) SYNTAX_ERROR
    if (!name_check(token[2])) SYNTAX_ERROR
    arg->table_name = token[2];
    arg->field_num = 0;
    //开始括号内的解析
    if (str_cmp(token[3], "(") != 0) SYNTAX_ERROR
    idx = 4;
    //必选子句
    if (!name_check(token[idx])) SYNTAX_ERROR
    arg->field_name[arg->field_num] = token[idx++];
    int type = type_name_check(token[idx]);
    if (type == 0) SYNTAX_ERROR//未知字段类型
    else if (type == CHAR) {
        if (str_cmp(token[idx + 1], "(") != 0 || str_cmp(token[idx + 3], ")") != 0) SYNTAX_ERROR
        for (int i = 0; token[idx + 2][i] != '\0'; ++i) {
            if (!isdigit(token[idx + 2][i])) SYNTAX_ERROR
        }//检查是否为数字
        strcat(token[idx], token[idx + 2]);
        arg->field_type[arg->field_num] = token[idx];
        idx += 4;
        get_limit(arg, token);
        arg->field_num++;
    } else {
        arg->field_type[arg->field_num] = token[idx];
        idx++;
        get_limit(arg, token);
        arg->field_num++;
    }
    //后续子句
    while (str_cmp(token[idx], ")") != 0) {
        if (str_cmp(token[idx], ",") != 0) SYNTAX_ERROR
        if (!name_check(token[idx + 1])) SYNTAX_ERROR
        arg->field_name[arg->field_num] = token[idx + 1];
        int type = type_name_check(token[idx + 2]);
        if (type == 0) SYNTAX_ERROR//未知字段类型
        else if (type == CHAR) {
            if (str_cmp(token[idx + 3], "(") != 0 || str_cmp(token[idx + 5], ")") != 0) SYNTAX_ERROR
            for (int i = 0; token[idx + 4][i] != '\0'; ++i) {
                if (!isdigit(token[idx + 4][i])) SYNTAX_ERROR
            }//检查是否为数字
            strcat(token[idx + 2], token[idx + 4]);
            arg->field_type[arg->field_num] = token[idx + 2];
            idx += 6;
            get_limit(arg, token);
            arg->field_num++;
        } else {
            arg->field_type[arg->field_num] = token[idx + 2];
            idx += 3;
            get_limit(arg, token);
            arg->field_num++;
        }
    }
    idx++;
    if (token[idx] != NULL) SYNTAX_ERROR
    return arg;
}

//insert语句解析
argument *parse_insert(char **token) {
    argument *arg = (argument *) malloc(sizeof(argument));
    init_arg(arg);
    arg->type = INSERT;
    if (str_cmp(token[1], "INTO") != 0 && str_cmp(token[3], "VALUES") != 0) SYNTAX_ERROR
    if (!name_check(token[2])) SYNTAX_ERROR
    arg->table_name = token[2];
    arg->value_num = 0;
    if (str_cmp(token[4], "(") != 0) SYNTAX_ERROR
    idx = 5;
    if (!value_check(token[idx])) SYNTAX_ERROR
    arg->field_value[arg->value_num] = token[idx];
    arg->value_num++;
    idx++;
    while (str_cmp(token[idx], ")") != 0) {
        if (token[idx] == NULL) SYNTAX_ERROR//缺少右括号
        if (str_cmp(token[idx++], ",") != 0) SYNTAX_ERROR
        if (!value_check(token[idx])) SYNTAX_ERROR
        arg->field_value[arg->value_num] = token[idx];
        arg->value_num++;
        idx++;
    }
    idx++;
    if (token[idx] != NULL) SYNTAX_ERROR
    return arg;
}

//update语句解析
argument *parse_update(char **token) {
    argument *arg = (argument *) malloc(sizeof(argument));
    init_arg(arg);
    arg->type = UPDATE;
    if (str_cmp(token[2], "SET") != 0) SYNTAX_ERROR
    if (!name_check(token[1])) SYNTAX_ERROR
    arg->table_name = token[1];
    arg->field_num = 0;
    //首个字段
    if (!name_check(token[3])) SYNTAX_ERROR
    arg->field_name[arg->field_num] = token[3];
    if (str_cmp(token[4], "=") != 0) SYNTAX_ERROR
    if (!value_check(token[5])) SYNTAX_ERROR
    arg->field_value[arg->field_num] = token[5];
    arg->field_num++;
    idx = 6;
    //后续字段
    if (token[idx] != NULL) {
        while (str_cmp(token[idx], "WHERE") != 0) {
            if (str_cmp(token[idx], ",") != 0) SYNTAX_ERROR
            if (!name_check(token[idx + 1])) SYNTAX_ERROR
            arg->field_name[arg->field_num] = token[idx + 1];
            if (str_cmp(token[idx + 2], "=") != 0) SYNTAX_ERROR
            if (!value_check(token[idx + 3])) SYNTAX_ERROR
            arg->field_value[arg->field_num] = token[idx + 3];
            arg->field_num++;
            idx += 4;
            if (token[idx] == NULL) break;
        }
    }
    //条件子句解析
    idx++;//跳过WHERE
    arg->con = (conditionClause *) malloc(sizeof(conditionClause));
    if (!condition_check(arg->con, token)) SYNTAX_ERROR
    if (token[idx] != NULL) SYNTAX_ERROR
    return arg;
}

//delete语句解析
argument *parse_delete(char **token) {
    argument *arg = (argument *) malloc(sizeof(argument));
    init_arg(arg);
    arg->type = DELETE;
    for (int i = 1; i <= 3; ++i) {
        if (token[i] == NULL) SYNTAX_ERROR
    }
    if (str_cmp(token[1], "FROM") != 0 || str_cmp(token[3], "WHERE") != 0) SYNTAX_ERROR
    if (!name_check(token[2])) SYNTAX_ERROR
    arg->table_name = token[2];
    //条件子句解析
    arg->con = (conditionClause *) malloc(sizeof(conditionClause));
    idx = 4;
    if (!condition_check(arg->con, token)) SYNTAX_ERROR
    if (token[idx] != NULL) SYNTAX_ERROR
    return arg;
}

//select语句解析
argument *parse_select(char **token) {
    argument *arg = (argument *) malloc(sizeof(argument));
    init_arg(arg);
    arg->type = SELECT;
    if (token[1] == NULL) SYNTAX_ERROR
    idx = 1;//当前解析位置
    if (str_cmp(token[1], "*") == 0) {
        arg->field_num = 0;
        idx++;
    }//全选
    else {
        arg->field_num = 0;
        if (!name_check(token[1])) SYNTAX_ERROR
        arg->field_name[arg->field_num] = token[1];
        arg->field_num++;//首个字段
        idx++;
        while (str_cmp(token[idx], "FROM") != 0) {
            if (str_cmp(token[idx], ",") != 0) SYNTAX_ERROR
            if (!name_check(token[idx + 1])) SYNTAX_ERROR
            arg->field_name[arg->field_num] = token[idx + 1];
            arg->field_num++;
            idx += 2;
        }//后续字段
    }
    if (str_cmp(token[idx++], "FROM") != 0) SYNTAX_ERROR
    if (!name_check(token[idx])) SYNTAX_ERROR
    arg->table_name = token[idx++];
    arg->con = NULL;
    //解析条件子句
    if (token[idx] == NULL) return arg;
    if (str_cmp(token[idx], "WHERE") == 0) {
        idx++;
        arg->con = (conditionClause *) malloc(sizeof(conditionClause));
        if (!condition_check(arg->con, token)) SYNTAX_ERROR
    }
    //解析ORDER BY
    if (token[idx] == NULL) return arg;
    if (str_cmp(token[idx], "ORDER") == 0) {
        if (str_cmp(token[idx + 1], "BY") != 0) SYNTAX_ERROR
        idx += 2;
        if (token[idx] == NULL) SYNTAX_ERROR//缺少排序字段
        //首个排序字段
        if (!name_check(token[idx])) SYNTAX_ERROR
        arg->order_by[arg->order_num] = token[idx];
        if (str_cmp(token[idx + 1], "DESC") == 0) {
            arg->order[arg->order_num] = 1;
            idx += 2;
        } else if (str_cmp(token[idx + 1], "ASC") == 0) {
            arg->order[arg->order_num] = 0;
            idx += 2;
        } else {
            arg->order[arg->order_num] = 0;
            idx++;
        }
        arg->order_num++;
        //后续排序字段
        while (token[idx] != NULL) {
            if (str_cmp(token[idx], ",") != 0) SYNTAX_ERROR
            if (!name_check(token[idx + 1])) SYNTAX_ERROR
            arg->order_by[arg->order_num] = token[idx + 1];
            if (str_cmp(token[idx + 2], "DESC") == 0) {
                arg->order[arg->order_num] = 1;
                idx += 3;
            } else if (str_cmp(token[idx + 2], "ASC") == 0) {
                arg->order[arg->order_num] = 0;
                idx += 3;
            } else {
                arg->order[arg->order_num] = 0;
                idx += 2;
            }
            arg->order_num++;
        }
    }
    if (token[idx] != NULL) SYNTAX_ERROR
    return arg;
}

//字段限制
void get_limit(argument *arg, char **token) {
    if (str_cmp(token[idx], "NOT") == 0 && str_cmp(token[idx + 1], "NULL") == 0) {
        arg->is_not_null[arg->field_num] = 1;
        idx += 2;
    }
    if (str_cmp(token[idx], "PRIMARY") == 0 && str_cmp(token[idx + 1], "KEY") == 0) {
        arg->is_primary_key[arg->field_num] = 1;
        idx += 2;
    }
    if (str_cmp(token[idx], "UNIQUE") == 0) {
        arg->is_unique[arg->field_num] = 1;
        idx++;
    }
}

//检查条件子句
bool condition_check(conditionClause *con, char **token) {
    int i = 0;
    //检查括号是否匹配
    int bracket = 0;
    while (token[idx + i] != NULL) {
        if (str_cmp(token[idx + i], "(") == 0) {
            bracket++;
        }
        if (str_cmp(token[idx + i], ")") == 0) {
            bracket--;
        }
        i++;
    }
    if (bracket != 0) return false;
    if (!condition_parse(con, token)) return false;
    if (con->num_of_clause == 0) return false;
    return true;
}

//条件子句解析
bool condition_parse(conditionClause *con, char **token) {
    init_con(con);
    con->num_of_operator = 0;
    con->num_of_clause = 0;
    bool have_read_not = false;
    bool have_read_operator = false;
    bool have_read_clause = false;
    while (token[idx] != NULL && str_cmp(token[idx], "ORDER") != 0 && str_cmp(token[idx], ")") != 0) {
        if (str_cmp(token[idx], "NOT") == 0) {
            con->not[con->num_of_clause] ^= 1;
            have_read_not = true;
            idx++;
        } else if (str_cmp(token[idx], "(") == 0) {
            idx++;
            if (have_read_clause) return false;//两个连续子句之间无运算符
            con->inner_clause[con->num_of_clause] = (conditionClause *) malloc(sizeof(conditionClause));
            if (!condition_parse(con->inner_clause[con->num_of_clause++], token)) return false;
            if (str_cmp(token[idx], ")") != 0) return false;
            have_read_not = false;
            have_read_operator = false;
            have_read_clause = true;
            idx++;
        } else if (str_cmp(token[idx], "AND") == 0 || str_cmp(token[idx], "OR") == 0) {
            if (have_read_not || have_read_operator) return false; //两个连续的运算符
            con->logic_operator[con->num_of_operator++] = token[idx++];
            have_read_operator = true;
            have_read_clause = false;
        } else {
            if (have_read_clause) return false;//两个连续子句之间无运算符
            con->inner_clause[con->num_of_clause] = malloc(sizeof(conditionClause));
            if (!atomic_clause_parse(con->inner_clause[con->num_of_clause++], token)) return false;
            have_read_not = false;
            have_read_operator = false;
            have_read_clause = true;
        }
    }
    return true;
}

//检查并解析原子子句
bool atomic_clause_parse(conditionClause *clause, char **token) {
    init_con(clause);
    clause->inner_clause = NULL;
    clause->atomic_clause = (atomicClause *) malloc(sizeof(atomicClause));
    clause->atomic_clause->operator = NULL;
    if (token[idx] == NULL) return false;//空
    if (!value_check(token[idx]) && !name_check(token[idx])) return false;
    clause->atomic_clause->value[0] = token[idx++];
    if (str_cmp(token[idx], "=") == 0 || str_cmp(token[idx], "<>") == 0 || str_cmp(token[idx], "<") == 0 ||
        str_cmp(token[idx], ">") == 0 || str_cmp(token[idx], "<=") == 0 || str_cmp(token[idx], ">=") == 0) {
        clause->atomic_clause->operator = token[idx++];
        if (!value_check(token[idx]) && !name_check(token[idx])) return false;
        clause->atomic_clause->value[1] = token[idx++];
    } else if (str_cmp(token[idx], "IS") == 0) {
        if (str_cmp(token[idx + 1], "NOT") == 0 && str_cmp(token[idx + 2], "NULL") == 0) {
            clause->atomic_clause->operator = "IS NOT NULL";
            idx += 3;
        } else if (str_cmp(token[idx + 1], "NULL") == 0) {
            clause->atomic_clause->operator = "IS NULL";
            idx += 2;
        } else {
            return false;
        }
    } else if (str_cmp(token[idx], "BETWEEN") == 0 && str_cmp(token[idx + 2], "AND") == 0) {
        clause->atomic_clause->operator = "BETWEEN";
        idx++;
        if (!value_check(token[idx]) && !name_check(token[idx])) return false;
        clause->atomic_clause->value[1] = token[idx++];
        idx++;//跳过AND
        if (!value_check(token[idx]) && !name_check(token[idx])) return false;
        clause->atomic_clause->value[2] = token[idx++];
    } else {
        return false;
    }
    return true;
}

//检查字段名
bool name_check(const char *name) {
    if (name == NULL) return false;
    size_t len = strlen(name);
    if (len > 35) return false;
    for (int i = 0; i < len; ++i) {
        if (!isalpha(name[i]) && name[i] != '_') return false;
    }
    for (int i = 0; i < 25; ++i) {
        if (str_cmp(name, key_word[i]) == 0) return false;
    }
    return true;
}

//检查字段类型名
int type_name_check(const char *type) {
    if (type == NULL) return false;
    if (str_cmp(type, "INT") == 0) return INT;
    if (type[0] == 'C' && type[1] == 'H' && type[2] == 'A' && type[3] == 'R') return CHAR;
    return false;
}

//检查值
int value_check(const char *value) {
    if (value == NULL) return 0;
    if (strcmp(value, "NULL") == 0) return -1;
    int len = strlen(value);
    if (len <= 0) return 0;
    if (len >= 2 && value[0] == '\'' && value[len - 1] == '\'') return CHAR;
    else {
        if (value[0] == '-') {
            if (len == 1) return 0;
            for (int i = 1; i < len; ++i) {
                if (!isdigit(value[i])) return 0;
            }
        } else {
            for (int i = 0; i < len; ++i) {
                if (!isdigit(value[i])) return 0;
            }
        }
    }
    return INT;
}

//执行函数
void execute(argument *arg) {
    switch (arg->type) {
        case CREATE: {
            execute_create(arg);
            return;
        }
        case INSERT: {
            execute_insert(arg);
            return;
        }
        case UPDATE: {
            execute_update(arg);
            return;
        }
        case DELETE: {
            execute_delete(arg);
            return;
        }
        case SELECT: {
            execute_select(arg);
            return;
        }
        default:
            return;
    }
}

void execute_create(argument *arg) {
    Table *table = (Table *) malloc(sizeof(Table));
    table->name = malloc(sizeof(char) * SIZE_OF_NAME);
    strcpy(table->name, arg->table_name);
    //检查表名是否重复
    if (Tables.head != NULL) {
        Table *p = Tables.head;
        while (p != NULL) {
            if (str_cmp(p->name, table->name) == 0) {
                free(table);
                printf("ERROR\n");
                return;
            }
            p = p->next;
        }
    }
    table->field_num = arg->field_num;
    table->field_name = malloc(sizeof(char *) * arg->field_num);
    table->field_type = malloc(sizeof(char *) * arg->field_num);
    table->is_not_null = malloc(sizeof(int) * arg->field_num);
    table->is_unique = malloc(sizeof(int) * arg->field_num);
    table->head = NULL;
    table->tail = NULL;
    int cnt = 0;
    for (int i = 0; i < arg->field_num; ++i) {
        table->field_name[i] = malloc(sizeof(char) * SIZE_OF_NAME);
        table->field_type[i] = malloc(sizeof(char) * SIZE_OF_NAME);
        strcpy(table->field_name[i], arg->field_name[i]);
        if (str_cmp(arg->field_type[i], "INT") != 0) {
            if (atoi(arg->field_type[i] + 4) <= 0) {
                printf("ERROR\n");
                free_table(table);
                return;
            }
        }
        strcpy(table->field_type[i], arg->field_type[i]);
        table->is_not_null[i] = arg->is_not_null[i];
        table->is_unique[i] = arg->is_unique[i];
        if (arg->is_primary_key[i]) {
            cnt++;
            table->primary_key_index = i;
        }
    }
    //检查主键是否唯一
    if (cnt != 1) {
        free_table(table);
        printf("ERROR\n");
        return;
    }
    if (Tables.head == NULL) {
        Tables.head = table;
        Tables.tail = table;
    } else {
        Tables.tail->next = table;
        Tables.tail = table;
    }
    Tables.tail->next = NULL;
    printf("CREATE TABLE SUCCESSFULLY\n");
}

void execute_insert(argument *arg) {
    Table *table = find_table(arg);
    if (table == NULL) {
        printf("ERROR\n");
        return;
    }
    if (arg->value_num != table->field_num) {
        printf("ERROR\n");
        return;
    }
    Record *record = (Record *) malloc(sizeof(Record));
    record->data = (char **) malloc(sizeof(char *) * arg->value_num);
    for (int i = 0; i < arg->value_num; ++i) {
        //限制条件检验
        if (!field_value_check(arg->field_value[i], table, i)) {
            free_record(record, i);
            printf("ERROR\n");
            return;
        }
        //赋值
        record->data[i] = malloc(sizeof(char) * SIZE_OF_DATA);
        strcpy(record->data[i], arg->field_value[i]);
    }
    //向表中添加记录
    if (table->head == NULL) {
        table->head = record;
        table->tail = record;
        table->head->prev = NULL;
    } else {
        record->prev = table->tail;
        table->tail->next = record;
        table->tail = record;
    }
    record->next = NULL;
    printf("1 RECORDS INSERTED\n");
}

void execute_delete(argument *arg) {
    Table *table = find_table(arg);
    if (table == NULL) {
        printf("ERROR\n");
        return;
    }
    int cnt = 0;
    int error = 0;
    Record *record = find_record(arg, table, &error);
    if (error == -1) {
        printf("ERROR\n");
        return;
    }
    while (record != NULL) {
        if (record->prev == NULL && record->next == NULL) {
            table->head = NULL;
            table->tail = NULL;
        } else if (record->prev == NULL) {
            table->head = record->next;
            table->head->prev = NULL;
        } else if (record->next == NULL) {
            table->tail = record->prev;
            table->tail->next = NULL;
        } else {
            record->prev->next = record->next;
            record->next->prev = record->prev;
        }
        free_record(record, table->field_num);
        cnt++;
        record = find_record(arg, NULL, &error);
        if (error == -1) {
            printf("ERROR\n");
            return;
        }
    }
    printf("%d RECORDS DELETED\n", cnt);
}

void execute_update(argument *arg) {
    Table *table = find_table(arg);
    if (table == NULL) {
        printf("ERROR\n");
        return;
    }
    //查找记录
    Record *records[NUM_OF_RECORD];
    int cnt = 0;
    int error = 0;
    records[cnt] = find_record(arg, table, &error);
    if (error == -1) {
        printf("ERROR\n");
        return;
    }
    while (records[cnt] != NULL) {
        cnt++;
        records[cnt] = find_record(arg, NULL, &error);
        if (error == -1) {
            printf("ERROR\n");
            return;
        }
    }
    //查找待更新字段索引
    int index_of_field[arg->field_num];
    for (int i = 0; i < arg->field_num; ++i) {
        index_of_field[i] = find_field(arg->field_name[i], table);
        if (index_of_field[i] == -1) {
            printf("ERROR\n");
            return;
        }
    }
    //更新
    for (int i = 0; i < arg->field_num; ++i) {
        if ((table->is_unique[index_of_field[i]] || index_of_field[i] == table->primary_key_index) && cnt > 1) {
            printf("ERROR\n");
            return;
        }
        if (!field_value_check(arg->field_value[i], table, index_of_field[i])) {
            printf("ERROR\n");
            return;
        }
        for (int j = 0; j < cnt; ++j) {
            strcpy(records[j]->data[index_of_field[i]], arg->field_value[i]);
        }
    }
    printf("%d RECORDS UPDATED\n", cnt);
}

void execute_select(argument *arg) {
    Table *table = find_table(arg);
    if (table == NULL) {
        printf("ERROR\n");
        return;
    }
    Record *records[NUM_OF_RECORD];
    int cnt = 0;
    int error = 0;
    records[cnt] = find_record(arg, table, &error);
    if (error == -1) {
        printf("ERROR\n");
        return;
    }
    while (records[cnt] != NULL) {
        cnt++;
        records[cnt] = find_record(arg, NULL, &error);
        if (error == -1) {
            printf("ERROR\n");
            return;
        }
    }
    printf("%d RECORDS FOUND\n", cnt);
    if (cnt == 0) return;
    //排序字段检验
    for (int i = 0; i < arg->order_num; ++i) {
        if (find_field(arg->order_by[i], table) == -1) {
            printf("ERROR\n");
            return;
        }
    }
    //排序
    sort(records, 0, cnt - 1, arg, table, comp);
    //输出
    //查询所有字段
    if (arg->field_num == 0) {
        for (int i = 0; i < table->field_num; ++i) {
            printf("%s\t", table->field_name[i]);
        }
        printf("\n");
        for (int i = 0; i < cnt; ++i) {
            for (int j = 0; j < table->field_num; ++j) {
                printf("%s\t", records[i]->data[j]);
            }
            printf("\n");
        }
    } else { //查询部分字段
        int index_of_field[arg->field_num];
        for (int i = 0; i < arg->field_num; ++i) {
            index_of_field[i] = find_field(arg->field_name[i], table);
            printf("%s\t", arg->field_name[i]);
        }
        printf("\n");
        for (int i = 0; i < cnt; ++i) {
            for (int j = 0; j < arg->field_num; ++j) {
                printf("%s\t", records[i]->data[index_of_field[j]]);
            }
            printf("\n");
        }
    }
}

//检查插入的字段值
bool field_value_check(char *value, Table *table, int index_of_field) {
    //非空检验
    if ((table->is_not_null[index_of_field] || index_of_field == table->primary_key_index) &&
        str_cmp(value, "NULL") == 0) {
        return false;
    }
    //唯一性检验
    if (table->is_unique[index_of_field] || index_of_field == table->primary_key_index) {
        Record *curr = table->head;
        while (curr != NULL) {
            if (str_cmp(value, curr->data[index_of_field]) == 0 && str_cmp(value, "NULL") != 0) {
                return false;
            }
            curr = curr->next;
        }
    }
    //字段类型检验
    if (!type_check(value, table->field_type[index_of_field])) {
        return false;
    }
    return true;
}

//检查数据与类型是否匹配
bool type_check(const char *value, const char *type) {
    if (str_cmp(type, "INT") == 0) {
        if (str_cmp(value, "NULL") == 0) return true;
        if (value[0] == '-') {
            if (strlen(value) == 1) return false;
            for (int i = 1; i < strlen(value); ++i) {
                if (!isdigit(value[i])) return false;
            }
        } else {
            for (int i = 0; i < strlen(value); ++i) {
                if (!isdigit(value[i])) return false;
            }
        }
    } else {
        if (str_cmp(value, "NULL")) return true;
        int len = atoi(type + 4);
        if (strlen(value) - 2 > len) return false;//检查长度，不包括两个单引号
    }
    return true;
}

//查找表
Table *find_table(argument *arg) {
    Table *table = Tables.head;
    while (table != NULL) {
        if (str_cmp(table->name, arg->table_name) == 0) {
            break;
        }
        table = table->next;
    }
    if (table == NULL) return NULL;
    return table;
}

//查找记录
Record *find_record(argument *arg, Table *table, int *error) {
    static Record *curr;
    static Table *temp;
    Record *ans = NULL;
    if (table != NULL) {
        curr = table->head;
        temp = table;
    }
    while (curr != NULL) {
        int res = execute_con(arg->con, curr, temp);
        if (res == -1) {
            *error = -1;
            return NULL;//ERROR
        }
        if (res == 1) {
            ans = curr;
            curr = curr->next;
            break;
        }
        curr = curr->next;
    }
    return ans;
}

//执行条件子句
int execute_con(conditionClause *con, Record *record, Table *table) {
    if (con == NULL) return 1;
    if (con->atomic_clause != NULL) return execute_atomic(con->atomic_clause, record, table);
    int res[con->num_of_clause];
    for (int i = 0; i < con->num_of_clause; ++i) {
        res[i] = execute_con(con->inner_clause[i], record, table);
        if (res[i] == -1) return -1;//ERROR
        if (con->not[i]) res[i] ^= 1;
    }
    int ans = res[0];
    for (int i = 0; i < con->num_of_operator; ++i) {
        if (str_cmp(con->logic_operator[i], "AND") == 0) {
            ans &= res[i + 1];
        } else {
            ans |= res[i + 1];
        }
    }
    return ans;
}

//获取原子子句的可比较值，并返回类型
int get_value(atomicClause *atomic_clause, Record *record, Table *table, char **value) {
    int type[3];
    int num_of_value = 2;
    bool have_null = false;
    if (str_cmp(atomic_clause->operator, "IS NULL") == 0 || str_cmp(atomic_clause->operator, "IS NOT NULL") == 0) {
        num_of_value = 1;
    } else if (str_cmp(atomic_clause->operator, "BETWEEN") == 0) num_of_value = 3;
    for (int i = 0; i < num_of_value; ++i) {
        type[i] = value_check(atomic_clause->value[i]);//INT/CHAR/NULL(-1)
        if (type[i] == 0) {//field
            int index_of_field = find_field(atomic_clause->value[i], table);
            if (index_of_field == -1) return 0;//找不到字段
            value[i] = record->data[index_of_field];
            if (str_cmp(value[i], "NULL") == 0) have_null = true;
            if (str_cmp(table->field_type[index_of_field], "INT") == 0) type[i] = INT;
            else type[i] = CHAR;
        } else {//const
            value[i] = atomic_clause->value[i];
        }
    }
    for (int i = 0; i < num_of_value - 1; ++i) {
        if (type[i] == -1) {
            have_null = true;
            continue;
        }
        if (type[i] != type[i + 1]) return 0;//类型不匹配
    }
    if (have_null) return -1;
    else return type[0];
}

//执行原子子句
int execute_atomic(atomicClause *atomic_clause, Record *record, Table *table) {
    char *value[3];
    int type = get_value(atomic_clause, record, table, value);
    if (!type) return -1;//ERROR
    if (str_cmp(atomic_clause->operator, "=") == 0) {
        if (type == -1) return 0;//UNKNOWN
        if (str_cmp(value[0], value[1]) == 0) return 1;
        else return 0;
    } else if (str_cmp(atomic_clause->operator, "<>") == 0) {
        if (type == -1) return 0;//UNKNOWN
        if (str_cmp(value[0], value[1]) != 0) return 1;
        else return 0;
    } else if (str_cmp(atomic_clause->operator, "<") == 0) {
        if (type == -1) return 0;//UNKNOWN
        if (type == INT) {
            int a = atoi(value[0]);
            int b = atoi(value[1]);
            return a < b;
        } else {
            if (str_cmp(value[0], value[1]) < 0) return 1;
            else return 0;
        }
    } else if (str_cmp(atomic_clause->operator, ">") == 0) {
        if (type == -1) return 0;//UNKNOWN
        if (type == INT) {
            int a = atoi(value[0]);
            int b = atoi(value[1]);
            return a > b;
        } else {
            if (str_cmp(value[0], value[1]) > 0) return 1;
            else return 0;
        }
    } else if (str_cmp(atomic_clause->operator, "<=") == 0) {
        if (type == -1) return 0;//UNKNOWN
        if (type == INT) {
            int a = atoi(value[0]);
            int b = atoi(value[1]);
            return a <= b;
        } else {
            if (str_cmp(value[0], value[1]) <= 0) return 1;
            else return 0;
        }
    } else if (str_cmp(atomic_clause->operator, ">=") == 0) {
        if (type == -1) return 0;//UNKNOWN
        if (type == INT) {
            int a = atoi(value[0]);
            int b = atoi(value[1]);
            return a >= b;
        } else {
            if (str_cmp(value[0], value[1]) >= 0) return 1;
            else return 0;
        }
    } else if (str_cmp(atomic_clause->operator, "IS NULL") == 0) {
        if (str_cmp(value[0], "NULL") == 0) return 1;
        else return 0;
    } else if (str_cmp(atomic_clause->operator, "IS NOT NULL") == 0) {
        if (str_cmp(value[0], "NULL") != 0) return 1;
        else return 0;
    } else if (str_cmp(atomic_clause->operator, "BETWEEN") == 0) {
        if (type == -1) return 0;//UNKNOWN
        if (type == INT) {
            int a = atoi(value[0]);
            int b = atoi(value[1]);
            int c = atoi(value[2]);
            return a >= b && a <= c;
        } else {
            if (str_cmp(value[0], value[1]) >= 0 &&
                str_cmp(value[0], value[2]) <= 0)
                return 1;
            else return 0;
        }
    }
    return -1;//ERROR
}

//查找字段索引
int find_field(char *field_name, Table *table) {
    for (int i = 0; i < table->field_num; ++i) {
        if (str_cmp(field_name, table->field_name[i]) == 0) {
            return i;
        }
    }
    return -1;
}

//比较函数
int comp(Record *a, Record *b, argument *arg, Table *table, int index_of_order) {
    int index_of_field;
    if (index_of_order >= arg->order_num) index_of_field = table->primary_key_index;
    else index_of_field = find_field(arg->order_by[index_of_order], table);
    bool is_null_a = str_cmp(a->data[index_of_field], "NULL") == 0;
    bool is_null_b = str_cmp(b->data[index_of_field], "NULL") == 0;
    if (is_null_a && is_null_b) return comp(a, b, arg, table, index_of_order + 1);//下一轮比较
    else if (is_null_a) return 1;
    else if (is_null_b) return -1;
    if (str_cmp(table->field_type[index_of_field], "INT") == 0) {
        int int_a = atoi(a->data[index_of_field]);
        int int_b = atoi(b->data[index_of_field]);
        int cmp = (int_a > int_b) - (int_a < int_b);
        if (cmp == 0 && index_of_field != table->primary_key_index)
            return comp(a, b, arg, table, index_of_order + 1);//下一轮比较
        if (arg->order[index_of_order] == 1) return -cmp;
        else return cmp;
    } else {
        int cmp = str_cmp(a->data[index_of_field], b->data[index_of_field]);
        if (cmp == 0 && index_of_field != table->primary_key_index)
            return comp(a, b, arg, table, index_of_order + 1);//下一轮比较
        if (arg->order[index_of_order] == 1) return -cmp;
        else return cmp;
    }
}

void swap(Record *a, Record *b) {
    Record temp = *a;
    *a = *b;
    *b = temp;
}

//快速排序
void sort(Record *arr[], int left, int right, argument *arg, Table *table,
          int (*comp)(Record *, Record *, argument *, Table *, int)) {
    if (left >= right) return;
    int i = left, j = right;
    Record *pivot = arr[left];
    while (i < j) {
        while (i < j && comp(arr[j], pivot, arg, table, 0) >= 0) j--;
        while (i < j && comp(arr[i], pivot, arg, table, 0) <= 0) i++;
        if (i < j) swap(arr[i], arr[j]);
    }
    swap(arr[left], arr[i]);
    sort(arr, left, i - 1, arg, table, comp);
    sort(arr, i + 1, right, arg, table, comp);
}

//字符串比较,为NULL返回-2
int str_cmp(const char *a, const char *b) {
    if (a == NULL || b == NULL) return -2;
    return strcmp(a, b);
}

//初始化参数
void init_arg(argument *arg) {
    arg->table_name = NULL;
    arg->field_num = 0;
    arg->value_num = 0;
    arg->order_num = 0;
    memset(arg->is_primary_key, 0, NUM_OF_FIELD * sizeof(arg->is_primary_key[0]));
    memset(arg->is_unique, 0, NUM_OF_FIELD * sizeof(arg->is_unique[0]));
    memset(arg->is_not_null, 0, NUM_OF_FIELD * sizeof(arg->is_not_null[0]));
    memset(arg->order, 0, NUM_OF_FIELD * sizeof(arg->order[0]));
    arg->con = NULL;
}

//初始化条件子句
void init_con(conditionClause *con) {
    con->atomic_clause = NULL;
    memset(con->not, 0, SIZE_FOR_CON * sizeof(con->not[0]));
    con->inner_clause = malloc(SIZE_FOR_CON * sizeof(conditionClause *));
    con->logic_operator = malloc(SIZE_FOR_CON * sizeof(char *));
    con->num_of_operator = 0;
    con->num_of_clause = 0;
}

//释放记录
void free_record(Record *record, int num_of_data) {
    for (int i = 0; i < num_of_data; ++i) {
        free(record->data[i]);
    }
    free(record->data);
    free(record);
}

//释放表
void free_table(Table *table) {
    Record *record = table->head;
    while (record != NULL) {
        Record *temp = record;
        record = record->next;
        free_record(temp, table->field_num);
    }
    for (int i = 0; i < table->field_num; ++i) {
        free(table->field_name[i]);
        free(table->field_type[i]);
    }
    free(table->name);
    free(table->field_name);
    free(table->field_type);
    free(table->is_not_null);
    free(table->is_unique);
    free(table);
}

//释放条件子句
void free_con(conditionClause *clause) {
    if (clause->atomic_clause != NULL) {
        free(clause->atomic_clause);
    }
    if (clause->inner_clause != NULL) {
        for (int i = 0; i < clause->num_of_clause; ++i) {
            free_con(clause->inner_clause[i]);
        }
        free(clause);
    }
}
