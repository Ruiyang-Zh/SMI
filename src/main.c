#include "smi.h"

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