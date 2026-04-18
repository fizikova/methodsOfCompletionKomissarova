#include <stdio.h>
#include <locale.h>

// обычный комментарий

/* многострочный
   комментарий */

int add(int a, int b) {
    return a + b;
}


int main() {

    setlocale(LC_ALL, "Russian");

    int x = 5;
    int y = 10;
    /*незакрытый комментарий
      int z = 5;*/

    char text[] = "Это не комментарий: // внутри строки";
    char text2[] = "И /* не комментарий */ тоже";
    
    int sum = add(x, y);

    if (sum > 10 && x < y) {
        printf("Сумма больше 10\n");
    }
    else {
        printf("Сумма меньше\n");
    }

    for (int i = 0; i < 3; i++) {
        printf("%d\n", i);
    }

    return 0;
}