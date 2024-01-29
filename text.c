//
// Created by AA on 2024/1/22.
//
case SELECT: {
if (result[1] == NULL) return NULL;
argument *arg = (argument *) malloc(sizeof(argument));
arg->type = SELECT;
idx = 1;//当前解析位置
if (strcmp(result[1], "*") == 0) {
arg->field_num = 0;
arg->field_name = NULL;
}//全选
else {
arg->field_num = 0;
arg->field_name[arg->field_num] = result[1];
arg->field_num++;//首个字段
while (strcmp(result[i], "FROM") != 0) {
if (result[i] == NULL) return NULL;//缺少FROM
if (strcmp(result[i], ",") != 0) return NULL;
if (!name_check(result[i + 1])) return NULL;
arg->field_name[arg->field_num] = result[i + 1];
arg->field_num++;
i += 2;
}//后续字段
}
i++;//跳过FROM
if (!name_check(result[i])) return NULL;
arg->table_name = result[i++];
int j = 0;//条件子句解析偏移量
arg->con = NULL;
//解析条件子句
if (strcmp(result[i], "WHERE") == 0) {
i++;
arg->con = (condition *) malloc(sizeof(condition));
j = condition_check(arg->con, result + i);
if (j == -1) return NULL;//条件子句语法错误
i += j;
}
//解析ORDER BY
if (strcmp(result[i], "ORDER") == 0) {
if (strcmp(result[i + 1], "BY") != 0) return NULL;
i += 2;
if (result[i] == NULL) return NULL;//缺少排序字段
//首个排序字段
arg->order_by[0] = result[i];
if (strcmp(result[i + 1], "DESC") == 0) {
arg->order[0] = 1;
i += 2;
} else if (strcmp(result[i + 1], "ASC") == 0) {
arg->order[0] = 0;
i += 2;
} else {
arg->order[0] = 0;
i++;
}
//后续排序字段
int k = 1;
while (result[i] != NULL) {
if (strcmp(result[i], ",") != 0) return NULL;
if (result[i + 1] == NULL) return NULL;
arg->order_by[k] = result[i + 1];
if (strcmp(result[i + 2], "DESC") == 0) {
arg->order[k] = 1;
i += 3;
} else if (strcmp(result[i + 2], "ASC") == 0) {
arg->order[k] = 0;
i += 3;
} else {
arg->order[k] = 0;
i += 2;
}
k++;
}
}
}
//void print(argument *arg) {
//
//    printf("type: %d\n", arg->type);
//    if (arg->table_name != NULL) printf("table_name: %s\n", arg->table_name);
//    printf("field_num: %d\n", arg->field_num);
//    printf("field_name: ");
//    for (int i = 0; i < arg->field_num; ++i) {
//        printf("%s ", arg->field_name[i]);
//    }
//    printf("\n");
//    printf("field_type: ");
//    if(arg->type==CREATE){
//        for (int i = 0; i < arg->field_num; ++i) {
//            printf("%s ", arg->field_type[i]);
//        }
//        printf("\n");
//    }
//    printf("value_num: %d\n", arg->value_num);
//    printf("field_value: ");
//    if(arg->type==UPDATE){
//        for (int i = 0; i < arg->field_num; ++i) {
//            printf("%s ", arg->field_value[i]);
//        }
//    }else{
//        for (int i = 0; i < arg->value_num; ++i) {
//            printf("%s ", arg->field_value[i]);
//        }
//    }
//    printf("\n");
//    printf("primary_key_index: %d\n", arg->primary_key_index);
//    printf("is_unique: ");
//    for (int i = 0; i < arg->field_num; ++i) {
//        printf("%d ", arg->is_unique[i]);
//    }
//    printf("\n");
//    printf("is_not_null: ");
//    for (int i = 0; i < arg->field_num; ++i) {
//        printf("%d ", arg->is_not_null[i]);
//    }
//    printf("\n");
//    printf("order_num: %d\n", arg->order_num);
//    printf("order_by: ");
//    for (int i = 0; i < arg->order_num; ++i) {
//        printf("%s ", arg->order_by[i]);
//    }
//    printf("\n");
//    printf("order: ");
//    for (int i = 0; i < arg->order_num; ++i) {
//        printf("%d ", arg->order[i]);
//    }
//    printf("\n");
//    if (arg->con != NULL) {
//        printf("con: \n");
//        if(arg->con->atomic_clause==NULL){
//            printf("num_of_operator: %d\n", arg->con->num_of_operator);
//            printf("num_of_clause: %d\n", arg->con->num_of_clause);
//            printf("logic_operator: ");
//            for (int i = 0; i < arg->con->num_of_operator; ++i) {
//                printf("%s ", arg->con->logic_operator[i]);
//            }
//            printf("\n");
//        }else{
//            printf("atomic_clause: ");
//            printf("%s\n",arg->con->atomic_clause);
//        }
//    }
//}